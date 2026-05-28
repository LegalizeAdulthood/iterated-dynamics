# Overwrite And Library Path Plan

## Summary

Refactor path handling by use case.

1. Input files are resolved through the ordered read libraries.
2. Output files are resolved through the single save library.
3. Overwrite checks happen only after the final save-library path is
   known.

The old split/build/merge helpers exist to splice filenames into old drive
and directory strings.  `std::filesystem::path` handles path component
work, and the library APIs now own input and output location policy.  Do
not preserve the old helpers as replacement APIs.

## Call-Site Classification

- `file_get_window.cpp` is input.  It scans images for browse mode and
  must use `find_wildcard_first(ReadFile::IMAGE, ...)`.
- `main_menu_switch.cpp` is input.  It loads the image chosen by browse
  mode and must use the selected browse path or
  `find_file(ReadFile::IMAGE, ...)`.
- `merge_path_names.cpp` is obsolete input glue.  Delete it after callers
  use library paths directly.
- `make_mig.cpp` has both use cases.  GIF input uses
  `find_file(ReadFile::IMAGE, ...)`; GIF output uses the save library.
- `rotate.cpp`, `make_batch_file.cpp`, sound, orbit, raytrace, light
  image, and autokey output paths must use the save library before
  overwrite.

## Slice 1: Delete Split/Build/Merge Helpers

Delete the obsolete path helper family after input callers migrate.

Work items:

- Delete `libid/io/split_path.cpp`.
- Delete `libid/include/io/split_path.h`.
- Delete `libid/io/make_path.cpp`.
- Delete `libid/include/io/make_path.h`.
- Delete `libid/io/merge_path_names.cpp`.
- Delete `libid/include/io/merge_path_names.h`.
- Remove all six files from `libid/CMakeLists.txt`.
- Remove obsolete helper tests and test CMake entries.

Tests:

- Remove `tests/libid/io/test_split_path.cpp`.
- Remove `tests/libid/io/test_make_path.cpp`.
- Remove `tests/libid/io/test_merge_path_names.cpp`.
- Verify `rg "split_path|make_path|merge_path_names" libid tests`
  finds no production or test use.
- Build proves no stale include remains.

## Slice 2: Add Save-Library Overwrite Helper

Introduce one output helper that combines save-library routing and
overwrite policy.

Work items:

- Add a path-based helper near the existing write/check path code.
- Proposed API:
  `std::filesystem::path get_checked_save_path(WriteFile kind,
  const std::filesystem::path &name)`.
- The helper calls `get_save_path(kind, name.string())` first.
- The helper applies the default extension through `get_save_path`.
- If `g_overwrite_file` is true, return the final path unchanged.
- If `g_overwrite_file` is false, advance the final filename until unused.
- Keep `check_write_file` only as a temporary compatibility wrapper.

Tests:

- Add or extend `tests/libid/io/test_check_write_file.cpp`.
- Verify `g_overwrite_file=false` advances an existing final path.
- Verify `g_overwrite_file=true` reuses an existing final path.
- Verify `g_save_dir` and save library state affect the checked path.
- Verify a current-directory collision is ignored when the final path is
  in the save library.
- Verify the final path keeps the correct `WriteFile` subdirectory.

## Slice 3: Migrate Existing Output Callers

Move output paths to `get_checked_save_path`.

Work items:

- Update sound save.
- Update orbit save.
- Update raytrace save.
- Update autokey save.
- Update existing light image saves that already use `get_save_path`.

Tests:

- Existing unit and image tests must pass.
- Verify these callers no longer call `check_write_file` directly.
- Verify each caller passes a `WriteFile` kind, not a prebuilt directory.

## Slice 4: Fix Wrong-Directory Light Name Check

Fix light-name overwrite handling in the 3D parameter flow.

Work items:

- Remove the pre-save `check_write_file(g_light_name, ".tga")`.
- Resolve the output with `get_checked_save_path(WriteFile::IMAGE, ...)`.
- Apply overwrite handling to that final save-library path.
- After save, update `g_light_name` from the chosen final filename.

Tests:

- Add focused helper coverage for this bug class.
- Verify a raw filename collision in the current directory does not affect
  a non-colliding final save-library path.
- Verify a collision in the final save-library path advances the filename
  when overwrite is off.

## Slice 5: Apply Overwrite To Direct Outputs

Route remaining user-visible outputs through `get_checked_save_path`.

Work items:

- `rotate.cpp`: palette and map save respects overwrite.
- `make_mig.cpp`: generated multi-image GIF respects overwrite.
- `make_batch_file.cpp`: generated `makemig.bat` respects overwrite.
- Leave temp swap files such as `id.tmp` out of overwrite policy.
- Leave parameter-entry replacement guarded by the existing
  duplicate-entry prompt, not by `overwrite`.

Tests:

- Add focused tests where seams already exist.
- Verify map save advances filename when overwrite is off.
- Verify MIG GIF save advances filename when overwrite is off.
- Verify `makemig.bat` advances filename when overwrite is off.
- Verify parameter entry replacement behavior is unchanged.

## Slice 6: Fold Legacy Search Dirs Into Libraries

Make the read-library list the only generic input search mechanism.

Work items:

- Replace `g_fractal_search_dir1` and `g_fractal_search_dir2` with read
  library entries.
- Populate read libraries from `librarydirs`.
- Append `FRACTDIR` entries to the read-library list.
- Append `SRCDIR` or the program directory fallback to the read-library
  list.
- Parse `FRACTDIR` with the platform path separator.
- Update `find_path` to use the read-library list before `PATH`.
- Update `locate_input_file` to use the read-library list.
- Keep `g_check_cur_dir` precedence unchanged.

Tests:

- Add or extend focused file-location tests.
- Verify multiple `FRACTDIR` entries are searched in order.
- Verify `librarydirs` order precedes `FRACTDIR`.
- Verify current directory precedence is unchanged.
- Verify save-library fallback is unchanged where currently supported.
- Verify missing files still fail the same way.

## Slice 7: Final Audit

Remove transitional APIs and verify policy coverage.

Work items:

- Remove `check_write_file` compatibility if no longer used.
- Confirm no production use remains for split/build/merge helpers.
- Confirm no input caller manually combines directory and filename.
- Confirm no output caller checks overwrite before save-library routing.
- Confirm no user-visible save-category writer bypasses overwrite.
- Keep explicit exemptions documented.
- The implementation commit for this slice closes issue 268.

Tests:

- Run `cmake --workflow rt-default`.
- Run `rg` audits for removed helpers and raw overwrite bypasses.
- Existing image tests must pass.

## Behavior Rules

- Input paths use read-library lookup or explicit input paths.
- Output paths use save-library routing.
- `overwrite=no` protects final destination files only.
- Save-library routing happens before overwrite checks.
- User-visible generated files must not overwrite existing files when
  `overwrite=no`.
- Temporary implementation files are exempt.
- Parameter-file entry replacement keeps its existing duplicate-entry
  prompt.
- New GIF, image, map, sound, orbit, raytrace, and key outputs use the
  same final-path overwrite rule.

## Assumptions

- Public browse behavior may change only by searching libraries instead of
  reconstructing paths from the previous image filename.
- Explicit input paths remain explicit and do not get combined with read
  libraries.
- Search directory order preserves current lookup precedence after it is
  expressed as read-library order.
- Temporary files, append-only logs, history files, and config rewrite
  scratch files are outside the overwrite policy.
