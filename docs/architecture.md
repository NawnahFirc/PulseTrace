# Architecture proposal

This document describes the intended engine. Milestone 1 implements synchronous
process discovery in pulsetrace_core, a PID-listing CLI, and fixture tests.
See milestone-1.md for the current implementation and its limits.

## Components and boundaries

| Component | Responsibility | Ownership / boundary |
| --- | --- | --- |
| ProcReader (linux) | Discover numeric PID directories and read proc files, with an injectable root path | Owns file handles through RAII; returns owned text and explicit read errors |
| ProcParser (linux) | Convert text into typed raw counters and metadata | Pure functions; no filesystem access or retained string views |
| Metric calculations (core) | Derive CPU deltas, bytes, elapsed time and system summaries | Value types; explicit units; no terminal or filesystem dependencies |
| MetricsCollector (core) | Coordinate one sampling pass and retain previous counters | Sole owner of sampling history keyed by PID and start-time ticks |
| DiagnosticsEngine (diagnostics) | Evaluate deterministic rules over valid samples and bounded history | Sole owner of rule timers; returns evidence, duration and severity |
| SnapshotStore (core, concurrency milestone) | Publish completed metric and diagnostic snapshots | Mutex-protected latest value; no unbounded queue |
| TerminalUI (cli) | Format snapshots, handle terminal input and render status | Main thread; never reads proc files |
| Application / main | Parse configuration, compose components, own lifetime and shutdown | Stack/value ownership by default |
| Optional server (networking) | Serialize published snapshots for clients | Later bounded I/O worker; never initiates collection |

These are responsibilities, not a requirement for one class per row. Parsers,
calculations and individual rules should be functions when no state is needed.
No EventBus, inheritance hierarchy, singleton logger or general plugin framework.
The core library target was introduced in Milestone 1 for reusable discovery.

## Data flow

1. Reader discovers visible processes and reads process/system files.
2. Pure parsers validate fields and produce raw values or structured errors.
3. Collector combines current and previous counters into a timestamped snapshot.
4. Diagnostics evaluates this sample and adds explainable events.
5. A completed snapshot is published; UI and optional server consume it.

A sampling pass is not an atomic view of Linux. Record monotonic start/end times
and incomplete-read information. Proc entries can vanish between discovery and
read. Permission denial, malformed data and disappearance are different outcomes;
none should become an invented zero. A missing proc root is a collection error.

## Metric semantics to implement and test

- Process identity is (PID, start-time ticks), not PID alone. Reused PIDs reset
  baselines and rule timers.
- First CPU samples are unavailable until a valid delta exists. Invalid time or
  counter regression invalidates the delta. Process CPU uses one-core semantics
  (100% is one fully occupied logical CPU; a multithreaded process can exceed it).
- System CPU uses aggregate counter deltas and a documented idle policy. Avoid
  double-counting guest counters. Define iowait treatment before implementing.
- RSS is resident memory, not exclusive physical ownership. System used RAM will
  use MemTotal minus MemAvailable, with unavailability reported explicitly.
- Process age comes from start-time ticks, clock tick frequency and system uptime;
  scheduling/rule durations use steady_clock, not the adjustable wall clock.
- FD counts are optional and permission-sensitive. Restrict expensive detail
  sampling if measurement later justifies it.
- Process disappearance means no longer observed in this proc view; it does not
  prove a crash. Thread/FD growth is evidence of possible resource growth, not a
  confirmed leak. Sleeping and unchanging counters alone do not prove a stall.
- Rules need thresholds, valid observation durations, hysteresis and reset policy.
  Missing samples must not silently count as sustained threshold violations.
- Output must escape process-controlled control characters before terminal display.
- Container /proc visibility and permissions bound what can be observed. Do not
  promise host-wide coverage or request root by default.

## Eventual threading

Start single-threaded. Introduce concurrency only after synchronous sampling works.
One std::jthread runs collection and diagnostics, keeping mutable history private.
The main thread renders completed snapshots. A mutex protects snapshot replacement
and a short copy-out; neither rendering nor proc I/O holds it. Begin with value
copies, then measure whether immutable shared snapshots are worth their ownership
cost. No speculative shared_ptr or atomics.

std::jthread supplies cooperative stop requests and joining through RAII. It does
not interrupt arbitrary blocking I/O. Use a stop-aware timed condition-variable wait
so shutdown need not wait for a full polling period. Use monotonic deadlines; skip
missed periods rather than spinning to catch up. Request stop and join workers
before destroying the store or other objects they reference.

At the signal milestone, block SIGINT/SIGTERM before worker creation and integrate
Linux signalfd with a pollable main loop. This avoids logging, allocating or locking
inside asynchronous signal handlers. Only the optional network milestone gets a
separate bounded I/O worker; no thread per PID and no thread per rule.

## Proposed repository growth

```text
PulseTrace/
  CMakeLists.txt
  cmake/version.hpp.in
  src/main.cpp
  src/core/              # calculations, collector, eventual snapshot store
  src/linux/             # proc reader and parsers
  src/diagnostics/       # rules and engine
  src/cli/               # eventual rendering and arguments
  src/networking/        # optional, later
  include/pulsetrace/    # reusable public headers as they become necessary
  tests/                # CLI check now; unit tests and fixtures from M1
  benchmarks/           # introduced with measurable engine operations
  docs/                 # architecture, milestones, evidence
  examples/             # reproducible workloads when monitoring exists
  scripts/              # added only for actual repeatable workflows
  Dockerfile            # packaging milestone; container visibility documented
  .clang-format
  .editorconfig
  .gitignore
  README.md
```

Directories marked later are intentionally not empty scaffolding in Git.
Testing and sanitizers accompany implementation; they are not postponed to M9.
