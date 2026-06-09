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

## Slice 1: Require Package Docs In CMake

Work:

- Add CMake options for required package HTML and PDF docs.
- Default both options to `OFF` for local developer builds.
- Keep optional local behavior when the options are `OFF`.
- In required mode, install docs from their normal generated-doc build
  locations.
- Do not make package jobs build `html-doc` or `pdf-doc`.
- Let CPack fail through the install rules when required docs are missing.
- Fix the PDF install rule to install `id.pdf`.

Tests:

- Configure with both options `OFF` and no AsciiDoctor tools.
- Configure with both options `ON` and missing doc artifacts; packaging
  must fail.
- Configure with both options `ON` and doc artifacts present; packaging
  must include `id.html` and `id.pdf`.

## Slice 2: Put CI Package Presets In Required Mode

Work:

- Add a hidden CMake preset fragment that enables required package docs.
- Inherit it from `ci-debug`, `ci-release`, `ci-wx-debug`, and
  `ci-wx-release`.
- Download `html-docs` and `pdf-docs` into the same build-tree locations
  used by local doc builds before package creation.
- Download artifacts inside each package build directory, not from the
  parent of the build directory.
- Remove dependency on downloaded source-root `id.html` and `id.pdf`.
- Keep the normal and wx package paths consistent.

Tests:

- Run `ci-release` far enough to prove the package target consumes the
  doc artifacts.
- Run `ci-wx-release` far enough to prove the wx package path also consumes
  the doc artifacts.
- Confirm missing docs fail before artifacts are uploaded.

## Slice 3: Align CMake Documentation Targets

Work:

- Make the CMake `pdf-doc` command use the same `asciidoctor-pdf` options
  currently used by CI.
- Ensure CMake-generated HTML has access to `help/images`.
- Keep `ascii-doc`, `html-doc`, and `pdf-doc` target dependencies explicit.
- Keep local developer builds free of required PDF or HTML tools unless the
  package-doc options are enabled.

Tests:

- Build `ascii-doc`, `html-doc`, and `pdf-doc` targets directly.
- Verify `id.html` references copied `help/images` paths.
- Verify `id.pdf` embeds images and does not require copied images.

## Slice 4: Package Documentation Artifacts For Pages

Work:

- Change the `html-docs` artifact to include `id.html` and `help/images`.
- Leave the `pdf-docs` artifact as only `id.pdf`.
- Keep artifact paths shaped so publishing can copy them directly into a
  Pages directory.
- Preserve the existing doc jobs as the source for GitHub Pages
  publication.

Tests:

- Inspect `html-docs` artifacts and confirm `id.html` plus `help/images`.
- Inspect `pdf-docs` artifacts and confirm it only contains `id.pdf`.

## Slice 5: Publish Master Documentation

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

## Slice 6: End-To-End Workflow Checks

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
