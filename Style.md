<!--
SPDX-License-Identifier: GPL-3.0-only
-->
# Contents

- [Design Principles](#design-principles)
  - [SOLID](#solid)
  - [DRY](#dry)
- [C++ Standard](#c-standard)
- [Names](#names)
  - [Types](#types)
  - [Macros](#macros)
  - [Constants](#constants)
  - [Enumerations](#enumerations)
  - [Functions and Methods](#functions-and-methods)
  - [Variables](#variables)
    - [Member Variables](#member-variables)
    - [Static Variables](#static-variables)
    - [Global Variables](#global-variables)
  - [Files](#files)
    - [Plain Functions](#plain-functions)
    - [Classes](#classes)
- [A Few Words About Enumerations](#a-few-words-about-enumerations)
- [Formatting](#formatting)
- [Braces](#braces)
- [Header Guards](#header-guards)
- [License Comment](#license-comment)
- [Additional Guidelines](#additional-guidelines)


# Design Principles

The source code of FRACTINT, the ancestor of Iterated Dynamics (Id),
evolved under the many constraints of early MS-DOS programs:
limited memory, limited hardware acceleration and limited
disk space.  Many clever tricks were used in order to squeeze
as much code into as little space as possible, resulting in
a certain amount of tangling up of the source code.
Every effort has been made with Iterated Dynamics to untangle
these tricks and make the code more straightforward.

## SOLID

For new code, Id follows the [SOLID](https://en.wikipedia.org/wiki/SOLID)
design principles.  While most of the code in Id is still
procedural in nature and the SOLID design principles come
from the experiences of object-oriented programming, the
principles apply to any style of programming.  The
[Single Responsibility Principle (SRP)](https://en.wikipedia.org/wiki/Single-responsibility_principle)
is the most important.

Simply put, the idea behind SRP is that any unit of code
should do one thing only.  Variables should be used for
only one purpose.  Functions should do only one thing.
Files should contain data and functions that achieve a
single goal.  Types (structs, classes, etc.) should only do
one thing.

## DRY

Another important design principle is
[DRY (Don't Repeat Yourself)](https://en.wikipedia.org/wiki/Don%27t_repeat_yourself).
Duplication of code represents a point of future divergence
and a source of bugs.  When duplicated code diverges,
it becomes difficult to see the differences.  When adding
code to Id, the best way to avoid duplication is to familiarize
yourself with the existing code and use existing mechanisms
instead of duplicating them.

# C++ Standard

Id uses the [C++17](https://en.cppreference.com/w/cpp/17) language
standard.  This provides access to the
[`<filesystem>`](https://en.cppreference.com/w/cpp/filesystem)
library and the
[`<any>`](https://en.cppreference.com/w/cpp/header/any),
[`<optional>`](https://en.cppreference.com/w/cpp/header/optional),
[`<string_view>`](https://en.cppreference.com/w/cpp/header/string_view),
and [`<variant>`](https://en.cppreference.com/w/cpp/header/variant)
headers.  Any valid C++17 code is permitted.

# Names

There are two hard problems in computer science:

- Cache coherency
- Naming things
- Off by one errors `:)`

Iterated Dynamics uses the following naming conventions.
Every attempt has been made to refactor the code to follow
these conventions, but there may still be isolated instances
where some things (most likely local variables) are not yet
following these conventions.  Pull requests that correct any
deviations are appreciated!  Please use isolated commits for
adjustments to conform to style conventions and keep functional
changes in separate commits.

In the C++ and C standards, certain names are reserved for use
by the standard or the implementation and are to be avoided.
The details can be found on the [identifiers page](https://en.cppreference.com/w/cpp/language/identifiers)
of [cppreference.com](https://cppreference.com), but generally
speaking avoid using names that begin with underscore (`_`) or
any name containing two consecutive underscores (`__`).

## Types

Use `UpperCamelCase` for the names of types, such as `enum`,
`struct`, `class` and `union` types.  Template arguments
that represent types also follow this convention.

## Macros

There are very few reasons to use macros anymore.  In almost
every case there are better mechanisms available to the C++
programmer.  In the few cases where a macro is the only thing
that can do the job, use `ALL_UPPER` names for macros.

## Constants

Constants use `ALL_UPPER` names and are best declared as
[`constexpr`](https://en.cppreference.com/w/cpp/language/constexpr),
which can be safely used in header files without violating the
[one definition rule](https://en.cppreference.com/w/cpp/language/definition).
For groups of related constants, consider using an `enum class`.

## Enumerations

Individual enumeration values use `ALL_UPPER` names, identical
to the names of constants.

## Functions and Methods

The names of functions and methods use `all_lower_case`
with underscores separating words in compound names.

## Variables

The names of variables use `all_lower_case` with underscores
separating words in compound names.

### Member Variables

The names of member variables additionally use an `m_` prefix.

### Static Variables

Variables of `static` scope (e.g. variables declared `static`
at file scope) additionally use an `s_` prefix.

Adding new static variables should be avoided.  Static variables
are just another form of global variable and hinder the evolution
towards multi-threading.  Iterated Dynamics has too much global
data already `:)`.

### Global Variables

Global variables declared `extern` in a header file and defined in a
source file additionally use a `g_` prefix.

Adding new global variables should be avoided.  Global variables
hinder the evolution towards multi-threading.  Iterated Dynamics has
too much global data already `:)`.

### Test Cases

Id uses [Google Test](https://google.github.io/googletest/) for unit
tests.  Google Test prohibits test fixture names and test case
names from containing an underscore (see the
[TEST and TEST_F reference](https://google.github.io/googletest/reference/testing.html#TEST)).
Use `UpperCamelCase` for the names of test suites and fixtures and
`lowerCamelCase` for the names of test cases.

## Files

In general, files are named according to the main item inside the
file.

### Plain Functions

Files declaring and defining plain functions are named
according to the main function for the file.  For example, the
function `decoder` is declared in a file named `decoder.h` and
implemented in a file named `decoder.cpp`.

### Classes

Files declaring and defining classes are named according to
the class.  For example, the help compiler class `Compiler`
is declared in `Compiler.h` and implemented in `Compiler.cpp`.

# A Few Words About Enumerations

Prefer scoped enumerations, declared with `enum class`, over plain
enumerations declared with `enum`.  Sometimes it can be convenient
to treat a scoped enumeration value as an integer.  Plain enums
are always implicitly convertible to an integer, but scoped enumerations
must be explicitly cast to an integer.  To cut down on the noise,
when a scoped enumeration is often used as an integer, e.g. as an index
into an array, declare a prefix `operator+` for the enumeration type
and use the expression `+EnumType::ENUM_VALUE` when the integer value
is desired.

Sometimes an enumeration type is used to represent a set of composable
flags, with the eumeration values assigned to powers of two.  In this
scenario, declare appropriate `operator|`, `operator&` and `operator~`
inline functions to support combination of enumeration values.  In
such cases, the name of the enumeration type typically ends with the word
`Flags`.  It can be convenient to define an inline function `bit_set`
that takes two such flags values and checks that the bits defined by
the second argument are set in the first argument:

```
inline FooFlags bit_set(FooFlags lhs, FooFlags rhs)
{
    return (lhs & rhs) == rhs;
}
// example usage:
//     if (bit_set(flags, FooFlags::VALUE))
//     { ... }
```

# Formatting

All new code should be formatted with `clang-format` before
submitting a pull request.  Visual Studio supports using formatting
with clang-format directly, but it can als be installed separately
and invoked directly on a source file from the command-line.
The `.clang-format` file at the top of the repository configures
the appropriate settings for clang-format.

Sometimes long expressions, such as a sum of polynomial terms,
or very long argument lists yield less than desirable formatting.
One way to get the desired formatting is to break up long expressions
or long lists with empty single-line comments (`//`) to get line
breaks at the desired locations.

In rare cases, usually when defining large tables of data,
formatting can be turned off and on with the comments:

```
// clang-format off
// clang-format on
```

# Braces

All control statements (`do`, `if`, `while`, `for`, etc.) should
have braces around the nested statements.  This is done even if
the nested statement is empty or a single statement.  For example:

```
char *text{get_something()};
char *end;
for (end = text; *end; ++end)
{
}
```

Formatting with clang-format will properly handle the indentation
of the nested statements and the formatting of brace position and
brace indentation.  However, clang-format won't automatically add
the braces if they are omitted.

# Header Guards

Use `#pragma once` for header guards on all include files.

# License Comment

Iterated Dynamics is licensed under the GNU General Public License v. 3.
All files should include a license comment -- using whatever
commenting convention is appropriate for the file -- designating
the license's SPDX identifier.  For example, for C++ source files:

```
// SPDX-License-Identifier: GPL-3.0-only
//
```

# Additional Guidelines

The C++ community collaborates on the
[C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
for C++ programmers.  Most of these guidelines apply to the code in Id
and are considered good advice, but there is no strict requirement to
follow each and every guideline listed there.  The main authors of Id
recommend skimming through the guidelines and pausing to read any individual
guideline that seems interesting to you.
