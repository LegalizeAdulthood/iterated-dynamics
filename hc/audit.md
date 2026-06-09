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

## Slice 1: Replace ADoc Pagination

Goal: remove the misleading online-pagination copy from the ADoc path.

Files:

- `hc/AsciiDocCompiler.cpp`
- possibly `hc/HelpCompiler.cpp` if shared link helpers move

Work:

- Replace `paginate_ascii_doc()` with an ADoc link reachability pass.
- Stop using `TokenMode::ONLINE` for ADoc bookkeeping.
- Rename messages so they describe ADoc generation, not HTML pagination.
- Keep document membership as an explicit Boolean if that is all ADoc
  needs.

Done when:

- ADoc link resolution no longer depends on online page layout.
- No ADoc status message says "Paginating HTML."

## Slice 2: Emit Explicit Anchors

Goal: stop guessing Asciidoctor-generated section IDs.

Files:

- `hc/AsciiDocCompiler.cpp`
- `hc/HelpCompiler.cpp` if `write_help_links()` should share code
- `hc/tests/ascii_doc/links.*`

Work:

- Generate stable explicit anchors for contents and topics.
- Link to those anchors instead of normalized section titles.
- Share one normalizer for HTML help links if practical.
- Add punctuation and duplicate-title tests.

Done when:

- Cross references do not rely on Asciidoctor's implicit ID algorithm.
- Titles with punctuation link predictably.

## Slice 3: Isolate Raw ADoc Blocks

Goal: keep raw AsciiDoc block structure away from prose newline logic.

Files:

- `hc/AsciiDocCompiler.cpp`
- `hc/tests/ascii_doc`

Work:

- Track raw block contexts before global blank-line compression.
- Cover `[stem]` passthrough blocks, tables, image macros, listing blocks,
  and other delimiter-based blocks used by the current sources.
- Preserve intentional blank lines inside raw blocks.
- Keep the escaped brace stem-matrix test.

Done when:

- Raw ADoc blocks bypass prose-only newline compression.
- Stem matrices do not require markup workarounds.

## Slice 4: Harden Inline Heuristics

Goal: reduce false positives in key and bullet conversion.

Files:

- `hc/AsciiDocCompiler.cpp`
- `hc/tests/ascii_doc/keys.*`
- `hc/tests/ascii_doc/bullets.*`

Work:

- Add false-positive tests for literal `<...>` text and placeholders.
- Add false-positive tests for prose beginning with `o `.
- Move recognition closer to paragraph/source context if practical.
- Preserve existing `kbd:[]` and legacy bullet behavior.

Done when:

- Known key names still become `kbd:[]`.
- Non-key angle-bracket text stays literal.
- Non-bullet `o ` lines stay literal.

## Slice 5: Add Rendered ADoc Tests

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

## Slice 6: Document ADoc Authoring Rules

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
