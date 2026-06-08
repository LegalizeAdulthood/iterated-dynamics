# Increase Image Size Limits

## Goal

Remove artificial image dimension limits so Id can read, save, and
generate images up to the maximum size allowed by GIF:

- 65535 pixels wide
- 65535 pixels high

This applies to images handled through disk video drivers, Windows
drivers, and X11 drivers.

## Scope

Only image dimension metadata changes.  Existing signed metadata fields
that can naturally hold negative values remain signed.

Allowed metadata relaxations:

- `FractalInfo::x_dots`
- `FractalInfo::y_dots`
- `ExtBlock6::x_dots`
- `ExtBlock6::y_dots`
- equivalent evolution image dimension fields

Do not change:

- screen offsets
- parameter grid positions
- transform offsets
- color or flag fields
- any non-dimension metadata field

## Current Limits

- GIF logical screen dimensions are unsigned 16-bit values.
- `MAX_PIXELS` is 32767, a signed 16-bit limit.
- `id.cfg` rejects modes above `MAX_PIXELS`.
- `load_config.cpp` casts parsed mode dimensions through `short`.
- GIF extension image dimensions are stored as `int16_t`.
- GIF header writing stores only two bytes from `int` values.
- GIF decoding uses `short` for `decoder(line_width)` and `buf_cnt`.
- Image reduction uses short skip counters.
- Some generated scripts already allow total dimensions up to 65535.

## Slice 1: Define GIF Dimension Limits

Goal: make the intended limit explicit.

Work:

- Add a named GIF image dimension limit of 65535.
- Keep `MIN_PIXELS`.
- Replace image-size validation that uses `MAX_PIXELS` when the format
  boundary is really GIF.
- Keep `OLD_MAX_PIXELS` for old fixed-array code paths.
- Update comments that describe the old signed 16-bit limit.

Tests:

- Add unit coverage for the new limit constants.
- Add `id.cfg` parsing coverage for 32767, 32768, and 65535.
- Add rejection coverage for 65536.

Done when:

- `id.cfg` can accept disk modes above 32767.
- Values above 65535 are rejected.
- Existing modes still parse unchanged.

## Slice 2: Relax GIF Dimension Metadata

Goal: store image dimensions as unsigned 16-bit metadata.

Work:

- Change only dimension fields in GIF extension structs to `uint16_t`.
- Use unsigned 16-bit extraction for dimension fields.
- Use unsigned 16-bit insertion for dimension fields.
- Preserve byte-identical output for dimensions from 0 through 32767.
- Leave all non-dimension metadata fields signed.

Tests:

- Add metadata round-trip tests for 32767, 32768, and 65535.
- Verify existing gold metadata tests remain unchanged for small images.
- Verify non-dimension signed metadata still round-trips negative values.

Done when:

- New Id reads unsigned dimension values above 32767 correctly.
- Existing files with dimensions below 32768 still read correctly.
- No non-dimension metadata field type changes.

## Slice 3: Save GIF Headers With Unsigned Dimensions

Goal: write GIF logical screen and image descriptor dimensions correctly.

Work:

- Replace raw two-byte writes from `int` dimension variables with
  explicit unsigned 16-bit writes.
- Check logical screen width and height before writing.
- Check image descriptor width and height before writing.
- Keep current error handling for unsupported sizes.
- Audit 16-bit potential and Targa work paths for width multiplication
  before writing dimensions.

Tests:

- Save GIFs at 32767, 32768, and 65535 in each dimension where practical
  without rendering a full image.
- Verify GIF header bytes contain the expected unsigned dimensions.
- Verify 65536 is rejected before writing.

Done when:

- Saved GIF headers are correct above 32767.
- No save path writes truncated dimensions.

## Slice 4: Read GIF Headers Above 32767

Goal: load large GIF dimensions without signed truncation.

Work:

- Ensure GIF header width and height are read as unsigned 16-bit values.
- Keep file dimensions in `int` runtime variables.
- Audit view reduction setup for large input images.
- Replace short reduction counters only where the larger image size can
  overflow them.
- Keep existing behavior for images that fit the selected display mode.

Tests:

- Add header-read tests for 32767, 32768, and 65535.
- Add image load tests that reduce a large GIF into a smaller display.
- Add rejection tests for malformed or inconsistent GIF dimensions.

