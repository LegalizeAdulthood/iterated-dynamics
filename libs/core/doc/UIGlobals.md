<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# UI Global Variables

Inventory of public `g_*` declarations in `libs/ui/include/ui`.  The help
binding uses namespace `id::help`; the other bindings below use `id::ui`.
The `id::ui` library depends on core, not the reverse.  These bindings
remain shared UI state rather than independent application contexts.

## Application And Image Coordination

Header: `big_while_loop.h`.

- `g_ask_video` - prompt for a video mode.
- `g_virtual_screens` - virtual-screen policy.
- `g_escape_exit` - skip exit confirmation.
- `g_compare_gif` - restored-image comparison request.
- `g_from_text` - text/graphics transition state.
- `g_out_line_cleanup` - active output cleanup callback.
- `g_evolve_info` - retained I/O evolution record.
- `g_have_evolve_info` - retained evolution resume information is present.

`MainState` controls application phases; menu handlers return
`CommandResult`.  Their state is also carried by `MainContext` and UI
objects rather than represented only by globals.

UI registers output cleanup callbacks and invokes core operations such as
`geometry::finish_line3d()`.  Geometry does not register UI callbacks.

## Video Requests And Key Bindings

Headers: `video_mode.h`, `video.h`, `diskvid.h`, `load_config.h`.

- `g_init_mode` - pending video catalog index, activated after parameter
  processing.
- `g_video_mode_keys` - function-key associations indexed by engine video
  catalog entry; zero means unassigned.
- `g_video_entry_key` - binding copied alongside the active or pending
  selected entry.
- `g_video_start_x`, `g_video_start_y` - display origin.
- `g_vesa_x_res`, `g_vesa_y_res` - retained video-resolution state.
- `g_good_mode` - disk-video UI setup validity flag.
- `g_bad_config` - `ConfigStatus::OK`, `BAD_WITH_MESSAGE`, or
  `BAD_NO_MESSAGE`.
- `g_cfg_line_nums` - source-line mapping for video catalog entries.

Active mode information and image dimensions belong to engine
`VideoInfo.h` and `LogicalScreen.h`.  `VideoInfo` contains no key.
UI owns key-name parsing, lookup, formatting, and the `video` parameter.
The UI configuration loader uses core `io::VideoConfigReader` records to
resolve drivers, populate the catalog, and assign key bindings.

Disk-storage flags `g_disk_flag`, `g_disk_16_bit`, and `g_disk_targa`
belong to `io/disk_storage.h`.  Decoded output position `g_row_count`
belongs to `io/decoder.h`, not UI video state.

## Text, Help, And Diagnostics

Headers: `text_screen.h`, `help.h`, `tab_display.h`, `intro.h`,
`comments.h`.

- `g_text_col`, `g_text_row` - current text cursor position.
- `g_text_col_base`, `g_text_row_base` - cursor coordinate bases.
- `id::help::g_help_mode` - context-sensitive help label.
- `g_tab_enabled` - Tab information-screen enablement.
- `g_tab_metadata` - shared const pending-image metadata used by Tab
  display without committing it to active calculation state.
- `g_slow_display` - introduction/display timing setting.
- `g_get_cpu_id` - replaceable CPU-description provider for command
  comment expansion.

The read-only map-name constants in `get_3d_params.h` and `starfield.h`
are not mutable `g_*` bindings.

## Mouse And Palette Tools

Headers: `mouse.h`, `editpal.h`, `find_special_colors.h`.

- `g_look_at_mouse` - mouse interpretation mode.
- `g_cursor_mouse_tracking` - cursor mouse-tracking enablement.
- `g_line_buff` - shared row buffer used by palette/JIIM display helpers.
- `g_using_jiim` - JIIM interaction is active.
- `g_color_dark`, `g_color_bright`, `g_color_medium` - selected palette
  indices for contrasting UI presentation.

Keyboard handler-stack and interrupt state are private to UI input
plumbing.  `CrossHairCursor` owns its saved pixels, position, and blink
state.  Calculation code does not subscribe to mouse events or poll keys.

## Zoom Box

Header: `zoom.h`.

