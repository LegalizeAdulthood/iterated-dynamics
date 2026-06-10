<!--
SPDX-License-Identifier: GPL-3.0-only

Copyright 2026 Richard Thomson
-->
# macOS X11 Port Plan

## Goal

Bring the existing X11 port up on macOS without adding a wxWidgets path.
The first target is an Intel macOS build that can run the same X11
driver used on Linux.  Apple Silicon and native application packaging are
follow-up work.

## Scope

- Use the existing `x11` driver.
- Keep `ID_USE_WX` off.
- Keep the program shape as a command-line executable that opens X11.
- Use XQuartz at runtime.
- Treat hosted macOS X11 testing as separate from Linux `xvfb-run`.
- Do not add Cocoa, Objective-C UI, or `.app` behavior in this slice.

## Current State

The top-level build already adds `x11` for non-Windows builds when wx is
off:

- `CMakeLists.txt` adds `x11` under `if(NOT WIN32 AND NOT ID_USE_WX)`.
- `x11/CMakeLists.txt` finds X11 and builds `os` plus `id`.
- `config/CMakeLists.txt` sets `ID_HAVE_X11_DRIVER` from
  `find_package(X11)`.
- `libid/misc/Driver.cpp` loads the disk driver before the X11 driver.

The source is close to portable Xlib code.  The known Linux-only
assumptions are concentrated and should be fixed before enabling macOS
CI.

## Known Gaps

### Program Identity

`x11/X11SpecialDirectories.cpp` uses `/proc/self/exe` to implement
`SpecialDirectories::program_dir()`.

Keep that boundary.  It should also report the program name for code that
must choose between `id` and `xid`.  macOS has no `/proc/self/exe`; use
`_NSGetExecutablePath` only inside the macOS source file selected by CMake
if exact discovery is needed there.

`libid/ui/make_mig_script_unix.cpp` currently queries `/proc/self/exe`.
It should not.  MIG script generation needs the X11 program name, not the
current executable path.

The dependency is not `make_mig_script_filename()`, which only names
`makemig.sh`.  The generated script stores the program in `id_bin`, and
the piece commands run that value.  That value should come from
`SpecialDirectories::program_name()` for X11.

### X11 Requirement

`x11/CMakeLists.txt` silently returns when X11 is missing.  That makes
sense only if X11 is optional.  Platform selection already means the X11
driver has been chosen.  If that source set is selected, X11 is required.
Make the X11 path fail clearly when X11 is not found.

Do not add `ID_REQUIRE_X11`.  The selected driver source set is the
requirement.

### X11 Runtime

All current X11-derived drivers call `XOpenDisplay`, including the disk
driver.  Image and acceptance tests that use `video=F6` still need a live
X11 display.  Linux CI uses `xvfb-run` for that job.  Hosted macOS does
not have the same clean wrapper.

For macOS CI, first try XQuartz: install it, launch it, set `DISPLAY`,
and run the smoke.  If hosted runners cannot provide a usable display,
keep macOS CI compile-only plus non-display tests until a self-hosted
macOS runner or another display provider is available.

### Dependencies

The vendored vcpkg `libx11` port expects system X libraries on
non-Windows unless `X_VCPKG_FORCE_VCPKG_X_LIBRARIES` is set in the
triplet.  For macOS, the lowest-risk first pass is to install XQuartz and
let CMake find system X11.

The `cpuid` port is x86-oriented.  Start with an explicit Intel macOS
runner.  Apple Silicon can come later after `libcpuid` is optional or
replaced for non-x86 targets.

## Implementation Slices

### Slice 1: Fix Program Identity

Keep program-directory and program-name lookup behind
`SpecialDirectories`.  Do not add a new public `io` helper.

Suggested shape:

- Add pure virtual `program_name()` to `SpecialDirectories`.
- Split X11 special directory code into shared and platform-selected
  source files if needed.
- Keep `program_dir()` as the public boundary.
- Select the Linux source file on Linux.
- Select the macOS source file on macOS.
- Keep `/proc/self/exe` only in the Linux source file.
- Use `_NSGetExecutablePath` only in the macOS source file.
- Override `program_name()` in X11 to return `xid`.
- Implement `program_name()` in the non-wx platform implementations and
  test doubles that include `SpecialDirectories`.
- Add a stub `program_name()` to the wx implementation only to satisfy the
  pure interface; do not otherwise work on the wx path.
- Use `g_special_dirs->program_name()` in unix MIG script generation
  instead of hardcoding `xid`.
- Remove the `/proc/self/exe` lookup from
  `libid/ui/make_mig_script_unix.cpp`.

Then remove direct `/proc/self/exe` use from:

- `x11/X11SpecialDirectories.cpp`
- `libid/ui/make_mig_script_unix.cpp`

Acceptance:

- Linux build still passes.
- Unix MIG script generation does not query `/proc`.
- Generated unix MIG scripts use the X11 program name.
- macOS configure and compile reach the next missing item, if any.

### Slice 2: Require X11 When X11 Is Selected

Behavior:

- In `x11/CMakeLists.txt`, replace the silent `return()` after
  `find_package(X11 QUIET)` with a clear `message(FATAL_ERROR)`.
