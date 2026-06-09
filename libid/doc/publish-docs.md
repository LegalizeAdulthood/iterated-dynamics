<!--
SPDX-License-Identifier: GPL-3.0-only
-->

# Publish Documentation Workflow Plan

Goal: make package documentation and GitHub Pages publication explicit.

The proposed direction is sound.  The important gaps are:

- CI package builds should not need documentation tools on every package
  platform.
- Required package docs must come from downloaded doc artifacts, not
  source-root fallback files.
- CI package jobs should unpack doc artifacts into the generated-doc build
  locations.
- The CMake PDF target must use the same AsciiDoctor PDF options as CI.
- The current PDF install rule should install `id.pdf`, not `id.html`.
- The HTML docs artifact must carry `help/images` with `id.html`.
- Documentation artifacts should be rooted at `hc/src`, without the
  `build-ci-ascii-doc` prefix.
- Publishing `/master` docs must run only for pushes to `master`.
- Tag documentation publication and current-doc promotion should stay
  unchanged.

## Slice 1: Publish Master Documentation

Work:

- Add a `master-docs` job in `build.yml`.
- Run it only on `push` events to the `master` branch.
- Make it depend on `html-docs` and `pdf-docs`.
- Checkout `gh-pages`.
- Replace the `master` directory with the downloaded docs.
- Publish `id.html` as `master/index.html`.
- Publish `id.pdf` as `master/id.pdf`.
- Publish HTML images under `master/help/images`.
- Commit and push only when the generated output changes.
- Add concurrency so two Pages publishes do not race.

Tests:

- Run the job on a `master` push and verify `/master/index.html`.
- Verify `/master/id.pdf` is present.
- Verify tag docs and current-doc promotion still behave as before.

## Slice 2: End-To-End Workflow Checks

Work:

- Verify CI package jobs fail when required docs are missing.
- Verify package artifacts include `id.html`, `id.pdf`, and HTML images.
- Verify release artifacts are unchanged except for containing docs.
- Verify `/master` docs update on `master` pushes.
- Verify versioned docs still update on tag pushes.
- Verify current docs still require environment approval.

Tests:

- Use a temporary branch to validate the `master-docs` job shape.
- Use a tag test run or dry run to validate versioned docs are untouched.
- Inspect generated packages and Pages contents from workflow artifacts.
