#pragma once

extern bool                  g_cursor_mouse_tracking;
extern int                   g_look_at_mouse;

// g_look_at_mouse is set to one of these positive values for handling mouse events,
// or it is set to a negative key code for primary button click to generate a key.
enum class MouseLook
{
    IGNORE = 0,            // ignoring the mouse
    NAVIGATE_GRAPHICS = 1, // <Return> + arrow keys at graphics cursor grid
    NAVIGATE_TEXT = 2,     // <Return> + arrow keys at text cursor grid
    POSITION = 3,          // for positioning things like the zoom box, palette editor, etc.
};
inline int operator+(MouseLook value)
{
    return static_cast<int>(value);
}
