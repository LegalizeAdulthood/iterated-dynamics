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

## Slice 1: Description Lists Rendered As Literals

Files:

- `help3.src:163-210`: Autokey accepted input forms.
- `help3.src:215-249`: Autokey authoring suggestions.
- `help3.src:428-444`: `logmap=` value descriptions.
- `help2.src:3837-3858`: Julibrot parameter screen descriptions.
- `help5.src:386-394`: fractal engine callback descriptions.

Nature:

These are prose lists.  The indentation is only an old visual convention
for "this text belongs under this term".

ADoc alternative:

Add whole-block `ADoc+` variants using AsciiDoc description lists.  Convert
short items such as `WAIT <nnn.n>` and `logmap=1` to `term:: description`.
For multi-paragraph items, use list continuations with `+` inside the ADoc
variant.

## Slice 2: Key And Command Reference Tables

Files:

- `help.src:964-967`: image restore choices.
- `help.src:1236-1252`: Orbits window keys.
- `help.src:1782-1786`: palette status legend.
- `help.src:2298-2319`: Evolver command keys.
- `help.src:1100-1108`: `<X>` screen links.

Nature:

These are compact key legends.  The first field is the key or screen label;
the remainder is explanatory prose.

ADoc alternative:

Use description lists for short legends.  Use an AsciiDoc table only
where multiple columns are significant.  Keep the source as one `ADoc+`
variant per whole legend block so line folding cannot split items into
literal blocks.

## Slice 3: Parameter And Library Tables

Files:

- `help4.src:30-275`: startup parameter summary.
- `help4.src:359-366`: preferred read-library subdirectories.
- `help4.src:372-383`: save-library subdirectories.
- `help4.src:487-491`: parameter syntax terms.
- `help4.src:573-581`: `comment=` variable expansion table.
- `help4.src:997-1038`: `textcolors=` meaning table and default.
- `help4.src:1076-1080`: color specification table.

Nature:

These are fixed-width column tables, not prose.  The indentation is
carrying column layout from the text-mode help.

ADoc alternative:

Use AsciiDoc tables for true columns.  For the long startup parameter
summary, prefer a two-column table with parameter and description,
preserving wrapped descriptions inside the table cell.  Use a source
block only for the `textcolors=` default value itself.

## Slice 4: File And Command Examples

Files:

- `help4.src:301`: simple command-line example.
- `help4.src:401-405`: `sstools.ini` example.
- `help4.src:420-427`: parameter-file examples.
- `help3.src:1598-1612`: terrain parameter sequence.
- `help4.src:1444-1451`: batch-mode setup steps.
- `help4.src:1463-1497`: batch-mode command examples.

Nature:

These are real examples, but their literal rendering should be intentional.
Some adjacent prose is currently pulled into literal blocks because the
whole example was not isolated in ADoc.

ADoc alternative:

Use `[source,console]`, `[source,ini]`, or plain listing blocks as
appropriate.  Wrap the entire example, including setup comments, in a
single ADoc variant.
Use normal AsciiDoc lists for the batch-mode steps.

## Slice 5: Math And Formula Blocks

Files:

- `help2.src:1048-1052`: Mandelbrot complex-coordinate display.
- `help2.src:2090-2096`: MandelbrotMix4 formula source.
- `help2.src:2114-2119`: DivideBrot5 formula source.
- `help2.src:3457-3467`: formula flow-control pseudo-code.
- `help2.src:3628`: formula expression example.
- `help2.src:4284-4667`: L-system grammar, commands, and examples.
- `help2.src:2781`: Lorenz default-value display.
- `help2.src:3983-3986`: Lyapunov string and numeric examples.
- `help2.src:3-950`: summary of fractal types.

Nature:

This is mixed mathematical notation and source syntax.  Monospace can be
correct for formula-file and L-system source, but mathematical prose
should be display math or normal prose.

ADoc alternative:

Use `[stem]` blocks for display equations and `[source]` blocks for formula
or L-system source.  The summary of fractal types is constrained by
`get_fract_params()` comments, so add ADoc variants around whole summary
entries rather than editing the rigid online/source structure in place.

## Slice 6: Quotes, Callouts, And Thematic Text

Files:

- `help5.src:493-496`: Herb Savage explanation quote.
- `help5.src:157-159`: "Who Is This Guy" quote.
- `help5.src:1283`: Stone Soup story separator.

Nature:

These are quoted or decorative text.  The indentation is presentation, not
code.

ADoc alternative:

Use quote blocks for quoted paragraphs.  Use an AsciiDoc thematic break
for the `***` separator if it is meant as a break, or keep it as normal
prose if it is part of the story text.

## Slice 7: Accepted Literal-Like Blocks

Files:

- `help5.src:1695-1733`: waveform diagrams.
- `help5.src:1796-1835`: sound-envelope diagrams.

Nature:

These are ASCII art or intentionally fixed-width diagrams.  They are not
prose bugs, but they still rely on accidental indentation today.

ADoc alternative:

Replace accidental indentation with explicit listing blocks.  Do this only
after the prose and table cases are fixed, because the rendered behavior is
already close to the intended output.
