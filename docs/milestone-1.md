# Milestone 1: Linux process discovery

PulseTrace 0.2.0 adds one synchronous operation: list numeric process directories
visible through a proc root. No process metadata, sampling thread, or diagnostics
engine has been implemented.

## Read the code in this order

1. `include/pulsetrace/linux/process_discovery.hpp`: the owned result and error model.
2. `src/linux/process_discovery.cpp`: name parsing, entry inspection, traversal.
3. `src/main.cpp`: CLI formatting and exit status.
4. `tests/process_discovery_test.cpp`: controlled filesystem cases and live smoke test.

`pulsetrace_core` is now a static library used by both the CLI and tests. The public
header exposes a small function, not a stateful ProcReader class: this operation
has no retained state to justify one. `src/linux/discovery_detail.hpp` is internal;
it exposes the candidate inspection step to tests without adding a public callback
or a virtual filesystem layer.

## Important C++ decisions

`ProcessId` has a private constructor and a parsing factory returning optional.
Invalid text cannot create a PID; zero, signs, spaces, non-ASCII digits, trailing
characters and int overflow are rejected. `from_chars` does not allocate or throw
for invalid input. Its structured binding exposes both conversion progress and the
error, so accepting only a prefix is impossible. Leading zeros are accepted and
numeric duplicates are removed after sorting.

`DiscoveryResult` is a variant because opening the root either fails with ScanError
or produces ProcessDiscovery. A successful open may still yield a partial scan,
whose errors preserve the operation, owned path and error_code. This distinguishes
an empty successful scan, a partial scan and a failed open without magic numbers.

The directory_iterator owns traversal resources through RAII. The loop borrows an
entry path only until iterator advancement. Results contain owned PID values and
owned error paths, so none of these references escape. Returning the local result
uses value/move semantics; writing `std::move(result)` is unnecessary.

Filesystem operations use error_code overloads for expected OS failures. This is
not an allocation-failure guarantee: vector/path allocation can still throw, so the
filesystem API is deliberately not marked noexcept. Only PID parsing is noexcept.

`symlink_status(path, error)` checks the path without following a final symlink.
The free function avoids depending on directory_entry's potentially cached status.
This is a best-effort observation: a process can disappear immediately afterwards.
A custom root is a trusted configuration input, not a security sandbox boundary.

We sort a contiguous vector rather than maintain a tree container during scanning.
Cost is O(D + P log P), plus total filename characters inspected, where D is the
number of root entries and P is the number of accepted numeric directories. Memory
is O(P + E), including owned paths for E errors. There is one status query per
numeric candidate and batched directory enumeration internally; no syscall or
performance measurements have yet been made. Do not present complexity as a benchmark.

## Behavior

`--list-pids` writes one ascending numeric PID per line, without a header, suitable
for shell pipelines. `--list-pids --proc-root PATH` uses a fixture or another mounted
proc view. Only this argument order is accepted at this milestone.

Exit 0: completed traversal and successful stdout write. Vanished candidates are
counted on stderr and do not fail the scan. Exit 1: root-open failure, incomplete
scan, or detected stdout failure. Exit 2: invalid arguments. Partial PIDs are still
written on an incomplete scan; consumers must check exit status. A complete scan
means traversal completed without reported errors, not that the set is atomic.

Names that are nonnumeric, numeric regular files, and final-component symlinks are
ignored. A root-open error is never converted to an empty success. PIDs can be
reused; this milestone does not yet claim process identity across samples.

## Real debugging record: incorrect error classification

1. Symptom: the unexpected-inspection-error fixture reported complete=true.
2. Reproduction: create a regular file named `file`, then inspect `file/17`.
   Run `./build/process_discovery_tests 'Unexpected inspection errors*'`.
3. Hypothesis: the missing-file classification also covers a malformed parent path.
4. Investigation: the failed Catch2 assertion led to inspection of the status/error
   branch. For this case libstdc++ provided not_found together with ENOTDIR.
5. Tools used: Catch2 assertion output, source inspection, rebuild and regression run.
   GDB, strace and perf were not used for this issue.
6. Root cause: checking not_found regardless of error_code discarded ENOTDIR.
7. Fix: classify ENOENT as vanished; only use a not_found status without an error.
   Preserve other errors as incomplete-scan evidence.
8. Regression: the fixture checks retained prior PIDs, ENOTDIR, path, operation,
   incomplete status and zero vanished entries. It failed before the correction.
9. Performance: no benchmark was justified for this error-branch correction; no
   performance improvement is claimed.

A second failed smoke assertion exposed an environment assumption: Python reported
getpid()=7 while /proc/self resolved to 33742 in this execution environment. The
live test now obtains self identity from the mounted proc view. We observed the
mismatch; we did not establish its underlying sandbox implementation.

## Verification and remaining limits

Catch2 3.8.1 replaces the originally proposed GoogleTest after package installation
was unavailable in the development environment. It was already an allowed framework.
Its upstream amalgamation is supplied explicitly outside this repository; CMake
never fetches it. Only test binaries link it.

Nine Catch2 cases cover parsing, empty roots, filtering/sorting/deduplication,
root failures, vanished entries, inspection errors, permissions, ownership and live
proc. The CLI contract adds controlled-root output, missing-root and argument checks.
The permission test explicitly skips when geteuid()==0; rerun unprivileged on Linux.
A deterministic iterator-increment I/O failure is not injected yet; this remains a
coverage limit. The vanished test targets the actual inspection function after a
fixture removal, not a probabilistic concurrent deletion.

Build and test commands are in README.md. ASan detects invalid memory accesses;
UBSan detects instrumented undefined operations. They cannot prove absence of bugs.
LeakSanitizer failed in this environment opening /proc/PID/task. The local ASan/UBSan
rerun disables only leak detection through an environment variable; project defaults
are unchanged. Leak detection and unprivileged permission behavior remain unverified.
No deliberate unsafe debugging exercise or multithreaded implementation is included.

## Your exercise

Create a fixture with directories 20, 003, 3, self and 0, a regular file 7, and a
symlink 8 pointing to 20. Predict the output before running --list-pids --proc-root.
Explain why deleting 20 afterwards cannot invalidate the returned vector, but can
make a later metadata read fail. Then explain why ENOTDIR should not be counted as
a disappearing process.

Next milestone: parse /proc/PID/stat metadata, including process names with spaces
and parentheses, while preserving missing-data and process-lifetime semantics.
