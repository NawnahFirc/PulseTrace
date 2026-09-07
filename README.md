# PulseTrace

A C++20 Linux process-monitoring and diagnostics project developed incrementally.
The goal is practical systems engineering: procfs, ownership, sampling correctness,
concurrency, explainable diagnostics, debugging and measured performance.

**Status: Milestone 1, version 0.2.0.** The CLI discovers visible PIDs. Process
metadata, CPU/memory metrics, rules, networking and the dashboard are future work.

## Requirements and build

Linux, GCC or Clang with C++20 support, CMake 3.24+, Ninja (or Make). Verified with
GCC 13.3.0, CMake 4.4.3 and Catch2 3.8.1. Clang is not yet verified. No third-party
runtime dependency; Catch2 is used only for tests.

Ubuntu 24.04 setup:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git
```

Clone PulseTrace and explicitly obtain the test dependency alongside it:

```bash
git clone https://github.com/NawnahFirc/PulseTrace.git
git clone --depth 1 --branch v3.8.1 https://github.com/catchorg/Catch2.git Catch2
cd PulseTrace
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DPULSETRACE_WARNINGS_AS_ERRORS=ON \
  -DPULSETRACE_CATCH2_DIR="$PWD/../Catch2/extras"
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/pulsetrace --version
./build/pulsetrace --list-pids
```

For an existing clone, use `git pull --ff-only` instead of cloning it again. The
Catch2 path must contain upstream catch_amalgamated.hpp and catch_amalgamated.cpp.
CMake does not download dependencies. A production-only build can use
`-DBUILD_TESTING=OFF` without Catch2.

Expected version: `PulseTrace 0.2.0`. PID output is ascending integers, one per line;
actual values vary on every machine and can change during scanning.

Controlled example from the repository root:

```bash
fixture_dir=$(mktemp -d)
mkdir "$fixture_dir/20" "$fixture_dir/3" "$fixture_dir/self"
./build/pulsetrace --list-pids --proc-root "$fixture_dir"
rmdir "$fixture_dir/20" "$fixture_dir/3" "$fixture_dir/self" "$fixture_dir"
```

Expected output:

```text
3
20
```

Exit status: 0 successful scan; 1 operational failure (partial output is possible);
2 invalid usage. Disappearing candidates are counted on stderr. This is a best-effort
view of the mounted proc filesystem, not an atomic snapshot or host-wide guarantee.

## Tests and sanitizers

CTest runs a Catch2 suite (nine cases) and a CLI contract script (nine scenarios).
Run `./build/process_discovery_tests` to see the detailed counts and any skips.
The permission-denial case skips as root: run tests unprivileged for that coverage.

```bash
cmake -S . -B build-sanitize -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DPULSETRACE_WARNINGS_AS_ERRORS=ON \
  -DPULSETRACE_SANITIZERS=ON \
  -DPULSETRACE_CATCH2_DIR="$PWD/../Catch2/extras"
cmake --build build-sanitize --parallel
UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build-sanitize --output-on-failure
```

In the development sandbox, LeakSanitizer could not open /proc/PID/task and aborted.
The ASan/UBSan-only verification used the following environment override. Do not use
it as evidence of a passing leak check:

```bash
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build-sanitize --output-on-failure
```

The sanitizer option instruments project code; the external Catch2 implementation
is built separately. ThreadSanitizer is deferred until concurrency exists.

## Design and debugging

- [Architecture](docs/architecture.md): component boundaries, data flow and planned threading.
- [Milestone 1 guide](docs/milestone-1.md): code walkthrough, ownership, complexity,
  actual failed tests and fixes, limitations and an exercise.
- [Milestone plan](docs/milestones.md): acceptance criteria and next steps.
- [Engineering journal](docs/engineering-journal.md): evidence we actually produced.

Optional install: `cmake --install build --prefix "$HOME/.local"`.
Warnings are target-scoped. Generated headers and build outputs are not committed.
Screenshots, benchmarks and deeper debugging exercises will be added when they
exist. No measured performance or production-readiness claims are made yet.