- Keep platform selection outside the X11 driver source.
- Do not add `ID_REQUIRE_X11`.
- If a future platform does not select X11, it should not add the `x11`
  subdirectory.

Acceptance:

- Windows builds do not add `x11`.
- Linux configure fails clearly if X11 is absent.
- macOS configure fails clearly if XQuartz or X11 libraries are absent.

### Slice 3: Local macOS Build Notes

Document local setup in `ReadMe.md` only after the path works.  Initial
notes can stay in this plan until verified.

Expected local setup:

```sh
brew install cmake ninja
brew install --cask xquartz
cmake --preset debug
cmake --build --preset debug
```

Runtime check:

```sh
open -a XQuartz
export DISPLAY=:0
./../build-debug/x11/xid
```

If CMake cannot find X11 from XQuartz, add hints in the workflow or local
command line rather than hard-coding them in source.  Candidate hints:

- `CMAKE_PREFIX_PATH=/opt/X11`
- `X11_ROOT=/opt/X11`

Acceptance:

- A clean macOS machine can build from documented steps.
- The executable opens an X11 window under XQuartz.

### Slice 4: macOS CI Compile

Enable one macOS matrix leg for compile-only validation first.

Use an explicit Intel runner label, not `macos-latest`, because current
hosted runner docs map `macos-latest` to arm64.  Candidate:

- `macos-15-intel`

CI steps:

- Bootstrap vcpkg.
- Install or expose X11 headers and libraries.
- Do not add macOS presets.
- Configure with existing `ci-debug` or `ci-release` presets.
- Build.
- Do not run image or acceptance tests in the first CI slice.
- Do not fail this slice because hosted macOS lacks a live X display.

Acceptance:

- CI builds `id` on Intel macOS.
- CI logs show `ID_HAVE_X11_DRIVER=ON` or equivalent configure evidence.
- No wx job changes.

### Slice 5: macOS Smoke Test

Add one minimal runtime smoke after compile works.

This slice depends on a live X display.  On Linux, keep using
`xvfb-run`.  On macOS, first try a launched XQuartz session.  If hosted
macOS cannot provide `DISPLAY`, keep this smoke local or self-hosted
until the CI display problem is solved.

The smoke should prove:

- `id` starts.
- It can initialize the X11 driver.
- It can exit without user input or leaked background processes.

Prefer an existing batch command if it can be made noninteractive.  If
every existing path requires a display and key input, add a small
command-line smoke mode that initializes drivers and exits.

Acceptance:

- Smoke runs on Linux and macOS.
- Failure clearly distinguishes "no display" from application failure.

### Slice 6: Image and Acceptance Tests

Only after the smoke is stable, enable selected tests on macOS.

Start with:

- One saved image test.
- One `makemig` test.
- One savedir acceptance test.

Likely changes:

- Extend `tests/cmake/PlatformArgs.cmake` to set a stable geometry for
  macOS.
- Decide whether Linux gold images are valid for macOS X11 output.
- Rename `linux` labels to `x11` when tests are not Linux-specific.

Acceptance:

- A small macOS X11 test set passes in CI.
- Linux results stay unchanged.
- Any macOS-specific gold image names are explicit.

### Slice 7: Apple Silicon

Treat Apple Silicon as separate work.

Options:

- Make `libcpuid` optional on non-x86 targets.
- Replace `$cpu$` expansion with a portable CPU-brand helper.
- Use `sysctlbyname("machdep.cpu.brand_string")` on macOS where useful.
- Skip `cpuid` dependency on arm64.

Acceptance:

- arm64 macOS configures without `cpuid`.
- `$cpu$` expansion returns a useful string or `(Unknown CPU)`.
- Existing x86 behavior stays unchanged.

### Slice 8: Packaging

Keep first packaging simple.

Initial artifact:

- `.tar.gz` containing `xid`, help files, and required data.

Do not create a `.app` bundle until the runtime story is clear.  X11 apps
are not native macOS apps, and packaging should not imply otherwise.

Acceptance:

- `cpack` produces a macOS tarball.
- Extracted artifact can run under XQuartz.
- Release workflow can upload macOS artifact without changing Windows
  assets.

## Risks

- XQuartz may not be installed on GitHub hosted runners.
- Headless X11 on hosted macOS may be harder than Linux `xvfb-run`.
- The first hosted macOS job may need to stay compile-only.
- X11 font availability may differ from Linux.
- X11 color masks and display depth may expose output differences.
- Apple Silicon may require removing the hard `cpuid` dependency.

## Preferred Order

1. Program identity through `SpecialDirectories`.
2. Required-X11 configure failure.
3. Local Intel macOS configure and build.
4. Local XQuartz runtime check.
5. Intel macOS CI compile.
6. Runtime smoke.
7. Small test subset.
8. Apple Silicon.
9. Packaging.

## Done Criteria

The macOS X11 port is ready for normal development when:

- `cmake --workflow --preset ci-debug` passes on Intel macOS.
- A runtime smoke test proves X11 initialization.
- At least one image test runs on macOS.
- The docs state that XQuartz is required.
- wx remains untouched.
