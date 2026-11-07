<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Release 1.5 Scope

Version 1.5 is an interim release on the way to version 2.0. It should
carry a small set of completed, reviewable architecture and performance
work.

## Completed Work

### [Polling I/O Refactor](inversion-of-control.md)

Calculation objects perform incremental work under UI controllers.
Keyboard and mouse interaction belongs to `libs/ui` and platform drivers,
not calculation code.  Existing polling behavior is retained.

- Standard rendering, FrothyBasin, and the other calculation types use
  the controller/calculation-object structure.
- JIIM keyboard and mouse interaction is owned by UI.
- Keyboard handler-stack support is implemented; consistent use across
  all contexts and centralized input delivery remain follow-up work.
- Core and UI are separate `id::core` and `id::ui` libraries, with separate
  test executables.  UI depends on core; core does not include UI headers
  or call keyboard polling functions.
- The broad driver interface still exposes input and output capabilities;
  splitting it into focused ports is not a release 1.5 prerequisite.

## Remaining Work

### [SIMD Phase 1](simd-highway.md)

The first SIMD path for standard escape-time rendering is not yet
implemented.  The calculation/pass boundaries are in place, but option
handling, grid views, Highway integration, and kernels remain to be added.

Release boundary:

- Complete Phase 1 slices 1-8, including all required sub-slices:
  option handling, eligibility reporting, grid views, scalar batch
  reference, Highway integration and kernel, and one-/two-pass activation.
- Keep slice 9 diagnostics optional.
- Defer slice 10 eligibility expansion.
- Support only the narrow Mandelbrot/Julia, `passes=1|2`, double-math
  eligibility set, with `periodicity=0` and basic coloring.
- Keep unsupported states on existing scalar paths under `simd=auto`;
  reject unsupported forced requests with a specific reason.
- Default `passes=g` remains scalar in this phase.
- Validate scalar-equivalent output, suspension/resumption, interaction
  cadence, platform builds, and optimized-build performance.

## Explicitly Deferred

- SIMD Phase 2 IFS rendering.
- SIMD Phase 3 non-standard renderers.
- BF formula parser work.
- Network and distributed rendering.
- Modal dialog conversion.
- Consistent handler-stack use and centralized application input dispatch.
- Event-driven GUI dispatch and worker-thread rendering.
- Tool-stack work and replacement orbit-inspection UI.
- Broader ports-and-adapters migration and driver interface segregation.
- Broad global-state refactoring outside the included work.
