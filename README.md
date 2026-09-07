# PulseTrace

A C++20 Linux process-monitoring and diagnostics project developed incrementally.
The goal is practical systems engineering: procfs, ownership, sampling correctness,
concurrency, explainable diagnostics, debugging and measured performance.

**Status: Milestone 0.** The executable provides help and version output. Process
monitoring, anomaly rules, networking and the dashboard are not implemented yet.

## Requirements and build

Linux, GCC or Clang with C++20 support, CMake 3.24+, and Ninja (or Make).
The current code has been verified with GCC 13.3.0 and CMake 4.4.3 on Linux.
There are no third-party C++ dependencies at this milestone.

Ubuntu 24.04 / Debian-family setup:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build git
```

From the repository root:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPULSETRACE_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/pulsetrace
./build/pulsetrace --version
./build/pulsetrace --help
```

Default output:

```text
PulseTrace 0.1.0
Usage: pulsetrace [--help | --version]
Linux process monitoring and diagnostics.
Milestone 0: build scaffold; monitoring is not implemented yet.
```

CTest currently runs one CLI contract script containing six cases. It checks exact
output and exit status for default/help/version, invalid options, extra arguments
and failed output. These are CLI integration checks, not monitoring unit tests.

Optional install without root:

```bash
cmake --install build --prefix "$HOME/.local"
"$HOME/.local/bin/pulsetrace" --version
```

For a separate Clang build, install clang then configure with a fresh build folder
and `-DCMAKE_CXX_COMPILER=clang++`. Never change compilers inside a configured tree.
Clang support is configured but was not verified in the initial environment.

## Engineering notes

See [architecture](docs/architecture.md) for boundaries, data flow, planned threading
and metric semantics; [milestones](docs/milestones.md) for acceptance criteria and
code-reading exercises; [engineering journal](docs/engineering-journal.md) for
verified evidence. Warnings belong to our target, not global compiler flags.
Build directories and generated headers are excluded from Git.

Benchmarks, screenshots and debugging case studies will be added when those
features and measurements exist. There are no performance or production-readiness
claims at this stage.
