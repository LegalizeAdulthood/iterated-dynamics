# Plan: Version Compatibility

## Goal

Drop the `g_release` variable and restore the Fractint compatibility model
using `g_version` and `g_file_version`.

Fractint has two version concepts:

- `release`: the compiled program version.
- `save_release`: the active compatibility version for the current image,
  parameter set, or loaded file.

Id should use:

- `current_id_version()`: derived from compile-time version macros.
- `g_version`: active compatibility version, equivalent to Fractint
  `save_release`.
- `g_file_version`: source file version used while loading and migrating
  GIF state.

All compatibility checks must use `g_version` or `g_file_version`, never a
parallel integer release value.  `<Insert>` must restore `g_version` to the
latest compiled Id version.

## Slice 3: Restore Fractint Reset Semantics

Work items:

- Keep bare `reset` as Fractint compatibility:
  `g_version = parse_legacy_version(1730)`.
- Make `reset=0` match Fractint and also select 17.30.
- Keep `reset=1730` and other `>= 100` values as legacy Fractint style
  versions, except existing Id special cases that must remain supported.
- Keep `reset=1/3/2` as modern Id version syntax.
- Set `g_file_version = g_version` after any parameter-file reset.
- Decide whether to implement Fractint `release=nnnn` as an alias for
  `reset=nnnn`; document the decision in tests.

Tests:

- `reset` verifies:
  `g_version == parse_legacy_version(1730)` and
  `g_file_version == parse_legacy_version(1730)`.
- `reset=0` verifies the same state.
- `reset=1906` verifies both globals are Fractint 19.06.
- `reset=1/3/2` verifies both globals are Id 1.3.2.
- `reset=y` verifies `BAD_ARG` and both globals are unchanged.
- If `release=1906` is supported, verify it sets both globals to Fractint
  19.06 and returns the same flags as `reset=1906`.

Verified state:

- `g_version` is the active compatibility version.
- `g_file_version` mirrors parameter-file compatibility after reset.

## Slice 4: Update GIF Load Version Flow

Work items:

- Keep `g_file_version` as the version decoded or inferred from the GIF.
- Use `g_file_version` for one-time GIF migration and load compatibility.
- After GIF load migration is complete, set `g_version = g_file_version`
  for recalculation compatibility.
- For modern Id GIFs, set `g_file_version` from the four Id version fields:
  `version_major`, `version_minor`, `version_patch`, and `version_tweak`.
- For Fractint GIFs, set `g_file_version` from the legacy `release` field.
- Prefer the four Id version fields when present.  Use legacy `release`
  only when the file has no Id four-part version.
- Preserve existing special inference for old file format versions.

Tests:

- In `tests/libid/io/test_loadfile.cpp`, load or synthesize a Fractint
  GIF info block with `release=1960`.
- Verify load migration uses:
  `g_file_version == parse_legacy_version(1960)`.
- Verify final active `g_version == parse_legacy_version(1960)`.
- Add a modern Id GIF version case with non-zero tweak and verify both
  globals become that full Id version after load.
- Verify a modern Id GIF ignores `FractalInfo.release=2004` for Id version
  selection when the four Id version fields are present.
- Existing `g_bad_outside`, EPS cross, distance-estimator, and palette
  compatibility tests should assert `g_file_version` for load decisions and
  `g_version` for later recalculation decisions.

Verified state:

- Load-time gates use `g_file_version`.
- Post-load recalculation gates use `g_version`.

## Slice 5: Replace History Version Storage

Work items:

- Replace `HistoryEntry::release` and `HistoryEntry::save_release` with:
  `Version version` and `Version file_version`.
- Save `g_version` into `HistoryEntry::version`.
- Save `g_file_version` into `HistoryEntry::file_version`.
- Restore both globals from history.
- Update history JSON read/write and equality checks.
- If old history JSON is supported, migrate `release` and `save_release` to
  `Version` values during decode.

Tests:

- Add or update `tests/libid/ui/test_history.cpp`.
- Seed `g_version = parse_legacy_version(1730)`.
- Seed `g_file_version = parse_legacy_version(2004)`.
- Save history and then overwrite both globals with current Id version.
- Restore history and verify both globals exactly match the seeded values.
- Verify serialized history contains `version` and `file_version`.
- If old JSON migration exists, verify `save_release` controls
  `g_version` and `release` is ignored for behavior.

Verified state:

- History restores both behavior and file-origin versions.

## Slice 6: Replace Display And Comment Uses

Work items:

- Change tab display to use `current_id_version()` for the compiled Id
  version.
- Format compiled Id version display as
  `major.minor[.patch[.tweak]]`.
- Omit patch and tweak when both are zero: `3.4`.
- Include patch when patch is non-zero: `3.4.5`.
- Include both patch and tweak when tweak is non-zero, even if patch is
  zero: `3.4.0.1`.
- Consider showing the active compatibility version separately if useful,
  using `to_display_string(g_version)`.
- Change comment expansion `version` to derive from the compiled Id
  version using the same formatting rule.
