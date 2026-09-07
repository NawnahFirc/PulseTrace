#include "catch_amalgamated.hpp"
#include "pulsetrace/linux/process_discovery.hpp"
#include "linux/discovery_detail.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;
using pulsetrace::ProcessDiscovery;
using pulsetrace::ProcessId;
using pulsetrace::ScanError;
using pulsetrace::discover_processes;

// Unique fixture ownership. Cleanup runs even when an assertion throws.
class TemporaryDirectory {
  public:
    TemporaryDirectory() {
        std::string pattern = (fs::temp_directory_path() / "pulsetrace-test-XXXXXX").string();
        const char* created = ::mkdtemp(pattern.data());
        if (created == nullptr) {
            throw std::system_error(errno, std::generic_category(), "mkdtemp");
        }
        path_ = created;
    }
    ~TemporaryDirectory() {
        std::error_code error;
        fs::permissions(path_, fs::perms::owner_all, fs::perm_options::add, error);
        fs::remove_all(path_, error);
        // Destructors cannot safely throw during assertion unwinding.
    }
    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
    [[nodiscard]] const fs::path& path() const noexcept { return path_; }

  private:
    fs::path path_;
};

std::vector<int> values(const ProcessDiscovery& scan) {
    std::vector<int> result;
    for (const auto pid : scan.pids) {
        result.push_back(pid.value());
    }
    return result;
}
} // namespace

TEST_CASE("PID parser enforces positive int and full ASCII decimal input") {
    for (const auto text : {"", "0", "000", "-1", "+1", " 1", "1 ", "1x", "1.5",
                            "self", "thread-self", "999999999999999999999999999", "\xFF"}) {
        INFO(text);
        REQUIRE_FALSE(ProcessId::parse(text));
    }
    REQUIRE(ProcessId::parse("00042")->value() == 42);
    const auto maximum = std::to_string(std::numeric_limits<int>::max());
    REQUIRE(ProcessId::parse(maximum)->value() == std::numeric_limits<int>::max());
    REQUIRE_FALSE(ProcessId::parse(maximum + "0"));
    REQUIRE_FALSE(ProcessId::parse(std::string_view{"12\0x", 4}));
}

TEST_CASE("An empty readable root is a successful empty scan") {
    TemporaryDirectory root;
    const auto result = discover_processes(root.path());
    REQUIRE(std::holds_alternative<ProcessDiscovery>(result));
    const auto& scan = std::get<ProcessDiscovery>(result);
    REQUIRE(scan.complete());
    REQUIRE(scan.pids.empty());
    REQUIRE(scan.vanished_entries == 0);
}

TEST_CASE("Discovery filters entries and sorts and deduplicates numeric IDs") {
    TemporaryDirectory root;
    for (const auto name : {"90", "2", "12", "002", "0", "-1", "1x", "self",
                            "thread-self", "9999999999999999999999999999999"}) {
        fs::create_directory(root.path() / name);
    }
    std::ofstream(root.path() / "33") << "not a directory";
    fs::create_directory_symlink(root.path() / "2", root.path() / "44");
    fs::create_symlink(root.path() / "missing", root.path() / "55");
    const auto result = discover_processes(root.path());
    REQUIRE(std::holds_alternative<ProcessDiscovery>(result));
    const auto& scan = std::get<ProcessDiscovery>(result);
    REQUIRE(values(scan) == std::vector<int>{2, 12, 90});
    REQUIRE(scan.complete());
    REQUIRE(scan.vanished_entries == 0);
}

TEST_CASE("Missing and non-directory roots are explicit open failures") {
    TemporaryDirectory root;
    const auto missing = discover_processes(root.path() / "absent");
    REQUIRE(std::holds_alternative<ScanError>(missing));
    REQUIRE(std::get<ScanError>(missing).operation == pulsetrace::ScanOperation::open_root);
    REQUIRE(std::get<ScanError>(missing).code == std::errc::no_such_file_or_directory);
    std::ofstream(root.path() / "file") << "data";
    const auto file = discover_processes(root.path() / "file");
    REQUIRE(std::holds_alternative<ScanError>(file));
    REQUIRE(std::get<ScanError>(file).code == std::errc::not_a_directory);
}

TEST_CASE("An entry removed before inspection is counted without invalidating other PIDs") {
    TemporaryDirectory root;
    const auto candidate = root.path() / "17";
    fs::create_directory(candidate);
    fs::remove(candidate);
    ProcessDiscovery scan;
    scan.pids.push_back(*ProcessId::parse("4"));
    pulsetrace::detail::inspect_candidate(candidate, *ProcessId::parse("17"), scan);
    REQUIRE(values(scan) == std::vector<int>{4});
    REQUIRE(scan.vanished_entries == 1);
    REQUIRE(scan.complete());
}

TEST_CASE("Unexpected inspection errors preserve partial results and evidence") {
    TemporaryDirectory root;
    std::ofstream(root.path() / "file") << "data";
    ProcessDiscovery scan;
    scan.pids.push_back(*ProcessId::parse("4"));
    const auto candidate = root.path() / "file" / "17";
    pulsetrace::detail::inspect_candidate(candidate, *ProcessId::parse("17"), scan);
    REQUIRE_FALSE(scan.complete());
    REQUIRE(values(scan) == std::vector<int>{4});
    REQUIRE(scan.errors.size() == 1);
    REQUIRE(scan.errors.front().code == std::errc::not_a_directory);
    REQUIRE(scan.errors.front().path == candidate);
    REQUIRE(scan.errors.front().operation == pulsetrace::ScanOperation::inspect_entry);
    REQUIRE(scan.vanished_entries == 0);
}

TEST_CASE("Unreadable roots report permission denial", "[permissions]") {
    if (::geteuid() == 0) {
        SKIP("Root may bypass directory permissions; run this test as an unprivileged user.");
    }
    TemporaryDirectory root;
    fs::permissions(root.path(), fs::perms::none);
    const auto result = discover_processes(root.path());
    REQUIRE(std::holds_alternative<ScanError>(result));
    REQUIRE(std::get<ScanError>(result).code == std::errc::permission_denied);
}

TEST_CASE("Returned IDs remain valid after fixture removal") {
    ProcessDiscovery scan;
    {
        TemporaryDirectory root;
        fs::create_directory(root.path() / "72");
        scan = std::get<ProcessDiscovery>(discover_processes(root.path()));
    }
    REQUIRE(values(scan) == std::vector<int>{72});
}

TEST_CASE("Live proc discovery includes this process without assuming a fixed count", "[live]") {
    const auto result = discover_processes();
    REQUIRE(std::holds_alternative<ProcessDiscovery>(result));
    const auto& scan = std::get<ProcessDiscovery>(result);
    REQUIRE(scan.complete());
    const auto pids = values(scan);
    REQUIRE(std::ranges::is_sorted(pids));
    REQUIRE(std::adjacent_find(pids.begin(), pids.end()) == pids.end());
    // Match the mounted proc view, which can differ from getpid() in a sandbox.
    const auto self = ProcessId::parse(fs::read_symlink("/proc/self").filename().string());
    REQUIRE(self.has_value());
    REQUIRE(std::ranges::find(pids, self->value()) != pids.end());
}