Done when:

- Id can inspect and select modes for GIFs wider or taller than 32767.
- View reduction does not overflow for maximum GIF dimensions.

## Slice 5: Remove Decoder Line-Width Limits

Goal: allow GIF decoding of lines up to 65535 pixels.

Work:

- Change `decoder(short line_width)` to use an integer width type.
- Change internal line counters such as `buf_cnt` away from `short`.
- Ensure the decoder line buffer can hold the maximum GIF line width.
- Audit code that assumes a decoded line is no wider than signed 16-bit.

Tests:

- Add decoder tests for line widths 32767, 32768, and 65535.
- Add regression tests for existing small GIFs.

Done when:

- The LZW decoder can process maximum-width GIF lines.
- Existing GIF decode behavior is unchanged.

## Slice 6: Increase Disk Video Generation Limits

Goal: let disk video generate GIF-sized images.

Work:

- Allow disk video modes up to 65535 by 65535.
- Keep disk video cache offsets and row calculations wide enough.
- Audit `common_start_disk()` arithmetic for overflow before allocation.
- Keep memory-backed disk video preferred when allocation succeeds.
- Fall back to disk-backed cache when memory allocation fails.
- Add predefined disk modes useful for high resolution output.

Suggested predefined modes:

- 15360 x 8640, 4x 2160p antialias
- 16384 x 16384, 16K square
- 65535 x 65535, maximum GIF dimensions, unnamed key

Tests:

- Add disk driver validation tests for 32768 and 65535.
- Add disk video initialization tests using mock memory failure.
- Add small-render smoke tests that exercise large mode setup without
  rendering every pixel.

Done when:

- Disk video accepts and initializes modes above 32767.
- Disk video rejects modes above 65535.
- Disk video reports allocation failure cleanly.

## Slice 7: Audit Windows Display Drivers

Goal: remove Id-only artificial limits from Windows display modes.

Work:

- Keep physical display validation for GDI modes.
- Ensure configured Windows modes above 32767 do not truncate.
- Audit backing store dimensions and row offsets.
- Ensure saved images from Windows display modes use unsigned dimensions.
- Keep disk driver behavior aligned with disk video generation limits.

Tests:

- Add GDI validation tests for large configured modes.
- Add Disk driver validation tests for 65535.
- Run Windows image and save tests.

Done when:

- GDI rejects only modes larger than the actual display or unsupported
  color depth.
- Windows disk driver accepts GIF-sized disk modes.

## Slice 8: Audit X11 Display Drivers

Goal: remove Id-only artificial limits from X11 display modes.

Work:

- Keep physical display validation for X11 window modes.
- Ensure configured X11 modes above 32767 do not truncate.
- Audit X11 plot backing store dimensions and row offsets.
- Ensure saved images from X11 display modes use unsigned dimensions.
- Keep X11 disk driver behavior aligned with disk video generation limits.

Tests:

- Add X11 validation tests where driver code is testable.
- Add Linux disk driver validation tests for 65535.
- Run Linux image and autokey tests under Xvfb.

Done when:

- X11 rejects only modes larger than the actual display or unsupported
  color depth.
- X11 disk driver accepts GIF-sized disk modes.

## Slice 9: Update User Documentation

Goal: document the new image size behavior.

Work:

- Update disk video help topics to describe the 65535 GIF limit.
- Update `id.cfg` comments for large disk modes.
- Add release note text for larger GIF-sized image support.
- Avoid implying that very large display windows are expected to work.

Tests:

- Build generated help.
- Run help compiler text and AsciiDoc comparison tests.

Done when:

- Online help matches the implemented limit.
- Generated help tests pass.

## Slice 10: Full Workflow Verification

Goal: verify the completed size-limit change.

Work:

- Run focused image metadata tests.
- Run disk video and image save/load tests.
- Run Windows display-driver tests.
- Run Linux X11 and disk-driver tests.
- Run the default workflow.

Tests:

- `cmake --workflow rt-default`
- Linux CI workflow with Xvfb
- Manual smoke test for one 16K disk mode

Done when:

- Default workflow is green.
- Linux workflow is green.
- A manually generated 16K disk-video GIF saves and reloads.
