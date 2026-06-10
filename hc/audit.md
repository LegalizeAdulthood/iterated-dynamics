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

## Remaining Slices

None.
