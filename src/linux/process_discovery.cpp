#include "pulsetrace/linux/process_discovery.hpp"
#include "discovery_detail.hpp"

#include <algorithm>
#include <charconv>

namespace pulsetrace {
std::optional<ProcessId> ProcessId::parse(std::string_view text) noexcept {
    if (text.empty() || !std::ranges::all_of(text, [](char c) { return c >= '0' && c <= '9'; })) {
        return std::nullopt;
    }
    int value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || value <= 0) {
        return std::nullopt;
    }
    return ProcessId{value};
}

namespace detail {
void inspect_candidate(const std::filesystem::path& path, ProcessId pid,
                       ProcessDiscovery& result) {
    std::error_code error;
    // Query now, without following symlinks or relying on directory_entry's cache.
    const auto status = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory ||
        (!error && status.type() == std::filesystem::file_type::not_found)) {
        ++result.vanished_entries;
    } else if (error) {
        result.errors.push_back({ScanOperation::inspect_entry, path, error});
    } else if (std::filesystem::is_directory(status)) {
        result.pids.push_back(pid);
    }
}
} // namespace detail

DiscoveryResult discover_processes(const std::filesystem::path& root) {
    std::error_code error;
    std::filesystem::directory_iterator iterator{root, error};
    if (error) {
        return ScanError{ScanOperation::open_root, root, error};
    }
    ProcessDiscovery result;
    const std::filesystem::directory_iterator end;
    while (iterator != end) {
        const auto& path = iterator->path();
        if (const auto pid = ProcessId::parse(path.filename().string())) {
            detail::inspect_candidate(path, *pid, result);
        }
        iterator.increment(error);
        if (error) {
            result.errors.push_back({ScanOperation::advance_iterator, root, error});
            break;
        }
    }
    std::ranges::sort(result.pids);
    const auto duplicates = std::ranges::unique(result.pids);
    result.pids.erase(duplicates.begin(), duplicates.end());
    return result;
}
} // namespace pulsetrace
