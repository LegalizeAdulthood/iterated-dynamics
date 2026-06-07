# Iterated Dynamics 1.4 Release Plan

## Scope

This plan tracks the open GitHub issues assigned to the 1.4 milestone,
excluding issues tagged `wx` or `wx-tool`.

Slices are ordered by release priority. When a slice is complete, remove
it from this plan and renumber the remaining slices from 1.

## Slices

### Slice 1: Add Chaos and Fractals bibliography entry

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

### Slice 2: Add online documentation permalinks

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
