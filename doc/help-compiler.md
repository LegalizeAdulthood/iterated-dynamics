# Iterated Dynamics Help Compiler

The Iterated Dynamics (Id) help compiler is based on the help compiler
from [Fractint](https://www.fractint.net/).

The help compiler performs the following tasks related to the
help text for Id:

- Compile `.src` files into a binary help `.hlp` file.
- Generate a header file for referencing help topics in the code.
- Compile `.src` files into an AsciiDoc markup file.
- Format a `.src` file for printing.
- Append binary help `.hlp` file to an executable.
- Delete binary help from an executable.

The main use of the help compiler is to compile the `.src` help
markup files into the binary `.hlp` file used by Id at runtime and
the header file for referencing help topics in code.
A secondary use is to translate the `.src` help markup into AsciiDoc
markup used as input to produce the HTML help.  Appending the binary
help to the executable is no longer used in Id.

The general command-line syntax is as follows:

```
hc <mode> [options] [file1] [file2]
```

The mode of operation is required and is usually the first argument:

| Option | Mode |
| --- | --- |
| /c | Compile help source file for online help. |
| /adoc | Compile help source file to AsciiDoc. |
| /p | Format help source file for printing. |
| /a | Append binary help to executable. |
| /d | Delete binary help from executable. |

Additional options are as follows:

| Option | Description |
| --- | --- |
| /s | Report statistics. |
| /m | Report memory usage. |
| /i `path` | Add `path` to the include search list. |
| /r `path` | Set the swap file location to `path`. |
| /q | Quiet; no status messages are printed. |

## Help Compilation

With the `/c` mode, the `.src` file is compiled to a binary `.hlp` file
and a C++ header file is written.  The header file defines the help
version and an enum for referencing help topics in code.  Only a single
file can be specified to name the input `.src` file.  If the filename
is omitted, then `help.src` is assumed.

The help compiler takes as input a help source file (`.src`).  See
[Source File Format](#source-file-format)
for the `.src` file format.  Id's help file sources are located in the `hc/src`
folder.

### The Binary Output File

The `.hlp` file is the binary file where compiled help information is
stored.  See `hc.cpp` and `help.cpp` if you're interested in the file format.  In
Id the file is named `id.hlp`.

### The Header Output File

The header (`.h`) file contains an enum for each non-private label
(see [Labels](#labels)) and a macro for the help file version (see 
[Defining the Help File Version](#defining-the-help-file-version)).
The header 
file is included in Id and the enums are used to set the current help mode.

The help compiler will only modify the header file when a non-private label
is added or deleted or when the help file version is
changed.  This minimizes the times when Id will need to be recompiled.

In Id this file is named `helpdefs.h`

### AsciiDoc Output

With the `/adoc` mode, the `.src` file is compiled to an AsciiDoc markup
file.  Two filenames are allowed, the first specifying the input `.src`
file and the second specifying the output `.adoc` file.  If omitted,
the names `help.src` and `id.adoc` are used, respectively.  In this mode,
no header file is written and no binary help file is written.

# Source File Format

The source file consists of comments, commands and help text arranged into
topics.  Commands are used to define output files, help file version, topics
and labels.  Within topics hypertext-style hot-links, defined within the text,
are surrounded by curly braces (`{` and `}`).  Comment lines may appear anywhere
in the file.

## Comments

Any line starting with a semicolon (;) is a comment.  The comment
continues to the end of the line and may appear anywhere in the file.
The semicolon must be in the first column of the line.

```
; This is a comment line; all text on this line is ignored.
```

## Commands

Commands start
with a tilde (`~`) in the first column of a line and continue to the end
of the line.
Several commands may be "strung together" by separating them with commas.
For example:

```
~Topic=Help on help, Label=HELPONHELP, Format-
```

To have a comma as part of a command (like in a topic) precede it with a
backslash.
Commands can be embedded in the text with the following format:

```
~(command)
```

The text before the tilde and immediately after the ending parenthesis will be
part of the help text.

Commands have either a local or global scope.  A local scope affects only the
current topic.

| Command | Scope | Description |
| --- | --- | --- |
| Format[+/-] | Local | Turns formatting on/off. |
| Online[+/-] | Local | Enables/disables display of text in the online help. |
| Doc[+/-] | Local | Enables/disables display of text in the printed document.  (Currently ignored.) |
| ADoc[+/-] | Local | Enables/disables display of text in the AsciiDoc output. |
| FormatExclude=NUM | Both | Set the format disable column number. |
| FormatExclude=n | Both | Turn this feature off. |
| FormatExclude[+/-] | Both | Temporarily enable/disable this feature. |
| FormatExclude= | Local | Resets FormatExclude to its global value. |
| Center[+/-] | Local | Enable/Disable automatic centering of text. |

The FormatExclude=NUM command sets the column number in which formatting is
automatically disabled.  (ie. all text after
column NUM is unformatted.)  If before any topics
sets global value, if in a topic sets only for
that topic.  By default, FormatExclude is off at the start of the source file.

At the start of each topic the following local settings are in effect:

```
~Online+,Doc+,ADoc+,FormatExclude=,Format+,Center-
```

### Defining the Output Filenames

The output filenames are defined in the source file by the following
commands:

```
~HdrFile=H_FILE
~HlpFile=HLP_FILE
```

`H_FILE` is the header filename and `HLP_FILE` is the binary filename.  These
commands must appear before the first topic.

#### Defining the Help File Version

The help file version number is stored in a special macro in the header
file (named `HELP_VERSION`) and stored in the binary file header.  If the
version number in `HELP_VERSION` does not match the version in the binary file
the file will not be used.  This is mainly to make sure Id doesn't
try to read a help file other than the one the program is expecting.  To
define the help version:

```
~Version=nnn
```

Where nnn is a positive decimal version number.  (Suggested format is 100
for version 1.00, 150 for version 1.50, etc.)  This command must appear
before the first help topic.  -1 will be used if the version is not defined
in the source file.


### Help Topics

The help topics come after the `HdrFile=`, `HlpFile=` and `Version=` commands
and continue until end-of-file.

To start a new help topic use the following command:

```
~Topic=TITLE
```

`~Topic=` is the command to start a topic and `TITLE` is the text to display
at the top of each page.  The title continues to the end of the line and may contain
up to 65 characters.

In the example:

```
~Title=Command Keys available while in Display Mode
```

`Command Keys avail...` is the `TITLE` which would displayed at the top of
each page.

The help text for each topic can be several pages long. Each page is 22
rows by 78 columns.    (The help compiler will warn you if any page gets longer
than 22 lines.)  For the best appearance the first and last lines should
be blank.

#### Paragraph Formatting

The compiler determines the end of a paragraph when it encounters:

- a blank line.
- a change of indentation after the second line of a paragraph.
- a line ending with a backslash (`\`).
- If FormatExclude is enabled any line starting beyond the cut-off value.

The compiler only looks at the left margin of the text to determine paragraph
breaks.  If you're not sure the compiler can determine a paragraph boundry
append a backslash to the end of the last line of the paragraph.

The following examples illustrate when you need to use a backslash to make
the compiler format correctly.

#### Paragraph Headings

```
Heading
Text which follows the heading.  This text is supposed
to be a separate "paragraph" from the heading but the HC
doesn't know that!
```

This text would be formatted into a single paragraph.  (The word "Text"
would immediately follow "Heading".)  To make it format correctly append
a backslash to the end of "Heading".

#### Single Line Bullets

```
o Don't branch.
o Use string instructions, but don't go much out of your way
to do so.
o Keep memory accesses to a minimum by avoiding memory operands
and keeping instructions short.
```

Since the HC cannot tell that the first bullet is a separate paragraph
from the second bullet it would put both bullets into one paragraph.  Any
bullet of two lines or more (assuming the intentation for the second line
of the bullet is different from the first) is OK.  Always add a backslash
to the end of single-line bullets.

In general, if you cannot determine where a paragraph boundry is by
looking at the indentation of each line use a backslash to force a
paragraph break.

#### Special Characters

To insert reserved characters (like `;`, `~`, `\` and `{`) into the help
text precede the character with a backslash (`\`).  To insert any character
(except null) into the text follow a backslash with a decimal (not hex
or octal) ASCII character code.  For example:

```
\~  - puts a tilde in the text.
\24 - places ASCII code 24 (Ctrl+X) in the text.
```

### Starting Another Page

To start a new page in the same topic use the `~FF` command:

```
~FF
```

### Labels

A label is a name which in used to refer to a help topic.  A label is
used in hot-links or in Id for context-sensitive help.  When help
goes to a label (when a hot-link is selected or context-sensitive help
is called from Id) it goes to the page of the topic where the label
was defined.

To define a label for a topic insert the following command into the
topic's text:

```
~Label=NAME
```

`NAME` is the name of the label.  The name follows C style conventions
for variable names.  Case is significant.  The label should start at the
beginning of a line and have no text following it on the line.  The line
line will not appear in the help text.

For example, if this line:

```
~Label=HELP_PLASMA
```

was placed in page three of a topic using it in a hot-link (see [Hot Links](#hot-links))
or as context-sensitive help would "go to" page three of that topic.  The
user would then be free to page up and down through the entire topic.

Each topic must have at least one label otherwise it could not be
referred to.

#### The Help Index Label

When the user wants to go to the "help index" (by pressing \<F1\> while
in help) help will go to a special label named `HELP_INDEX`.  Other than
the requirement that it be in every `.src` file you may treat it as any
other label.  It can be used in links or as context-sensitive help.

#### Private Labels

A private label is a label which is local to the help file.  It can be
used in hot-links but cannot be used as context-sensitive help.  A private
label is a label with an "at sign" (`@`) as the first character of its
name.  The "at sign" is part of the name.  In the example:

```
~Label=@HELP_PLASMA
```

`@HELP_PLASMA` is a private label.

Private labels are not included in the .H file so you may add or delete
private labels without re-compiling Id.  Each non-private label
takes up some memory (4 bytes) in Id so it's best to use private
labels whenever possible.  Use private labels except when you want to use
the label as context-sensitive help.

### Hot Links

A hypertext-style hot-link to a label can appear anywhere in the help
text.  To define a hot-link use the following syntax:

```
{=LABEL TEXT}
```

`LABEL` is the label to link to and `TEXT` is the text that will be highlighted.
Only the `TEXT` field will appear on the help screen.  No blanks are allowed
before the `LABEL`.  In the sample hot-link:

```
{=HELP_MAIN Command Keys in display mode}
```

`HELP_MAIN` is the `LABEL` and `Command keys in display mode` is the
`TEXT` to will be hightlighted.

Hot-links support implicit links.  These are links
which link to the topic whose title matches the highlighted text.  They have
no label field.  In the example:

```
Press <C> to goto {Color Cycling Mode}.
```

The link will link to the topic whose title is "Color Cycling Mode".  The
link must match the topics' title exactly except for case and leading or
trailing blanks.

The compiler will ignore quotes around implicit hot-links when searching for
the matching title.  This allows hot-links like:

```
{"Video adapter notes"}
```

intead of:

```
"{Video adapter notes}"
```

This is so that the hot-link page numbers don't end up inside the
quotes like: "Video adapter notes (p. 21)".  It also keeps quotes from
being separated from the text in the online-help.

#### Special Hot Links

In addition to normal hot-links to labels "special" links to custom
functions are allowed.  These hot-links have a
negative number (-2..) in place of the `LABEL`.  No special hot-links are
currently supported in Id.

### Tables

A table is a way of arranging a group of hot-links into evenly-spaced
columns.  The format for a table is:

```
~Table=WIDTH COLS INDENT

<< LINKS... >>

~EndTable
```

`WIDTH` is the width of each item, `COLS` is the number of columns in the
table and `INDENT` is the number of spaces to indent the table.  `LINKS` is
a list of hot-links (see [Hot Links](#hot-links)) which will be arranged.
Only blanks and new-lines are allowed between the hot-links; other characters
generate an error.

In the example table:

```
~Table=20 3 9
{@ONE One}
{@TWO Two}
{@THREE Three}
{@FOUR Four}
{@FIVE Five}
{SIX Six}
{SEVEN Seven}
{EIGHT Eight}
~EndTable
```

20 is the `WIDTH`, 3 is `COLS` and `INDENT` is 9.  The following text would
be produced by the table:

```
         One         Two         Three
         Four        Five        Six
         Seven       Eight
```

Each item would be a hot-link linking to its corresponding label (`@ONE` for
"One", etc.)  Any legal hot-link (to private or non-private labels) may be
used in a table.

The same effect could be produced by arranging the hot-links into
columns yourself but using a table is usually easier (especially if the
label names vary in length).

### Multi Line Comments

```
~Comment, ~EndComment
```

This can be used to start and end multi-line comments.  This can be used
to comment out sections of a source file without having to prefix each line
with a semi-colon (`;`).

### Space Compression

```
~CompressSpaces[+/-]
```
Turn on/off the automatic compression of spaces.  Used for data topics
and when reading normal topics with read_topic()

### Data Topics

```
~Data=label_name
```
Starts a topic which contains data.  Think of it as a macro for:

```
~Topic=, Label=label_name, Format-, CompressSpaces-
```

Data labels cannot be part of the document or displayed as online help.

### Binary Inclusion

```
~BinInc filename
```
Includes a file in binray format (no processing) into a data topic.
This command only works for data topics.  Example:
```
~Data=TPLUS_DAT
~BinInc tplus.dat
```

### Table of Contents

```
~DocContents
```
Defines the documents table of contents.  Text is allowed.  Table of
content entries have the following format:

```
{ id, indent, name, [topic, ...] [, FF] }
```

| Name | Description |
| --- | ---- |
| `id`  | is it's identification (ie, 1.2.1) |
| `indent` | is the indentation level, (ie. 2) |
| `name` | is the text to display to display (ie. Id Commands). If `name` is in quotes it is also assumed to also be the title of the first topic. |
| `topic`  | list of 0 or more topics to print under this item.  Entries in quotes are assumed to be the title of a topic, other entries are assumed to be labels. |
| `FF` | If this keyword is present the entry will start on a new page in the document.  It isn't as complex as it sounds; see `~DocContents` in `help.src` for examples. |

## Document Printing

The `/p` command-line option compiles the `.src` file and prints the
document.  It does **not** write the `.hlp` file.

The function prototype (in `help.cpp`) to print the document is:

```
void print_document(char *outfname, int (*msg_func)(int,int));
```

`outfname` is the file to "print" to (usually `id.txt`), `msg_func`
is a function which is called at the top of each page.

See `print_doc_msg_func()` in `help.cpp` for an example of how to use `msg_func`.
If `msg_func` is `nullptr` no `msg_func` is called.


### Printing from Help

There are hooks to print the document from a help topic by selecting a
"special" hot-link. I suggest a format like:

```
~Topic=Generating id.txt

Explain that selecting yes will print the document, etc...

{=-2  Yes, generate id.txt now. }
{=-1  No, do NOT generate id.txt. }
```

Selecting yes will generate the document and then return to the previous
topic.  Selecting no will simply return to the previous topic (like
backspace does.)
