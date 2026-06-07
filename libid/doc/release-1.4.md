# Iterated Dynamics 1.4 Release Plan

## Scope

This plan tracks the open GitHub issues assigned to the 1.4 milestone,
excluding issues tagged `wx` or `wx-tool`.

Slices are ordered by release priority. When a slice is complete, remove
it from this plan and renumber the remaining slices from 1.

## Slices

### Slice 1: Fix Windows executable resources

Issue: #386

Goal: compile Windows resources directly into the executable.

Work:

- Stop relying on a resource-only static library to carry resources.
- Attach the resource script to each Windows executable that needs it.
- Keep shared resource definitions in one source location.

Acceptance:

- Windows executables contain the expected icon and version resources.
- Non-Windows builds do not see resource files.

Verification:

- Verify resources in a Windows build artifact.
- Run `cmake --workflow rt-default`.

### Slice 2: Normalize version-sensitive test output

Issue: #387

Goal: keep text gold tests stable across version changes.

Work:

- Find tests whose gold output embeds the application version.
- Normalize version strings before comparing to gold files.
- Update gold files to use a stable placeholder.

Acceptance:

- Updating the application version does not fail unrelated text tests.
- Real output differences are still reported clearly.

Verification:

- Run the affected text, help, and raytrace tests directly.
- Run `cmake --workflow rt-default`.

### Slice 3: Prefer inline formulas from parameter files

Issue: #389

Goal: avoid an error when a parameter file contains the needed formula
inline.

Work:

- Detect formulas embedded in the selected parameter file.
- Prefer the embedded formula over a missing external formula file.
- Decide whether an existing external file should be ignored or warned.

Acceptance:

- The 1997.04.17 FOTD parameter set loads without a missing-file error.
- Existing external formula file loading remains unchanged otherwise.

Verification:

- Add a regression test using an inline formula and missing `formulafile`.
- Manually verify the reported FOTD parameter set if assets are present.
- Run `cmake --workflow rt-default`.

### Slice 4: Add periodicity-checking image coverage

Issue: #354

Goal: cover non-default periodicity checking through image tests.

Work:

- Pick one stable fractal with visible periodicity-checking behavior.
- Add an image test with non-default periodicity parameters.
- Keep the gold image deterministic across supported platforms.

Acceptance:

- Periodicity-related render paths are covered by at least one image test.
- The test is stable on Windows and Linux.

Verification:

- Run the new image test on Windows and Linux.
- Run `cmake --workflow rt-default`.

### Slice 5: Add Chaos and Fractals bibliography entry

Issue: #365

Goal: add the requested bibliography item to the help source.

Work:

- Add the book to the bibliography in the help source.
- Keep help formatting consistent with nearby entries.

Acceptance:

- The generated help includes the new bibliography entry.
- No unrelated revision-history text is changed.

Verification:

- Run the help generation or affected help tests.

### Slice 6: Add online documentation permalinks

Issue: #366

Goal: publish online documentation at stable versioned URLs.

Work:

- Keep the latest documentation at the default GitHub Pages URL.
- Publish the same documentation under a versioned `vMajor.Minor.Patch`
  path.
- Preserve older published documentation when publishing a new release.

Acceptance:

- The latest docs and the versioned 1.4 docs are both available.
- Older versioned docs remain available.

Verification:

- Inspect the generated Pages artifact layout.
- Verify links after a Pages deployment.
