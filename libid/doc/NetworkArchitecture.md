<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Parallel And Network Rendering

## Direction

Move from one active calculation on one CPU execution thread to workers
using multiple CPU cores, local processes, and networked machines.
Use a consistent ZeroMQ messaging boundary rather than a different
rendering architecture for each placement.

Apply [ports and adapters](hexagonal.md) first.  The engine should not
know whether work was requested by a local UI, a batch client, or a remote
coordinator.  It should not know whether its output reaches a window,
an image file, or a message transport.

This is an architectural direction, not an implementation schedule or a
commitment to particular message layouts, socket topology, or serializer.

## Starting Point

The application currently drives one active render operation.  Calculation
objects already advance incrementally, and keyboard/mouse input belongs
to UI controllers.  That provides useful return points for future worker
control without placing message handling inside calculation loops.

The separation is not yet sufficient for parallel execution:

- Calculation still uses shared global bindings and numeric temporaries.
- Some geometry and I/O code still depends on UI-owned state.
- The driver exposes input, presentation, and output in one broad API.
- Batch mode uses application orchestration; it is not inherently headless.

[Global variable inventories](GlobalsIndex.md) describe the current state.
[The input boundary](inversion-of-control.md) describes the completed
incremental calculation/UI split and its remaining limits.

Moving calculation to another thread can improve UI responsiveness.
It does not, by itself, parallelize an image or make multiple simultaneous
calculations safe.

## Establish The Hexagonal Boundary First

Separate application coordination, calculation, and external capabilities
before distributing them.

- Calculation owns algorithms, image state, work traversal, lifecycle,
  and calculation timing.
- Application coordination owns requests, scheduling, cancellation, and
  assembly of accepted results.
- UI adapters own user interaction and presentation.
- I/O adapters own persistence and external data loading.
- Output adapters implement the capabilities needed by calculation,
  without exposing keyboard, window, or transport operations.

This follows the dependency direction in [hexagonal.md](hexagonal.md):
UI uses engine and I/O; I/O uses engine; engine uses geometry.  ZeroMQ is
an adapter technology, not a new dependency of the numerical algorithms.

First resolve ownership of shared data and remove upward UI dependencies.
Then narrow driver capabilities into appropriate ports.  These changes
remain useful even if rendering never leaves the local process.

### Why This Simplifies Multiprocessing

A worker can compose the engine with noninteractive adapters instead of
constructing a GUI or emulating keyboard-driven startup.  Interactive and
batch clients can request the same calculation through different adapters.

Explicit render inputs become the data sent to a worker.  Calculation
state remains worker-owned, while output ports provide a natural boundary
for collecting results.  Diagnostics return as data rather than selecting
a dialog or terminating the application.

The same separation makes worker behavior testable without launching a
window or opening a network connection.  Transport failures can be tested
at the adapter boundary without changing fractal algorithms.

Hexagonal structure does not automatically remove global state or ensure
thread safety.  A separate process can isolate legacy process-wide state
while executing one job at a time.  Concurrent workers within one process
require independent calculation state and output bindings first.

Do not replace upward calls with interfaces that merely expose the same
UI operations to the worker.  The ports must express calculation needs.

## One Work And Result Model

The logical arrangement is:

```text
interactive UI or batch client
              |
              v
       render coordinator
              |
      ZeroMQ work/control
              |
              v
         render workers
              |
       ZeroMQ results
              |
              v
       render coordinator
              |
     presentation or storage
```

The coordinator decides what should be computed and where it may run.
Workers execute assigned calculation work and return results, progress,
completion, or diagnostics.  They retain state while a task is active;
they need not be internally stateless.

Requests need explicit render settings and identities for the image
generation and assigned task.  They must describe numerical values and
assets, not pointers, function addresses, arena bytes, or a dump of
globals.  Arbitrary-precision values must preserve their required
precision.

Formula, IFS, L-system, palette, and texture inputs need a defined asset
policy.  A filename on the submitting machine is not automatically a
usable filename on another machine.  Assets may be supplied with a job or
resolved through an agreed immutable identifier.

Results should identify their task and generation so the coordinator can
assemble them and reject stale or repeated output.  Runtime sockets and
worker identities are transport details, not part of fractal calculation.

Serialization is required across process boundaries.  Prefer one logical
protocol for local and remote workers; its encoding can be chosen later.
Local optimizations must not make pointer sharing a requirement of that
protocol.

## Worker Placement

Keep the work/result contract stable while changing execution placement:

- One local worker establishes the boundary and lifecycle without adding
  parallel scheduling.
- Multiple local processes can use additional CPUs while isolating legacy
  globals, provided each process runs only one calculation at a time.
- Multiple local threads can reduce process overhead once calculation
  state is genuinely independent.
- Remote workers add machines without introducing another rendering API.

ZeroMQ [in-process transport][inproc] connects threads sharing one context.
[TCP transport][tcp] supports communication between processes on the same
machine or across machines.  Local IPC is another possible adapter choice
where supported; it need not be a prerequisite for local multiprocessing.

