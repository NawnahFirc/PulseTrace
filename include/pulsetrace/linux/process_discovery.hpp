#pragma once

#include <compare>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>
#include <system_error>
#include <variant>
#include <vector>

namespace pulsetrace {

// Positive Linux PID representable by int; construction always validates input.
class ProcessId {
  public:
    [[nodiscard]] static std::optional<ProcessId> parse(std::string_view text) noexcept;
    [[nodiscard]] constexpr int value() const noexcept { return value_; }
    auto operator<=>(const ProcessId&) const = default;

  private:
    explicit constexpr ProcessId(int value) noexcept : value_{value} {}
    int value_;
};

enum class ScanOperation { open_root, inspect_entry, advance_iterator };
struct ScanError {
    ScanOperation operation;
    std::filesystem::path path;
    std::error_code code;
};
struct ProcessDiscovery {
    std::vector<ProcessId> pids;
    std::vector<ScanError> errors;
    std::size_t vanished_entries{0};
    // Complete traversal, not an atomic snapshot or a guarantee of liveness.
    [[nodiscard]] bool complete() const noexcept { return errors.empty(); }
};

// A root-open failure is distinct from a successfully opened, possibly partial scan.
using DiscoveryResult = std::variant<ProcessDiscovery, ScanError>;
[[nodiscard]] DiscoveryResult discover_processes(const std::filesystem::path& root = "/proc");

} // namespace pulsetrace
