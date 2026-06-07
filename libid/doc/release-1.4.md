# Iterated Dynamics 1.4 Release Plan

## Scope

This plan tracks the open GitHub issues assigned to the 1.4 milestone,
excluding issues tagged `wx` or `wx-tool`.

Slices are ordered by release priority. When a slice is complete, remove
it from this plan and renumber the remaining slices from 1.

## Slices

### Slice 1: Make formula rand repeatable with rseed

Issue: #363

Goal: make formulas using `rand` repeatable when `rseed` is specified.

Work:

- Remove unconditional reseeding from the formula interpreter.
- Route formula random use through the image random seed path.
- Preserve existing behavior when no seed is specified.

Acceptance:

- A formula using `rand` produces repeatable output with the same `rseed`.
- Different seeds still produce different random streams.

Verification:

- Add an image or parameterized test for a formula that calls `rand`.
- Run `cmake --workflow rt-default`.

### Slice 2: Preserve user input case for named entries

Issue: #364

Goal: preserve user-entered case for formula, IFS, and L-system names.

Work:

- Keep `formulaname`, `ifs`, and `lname` values in their input case.
- Continue matching entry names case-insensitively.
- Avoid changing filename case handling.

Acceptance:

- Parameter output preserves the user's entry name case.
- Existing files still load when entry-name case differs.

Verification:

- Add focused tests for formula, IFS, and L-system name handling.
- Run `cmake --workflow rt-default`.

### Slice 3: Make makepar work from any directory

Issue: #368

Goal: make `makepar` independent of the current working directory.

Work:

- Identify config, formula, image, and output paths used by `makepar`.
- Resolve paths from the configured home/install locations as needed.
- Avoid depending on the process current directory.

Acceptance:

- `makepar` works when launched outside the install or home directory.
- Existing in-directory behavior is unchanged.

Verification:

- Add an acceptance test that runs `makepar` from a temporary directory.
- Run `cmake --workflow rt-default`.

### Slice 4: Generate Linux makemig scripts

Issue: #370

Goal: generate a Linux shell equivalent of `makemig.bat`.

Work:

- Split platform-specific script text from shared `makemig` behavior.
- Generate a shell script on Linux and a batch file on Windows.
- Quote paths and arguments safely for the generated shell script.

Acceptance:

- Linux `makemig` output can regenerate the expected image sequence.
- Windows `makemig.bat` output is unchanged.

Verification:

- Add Linux-only acceptance coverage for generated shell script output.
- Keep existing Windows batch expectations passing.
- Run `cmake --workflow rt-default`.

### Slice 5: Fix Windows executable resources

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

### Slice 6: Normalize version-sensitive test output

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

### Slice 7: Prefer inline formulas from parameter files

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

### Slice 8: Add periodicity-checking image coverage

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

### Slice 9: Add Chaos and Fractals bibliography entry

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

### Slice 10: Add online documentation permalinks

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
