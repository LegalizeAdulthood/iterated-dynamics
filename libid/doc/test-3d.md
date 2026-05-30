# Plan: 3D Potential And Targa Tests

## Goal

Cover the `radar.par` `3dlook01` and `3dlook` workflow end to end:

- render `3dlook01` as a 16-bit potential image and save `potntial.pot`;
- compare that POT file as GIF data;
- render `3dlook` from the POT file through the 3D UI defaults;
- write the full-color Targa lightfile;
- compare the TGA output against a gold PNG image.

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
   `gold-3dlook-targa.png`.

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
- `tests/autokey/gold-3dlook-targa.png`

Add `tests/cmake` to `CMAKE_MODULE_PATH` from `tests/CMakeLists.txt`, then
include these helpers only from the tests that need them.

## Common CMake Helpers

Add small helpers before changing test behavior:

- `id_make_test_home(<name> <out-var> [PARS <par>...])`

  Create `${CMAKE_CURRENT_BINARY_DIR}/<name>/home` and copy `sstools.ini`
  and `id.cfg`. Do not precreate empty library subdirectories; write paths
  create them as needed. Create parent directories only when the helper
  copies a file into them.

  If `PARS` is supplied, copy each named file from
  `${CMAKE_SOURCE_DIR}/home/par` to the test home `par` directory.
  `PARS` entries are plain filenames. Additional parameter collections
  should be modeled as separate library roots, not subdirectories under
  `home/par`.

- `id_run(...)`

  Wrap `execute_process`, echo commands consistently, and print
  `debug/stopmsg.txt` when Id fails.

- `id_compare_image(...)`

  Wrap `image-compare`, pass optional compare flags, and keep generated
  files in the binary directory for manual inspection.

Use the helpers for the new tests first. Migrate older image or autokey
tests only when it removes immediate duplication.

## Work Slices

1. Add true-color loading to `image-compare`.

   Keep the existing GIF/POT comparison path unchanged. Add a second path
   for true-color images when either input is `.tga` or `.png`.

   Use the local vcpkg `tgautils` package for TGA loading. Do not hand-roll
   a TGA parser in the test tool.

   Use libpng from vcpkg for PNG loading. Use `find_package(PNG REQUIRED)`
   and link `image-tool` or `image-compare` with `PNG::PNG`.

   Normalize both loaders to one RGB buffer shape and compare RGB values
   exactly.

2. Add `AutokeyTest.3dlook-targa`.

   Use a private home directory. In the CMake script, generate
   `potntial.pot` first, then run Id with `@radar/3dlook` and an autokey
   file that accepts the default 3D screens. Use a deterministic lightfile
   name, such as `lightname=3dlook-targa`.

   Use FRACTINT to generate the initial full-color Targa baseline from the
   same two-stage parameter flow, convert that verified TGA to
   `gold-3dlook-targa.png`, and compare Id's generated TGA to it. The first
   Id test version should fail cleanly if the gold PNG does not exist and
   should print the full generated TGA path.

3. Check in gold images after manual verification.

   Accept `gold-3dlook01.pot` after the potential image is verified.
   Convert the verified TGA to PNG and check it in as
   `gold-3dlook-targa.png`.

4. Optional cleanup.

   If the new helpers are clearly better, migrate `ImageTest.make-mig` and
   `AutokeyTest.makepar-mig-pieces` to them in a later mechanical slice.
   Keep that separate from the 3D behavior test.

5. Validate.

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
