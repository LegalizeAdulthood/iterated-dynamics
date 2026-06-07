# X11 Driver

The raw X11 driver is the non-wx Linux GUI path.  It is implemented
directly on Xlib, keeps Xlib headers out of the shared `Driver` surface,
and mirrors the Win32 GDI driver split:

- `X11Driver` registers 256-color video modes.
- `X11BaseDriver` owns driver-level behavior and text/graphics switching.
- `X11Frame` owns the top-level window, event pump, keyboard queue, mouse
  notifications, focus, close handling, and fixed client sizing.
- `X11Text` draws 80x25 CGA-style text screens.
- `X11Plot` stores one byte per image pixel and expands indexed pixels to
  the X server visual during flush.

## Build

On Linux, configure with `ID_USE_WX=OFF`.  When X11 is found, CMake builds
the `x11/` executable and links the raw X11 `os` library.  `ID_USE_WX=ON`
continues to select the wxWidgets path, and Windows continues to use GDI.

The driver assumes a modern TrueColor visual.  It does not require an
8-bit PseudoColor visual; the application-owned 8-bit buffer remains the
source image, and `X11Plot` maps palette entries to server pixels.

## Manual Validation

Run from `home` after a Linux build:

```sh
../../build-rt-default/x11/Release/id
```

Validate these workflows on a real X server:

- fixed-size behavior when the window manager tries to resize the window
- command shell return to the X11 application

Use `Xvfb` only as a smoke environment unless the event loop is made
deterministic enough for reliable automated GUI assertions.

## Current Limitations

- File selection has no X11 dialog.  The driver returns cancel, letting
  the existing text/browser fallback handle selection.
- Continuous sound output is not implemented.  `buzzer()` uses `XBell()`;
  `sound_on()`, `sound_off()`, `mute()`, and `init_fm()` are stubs.
- No automated X11 smoke test is enabled.  Manual integration testing
  remains the acceptance path for display, event, and window-manager
  behavior.
- PseudoColor visuals are optional compatibility territory and are not a
  validation target.
