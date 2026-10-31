<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# Engine Global Variables

Inventory of public `g_*` declarations in `libid/include/engine`, grouped
by responsibility.  Member names describe the contents of a global
object, not additional globals.  Header ownership records the current
implementation, not the target layering in [hexagonal.md](hexagonal.md).

## Display And Viewport

Headers: `VideoInfo.h`, `LogicalScreen.h`, `Viewport.h`.

- `g_adapter` - active index in `g_video_table`.
- `g_colors` - active palette size.
- `g_screen_x_dots`, `g_screen_y_dots` - physical image dimensions.
- `g_video_entry` - active video-mode description.
- `g_video_table`, `g_video_table_len` - available modes and count.
- `g_logical_screen` - logical dimensions, physical offsets, and
  `x_size_dots`/`y_size_dots` values equal to dimension minus one.
- `g_viewport` - view-window enablement, aspect/cropping policy, scrolling,
  reduction, and requested dimensions.

`VideoInfo` has no key field.  Pending mode requests and key bindings
belong to `ui/video_mode.h`, not this inventory.

## Image Coordinates

Headers: `ImageRegion.h`, `calc_frac_init.h`,
`convert_center_mag.h`.

- `g_image_region`, `g_save_image_region` - active and saved image bounds.
- `g_delta_x`, `g_delta_y`, `g_delta_x2`, `g_delta_y2` - pixel-to-plane
  coordinate increments.
- `g_plot_mx1`, `g_plot_mx2`, `g_plot_my1`, `g_plot_my2` - plane-to-screen
  conversion coefficients.
- `g_aspect_drift` - permitted aspect-ratio drift.
- `g_screen_aspect` - physical screen aspect ratio.
- `g_delta_min` - minimum coordinate increment used by calculation setup.
- `g_math_tol` - coordinate/math-transition tolerances.
- `g_use_center_mag` - center/magnification coordinate presentation flag.

For historical reasons, `ImageRegion::m_min` and `m_max` are not directly
the displayed top-left and bottom-right points.  The corners are:

- Top left: `(m_min.x, m_max.y)`.
- Bottom left: `(m_3rd.x, m_3rd.y)`.
- Bottom right: `(m_max.x, m_min.y)`.

## Calculation Status And Timing

Headers: `calcfrac.h`, `engine_timer.h`.

- `g_calc_status` - `NO_FRACTAL`, `PARAMS_CHANGED`, `IN_PROGRESS`,
  `RESUMABLE`, `NON_RESUMABLE`, or `COMPLETED`.
- `g_calc_time` - accumulated calculation time in hundredths of a second.
- `g_engine_timer_start` - interval start from `std::clock()`, in ticks.
- `g_timer_interval` - most recently measured interval in hundredths of a
  second.
- `g_std_calc_mode`, `g_old_std_calc_mode` - active and previous traversal
  modes.
- `g_passes` - progress-display classification of the current work.
- `g_current_pass`, `g_total_passes` - pass progress.
- `g_quick_calc`, `g_three_pass` - quick-calculation and three-pass state.

The `engine_timer()` wrapper and benchmark output are gone.  Explicit
timer operations remain.  Calculation dispatch is `fractals::g_dispatch`;
there is no `g_calc_type` global.

## Iteration And Bailout

Headers: `calcfrac.h`, `bailout_formula.h`.

- `g_max_iterations` - iteration limit.
- `g_color_iter`, `g_old_color_iter`, `g_real_color_iter` - current,
  previous, and retained iteration/coloring counters.
- `g_magnitude`, `g_magnitude_calc` - magnitude value and calculation flag.
- `g_magnitude_limit` - effective bailout limit.
- `g_magnitude_limit2` - square root of `g_magnitude_limit`.
- `g_periodicity_check` - periodicity setting.
- `g_first_saved_and`, `g_periodicity_next_saved_incr` - periodicity save
  schedule settings.
- `g_use_old_periodicity`, `g_reset_periodicity` - compatibility and reset
  controls.
- `g_close_enough`, `g_close_proximity` - comparison thresholds.
- `g_bailout_test` - selected bailout test.
- `g_bailout_float`, `g_bailout_bignum`, `g_bailout_bigfloat` - active
  bailout implementations for the numeric representations.

## Complex Working Values And Parameters

Headers: `calcfrac.h`, `fractals.h`.

