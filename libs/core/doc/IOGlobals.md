<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson.
-->

# I/O Global Variables

Inventory of public `g_*` declarations in `libs/core/include/io`. Codec
objects, metadata values, and private storage/cache state are separate from
these shared bindings.

## Image Loading And Metadata Application

Header: `loadfile.h`.

- `g_read_filename` - requested image path.
- `g_show_file` - `REQUEST_IMAGE`, `LOAD_IMAGE`, or `IMAGE_LOADED`.
- `g_file_x_dots`, `g_file_y_dots` - restored image dimensions.
- `g_file_colors` - restored image palette size.
- `g_file_aspect_ratio` - restored image aspect ratio.
- `g_file_version` - producer program version, not GIF metadata revision.
- `g_skip_x_dots`, `g_skip_y_dots` - horizontal/vertical skip settings
  used when reducing a loaded image.
- `g_fast_restore` - view-window reset and mode-warning suppression policy.
- `g_loaded_3d` - loaded-image 3D flag.
- `g_overlay_3d` - restore as a 3D overlay.
- `g_bad_outside` - loaded outside-color compatibility state.
- `g_new_bifurcation_functions_loaded` - loaded bifurcation compatibility
  state.

`ImageMetadata` contains dimensions, palette, fractal information, and
extension payloads without first changing the active image.
`FractalInfo::info_version` is the metadata-format revision, distinct
from `g_file_version`.  UI owns pending video/3D choices and cancellation;
accepted metadata is then applied to runtime state.

## Image Saving

Headers: `encoder.h`, `check_write_file.h`.

- `g_save_filename` - current save-name/path state.
- `g_overwrite_file` - allow replacement instead of selecting another
  unused filename.

`ImageSave` owns scanline encoding, file state, metadata extensions, and
temporary-file cleanup.  `GifWriter` owns the giflib writer.
`libs/ui/save_image.cpp` captures optional evolution metadata and owns
overwrite interaction, feedback, notifications, and exit policy.
`ImageSave` receives the metadata as a value rather than reading UI state.
There is no keyboard-interrupted partial-save workflow.

## Palette Loading

Header: `loadmap.h`.

- `g_last_map_name` - last map selected by loading, saving, or
  `colors=@filename`.
- `g_map_clut` - saved/default map palette selected by `map=`.
- `g_map_specified` - default map selection flag.

The active rendering palette is `engine::g_dac_box`, declared in
`engine/spindac.h`.  Loading returns diagnostics without opening UI.
Missing files preserve palette data; malformed maps retain their valid
prefix and fill the remainder gray using the existing conversion rules.

## GIF Scanline Output

Headers: `gifview.h`, `decoder.h`.

- `g_dither_flag` - restore dithering request.
- `g_height` - shared GIF height state.
- `g_num_colors` - source GIF palette size.
- `g_out_line` - selected decoded-scanline output function.
- `g_row_count` - output row counter used by `out_line()` and alternate
  restore output paths; declared in `decoder.h`, not UI video code.

`GifRestore` owns giflib decode state and exposes scanlines.
`RestoredImageOutput` owns coordinated per-pixel output when sound is
enabled.  UI services sound readiness between bounded output increments.

Loading and saving remain atomic with respect to keyboard input.
Scanline feedback does not authorize the codec to poll input or invoke
modal UI.  See [inversion-of-control.md](inversion-of-control.md).

## Disk Pixel Storage

Header: `disk_storage.h`.

- `g_disk_flag` - active disk-pixel storage.
- `g_disk_16_bit` - 16-bit potential-image storage.
- `g_disk_targa` - Targa backing-store mode.

Cache, backing handles, pixel packing, and storage progress are private to
`io/disk_storage.cpp`.  The configured cache is 4096 KiB with 2048-byte
blocks.  Storage may be memory- or disk-backed; these names do not imply
that every access performs filesystem I/O.

UI owns storage allocation retry/cancel, progress presentation, and pixel
target activation.  Backing operations themselves do not call UI or
drivers.  `ui::g_good_mode` is not an I/O storage flag.

## File Search And Directories

Headers: `find_file.h`, `library.h`, `special_dirs.h`.

- `g_dta` - current wildcard-search result: path, filename, and attributes.
- `g_check_cur_dir` - include the current directory in file lookup.
- `g_special_dirs` - shared platform-specific directory provider.
- `g_save_dir` - output directory.
- `g_temp_dir` - temporary-file directory.
- `g_working_dir` - miscellaneous working-file directory.

Ordered read libraries are managed through `library.h` operations, not an
additional public global array.  They are populated from `librarydirs`,
`FRACTDIR`, and the executable fallback.  Checked output paths enforce
the existing save-directory and naming policies.

## Video Catalog Configuration

Header: `load_config.h`.

There are no public globals in the core configuration reader.
`VideoConfigReader` parses stream records into `VideoConfigEntry` values,
including the unconverted key name and source line number.  It does not
resolve drivers or key bindings or modify the active catalog.

`libs/ui/load_config.cpp` interprets keys and assembles the catalog from
key-free engine `VideoInfo` values.  Configuration status, line mappings,
and key associations are UI-owned; see [UIGlobals.md](UIGlobals.md).

## Autosave

Header: `save_timer.h`.

- `g_resave_flag` - repeated-save filename state.
- `g_save_time_interval` - autosave interval setting in minutes; its
  absolute value supplies the timer duration.
- `g_started_resaves` - repeated-save sequence has started.
- `g_timed_save` - pending timed-save state.

`TimedSave` values are `NONE`, `STARTED`, and `FINAL`.
Core `save_timer.cpp` only defines these shared settings and flags.
`ui::SaveTimer` owns start/duration ticks and the pending row boundary;
UI supplies `read_ticker()` values and coordinates save requests.

## Persistent Records

GIF extension records are I/O-owned values.  They hold fractal settings,
optional resume bytes, formula information, ranges, arbitrary-precision
parameters, evolution information, and orbit settings.
Resume support still depends on the calculation type and saved state;
saving metadata is not a promise of exact resume from every operation.

UI captures evolution records before saving and applies normalized
restored records to its controls.  Core metadata loading and serialization
do not read or modify UI evolution state.  Parameter-file generation
policy also remains in UI, outside metadata application.

See [hexagonal.md](hexagonal.md) for the dependency direction and
[EngineGlobals.md](EngineGlobals.md) and [UIGlobals.md](UIGlobals.md) for
the related runtime bindings.
