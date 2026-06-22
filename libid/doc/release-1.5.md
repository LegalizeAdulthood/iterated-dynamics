<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Release 1.5 Scope

Version 1.5 is an interim release on the way to version 2.0. It should
carry a small set of completed, reviewable architecture and performance
work.

## Included Work

### [Polling I/O Refactor](inversion-of-control.md)

Move direct keyboard interaction out of engine calculation code and into
`libid/ui`, preserving current polling behavior.  Also move remaining JIIM
input ownership out of engine code and into `libid/ui`.

Release boundary:

- Complete the keyboard handler-stack slices needed for engine code to stop
  interpreting keys directly.
- Only `libid/ui` directly interacts with keyboard and mouse input.
- Direct `Driver` key calls no longer appear outside `libid/ui` and driver
  classes.
- Remaining JIIM keyboard and mouse input code is owned by `libid/ui`.
- Keep modal dialogs, event dispatch, worker threading, and tool-stack work
  out of 1.5.

### [SIMD Phase 1](simd-highway.md)

Add the first SIMD path for standard escape-time rendering.

Release boundary:

- Complete Phase 1 slices 1-8.
- Keep slice 9 diagnostics optional.
- Defer slice 10 eligibility expansion.
- Support only the narrow Mandelbrot/Julia, `passes=1|2`, double-math
  eligibility set.
- Keep unsupported states on existing scalar paths.

## Explicitly Deferred

- SIMD Phase 2 IFS rendering.
- SIMD Phase 3 non-standard renderers.
- BF formula parser work.
- Network and distributed rendering.
- Modal dialog conversion.
- Event-driven GUI dispatch and worker-thread rendering.
- Broad global-state refactoring outside the included work.
