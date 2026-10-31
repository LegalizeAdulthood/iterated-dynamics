<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Global Variable Inventories

These inventories describe public `g_*` declarations in the current
`libid/include` subsystem headers.  They group related settings and working
values, identify declaration ownership, and explain important units and
lifetimes.

They are implementation references, not refactoring plans.  Header
ownership is not proof of a clean dependency boundary, thread isolation,
or exclusive use by the declaring subsystem.

## Documents

- [EngineGlobals.md](EngineGlobals.md) - display and coordinates,
  calculation status/timing, iteration, coloring, work lists, plot
  bindings, sound output, parameters, and shared render configuration.
- [FractalGlobals.md](FractalGlobals.md) - immutable specifications,
  active dispatch, compiled formula/runtime state, loaded definitions,
  orbit configuration, Julibrot, and remaining fractal-specific globals.
- [GeometryGlobals.md](GeometryGlobals.md) - transforms, lighting,
  surface rendering, Targa/ray output, stereo projection, and stereogram
  inputs.
- [IOGlobals.md](IOGlobals.md) - image metadata/application, save policy,
  scanline output, palette loading, disk storage, paths, video catalog
  configuration, and autosave.
- [MathGlobals.md](MathGlobals.md) - numeric representation, arena
  allocation, temporaries, bounds/parameters, interpreter bindings,
  overflow, and arithmetic contracts.
- [MiscGlobals.md](MiscGlobals.md) - debug selection, active driver,
  compatibility version, and stack diagnostics.
- [UIGlobals.md](UIGlobals.md) - application coordination, pending video
  requests/key bindings, presentation, input tools, history, evolution,
  and stereogram/parameter-output policy.

Scope follows declaration directories.  For example, `g_help_mode` is
declared under `ui` but uses namespace `id::help`.  Read-only `g_*`
tables are identified as such.  Non-`g_*` constants, private file state,
and individual instance members are outside the exhaustive name listing;
selected members and private state are discussed to explain ownership.

## Important Ownership Boundaries

- `fractals::g_fractal_specific` is const specification data.
  `fractals::g_dispatch` is the mutable active dispatch binding.
- `fractals::g_formula` holds compiled formula data;
  `fractals::g_runtime` holds interpreter stack and execution state.
- `engine::g_video_entry` and `g_video_table` contain key-free mode data.
  `ui::g_init_mode`, `g_video_mode_keys`, and `g_video_entry_key` own
  pending requests and key associations.
- `engine::g_dac_box` is the active palette;
  `io::g_map_clut` is the saved/default map palette.
- `engine::g_sound` is the active output port.  UI owns the configured
  adapter, sound-session lifetime, playback servicing, and waits.
- `io::g_row_count` and disk-storage flags are I/O state, not UI video
  declarations.
- `io::EvolutionInfo` is a persistent record type; evolution controls,
  gene bindings, and retained application state remain UI-owned.
- Calculation timing remains engine-owned.  `g_calc_time` and
  `g_timer_interval` use hundredths of a second;
  `g_engine_timer_start` uses `std::clock()` ticks.

## State Already Held By Objects

Global inventories do not describe all calculation state.
`StandardFractal` owns its active pass, pixel, dispatch snapshot, and
perturbation engine.  Concrete pass objects retain traversal and fill
state.  `OrbitPlot` owns saved overlay pixels.

Standalone fractal objects retain incremental calculation state.
`ImageRenderOperation` selects calculation operations; UI render
controllers own interaction and presentation.  Commands, GIF decoding,
GIF encoding, image saving, and sound sessions also have explicit objects
instead of being represented only by public globals.

The completed input split does not imply that every non-UI module is free
of UI dependencies.  Geometry cleanup, evolution metadata, video catalog
bindings, and autosave still have upward coupling.

## Precision And Memory

Arbitrary precision uses one static 1 MiB arena, not a separate heap
allocation for every temporary.  Working values, scratch storage, and
persistent bounds/parameters share it.  Increasing capacity did not
remove the supported precision limits.

Disk pixel storage has a configured 4096 KiB cache.  Containers, buffers,
active calculation objects, and driver allocations contribute additional
memory.  No total-memory estimate is maintained here; a count of public
globals would not establish runtime memory use.

## Concurrency And Testing

Global bindings, numeric temporaries, palette state, and pixel working
values remain shared mutable state.  Incremental calculation and a
UI-owned polling loop do not make concurrent in-process renders safe.
A shared pointer protects an output object's lifetime, not its thread
safety or completion protocol.

Tests use fixtures, scoped state restoration, and replaceable output
bindings to isolate legacy state.  Changing ownership should preserve
existing calculation results and interaction cadence, not introduce
per-pixel synchronization merely because state is moved into an object.

## Related Documentation

- [Ports And Adapters](hexagonal.md) - target dependencies and incremental
  ownership/interface segregation.
- [Polling I/O Boundary](inversion-of-control.md) - implemented calculation
  and UI separation, current coupling, and event-driven follow-ups.
- [Sound Output Port](sound.md) - samples, injection, session ownership,
  readiness, and interactive semantics.
- [Debug Flags](../../home/doc/debug-flags.md) - supported numeric debug
  selections.
- [Style](../../Style.md) - coding conventions.
- [ReadMe](../../ReadMe.md) - project overview and build instructions.
