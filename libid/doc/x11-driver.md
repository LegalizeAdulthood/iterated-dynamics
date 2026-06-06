# Plan: Raw X11 Driver

## Goal

Add a Linux driver implemented directly on libX11/Xlib, comparable to the
Win32 GDI driver, without GTK, wxWidgets, SDL, Qt, or another GUI toolkit.

The target is a normal `Driver` implementation that supports the same user
visible responsibilities as `GDIDriver`:

- 256-color graphics modes backed by the existing indexed pixel model.
- 80x25 text screens for menus, prompts, help, and stacked text overlays.
- Keyboard and mouse input translated into the existing `ID_KEY_*` and mouse
  notification paths.
- Text/graphics switching, palette cycling, save/restore graphics, flushing,
  resize handling, pause/resume, shell, and file selection fallback behavior.
- Driver registration through `driver_types.h` and `init_drivers()`.

## Non-Goals

- Do not build a GTK or wxWidgets integration.
- Do not replace the existing wx driver.
- Do not change fractal rendering code, `Driver`, or global video semantics
  unless an X11 requirement exposes a narrow bug.
- Do not assume an 8-bit PseudoColor visual exists. Modern TrueColor visuals
  are the baseline: keep the application-owned 8-bit buffer and expand it to
  the X server pixel format during flush.

## Current GDI Shape To Mirror

The Win32 implementation is split into four roles:

- `GDIDriver` registers modes, switches between text and graphics, owns the
  `Plot` surface, and delegates common driver behavior to `Win32BaseDriver`.
- `Win32BaseDriver` owns driver-level behavior: input buffering, auto-save fake
  keys, `handle_special_keys()`, shell, text screen stack, cursor, delay,
  debug text, memory check, file dialog, sound stubs, and common video-mode
  setup.
- `Frame` owns the top-level window, message pump, keyboard queue, mouse
  notifications, timer timeouts, focus, and fixed client sizing.
- `WinText` and `Plot` own the two child surfaces. `WinText` draws the 80x25
  CGA-style text buffer, while `Plot` stores one byte per image pixel and uses
  the palette when repainting.

The X11 driver should keep the same separation. That lets each slice land with
small reviews and keeps platform-specific Xlib code out of the shared engine.

## Target File Layout

Create a new top-level `x11/` directory, parallel to `win32/` and `wx/`:

- `x11/CMakeLists.txt`
- `x11/X11Connection.h`, `x11/X11Connection.cpp`
- `x11/X11Frame.h`, `x11/X11Frame.cpp`
- `x11/X11Text.h`, `x11/X11Text.cpp`
- `x11/X11Plot.h`, `x11/X11Plot.cpp`
- `x11/X11BaseDriver.h`, `x11/X11BaseDriver.cpp`
- `x11/X11Driver.cpp`
- `x11/x11_main.cpp`
- `x11/X11SpecialDirectories.cpp`

Use RAII wrappers for Xlib handles where practical. Xlib itself is C, but
ownership must still be explicit: `Display *`, `Window`, `GC`, `XImage`,
`Pixmap`, `XFontStruct *`, atoms, and allocated memory all need one clear owner.

## Slice 1: X11 Key Translation

Goal: translate X11 `KeyPress` events into the same values used by Win32 and
wx.

Work:

- Use `XLookupString()` or `Xutf8LookupString()` if an input context is added.
  Start with `XLookupString()` for ASCII and KeySym handling.
- Map function keys F1-F10, arrows, Home/End/Page Up/Page Down, Insert/Delete,
  keypad enter/keypad 5, Tab, plus, minus, and Enter to the existing `ID_KEY_*`
  constants.
- Respect Shift, Control, and Mod1/Alt combinations used by the current
  drivers, including shifted function keys, control arrows, Alt+A, Alt+S, and
  Alt+1 through Alt+7.
- Keep unknown printable ASCII as the character value.
- Add focused unit tests for the pure mapping helper if the mapping is factored
  away from raw `XKeyEvent`.

Review boundary:

- The application can navigate text prompts with normal keys, arrows, function
  keys, Ctrl combinations, Shift+Tab, and Alt shortcuts.
- The event pump remains toolkit-free.

## Slice 2: Text Screen Model

Goal: implement the 80x25 text buffer independent of X drawing details.