- `g_init`, `g_old_z`, `g_new_z`, `g_tmp_z` - complex iteration values.
- `g_param_z1`, `g_param_z2`, `g_power_z` - complex parameters and power.
- `g_temp_sqr_x`, `g_temp_sqr_y` - squared-component temporaries.
- `g_sin_x`, `g_cos_x` - trigonometric working values.
- `g_float_param` - selected complex parameter pointer.
- `g_params` - numerical fractal parameters.
- `g_param_text` - retained parameter text for precision-sensitive input.
- `g_basin`, `g_degree`, `g_c_exponent` - fractal-specific settings.
- `g_quaternion_c`, `g_quaternion_ci`, `g_quaternion_cj`,
  `g_quaternion_ck` - quaternion constant components.
- `g_biomorph` - active biomorph setting.
- `g_bof_match_book_images` - BOF compatibility setting.
- `g_fudge_half` - legacy arithmetic working value.
- `g_max_color` - calculation color limit.

## Coloring And Palette

Headers: `calcfrac.h`, `spindac.h`, `color_state.h`, `text_color.h`.

- `g_inside_method`, `g_outside_method` - coloring method selectors.
- `g_inside_color`, `g_outside_color` - numerical color settings, separate
  from the method selectors.
- `g_fill_color` - inferred-fill override; `-1` means normal behavior.
- `g_atan_colors`, `g_and_color`, `g_decomp` - atan, mask, and
  decomposition coloring settings.
- `g_iteration_ranges` - iteration-to-color range mapping.
- `g_old_demm_colors` - legacy distance-estimator coloring flag.
- `g_dac_box` - active 256-entry RGB palette.
- `g_old_dac_box` - saved palette.
- `g_got_real_dac` - a real palette was loaded.
- `g_dac_count` - number of palette entries rotated per cycling update;
  this controls cycling speed, not palette size.
- `g_color_state` - palette provenance/state.
- `g_color_cycle_range_lo`, `g_color_cycle_range_hi` - cycling bounds.
- `g_cycle_limit`, `g_init_cycle_limit` - cycling limits.
- `g_map_name`, `g_map_set` - selected map name and selection flag.
- `g_is_true_color` - active true-color output state.
- `g_true_color`, `g_true_mode` - requested true-color behavior and mode.
- `g_text_color` - text-screen color attributes, currently engine-declared.

The default map palette `io::g_map_clut` is distinct from `g_dac_box`.
Palette-cycle interaction and its delay belong to UI.

## Logarithmic Color Mapping

Header: `log_map.h`.

- `g_log_map_flag` - logarithmic mapping setting.
- `g_log_map_auto_calculate` - automatic range selection.
- `g_log_map_calculate` - table calculation state.
- `g_log_map_fly_calculate` - on-the-fly versus table-based calculation.
- `g_log_map_table` - iteration-to-color lookup values.
- `g_log_map_table_max_size` - table size limit.

## Work List, Traversal, And Plotting

Headers: `work_list.h`, `calcfrac.h`, `SolidGuess.h`,
`SynchronousOrbit.h`, `StickyOrbits.h`.

- `g_num_work_list`, `g_work_list` - queued calculation work.
- `g_start_pt`, `g_stop_pt` - active work item's bounds.
- `g_i_start_pt`, `g_i_stop_pt` - calculation bounds after symmetry setup.
- `g_begin_pt` - retained starting position.
- `g_work_pass`, `g_work_symmetry` - active work-item metadata.
- `g_current_row`, `g_current_column` - reported calculation position.
- `g_row`, `g_col`, `g_color` - current pixel and color working values.
- `g_symmetry`, `g_force_symmetry` - active and requested symmetry.
- `g_plot`, `g_put_color` - symmetry-aware and underlying pixel outputs.
- `g_pi_in_pixels` - period used by periodic symmetry plotting.
- `g_stop_pass` - requested solid-guess stopping pass.
- `g_draw_mode` - sticky-orbit seed traversal: rectangle, line, or
  function.
- `g_max_rhombus_depth` - synchronous-orbit subdivision depth limit.
- `g_rhombus_stack`, `g_soi_min_stack`, `g_soi_min_stack_available` -
  synchronous-orbit stack-availability diagnostics.
- `g_keyboard_check_interval`, `g_max_keyboard_check_interval` - legacy
  work-budget counters retained by calculation checkpoints.

Despite their names, those work-budget counters do not authorize
calculation to poll input.  UI controllers own keyboard checks.

`StandardPass` routes concrete traversal objects.  Their scan, fill,
subdivision, and suspension state belongs to those objects, not a new
collection of globals.  Diffusion's old counter globals are gone.
`SynchronousOrbit::m_frame_stack`, not `g_rhombus_stack`, owns its
subdivision frames.

## Orbit And Progress Display Settings

Headers: `calcfrac.h`, `OrbitPlot.h`, `show_dot.h`.

