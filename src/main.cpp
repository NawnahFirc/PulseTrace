#include "pulsetrace/version.hpp"
#include "pulsetrace/linux/process_discovery.hpp"

#include <iostream>
#include <string_view>

namespace {
void print_help() {
    std::cout << "Usage: pulsetrace [--help | --version | --list-pids [--proc-root PATH]]\n"
                 "Linux process monitoring and diagnostics.\n"
                 "Milestone 1: process discovery; metadata and metrics are not implemented yet.\n";
}
int list_pids(const std::filesystem::path& root) {
    auto result = pulsetrace::discover_processes(root);
    if (const auto* error = std::get_if<pulsetrace::ScanError>(&result)) {
        std::cerr << "Cannot open process root: " << error->code.message() << '\n';
        return 1;
    }
    const auto& scan = std::get<pulsetrace::ProcessDiscovery>(result);
    for (const auto pid : scan.pids) {
        std::cout << pid.value() << '\n';
    }
    for (const auto& error : scan.errors) {
        // Avoid printing untrusted path bytes directly into a terminal.
        std::cerr << "Incomplete process scan: " << error.code.message() << '\n';
    }
    if (scan.vanished_entries != 0) {
        std::cerr << "Entries vanished during scan: " << scan.vanished_entries << '\n';
    }
    std::cout.flush();
    return scan.complete() && std::cout ? 0 : 1;
}
} // namespace

int main(int argc, char* argv[]) {
    if (argc == 1) {
        std::cout << "PulseTrace " << pulsetrace::version << '\n';
        print_help();
    } else if (argc == 2 && std::string_view{argv[1]} == "--help") {
        print_help();
    } else if (argc == 2 && std::string_view{argv[1]} == "--version") {
        std::cout << "PulseTrace " << pulsetrace::version << '\n';
    } else if (argc == 2 && std::string_view{argv[1]} == "--list-pids") {
        return list_pids("/proc");
    } else if (argc == 4 && std::string_view{argv[1]} == "--list-pids" &&
               std::string_view{argv[2]} == "--proc-root") {
        return list_pids(argv[3]);
    } else {
        std::cerr << "Invalid arguments. Use pulsetrace --help.\n";
        return 2;
    }

    // Report a failed output operation, e.g. stdout redirected to /dev/full.
    std::cout.flush();
    return std::cout ? 0 : 1;
}
