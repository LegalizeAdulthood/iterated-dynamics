<!--
SPDX-License-Identifier: GPL-3.0-only

Copyright 2026 Richard Thomson
-->
# macOS X11 Port Status

## Goal

Track the remaining macOS X11 work.  The native macOS port uses the same
X11 driver as Linux under XQuartz.  It is not a Cocoa port and it is not
the wxWidgets port.

## Current Implementation

- The native non-Windows, non-wx build selects the `x11` driver.
- The X11 executable is named `xid`.
- The release build matrix includes `macos-latest` and `macos-15-intel`.
- The macOS release workflow installs XQuartz and runs the normal CMake
  workflow under `/opt/X11/bin/Xvfb`.
- The wx build matrix includes `macos-latest` release validation.
- The default save library is `$HOME/Documents/Iterated Dynamics`.
- The X11 window position is saved in
  `$HOME/Library/Preferences` as
  `com.legalizeadulthood.iterated-dynamics.plist`.
- X geometry, X fonts, video mode selection, image tests, acceptance
  tests, and `makemig` use the shared X11 path.

## Remaining Work

### Program Directory

`x11/X11SpecialDirectories.cpp` still implements `program_dir()` with
`/proc/self/exe`.  On macOS that fails and falls back to the current
working directory.  If macOS needs executable-relative fallback library
lookup, add a macOS branch that uses `_NSGetExecutablePath`.

Keep the executable name as `xid`.

### X11 Requirement

`x11/CMakeLists.txt` still returns quietly when `find_package(X11)` fails.
For a selected native X11 build, missing X11 should be a configure error.
The top-level driver selection already decides whether the X11 directory
is active.

Do not add a separate `ID_REQUIRE_X11` option.

## Out of Scope

- Native Cocoa windows.
- A `.app` bundle.
- Replacing XQuartz.
- Treating the wxWidgets port as the native macOS X11 port.
