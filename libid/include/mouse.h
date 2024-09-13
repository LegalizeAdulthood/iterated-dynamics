#pragma once

extern int                   g_look_at_mouse;

// g_look_at_mouse is set to one of these positive values for handling mouse events,
// or it is set to a negative key code for primary button click to generate a key.
enum class MouseLook
{
    IGNORE = 0,      // ignoring the mouse
    TEXT_SCROLL = 2, // for scrolling through text lists
    POSITION = 3,    // for positioning things like the zoom box, palette editor, etc.
};
inline int operator+(MouseLook value)
{
    return static_cast<int>(value);
}
