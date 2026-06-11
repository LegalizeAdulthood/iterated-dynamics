# Security Hotspots Plan

This plan tracks review and cleanup of all SonarCloud security hotspots for
the project.

The full inventory is in `security-hotspots-inventory.csv`.  It was fetched
from SonarCloud with project key `LegalizeAdulthood_iterated-dynamics` and
status `TO_REVIEW`.

## Inventory Summary

Total hotspots: 271.

By rule:

- 78 `cpp:S5801`: `strcpy` or `wcscpy`.
- 55 `cpp:S5813`: `strlen` or `wcslen`.
- 50 `cpp:S6069`: `sprintf` or related formatting.
- 40 `cpp:S5814`: `strcat` or `wcscat`.
- 15 `cpp:S5816`: `strncpy` or `wcsncpy`.
- 14 `githubactions:S7636`: secret expansion in run blocks.
- 11 `cpp:S2245`: pseudorandom number generator use.
- 3 `githubactions:S7637`: short action dependency hashes.
- 3 `c:S5801`: C `strcpy`.
- 2 `cpp:S5443`: public writable directory use.

By area:

- 196 `libid`.
- 35 `tests`.
- 17 `hc`.
- 17 `.github`.
- 3 `win32`.
- 3 `legacy`.

## Review Policy

Work in small slices.  Each slice removes hotspots or documents why one is
safe and ready for Sonar review.

Prefer eliminating raw C buffer ownership where a local `std::string`,
`std::array`, `std::string_view`, `fmt`, or parser abstraction makes the
contract simpler.

Do not mechanically replace one C API with another.  The replacement must
make the size contract explicit and testable.  Avoid swapping `strcpy` for
`strncpy` as the terminal fix.

Give production code priority over tests, but keep test-only hotspots in
the inventory so final review reaches zero unreviewed items.

## Slices

### Slice 1: Command-Line And File Command Inputs

Start with parser entry points that consume command-line, command-file, or
environment input.  These are the highest-risk buffer-overflow hotspots.

Candidate files:

- `libid/engine/cmdfiles.cpp`
- `libid/fractals/parser.cpp`
- `libid/fractals/lsystem.cpp`
- `libid/io/load_config.cpp`

Expected fixes:

- Use `std::string_view` for read-only routing.
- Create mutable parser buffers only at APIs that still require mutation.
- Preserve existing parser behavior before changing call signatures deeper
  in the stack.
- Add tests for oversized command-line and command-file inputs.

### Slice 2: UI Prompt And Menu Buffers

Clean prompt assembly and menu display buffers.  These are often good
`std::string` or `fmt::format` candidates.

Candidate files:

- `libid/ui/full_screen_prompt.cpp`
- `libid/ui/full_screen_choice.cpp`
- `libid/ui/get_3d_params.cpp`
- `libid/ui/get_file_entry.cpp`
- `libid/ui/get_rds_params.cpp`
- `libid/ui/get_video_mode.cpp`
- `libid/ui/make_batch_file.cpp`
- `libid/ui/slideshw.cpp`
- `libid/ui/tab_display.cpp`

Expected fixes:

- Replace manual label construction with `std::string`.
- Replace output formatting with `fmt`.
- Keep fixed-width UI fields explicit at the boundary.
- Add focused tests around extracted formatting helpers where possible.

### Slice 3: IO Path And File Metadata Buffers

Handle path, filename, and encoder metadata buffers.

Candidate files:

- `libid/io/decode_info.cpp`
- `libid/io/encoder.cpp`
- `libid/io/ends_with_slash.cpp`
- `libid/io/expand_dirname.cpp`
- `libid/io/file_item.cpp`
- `libid/io/fix_dirname.cpp`
- `libid/io/gifview.cpp`
- `libid/io/loadfile.cpp`

Expected fixes:

- Prefer `std::filesystem::path`, `std::string`, and explicit views.
- Remove manual slash and extension buffer edits where filesystem APIs fit.
- Keep encoder binary layout code narrow and covered by existing tests.

### Slice 4: Math And Number Formatting

Review number-string conversion and bignum formatting.

Candidate files:

- `libid/math/bigflt.cpp`
- `libid/math/bignum.cpp`
- `libid/math/round_float_double.cpp`
- `libid/ui/double_to_string.cpp`

Expected fixes:

- Prefer `fmt` for formatting.
- Keep historical numeric output stable.
- Add snapshot-style tests where formatting is observable.

### Slice 5: Help Compiler

Review help compiler string length and formatting hotspots.

Candidate files:

- `hc/AsciiDocCompiler.cpp`
- `hc/HelpSource.cpp`

Expected fixes:

- Replace raw length probes with string-aware operations.
- Preserve generated help output.
- Run help compiler tests after each slice.

### Slice 6: Randomness Hotspots

Classify each `rand` use as deterministic UI/rendering behavior, or replace
it where unpredictability is required.

Candidate files:

- `libid/io/gifview.cpp`
- `libid/ui/evolve.cpp`
- `libid/ui/intro.cpp`
- `libid/ui/rotate.cpp`
- `libid/ui/starfield.cpp`
- `tests/libid/engine/test_random_seed.cpp`

Expected fixes:

- Do not replace deterministic rendering randomness blindly.
- Prefer documented safe review when randomness is not security-sensitive.
- Add comments only where the non-security contract is not obvious.

### Slice 7: GitHub Workflow Hotspots

Review workflow secret handling and dependency pinning.

Candidate files:

- `.github/workflows/analysis.yml`
- `.github/workflows/build.yml`
- `.github/workflows/docs.yml`

Expected fixes:

- Move secrets from shell interpolation into environment variables.
- Pin action dependencies with full commit hashes where appropriate.
- Keep permissions at the narrowest working scope.

### Slice 8: Tests, Legacy, And Win32

Clean or review lower-risk areas after production paths are done.

Candidate files:

- `tests/libid/engine/test_cmdfiles.cpp`
- `tests/libid/engine/test_lowerize_parameter.cpp`
- `tests/libid/io/test_expand_dirname.cpp`
- `tests/libid/ui/test_ChoiceBuilder.cpp`
- `tests/libid/ui/test_comments.cpp`
- `legacy/dos/sound.c`
- `win32/Win32BaseDriver.cpp`
- `win32/WinText.cpp`
- `win32/create_minidump.cpp`

Expected fixes:

- Prefer `std::string` or `std::vector<char>` in tests.
- Mark legacy code separately if it is not built or shipped.
- Keep Win32 changes narrow and build-verified.

## Completion Criteria

- Every item in the inventory is marked done or reviewed.
- SonarCloud shows no unreviewed hotspots for the project, or remaining
  rows are intentionally accepted in Sonar with rationale.
- Each code-changing slice has focused tests or a documented reason tests
  were not practical.
- Full build and relevant focused tests pass before final cleanup.
