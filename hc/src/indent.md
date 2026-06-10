<!--
SPDX-License-Identifier: GPL-3.0-only
Copyright 2026 Richard Thomson
-->

# Indented Paragraph ADoc Cleanup Plan

The generated AsciiDoc still has several paragraphs that begin with spaces.
Asciidoctor interprets those as literal blocks.  Some are real examples
or ASCII diagrams and should stay literal-like, but they should be
expressed with explicit AsciiDoc source, stem, listing, or table markup.
The problem cases are old help-viewer layout conventions where
indentation meant "description" or "align this text", not "make a
literal block".

## Classification

Definition-list prose:

Use AsciiDoc description lists.  These blocks usually have an unindented
term followed by indented prose, or every item is indented to align in the
online help viewer.  They should not render in monospace.

Command and option tables:

Use AsciiDoc tables when alignment carries columns, or description lists
when the first field is a command and the rest is prose.  Keep any
existing online fixed-width text under `ADoc-`, and add an `ADoc+`
whole-block variant.

Examples and command lines:

Use explicit `[source]` or listing blocks.  These are valid literal
content, but should not depend on accidental paragraph indentation.

Math and formula sketches:

Use `[stem]` blocks for display math where practical.  Use listing blocks
for formula-file syntax or parser examples that must remain source text.

Quotes and separators:

Use quote blocks or thematic breaks.  These should not be monospace just
because the source was indented for visual effect.

## Slicing Rule

Each slice below changes one source block or one topic.  When a slice is
implemented, remove it and renumber the remaining slices from 1.

## Slice 1: Autokey Input Forms

Source: `help3.src:163-210`

Nature: prose descriptions with embedded examples.

ADoc: use a whole-block `ADoc+` variant with `Format-`.  Use a
description list for input forms and listing blocks for examples.

## Slice 2: Autokey Authoring Suggestions

Source: `help3.src:215-249`

Nature: advice list.

ADoc: use a whole-block `ADoc+` variant with `Format-`.  Use an
unordered list, with listing blocks for `.KEY` fragments.

## Slice 3: Logmap Value Descriptions

Source: `help3.src:428-444`

Nature: parameter-value descriptions.

ADoc: use a description list.  Terms are `logmap=1`, `logmap=N`,
`logmap=-N`, `logmap=2 or -2`, and `logmap=-1`.

## Slice 4: Julibrot Parameter Screens

Source: `help2.src:3837-3858`

Nature: screen-field descriptions.

ADoc: use a description list.  Keep `From/To Parameters` as its own term
rather than as indented prose under `Orbit parameters`.

## Slice 5: Fractal Engine Callbacks

Source: `help5.src:386-394`

Nature: ordered prose list.

ADoc: use an ordered list.  Do not use a description list here.

## Slice 6: Image Restore Choices

Source: `help.src:964-967`

Nature: compact key legend.

ADoc: use a description list.

## Slice 7: Orbits Window Keys

Source: `help.src:1236-1252`

Nature: key legend with wrapped descriptions.

ADoc: use a description list, with list continuations for wrapped text.

## Slice 8: Palette Status Legend

Source: `help.src:1782-1786`

Nature: compact status-symbol legend.

ADoc: use a two-column table or a description list.

## Slice 9: Evolver Command Keys

Source: `help.src:2298-2319`

Nature: key-command legend.

ADoc: use a description list, with continuations for long key
descriptions.

## Slice 10: X Options Screen Links

Source: `help.src:1100-1108`

Nature: topic link list.

ADoc: use an unordered list or description list with cross references.

## Slice 11: Startup Parameter Summary

Source: `help4.src:30-275`

Nature: large fixed-width parameter table.

ADoc: use a two-column table with parameter and description cells.

## Slice 12: Read-Library Subdirectories

Source: `help4.src:359-366`

Nature: directory-name table.

ADoc: use a two-column table.

## Slice 13: Save-Library Subdirectories

Source: `help4.src:372-383`

Nature: directory-name table.

ADoc: use a two-column table.

## Slice 14: Parameter Syntax Terms

Source: `help4.src:487-491`

Nature: compact syntax legend.

ADoc: use a description list.

## Slice 15: Comment Variable Expansion Table

