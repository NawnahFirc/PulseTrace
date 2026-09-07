#include "pulsetrace/version.hpp"

#include <iostream>
#include <string_view>

namespace {
void print_help() {
    std::cout << "Usage: pulsetrace [--help | --version]\n"
                 "Linux process monitoring and diagnostics.\n"
                 "Milestone 0: build scaffold; monitoring is not implemented yet.\n";
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
    } else {
        std::cerr << "Invalid arguments. Use pulsetrace --help.\n";
        return 2;
    }

    // Report a failed output operation, e.g. stdout redirected to /dev/full.
    std::cout.flush();
    return std::cout ? 0 : 1;
}