- `g_box_x`, `g_box_y` - saved box pixel coordinates.
- `g_box_values` - original pixels under the box.
- `g_box_color` - box drawing color.
- `g_box_count` - number of saved box points.
- `g_zoom_box_x`, `g_zoom_box_y` - box position.
- `g_zoom_box_width`, `g_zoom_box_height` - box dimensions.
- `g_zoom_box_skew` - shear factor, not an angle.
- `g_zoom_box_rotation` - box rotation setting.
- `g_zoom_enabled` - zoom interaction enablement.

UI converts accepted box geometry into engine image coordinates.
Manipulating the displayed box is not itself a calculation step.

## Slideshow And History

Headers: `slideshw.h`, `history.h`.

- `g_slides` - slideshow mode: off, playback, or recording.
- `g_auto_name` - slideshow keystroke script path, not an automatically
  generated image filename.
- `g_busy` - calculation/file-operation state observed by slideshow
  playback.
- `g_history_flag` - image-history control flag.
- `g_history_ptr` - current history position.
- `g_max_image_history` - history capacity.

Image history is UI behavior.  Batch rendering still uses UI-owned
orchestration and can be interrupted; batch mode does not imply a
headless application or a separate input-free engine.

## Autosave Scheduling

Header: `save_timer.h`.

`SaveTimer` owns start/duration ticks and the pending row boundary in
instance members.  UI supplies the clock through `read_ticker()` and
checks whether a timed save is due.  There is no `g_finish_row` global.
Shared interval, repeated-save, and pending-save flags remain I/O-owned;
see [IOGlobals.md](IOGlobals.md).

## Evolution

Header: `evolve.h`.

- `g_evolving` - evolution bit flags.
- `g_evolve_image_grid_size` - field-map grid size.
- `g_evolve_this_generation_random_seed` - generation seed.
- `g_evolve_max_random_mutation` - mutation magnitude.
- `g_evolve_mutation_reduction_factor` - mutation reduction setting.
- `g_evolve_x_parameter_range`, `g_evolve_y_parameter_range` - parameter
  ranges.
- `g_evolve_x_parameter_offset`, `g_evolve_y_parameter_offset` - active
  offsets.
- `g_evolve_new_x_parameter_offset`, `g_evolve_new_y_parameter_offset` -
  pending offsets.
- `g_evolve_discrete_x_parameter_offset`,
  `g_evolve_discrete_y_parameter_offset` - discrete active offsets.
- `g_evolve_new_discrete_x_parameter_offset`,
  `g_evolve_new_discrete_y_parameter_offset` - discrete pending offsets.
- `g_evolve_param_grid_x`, `g_evolve_param_grid_y` - selected grid cell.
- `g_evolve_param_box_count`, `g_evolve_param_zoom` - parameter-box state.
- `g_evolve_dist_per_x`, `g_evolve_dist_per_y` - parameter-grid increments.
- `g_gene_bank` - variable addresses, variation functions, mutation modes,
  and descriptions.

These runtime controls are distinct from the I/O-owned serialized
`EvolutionInfo` type.  UI captures a record before saving and applies
normalized restored records to these controls.  I/O receives or returns
record values without reaching into UI state.

## Stereogram And Parameter-Output Policy

Headers: `stereo.h`, `make_batch_file.h`.

- `g_auto_stereo_batch` - automatic stereogram conversion policy.
- `g_save_rds_params` - include stereogram parameters in PAR output.
- `g_stereo_texture_reuse` - reuse the selected texture filename.
- `g_make_parameter_file` - parameter-file generation request/state.
- `g_make_parameter_file_map` - palette output policy for generation.

Stereogram depth, physical width, calibration bars, gray-value selection,
texture enablement, and the texture filename are geometry-owned inputs
in `geometry/stereo_params.h`.  UI owns the complete `rds` and
`rds-texture` handlers and coordinates generation actions.

## Current Boundary

UI owns keyboard/mouse polling, key meaning, diagnostics, markers, and
playback/presentation waits.  Render controllers and sound sessions also
retain state in objects rather than public globals.

Core code does not include UI headers or access these UI-owned bindings.
The broad driver interface and shared mutable core state still need
separate ownership/interface work before independent concurrent renders.
See [hexagonal.md](hexagonal.md) for that direction and
[inversion-of-control.md](inversion-of-control.md) for remaining handler
and input-dispatch work.