Source: `help4.src:573-581`

Nature: two side-by-side fixed-width tables.

ADoc: use one table with variable, expansion, and example columns.

## Slice 16: Textcolors Meaning Table

Source: `help4.src:997-1038`

Nature: grouped numeric legend plus a long default value.

ADoc: use a table for meanings and a source block for the default value.

## Slice 17: Color Specification Table

Source: `help4.src:1076-1080`

Nature: compact value legend.

ADoc: use a two-column table.

## Slice 18: Simple Command-Line Example

Source: `help4.src:301`

Nature: command-line example.

ADoc: use a `[source,console]` or listing block.

## Slice 19: Sstools.ini Example

Source: `help4.src:401-405`

Nature: `.ini` file example.

ADoc: use a `[source,ini]` block.

## Slice 20: Parameter-File Examples

Source: `help4.src:420-427`

Nature: parameter-file examples.

ADoc: use a source block.

## Slice 21: Terrain Parameter Sequence

Source: `help3.src:1598-1612`

Nature: commented parameter sequence.

ADoc: use a source block around the complete sequence.

## Slice 22: Batch Setup Steps

Source: `help4.src:1444-1451`

Nature: procedural steps.

ADoc: use an unordered list, not a literal block.

## Slice 23: Batch Command Examples

Source: `help4.src:1463-1497`

Nature: command-line examples.

ADoc: use `[source,console]` blocks around the commands.

## Slice 24: Mandelbrot Coordinate Display

Source: `help2.src:1048-1052`

Nature: displayed math expression.

ADoc: use inline prose or a `[stem]` block.

## Slice 25: MandelbrotMix4 Formula Source

Source: `help2.src:2090-2096`

Nature: formula-file source.

ADoc: use a source block.

## Slice 26: DivideBrot5 Formula Source

Source: `help2.src:2114-2119`

Nature: formula-file source.

ADoc: use a source block.

## Slice 27: Formula Flow-Control Pseudo-Code

Source: `help2.src:3457-3467`

Nature: pseudo-code.

ADoc: use a source block.

## Slice 28: Formula Expression Example

Source: `help2.src:3628`

Nature: formula expression.

ADoc: use a source block or inline monospace.

## Slice 29: L-System Source And Command Reference

Source: `help2.src:4284-4667`

Nature: one topic containing grammar, examples, and command reference.

ADoc: add ADoc variants inside the topic one block at a time, but keep the
slice scoped to this topic.

## Slice 30: Lorenz Default-Value Display

Source: `help2.src:2781`

Nature: parenthetical note displayed by indentation.

ADoc: make it normal prose.

## Slice 31: Lyapunov String Example

Source: `help2.src:3983-3986`

Nature: worked string-to-number example.

ADoc: use a source block or a table.

## Slice 32: Summary Barnsleyj3 Formula

Source: `help2.src:57-64`

Nature: summary formula source.

ADoc: add a whole-entry `ADoc+` variant around the `barnsleyj3` summary.

## Slice 33: Summary Barnsleym3 Formula

Source: `help2.src:83-90`

Nature: summary formula source.

ADoc: add a whole-entry `ADoc+` variant around the `barnsleym3` summary.

## Slice 34: Summary Test Fractal Description

Source: `help2.src:897-901`

Nature: short prose description accidentally split into a literal block.

ADoc: add a whole-entry `ADoc+` variant with normal prose.

## Slice 35: Herb Savage Explanation Quote

Source: `help5.src:493-496`

Nature: quoted explanation.

ADoc: use a quote block.

## Slice 36: Mandelbrot Impact Quote

Source: `help5.src:157-159`

Nature: quoted statement.

ADoc: use a quote block.

## Slice 37: Stone Soup Story Separator

Source: `help5.src:1283`

Nature: thematic break.

ADoc: use an AsciiDoc thematic break.

## Slice 38: Waveform Diagrams

Source: `help5.src:1695-1733`

Nature: intentional ASCII art.

ADoc: use explicit listing blocks.

## Slice 39: Sound-Envelope Diagrams

Source: `help5.src:1796-1835`

Nature: intentional ASCII art.

ADoc: use explicit listing blocks.