- Change comment expansion `patch` to derive from
  `current_id_version().patch`.

Tests:

- In `tests/libid/ui/test_comments.cpp`, set:
  `g_version = parse_legacy_version(1730)`.
- Verify comment variable `version` expands to the compiled Id version.
- Verify `version` omits zero patch and tweak fields.
- Verify `version` includes `.0.tweak` when tweak is non-zero and patch is
  zero.
- Verify `patch` expands from the compiled version.
- Add a tab display test only if existing test coverage can observe rows
  without large UI changes.

Verified state:

- User-visible compiled version is independent of `g_version`.
- Compatibility behavior remains controlled only by `g_version`.
- `g_patch_level` no longer has runtime users after display and comment
  expansion use `current_id_version()`.

## Slice 7: Remove `g_release`

Work items:

- Remove `extern int g_release` from `libid/include/misc/version.h`.
- Remove `extern const int g_patch_level` from
  `libid/include/misc/version.h`.
- Remove the definition from `libid/misc/version.cpp`.
- Replace all remaining reads with helper calls or `g_version`.
- Replace all remaining writes with `reset_version_to_current()` or direct
  `g_version` assignment.
- Remove `ValueSaver<int>` usage for `g_release` from tests.
- Remove tests for `g_patch_level`.
- Update comments that describe `g_release` or `g_patch_level`.

Tests:

- Run a source search for `g_release`; expected result is no matches.
- Run a source search for `g_patch_level`; expected result is no matches.
- Run a source search for `save_release`; expected matches only in
  Fractint reference sources or documentation.
- Build should fail if any stale declaration remains.

Verified state:

- The active runtime has one compatibility global: `g_version`.
- Loaded files retain source-version context in `g_file_version`.

## Slice 8: Update Parameter Save And GIF Save

Work items:

- Confirm `make_batch_file` writes `reset=` from `g_version`.
- Confirm parameter save for current Id compatibility writes the current Id
  version.
- Confirm parameter save for legacy compatibility writes the legacy version
  selected by `g_version`.
- Confirm GIF save keeps `FractalInfo.release` hardcoded to Fractint 20.04.
- Confirm GIF save writes the real Id version in the four Id version
  fields: `version_major`, `version_minor`, `version_patch`, and
  `version_tweak`.
- Document that `FractalInfo.release` is legacy Fractint metadata.

Tests:

- Update `tests/libid/ui/test_make_batch_file.cpp`.
- Seed `g_version = parse_legacy_version(1730)` and verify saved
  parameter output contains `reset=1730`.
- Seed `g_version = Version{1, 3, 2, 0, false}` and verify output contains
  `reset=1/3/2`.
- Verify `FractalInfo.release == 2004`.
- Verify GIF Id version fields preserve all four current Id components.
- Verify a non-zero tweak round-trips through GIF metadata.

Verified state:

- Saved parameter sets preserve active compatibility.
- New GIF files preserve legacy and Id version metadata.

## Slice 9: End-To-End Compatibility Tests

Work items:

- Keep the Lyapunov unit tests for `< 17.32` gates.
- Keep the `lyapunov-Blob1` image test using `@lyapunov/Blob1`.
- Add an image or integration test where startup current compatibility
  differs from bare `reset` compatibility.
- Add a focused test for `@file/entry` without `.par`, since Blob1 depends
  on it.

Tests:

- `ImageTest.lyapunov-Blob1` verifies `@lyapunov/Blob1` renders exactly
  equal to `gold-lyapunov-Blob1.gif`.
- A command parser test verifies `@lyapunov/Blob1` resolves
  `lyapunov.par`.
- A restart test verifies current Id compatibility after `<Insert>`.
- Existing `Egg`, `EvilFrog`, `mandala`, and Barnsley tests continue to
  verify legacy reset behavior.

Verified state:

- The Fractint compatibility model is restored:
  restart means current; bare reset means 17.30; explicit reset means that
  requested version.

## Slice 10: Documentation And Cleanup

Work items:

- Update `hc/src/help.src` to describe:
  `<Insert>` resets compatibility to the compiled Id version.
- Document bare `reset` as Fractint 17.30 compatibility.
- Document explicit `reset=` as the way to preserve old behavior in par
  files.
- Remove stale references to `g_release` from developer docs.

Tests:

- Run help generation through the normal workflow.
- Verify generated help contains the new compatibility wording.
- Verify markdown/source files contain no non-ASCII characters.

Verified state:

- User-facing docs match runtime behavior.
- Developer docs describe `g_version` and `g_file_version` only.

## Final Validation

Run from the source directory:

```text
cmake --workflow rt-default
```

Also run these searches:

```text
rg "g_release" libid tests hc home
rg "save_release" libid tests hc home
```

Expected:

- No `g_release` matches.
- No `g_patch_level` matches.
- No `save_release` matches outside documentation that explains Fractint.
- All behavior checks use `g_version` or `g_file_version`.
- `<Insert>` restores `g_version == current_id_version()`.
