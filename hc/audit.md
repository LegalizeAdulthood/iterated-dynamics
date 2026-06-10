<!-- Copyright 2026 Richard Thomson -->

# AsciiDoc Help Compiler Implementation Plan

Scope: harden the `/adoc` path in `hc`, `helpcom`, and the AsciiDoc
acceptance tests.  Each slice is intended to be one small reviewable
commit.

## Ground Rules

- Keep source edits CRLF, no BOM, ASCII.
- Use focused `cmake --workflow temp` runs for code/test slices.
- Do not run publishing or documentation site workflows for these fixes.
- Keep AsciiDoc source variants at whole paragraph or whole block scope.

## Input Invariants

The input syntax in `home/doc/help-compiler.md` is part of the contract.

- `ADoc+` and `ADoc-` only select text for AsciiDoc output.  They do not
  create an Asciidoctor block boundary.
- `Format+` folds physical source lines into compiler paragraphs.
- A compiler paragraph ends on a blank line, an indentation change after
  the second line, a trailing backslash, or `FormatExclude`.
- Raw AsciiDoc blocks need `Format-`.
- Escaped reserved characters such as `\{` prevent hot-link parsing, but
  the escape marker is not retained by the ADoc output layer.

## Slice 1: Add Rendered ADoc Tests

Goal: catch cases where generated `.adoc` compares clean but renders
wrong.

Files:

- `hc/tests/ascii_doc`
- `hc/tests/cmake` if a shared rendered-compare helper is useful
- `hc/src/CMakeLists.txt` only if test discovery needs Asciidoctor paths

Work:

- Keep current gold-file tests as fast source-output checks.
- Add a small rendered HTML test set for stem matrices, tables, images,
  key macros, and cross references.
- Skip rendered tests cleanly when Asciidoctor is unavailable.

Done when:

- Syntax-valid but badly rendered ADoc has a focused test path.
- Machines without Asciidoctor can still run the normal unit suite.

## Slice 2: Document ADoc Authoring Rules

Goal: make the source authoring contract explicit.

Files:

- `home/doc/help-compiler.md`

Work:

- Document that ADoc variants should cover whole compiler paragraphs or
  blocks.
- Document that raw AsciiDoc blocks require `Format-`.
- Document the escape behavior for reserved characters in raw ADoc.

Done when:

- Future doc edits do not need to rediscover the paragraph-boundary rule.