Changing transport does not erase differences in latency, data movement,
failure, or deployment.  The reusable part is the calculation and logical
protocol, not an assumption that changing an endpoint string solves all
distributed-system requirements.

## Socket Patterns

The [ZeroMQ socket documentation][sockets] defines these patterns.
They are useful choices behind the work/control boundary, not decisions
that calculation classes should encode.

### Request And Reply

`REQ/REP` alternates requests and replies.  It suits an initial worker or
short control exchanges.  A long render request needs another control
arrangement if cancellation must arrive before its reply.

### Work Pipeline

`PUSH/PULL` distributes work among downstream peers rather than
broadcasting each task.  A return pipeline can collect results.
Assignment tracking, cancellation, and failure recovery still need an
application protocol.

### Addressed Asynchronous Exchange

`ROUTER/DEALER` permits asynchronous exchange with routing identities.
It suits explicit worker scheduling and directed control, with more
protocol responsibility than a simple pipeline.

### Publish And Subscribe

`PUB/SUB` broadcasts to subscribers and can drop messages.
It suits replaceable telemetry or previews, not the sole delivery path
for required tasks, cancellation, or final results.

These patterns can be combined.  Choose topology from the control and
delivery requirements, not from a desire to use every socket type.
Give sockets clear thread ownership rather than sharing ordinary sockets
between worker threads.

## Choose Real Units Of Parallel Work

Not every renderer can be divided into independent rectangular tiles.
Some calculations use neighboring image values, ordered trajectories,
random sequences, symmetry, or traversal-specific inference.

Start with demonstrably independent work.  Separate images or parameter
sets may be a simpler first target than dividing one image.  Within-image
parallelism needs an explicit algorithm-specific partition and merge
policy that preserves the intended calculation.

Keep image access and dependent intermediate state near the calculation.
Do not turn each pixel read, plot, or orbit iteration into a synchronous
remote call.  Batch results into useful spans, regions, or other
calculation-appropriate units.

Task size balances computation against messaging cost, load balancing,
and interruption latency.  Preserve existing useful work checkpoints;
do not increase input polling or synchronization to make distribution
appear more responsive.

Different machines may produce small floating-point differences.
Compatibility tests should use the appropriate image or semantic oracle
for each algorithm, rather than promise identical pixels universally.

## Control, Output, And Failure

Interactive changes create a new render generation.  The coordinator
requests cancellation of obsolete work and ignores late results from it.
Workers service control between bounded calculation increments; the
engine does not poll a ZeroMQ socket as a replacement for keyboard I/O.

Pause, cancellation, and resumability are distinct policies.  An operation
may support retaining local state without supporting migration of that
state to a different process or machine.

Bound outstanding work and result buffering so slow presentation or a
disconnected client cannot cause unlimited accumulation.  UI event
processing must remain responsive when transport queues are full.
Backpressure belongs at the coordination and adapter boundaries.

Preserve [sound output semantics](sound.md): interactive plotting and
audio remain coordinated, while file-only output need not run in real
time.  Network placement is not permission to drop samples, duplicate
plots, or silently change pacing.

ZeroMQ messaging does not establish that a worker completed a task.
Define acknowledgements, completion, worker loss, retry, and duplicate
handling before depending on recovery.  Retrying work must not duplicate
file writes or other externally visible effects.

Network deployment additionally needs protocol compatibility, resource
limits, authentication, and an explicit trust boundary for supplied
definitions and assets.  These are separate from proving local execution.

## Migration In Broad Strokes

1. Finish ownership boundaries and segregate the driver capabilities.
   Make calculation usable with noninteractive adapters.
2. Establish explicit requests, outputs, and lifecycle with one worker.
   Keep the existing single-worker behavior as the reference.
3. Introduce ZeroMQ adapters using the same work/result model.  Exercise
   completion, interruption, backpressure, and failure locally.
4. Add parallel workers only where state isolation and work partitioning
   are established.  Local processes and threads need not arrive in the
   same step.
5. Extend the same protocol to remote workers, adding asset transfer,
   compatibility checks, security, and recovery as explicit concerns.

Choose small implementation slices from the current code when each stage
is ready.  Do not combine boundary cleanup, parallel algorithm changes,
a new GUI, and network deployment into one refactoring.

## Open Decisions

The direction does not yet choose:

- Which calculation families are the first parallel targets.
- Process versus thread placement for the first background worker.
- Socket topology, protocol encoding, and asset packaging.
- Task sizes, buffering limits, and result-composition policy.
- Worker discovery, deployment, recovery, and security mechanisms.

The architectural benefit is a single calculation core with explicit
inputs and outputs.  ZeroMQ then connects those boundaries across
execution locations instead of requiring a new engine for every one.

[inproc]: https://libzmq.readthedocs.io/en/latest/zmq_inproc.html
[tcp]: https://libzmq.readthedocs.io/en/latest/zmq_tcp.html
[sockets]: https://libzmq.readthedocs.io/en/latest/zmq_socket.html