- `g_init_orbit`, `g_use_init_orbit` - initial orbit value and mode.
- `g_show_orbit`, `g_start_show_orbit` - current overlay enablement and
  initial preference.
- `g_orbit_color` - XOR overlay color.
- `g_orbit_save_flag` - indicates saved overlay pixels; it is not a count.
- `g_orbit_delay` - legacy 100-microsecond-unit setting interpreted by UI
  display and sound policies.
- `g_orbit_skip_points` - initial points skipped by sticky-orbit work.
- `g_auto_show_dot` - automatic marker color selection.
- `g_show_dot` - marker color setting; negative disables it.
- `g_size_dot` - marker size setting.

`OrbitPlot` owns its saved overlay vector.  Standard and Lyapunov markers
belong to `ui::ShowDotController`; FrothyBasin retains a simple calculation
marker.  Display pacing and input remain UI responsibilities.  See
[inversion-of-control.md](inversion-of-control.md) for current policies.

## Attractors, Distance Estimation, Potential, And Inversion

Headers: `calcfrac.h`, `calc_frac_init.h`, `Potential.h`, `Inversion.h`.

- `g_attractor` - enabled flag, count, radius, complex attractors, and
  periods.
- `g_distance_estimator` - active distance-estimator setting.
- `g_distance_estimator_width_factor` - boundary width factor.
- `g_distance_estimator_x_dots`, `g_distance_estimator_y_dots` - reference
  dimensions for video-independent distance estimation.
- `g_use_old_distance_estimator` - compatibility flag in `calcfrac.h`.
- `g_potential` - enablement, three potential parameters, and 16-bit
  storage request.
- `g_inversion` - inversion mode, radius, complex center, and original
  radius/center parameters.

Distance-estimator working state also lives in calculation objects.
These globals do not describe all per-pixel state.

## Sound Output

Headers: `sound.h`, `SoundOutput.h`.

- `g_sound` - active `SoundOutputPtr`, initialized to `NullSoundOutput`.
- `g_sound_flag` - sound mode and compatibility flags.
- `g_base_hertz` - base tone frequency.
- `g_fm_attack`, `g_fm_decay`, `g_fm_sustain`, `g_fm_release` - retained
  FM envelope settings.
- `g_fm_wave_type`, `g_fm_volume` - retained waveform and volume settings.
- `g_polyphony`, `g_hi_attenuation` - retained voice/attenuation settings.
- `g_scale_map` - note-to-scale mapping.

Retaining a sound setting does not imply that a modern driver implements
it.  Calculation submits samples through `g_sound`; UI services playback
and waits outside calculation.  See [sound.md](sound.md).

## Commands, Comments, And User Intent

Headers: `cmdfiles.h`, `command_comments.h`, `UserData.h`.

- `g_parameter_file`, `g_parameter_set_name` - selected parameter source.
- `g_first_init` - startup/default-initialization gate.
- `g_image_filename_mask` - image search mask.
- `g_init_batch` - current `BatchMode` state.
- `g_colors_preloaded`, `g_read_color` - palette-loading controls.
- `g_record_colors` - palette recording policy for parameter output.
- `g_max_line_length` - parameter-output line length.
- `g_par_comment`, `g_command_comment` - parameter and command comments.
- `g_user` - requested biomorph, distance estimator, periodicity,
  traversal, bailout, perturbation mode/tolerance, and legacy-float intent.

Requested values may differ from effective calculation settings.
`CommandProcessor` owns incremental source state.  UI owns routing of UI
parameters and `g_escape_exit`; it is not an engine global.

## Resume, Precision, And Other Configuration

Headers: `resume.h`, `fractalb.h`, `random_seed.h`, `trig_fns.h`,
`jiim.h`, `Browse.h`.

- `g_resume_data`, `g_resume_len`, `g_resuming` - saved calculation bytes,
  valid length, and resume flag.
- `g_precision_diagnostics` - debug snapshots returned for UI inspection.
- `g_random_seed`, `g_random_seed_flag` - seed and explicit-seed flag.
- `g_trig_fn`, `g_num_trig_functions` - named function table and count.
- `g_trig_index` - selected function indices.
- `g_has_inverse` - inverse-iteration availability.
- `g_julia_c`, `g_save_c` - current and saved Julia constants.
- `g_browse` - browsing preferences, flags, selected path, and path stack.

Arbitrary-precision storage is declared in `math`, not engine; see
[MathGlobals.md](MathGlobals.md).  Some application-facing settings still
have engine declarations.  This inventory records that ownership without
claiming the ports-and-adapters dependency boundary is complete.
