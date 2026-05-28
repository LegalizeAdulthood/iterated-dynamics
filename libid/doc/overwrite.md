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
- `make_mig.cpp` has both use cases.  GIF input uses
  `find_file(ReadFile::IMAGE, ...)`; GIF output uses the save library.
- `rotate.cpp`, `make_batch_file.cpp`, sound, orbit, raytrace, light
  image, and autokey output paths must use the save library before
  overwrite.

## Slice 1: Fold Legacy Search Dirs Into Libraries

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

## Slice 2: Final Audit

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
