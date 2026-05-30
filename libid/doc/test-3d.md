# Plan: 3D Potential And Targa Tests

## Goal

Cover the `radar.par` `3dlook01` and `3dlook` workflow end to end:

- render `3dlook01` as a 16-bit potential image and save `potntial.pot`;
- compare that POT file as GIF data;
- render `3dlook` from the POT file through the 3D UI defaults;
- write the full-color Targa lightfile;
- compare the TGA output against a gold TGA image.

Do not make tests depend on the sibling `id-libraries` checkout. Add
`radar.par` to `home/par` and retain the original top comment authorship.

## Test Shape

Use two focused tests. The second test still creates its own POT input.

1. `ImageTest.3dlook01-potential`

   Run `@radar/3dlook01` in batch mode with `savename=potntial` and
   compare `image/potntial.pot` to `gold-3dlook01.pot`.

2. `AutokeyTest.3dlook-targa`

   Create an isolated test home, generate `potntial.pot` with
   `@radar/3dlook01`, then run `@radar/3dlook` with an autokey script that
   accepts the 3D parameter screens. Compare the generated TGA lightfile to
   `gold-3dlook-targa.tga`.

This keeps the POT test simple while making the Targa test exercise the
real two-stage flow in one CTest invocation.

## Proposed Files

- `tests/cmake/IdTestHome.cmake`
- `tests/cmake/IdRun.cmake`
- `tests/cmake/IdImageCompare.cmake`
- `vcpkg-configuration.json`
- `vcpkg-overlays/tgautils/vcpkg.json`
- `vcpkg-overlays/tgautils/portfile.cmake`
- `home/par/radar.par`
- `tests/images/gold-3dlook01.pot`
- `tests/autokey/3dlook_targa.key`
- `tests/autokey/3dlook_targa_test.cmake`
- `tests/autokey/gold-3dlook-targa.tga`

Add `tests/cmake` to `CMAKE_MODULE_PATH` from `tests/CMakeLists.txt`, then
include these helpers only from the tests that need them.

## Common CMake Helpers

Add small helpers before changing test behavior:

- `id_make_test_home(<name> <out-var>
  [IMAGES <image>...]
  [KEYS <key>...]
  [PARS <par>...])`

  Create `${CMAKE_CURRENT_BINARY_DIR}/<name>/home` and copy `sstools.ini`
  and `id.cfg`. Do not precreate empty library subdirectories; write paths
  create them as needed. Create parent directories only when the helper
  copies a file into them.

  If `PARS` is supplied, copy each named file from
  `${CMAKE_SOURCE_DIR}/home/par` to the test home `par` directory.
  `PARS` entries are plain filenames. Additional parameter collections
  should be modeled as separate library roots, not subdirectories under
  `home/par`.

  If `KEYS` is supplied, copy each named file from the current test
  directory to the test home `key` directory.

  If `IMAGES` is supplied, copy each named file from
  `${CMAKE_SOURCE_DIR}/tests/images` to the test home `image` directory.
  Use `source=dest` when a test needs a staged image renamed, such as
  `gold-3dlook01.pot=potntial.pot`.

- `id_run(...)`

  Wrap `execute_process`, echo commands consistently, and print
  `debug/stopmsg.txt` when Id fails.

- `id_compare_image(...)`

  Wrap `image-compare`, pass optional compare flags, and keep generated
  files in the binary directory for manual inspection.

Use the helpers for the new tests first. Migrate older image or autokey
tests only when it removes immediate duplication.

## Work Slices

1. Add focused autokey tests for the remaining 3D restore and overlay UI.

   These should use isolated homes from `id_make_test_home`, stage key
   files with `KEYS`, stage POT or GIF inputs with `IMAGES`, and keep
   source `home` untouched. When a slice needs a new gold image, stop and
   ask for the gold image before running the test that requires it.

   1. `AutokeyTest.3d-map-prompt`

      Drive `@radar/3dlook`, choose a fill/color mode that opens the map
      prompt, enter a staged map filename, and compare output. This covers
      the field prompt that resolves map files through the library
      mechanism.

   2. `AutokeyTest.3d-stereo`

      Drive `@radar/3dlook`, enable stereo or funny-glasses output, accept
      the funny-glasses parameter screen, and compare the resulting image.
      Start with the stereo mode most likely to write a deterministic
      image.

   3. `AutokeyTest.3d-overlay`

      Load a normal GIF input, invoke `#` for 3D overlay instead of `3`,
      accept defaults, and compare the overlay result. This should prove
      the overlay path uses the current image and does not rely only on
      restore-from-file behavior.

2. Optional cleanup.

   If the new helpers are clearly better, migrate `ImageTest.make-mig` and
   `AutokeyTest.makepar-mig-pieces` to them in a later mechanical slice.
   Keep that separate from the 3D behavior test.

3. Validate.

   Run focused tests first:

   ```text
   ctest --test-dir ../build-rt-default -C Release -R "3dlook|pot|targa"
   ```

   Then run:

   ```text
   cmake --workflow rt-default
   ```

## Risks

- Autokey timing can be fragile. Passing `video=F6` should skip the intro
  screen and keep the script short.
- The Targa prompt may store a full save path in `g_light_name`; the test
  should compare the actual generated path reported by the CMake script.
- The two tests must not share the source `home` directory or each other's
  generated `potntial.pot`.
- The local `tgautils` port should pin a commit and hash so the dependency
  is reproducible.
- True-color compare should start exact. Add tolerance only if the output
  is proven to vary across supported build environments.
