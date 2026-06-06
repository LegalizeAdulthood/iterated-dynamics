// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <X11/Xlib.h>

namespace id::misc
{

class X11Connection
{
public:
    X11Connection() = default;
    ~X11Connection();

    X11Connection(const X11Connection &) = delete;
    X11Connection &operator=(const X11Connection &) = delete;

    bool open();
    void close();

    bool is_open() const
    {
        return m_display != nullptr;
    }

    Display *display() const
    {
        return m_display;
    }

    int screen() const
    {
        return m_screen;
    }

    Window root_window() const
    {
        return m_root_window;
    }

    Visual *visual() const
    {
        return m_visual;
    }

    int depth() const
    {
        return m_depth;
    }

    Colormap colormap() const
    {
        return m_colormap;
    }

    Atom wm_delete_window() const
    {
        return m_wm_delete_window;
    }

private:
    Display *m_display{};
    int m_screen{};
    Window m_root_window{};
    Visual *m_visual{};
    int m_depth{};
    Colormap m_colormap{};
    Atom m_wm_delete_window{};
};

} // namespace id::misc