Work:

- Add an X11-local `Screen` equivalent to Win32 `Screen`, or move the small
  plain data structure to a shared header if reuse is clearly cleaner.
- Implement `put_string()`, `scroll_up()`, `set_attr()`, `clear()`,
  `get_screen()`, `set_screen()`, `get_char_attr()`, and `put_char_attr()` with
  the same row/column semantics as `WinText`.
- Port text screen stack handling into `X11BaseDriver`, using row/column cursor
  storage rather than packing into one integer.
- Keep the buffer testable without opening an X display.

Review boundary:

- Unit tests cover wrapping, CR/LF handling, attribute updates, scroll, screen
  save/restore, and char/attribute packing.
- No X drawing is required for this slice.

## Slice 3: X11 Text Rendering

Goal: draw the text screen into an X11 child window.

Work:

- Implement `X11Text` as a child window sized to 80 columns by 25 rows.
- Load a fixed-width font with `XLoadQueryFont()`. Prefer `"fixed"` first, and
  keep the font name isolated so a better XLFD can be selected later.
- Compute character width, ascent, descent, and cell height from the font.
- Draw runs with common foreground/background attributes using `XFillRectangle`
  and `XDrawString`.
- Use the CGA color table from `WinText`, converted through the frame visual.
- Implement dirty rectangle invalidation at cell granularity and repaint on
  `Expose`.
- Implement cursor show, move, hide, and blink-free rendering by invalidating
  the affected cell. Start with underline/block shapes matching Win32 closely
  enough for prompt visibility.

Review boundary:

- Text mode can show menus and prompts in the X11 window.
- Text stack/unstack restores content and cursor position.
- Graphics mode is still allowed to be absent or hidden.

## Slice 4: Graphics Backing Store

Goal: implement the indexed graphics pixel buffer without palette cycling yet.

Work:

- Implement `X11Plot` as a child window with one byte per image pixel, matching
  `Plot::m_pixels`.
- Keep `read_pixel()`, `write_pixel()`, `clear()`, `save_graphics()`, and
  `restore_graphics()` semantics compatible with GDI.
- Track a dirty rectangle in image coordinates.
- Convert dirty indexed pixels into an X server image using the current color
  lookup table and the selected visual masks. The default path must work for
  TrueColor visuals and must not rely on mutable X colormap entries.
- Use `XCreateImage()`/`XPutImage()` for the first version. Prefer a
  caller-owned byte vector for image data so destruction is deterministic.
- Keep coordinate orientation compatible with the current drivers:
  fractal coordinates are top-left for drawing, while stored pixels follow the
  existing `m_height - y - 1` read/write convention.
- Implement `flush()` to push the dirty region and `Expose` handling to repaint
  either the exposed region or the whole image.

Review boundary:

- A simple generated image draws and redraws after expose.
- `read_pixel()` returns the last written indexed color.
- `save_graphics()` and `restore_graphics()` round-trip the indexed buffer.

## Slice 5: Palette and Graphics Text

Goal: match GDI palette behavior closely enough for palette cycling and labels.

Work:

- Store a 256-entry application palette as RGB bytes owned by `X11Plot`.
- Implement `read_palette()` and `write_palette()` through `g_dac_box`.
- On `write_palette()`, update the lookup table and repaint the visible image
  from the indexed buffer.
- Implement `display_string()` using the same 8x8 bitmap font strategy as
  `win32/Plot.cpp`, or extract that bitmap into a shared internal helper to
  avoid another copy.
- Ensure `display_string()` writes indexed pixels, not only transient X text,
  so save/restore and expose remain correct.

Review boundary:

- Palette cycling visibly updates existing pixels.
- Graphics labels survive expose and save/restore.

## Slice 6: Text/Graphics Switching and Mode Setup

Goal: make `X11Driver` comparable to `GDIDriver`.

Work:

- Add `X11Driver : public X11BaseDriver` with driver name `"x11"` and
  description `"X11"`.
- Add the same built-in 256-color modes as `GDIDriver`, filtered by
  `get_max_screen()`.
- Implement `create_window()` to create the frame, text child, and plot child,
  then center the smaller child when text and graphics sizes differ.
- Implement `resize()` using the current `g_video_table[g_adapter]` dimensions
  and text max size.
