# Remove Boost Algorithm

## Goal

Remove the `boost-algorithm` vcpkg dependency by replacing the small set of
string helpers used by Id with local, straightforward implementations.

Keep the replacement narrow.  This is not a general string library.  It
only needs to cover the algorithms used today:

- ASCII lowercase copy
- ASCII uppercase copy
- replace all occurrences of one string with another string
- split a string on one delimiter character
- split a string on any character from a delimiter set

## Current Use

Production use:

- `hc/AsciiDocCompiler.cpp`
  - `to_lower_copy`
  - `replace_all`
- `libid/io/search_path.cpp`
  - `split`
  - `is_any_of`

Test-only use:

- `tests/libid/engine/test_lowerize_parameter.cpp`
  - `to_lower_copy`
  - `to_upper_copy`
- `tests/libid/io/test_find_file.cpp`
  - `to_lower_copy`
- `tests/libid/io/test_load_entry_text.cpp`
  - `split`

Build wiring:

- `hc/CMakeLists.txt`
  - `find_package(boost_algorithm CONFIG REQUIRED)`
  - `Boost::algorithm`
- `libid/CMakeLists.txt`
  - `find_package(boost_algorithm CONFIG REQUIRED)`
  - `Boost::algorithm`
- `tests/libid/CMakeLists.txt`
  - `find_package(boost_algorithm CONFIG REQUIRED)`
  - `Boost::algorithm`
- `vcpkg.json`
  - `boost-algorithm` in the `hc` and `id` features

## Slice 1: Move Help Compiler Off Boost

Goal: remove the help compiler production dependency on Boost Algorithm.

Work:

- Replace `boost::algorithm::to_lower_copy` in
  `hc/AsciiDocCompiler.cpp`.
- Replace `boost::algorithm::replace_all` in
  `hc/AsciiDocCompiler.cpp`.
- Link `libhc` to the local string-algorithm target.
- Remove `Boost::algorithm` from `libhc`.
- Remove `find_package(boost_algorithm CONFIG REQUIRED)` from
  `hc/CMakeLists.txt` if no other help compiler target needs it.

Tests:

- Build the help compiler.
- Run the help compiler, AsciiDoc, HTML, PDF, and text comparison tests
  that exercise generated help output.

Done when:

- `hc` has no Boost Algorithm includes.
- Generated help output is unchanged.

## Slice 2: Move libid Off Boost

Goal: remove the libid production dependency on Boost Algorithm.

Work:

- Replace Boost splitting in `libid/io/search_path.cpp`.
- Link `libid` to the local string-algorithm target.
- Remove `Boost::algorithm` from `libid`.
- Remove `find_package(boost_algorithm CONFIG REQUIRED)` from
  `libid/CMakeLists.txt` if no other libid source needs it.

Tests:

- Run `TestSearchPath.*`.
- Run any file lookup tests that cover environment search paths.
- Run the full `test-id` target if isolated tests pass.

Done when:

- `libid` has no Boost Algorithm includes.
- Search path behavior is unchanged on Windows and Linux.

## Slice 3: Remove Package Dependency

Goal: remove `boost-algorithm` from build configuration after all callers
are gone.

Work:

- Remove `find_package(boost_algorithm CONFIG REQUIRED)` from
  `tests/libid/CMakeLists.txt`.
- Remove `Boost::algorithm` from `test-id`.
- Remove `boost-algorithm` from the `hc` feature in `vcpkg.json`.
- Remove `boost-algorithm` from the `id` feature in `vcpkg.json`.
- Search the repository, excluding bundled vcpkg metadata, for remaining
  `boost/algorithm`, `boost::algorithm`, `Boost::algorithm`, and
  `boost_algorithm` references.

Tests:

- Reconfigure from a clean or refreshed build tree.
- Run `cmake --workflow rt-default`.

Done when:

- The repository no longer depends on `boost-algorithm`.
- `boost-endian` remains in place for GIF serialization.
- The default workflow is green.
