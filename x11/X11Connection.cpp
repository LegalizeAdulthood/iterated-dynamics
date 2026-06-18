// SPDX-License-Identifier: GPL-3.0-only
//
#include <X11Driver/X11Connection.h>

namespace id::misc
{

X11Connection::~X11Connection()
{
    close();
}

bool X11Connection::open()
{
    if (m_display != nullptr)
    {
        return true;
    }

    m_display = XOpenDisplay(nullptr);
    if (m_display == nullptr)
    {
        return false;
    }

    m_screen = DefaultScreen(m_display);
    m_root_window = RootWindow(m_display, m_screen);
    m_visual = DefaultVisual(m_display, m_screen);
    m_depth = DefaultDepth(m_display, m_screen);
    m_colormap = DefaultColormap(m_display, m_screen);
    m_wm_delete_window = XInternAtom(m_display, "WM_DELETE_WINDOW", False);
    return true;
}

void X11Connection::close()
{
    if (m_display == nullptr)
    {
        return;
    }

    XCloseDisplay(m_display);
    m_display = nullptr;
    m_screen = 0;
    m_root_window = None;
    m_visual = nullptr;
    m_depth = 0;
    m_colormap = None;
    m_wm_delete_window = None;
}

} // namespace id::misc