- Implement `set_for_text()`, `set_for_graphics()`, `is_text()`, and
  `set_clear()` by mapping/unmapping or raising/hiding the child windows.
- Implement `set_video_mode()` with the same global setup as GDI/wx:
  `g_is_true_color`, `g_good_mode`, `g_and_color`, DAC state, disk end,
  `set_normal_dot()`, `set_normal_span()`, graphics switch, and clear.

Review boundary:

- The driver can start, choose a mode, switch between text and graphics, and
  return to graphics after stacked text screens.

## Slice 7: Mouse Handling

Goal: feed mouse movement and buttons into the existing UI mouse notification
paths.

Work:

- Translate `ButtonPress`, `ButtonRelease`, and `MotionNotify` from both text
  and plot child windows.
- Track cursor position in frame/client coordinates for
  `Driver::get_cursor_pos()`.
- Convert X button/modifier state to a stable internal mask before passing it
  to `mouse_notify_*()`.
- Match the Win32 behavior for primary, secondary, and middle buttons.
- Preserve the `g_look_at_mouse` key-injection behavior used by `Frame`.
- Add debug logging behind the existing debug text path, not unconditional
  stdout/stderr spam.

Review boundary:

- Mouse zoom workflows work in graphics mode.
- `get_cursor_pos()` reports the latest pointer location.

## Slice 8: Platform Services

Goal: finish the non-rendering `Driver` surface.

Work:

- Implement `shell()` with the existing `cmd_shell()` helper and an X event
  pump timeout callback.
- Implement `debug_text()` to write to stderr or syslog-style output. Keep it
  simple and deterministic.
- Implement `buzzer()` with `XBell()`.
- Keep `sound_on()`, `sound_off()`, `mute()`, and `init_fm()` as stubs unless a
  later audio requirement appears.
- Implement `check_memory()` as a no-op or debug assertion hook on Linux.
- Implement `get_filename()` as a terminal fallback: preserve the current path
  handling, but return cancel unless a simple prompt-based implementation is
  explicitly acceptable. Do not introduce GTK just for file selection.
- Implement `X11SpecialDirectories.cpp` if the non-wx Linux executable needs a
  platform-specific `SpecialDirectories` provider.

Review boundary:

- All pure virtual `Driver` methods are implemented.
- No toolkit dependency is introduced.

## Slice 9: End-to-End Validation

Goal: harden behavior against real X servers and CI constraints.

Work:

- Build with `ID_USE_WX=OFF` on Linux.
- Run unit tests that do not require a display.
- Run the executable manually under a real X server and under `Xvfb` when
  available.
- Validate:
  - startup and shutdown
  - default video mode selection
  - text menu rendering
  - graphics rendering
  - palette cycling
  - keyboard navigation and interrupt keys
  - mouse zoom
  - expose redraw after covering/uncovering the window
  - resize after mode changes
  - pause/resume when switching drivers, if more than one driver is present
- Add a minimal smoke-test hook only after the event loop is deterministic
  enough to avoid flaky CI behavior.

Review boundary:

- The Linux X11 driver is usable as the non-wx GUI path.
- Known limitations are documented in the same file or a follow-up issue.

## Implementation Notes

- Keep all Xlib headers out of `libid/include/misc/Driver.h`.
- Prefer small pure helpers for key mapping, color conversion, dirty-rectangle
  merging, and text buffer operations so most behavior is testable without an X
  server.
- Xlib is not thread-safe by default. Keep all X calls on the main UI thread
  and do not introduce background flushing.
- Call `XFlush()` only when needed. The driver already has a `flush()` contract;
  use that as the main rendering boundary.
- Be careful with `XImage` ownership. If `XDestroyImage()` would free memory
  owned by a `std::vector`, clear `image->data` before destroy or wrap the
  image so ownership is unambiguous.
- For TrueColor visuals, compute server pixel values from visual masks instead
  of assuming 24-bit RGB byte order.
- Treat X11 PseudoColor support as optional compatibility code only if it ever
  becomes useful. The driver must build, start, draw, and cycle palettes without
  PseudoColor by expanding indexed pixels into the server visual format.
- Avoid copying the large 8x8 bitmap font a third time if a focused extraction
  is acceptable during Slice 8. If extraction makes the review noisy, copy it
  first and deduplicate later.
