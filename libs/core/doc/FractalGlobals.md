<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Fractal Global Variables

Inventory of public `g_*` declarations in `libs/core/include/fractals`.
Calculation objects retain additional instance state; these declarations
are not a complete inventory of rendering state.

## Fractal Definitions And Active Dispatch

Headers: `fractype.h`, `fractalp.h`.

- `g_fractal_type` - selected `FractalType`.
- `g_cur_fractal_specific` - pointer to the selected const specification.
- `g_fractal_specific` - const table of fractal specifications.
- `g_num_fractal_types` - number of specifications.
- `g_alternate_math` - const alternate-numeric-implementation table.
- `g_more_fractal_params` - const extended parameter descriptions.
- `g_dispatch` - mutable `FractalDispatch` with the active orbit,
  per-pixel, per-image, and calculation callbacks.

Specifications supply defaults and supported operations.  Runtime
overrides update `g_dispatch`, not `g_fractal_specific`.
`set_fractal_type()` selects the specification, initializes dispatch, and
clears its runtime calculation override.  Callers use dispatch operations
such as `calc_type()`; the old `g_calc_type` global is gone.

## Formula Compilation And Execution

Headers: `formula.h`, `parser.h`, `interpreter.h`.

- `g_formula_filename`, `g_formula_name` - selected formula source.
- `g_frm_is_mandelbrot` - current formula Mandelbrot/Julia intent.
- `g_formula` - `CompiledFormula` containing source text, function-pointer
  operations, load/store tables, variables, jumps, compilation indices,
  operation counts, and formula-use flags.
- `g_runtime` - `RuntimeState` containing the operand stack, execution and
  initialization indices, and formula random-number state.

Former standalone globals such as operation/load/store indices and
`uses_p1` through `uses_p5` are now members of these objects.
The read-only `VARIABLES` name table in `interpreter.h` is not a mutable
`g_*` binding.

Formula compilation and interpretation still use double-valued
`math::Arg` storage.  Engine inputs are copied into formula variables;
evaluation publishes specific orbit results, including `g_old_z` and
`g_new_z`.  A formula does not have unrestricted access to arbitrary
application globals.  Parser lookup and compilation report diagnostics
without entering UI.

## IFS Definitions

Header: `ifs.h`.

- `g_ifs_filename`, `g_ifs_name` - selected IFS source.
- `g_ifs_definition` - loaded transformation coefficients.
- `g_ifs_dim` - definition dimension.
- `g_num_affine_transforms` - transformation count.

`IFS2D` and `IFS3D` retain orbit and rendering state in calculation
objects.  The definition dimension selects the operation; `type=ifs3d`
remains compatibility syntax.  UI controllers own input and stereo
presentation, not coefficient parsing or orbit math.

## L-System Definitions

Header: `lsystem.h`.

- `g_l_system_filename`, `g_l_system_name` - selected definition source.
- `g_max_angle` - number of angular divisions, not a maximum angle in
  degrees.

Rules and the axiom are cached internally by the loader.  `LSystem`
retains sizing/drawing traversal state; its UI controller owns thinking
screens and interruption.  Definition errors are reported without UI.

## Orbit And 3D Configuration

Header: `lorenz.h`.

- `g_display_3d` - 3D display mode.
- `g_orbit_corner` - orbit image region.
- `g_orbit_interval` - orbit point plotting interval, not file-record
  spacing.
- `g_orbit_save_flags` - orbit-output flags, including `OSF_RAW` and
  `OSF_MIDI`.
- `g_orbit_save_name` - orbit data filename.
- `g_set_orbit_corners` - explicit orbit-corner flag.
- `g_keep_screen_coords` - retain screen-coordinate settings.
- `g_max_count` - orbit iteration limit.
- `g_major_method` - inverse-Julia traversal method.
- `g_inverse_julia_minor_method` - inverse-Julia branch ordering.

`Orbit2D`, `Orbit3D`, and `Dynamic2D` own incremental calculation state.
Sound samples go to `engine::g_sound`; optional legacy tone recording
belongs to the sound adapter.  `OSF_MIDI` names a text tone log, not a
MIDI encoder.  See [sound.md](sound.md).

## Julibrot

Header: `julibrot.h`.

- `g_julibrot` - Julibrot rendering flag.
- `g_julibrot_3d_mode` - mono/eye/anaglyph viewing mode.
- `g_julibrot_x_min`, `g_julibrot_x_max`, `g_julibrot_y_min`,
  `g_julibrot_y_max` - parameter-plane bounds.
- `g_julibrot_width`, `g_julibrot_height`, `g_julibrot_depth` - 3D
  dimensions.
- `g_julibrot_dist`, `g_julibrot_origin` - viewing configuration.
- `g_julibrot_z_dots` - depth sampling count.
- `g_eyes` - stereo eye separation.
- `g_new_orbit_type` - selected orbit type.
- `g_save_dac` - palette-save behavior.
- `g_julibrot_3d_options` - viewing-mode option strings.

Standard-4D calculation state resides in its calculation object.
`ui::Standard4DController` owns interaction and palette preparation.

## Other Fractal State

Headers: `Cellular.h`, `FrothyBasin.h`, `divide_brot.h`,
`peterson_variations.h`, `population.h`.

- `g_cellular_next_screen` - Cellular next-screen/continuous-generation
  request.
- `g_frothy_basin` - shared `FrothyBasin` calculation object.
- `g_b_const` - DivideBrot calculation constant.
- `g_marks_coefficient` - coefficient used by Marks/Peterson variations.
- `g_population`, `g_rate` - population-map iteration values used by
  bifurcation calculations.

## Related State And Ownership

Shared polynomial, complex, quaternion, and coloring settings declared in
`engine/fractals.h` and `engine/calcfrac.h` are documented in
[EngineGlobals.md](EngineGlobals.md), not duplicated here.

`NonStandardFractal` selects and owns standalone calculation operations.
Per-image callbacks perform setup, not UI render loops.  Fractal
calculation code neither includes UI headers nor polls keyboard/mouse
input.  Remaining globals and shared bindings still prevent independent
concurrent renders; the input split is not complete state isolation.
