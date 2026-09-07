#pragma once

#include "pulsetrace/linux/process_discovery.hpp"

namespace pulsetrace::detail {
// Internal operation shared by traversal and deterministic vanished-entry tests.
void inspect_candidate(const std::filesystem::path& path, ProcessId pid,
                       ProcessDiscovery& result);
} // namespace pulsetrace::detail
