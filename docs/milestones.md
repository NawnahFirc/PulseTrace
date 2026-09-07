# Milestones and learning checkpoints

## Milestone 0: reproducible Linux C++20 executable

Scope: Linux/GCC/Clang configuration gates; target-scoped C++20 without GNU language
extensions; aggressive warnings and optional -Werror; out-of-source builds;
CMake-generated version; help/version/default and invalid argument behavior;
CTest CLI contract; install target; formatting policy; architecture documentation.

Acceptance: configure and compile a Debug build with -Werror; no diagnostics;
run executable; CTest verifies exit codes and stdout/stderr independently,
including output failure. No monitoring, dependencies, worker threads or network.
CMake script testing is enough for this CLI boundary; Catch2 enters with M1 (the original GoogleTest choice was revised).

Code reading:
- target_compile_features declares the target's language requirement. PRIVATE
  prevents exporting implementation options to consumers. CXX_EXTENSIONS OFF
  requests standard C++20 rather than gnu++20 on this compiler.
- configure_file makes project(VERSION) the version's single source of truth.
  Generated headers live in the build tree and must not be edited manually.
- string_view borrows argv text for comparisons without allocations. argv lives
  throughout main; no borrowed reference escapes. There is no owning raw pointer.
- inline constexpr version has immutable static storage. It is not mutable global
  state. No class or heap allocation is needed for this milestone.
- main returns 0 on success, 2 on invalid usage, 1 on detected output failure.
  Explicit flush exposes buffered write failures before choosing the exit status.

Exercise: change project VERSION to 0.1.1, rebuild and run --version. Explain why
editing the generated header is the wrong fix. Restore 0.1.0 afterwards. Then
explain why --help extra fails, and why borrowing argv here is safe.

Commits: build: initialize Linux C++20 executable and CMake configuration;
test: verify command-line output and exit contracts;
docs: define architecture and incremental development plan.

## Milestone 1: process discovery (implemented; validation limits documented)

Scope: reusable core target; Linux process discovery with explicit injectable
std::filesystem::path root defaulting to /proc; small PID value type with validated
positive range; CLI --list-pids; Catch2 fixture tests. No metadata or CPU yet.

Implementation policy:
- Inspect root entries; accept only directory names made entirely of ASCII digits,
  converting with from_chars and checking full consumption and overflow.
- Ignore self, thread-self, nonnumeric names, regular files and symlink entries.
- Sort ascending, deduplicate numeric IDs; do not rely on directory iteration order.
- Return owned values. Report a missing/unreadable root as a failure, not an empty
  successful scan. Individual vanished entries are skipped and counted; other
  traversal failures report an incomplete result with explicit errors.
- Listing PIDs does not guarantee they remain alive on return. Never open metadata
  just to make discovery appear stable.
- No abstract filesystem interface yet: temporary directory fixtures and an injected
  root provide the needed testability without a virtual hierarchy.

Acceptance tests: empty root; valid unordered PIDs; zero; nonnumeric and mixed
names; integer overflow; symlinks; numeric regular files; missing root; per-entry
vanish/error policy; valid output sorted and unique. Permission tests must run as
an unprivileged user or explicitly skip with reason when run as root. Live /proc
smoke test must not assert a fixed process count. Compile warning-clean and run
unit tests under AddressSanitizer/UndefinedBehaviorSanitizer where supported.

Dependency: Catch2 3.8.1, supplied explicitly rather than silently downloaded by
CMake. See README.md for explicit dependency setup. CTest drives both
unit and CLI tests. This dependency improves fixture/assertion quality.

Exercise: why can a PID disappear immediately after discovery? Why is a vector
of owned IDs a safer boundary than retaining directory-iterator references?
Suggested commits: feat: add /proc process discovery; test: cover process discovery
fixtures and transient entries.

## Subsequent order

2. Metadata parsing with adversarial fixtures (including spaces/parentheses in names).
3. CPU/memory calculations, units, PID reuse and missing data.
4. Synchronous periodic sampling and monotonic timing.
5. Safe concurrency, stop/join behavior, race tests and ThreadSanitizer.
6. Explainable stateful diagnostics with simulated time.
7. Terminal rendering with output escaping and non-TTY behavior.
8. Logging and graceful signal integration.
9. Deeper debugging exercises and accumulated test review (tests start at M0).
10. Benchmarks, syscall counts and profiling based on repeatable workloads.
11. Optional metrics API with bounded resources.
12. Packaging, Docker, screenshots and documentation polish.

Unsafe debugging exercises will live in explicitly isolated exercise directories or
branches, excluded from normal builds. Each records symptoms, reproduction,
hypothesis, investigation/tools, root cause, fix, regression test and measured
performance impact when relevant. Do not claim an exercise has occurred in advance.

Milestone 1 implementation and actual validation limits are recorded in [the guide](milestone-1.md).
