# Plan: Version Compatibility

## Goal

Drop the old integer release global and restore the Fractint compatibility
model using `g_version` and `g_file_version`.

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
- Remove stale references to retired integer version globals from developer
  docs.

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
rg "save_release" libid tests hc home
```

Expected:

- No `save_release` matches outside documentation that explains Fractint.
- All behavior checks use `g_version` or `g_file_version`.
- `<Insert>` restores `g_version == current_id_version()`.
