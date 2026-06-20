<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Release 1.5 Scope

Version 1.5 is an interim release on the way to version 2.0. It should
carry a small set of completed, reviewable architecture and performance
work.

## Included Work

### [`g_calc_type` Correctness](calc_type-divergence.md)

Protect the distinction between static fractal table metadata and the
runtime pixel calculator.

Release boundary:

- Add tests or invariants for Mandel/Julia optimized selection, show-dot
  wrapping, stale values before setup, and alternate-math behavior where
  practical.
- Use `g_cur_fractal_specific->calc_type` only for static capability
  checks.
- Use `g_calc_type` only after setup when runtime calculator identity is
  needed.

### [Polling I/O Refactor](inversion-of-control.md)

Move direct keyboard and mouse polling out of calculation code and into
`libid/ui`, preserving current behavior.

Release boundary:

- Complete slices 1-14.
- Direct `Driver` key calls no longer appear under `libid/engine` or
  `libid/fractals`.
- Inverse-Julia mouse handling is owned by `libid/ui`.
- Keep future event-driven and tool-stack work out of 1.5.

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
- Broad global-state refactoring outside the included work.
