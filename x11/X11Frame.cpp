// SPDX-License-Identifier: GPL-3.0-only
//
#include "X11Frame.h"

#include "ui/goodbye.h"

#include <X11/Xutil.h>

namespace id::misc
{

namespace
{

long window_event_mask()
{
    return ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
        FocusChangeMask;
}

} // namespace

bool X11Frame::init(const char *title)
{
    m_title = title;
    return m_connection.open();
}

void X11Frame::terminate()
{
    destroy_window();
    m_connection.close();
}

void X11Frame::create_window(const int width, const int height)
{
    if (!m_connection.is_open())
    {
        return;
    }
    if (m_window != None)
    {
        resize(width, height);
        resume();
        return;
    }

    Display *display = m_connection.display();
    XSetWindowAttributes attributes{};
    attributes.background_pixel = BlackPixel(display, m_connection.screen());
    attributes.border_pixel = BlackPixel(display, m_connection.screen());
    attributes.colormap = m_connection.colormap();
    attributes.event_mask = window_event_mask();

    m_window = XCreateWindow(display, m_connection.root_window(), 0, 0, width, height, 0, m_connection.depth(),
        InputOutput, m_connection.visual(), CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &attributes);
    if (m_window == None)
    {
        return;
    }

    m_width = width;
    m_height = height;
    XStoreName(display, m_window, m_title.c_str());
    Atom delete_window = m_connection.wm_delete_window();
    XSetWMProtocols(display, m_window, &delete_window, 1);
    set_fixed_size(width, height);
    XMapWindow(display, m_window);
    XFlush(display);
    m_mapped = true;
}

bool X11Frame::resize(const int width, const int height)
{
    if (m_window == None)
    {
        m_width = width;
        m_height = height;
        return false;
    }
    if (width == m_width && height == m_height)
    {
        return false;
    }

    m_width = width;
    m_height = height;
    XResizeWindow(m_connection.display(), m_window, width, height);
    set_fixed_size(width, height);
    XFlush(m_connection.display());
    return true;
}

void X11Frame::pause()
{
    if (m_window == None || !m_mapped)
    {
        return;
    }

    XUnmapWindow(m_connection.display(), m_window);
    XFlush(m_connection.display());
    m_mapped = false;
}

void X11Frame::resume()
{
    if (m_window == None || m_mapped)
    {
        return;
    }

    XMapWindow(m_connection.display(), m_window);
    XFlush(m_connection.display());
    m_mapped = true;
}

void X11Frame::pump_messages()
{
    if (!m_connection.is_open())
    {
        return;
    }

    while (XPending(m_connection.display()) > 0)
    {
        XEvent event{};
        XNextEvent(m_connection.display(), &event);
        handle_event(event);
    }
}

void X11Frame::get_max_screen(int &width, int &height) const
{
    if (!m_connection.is_open())
    {
        width = -1;
        height = -1;
        return;
    }

    Display *display = m_connection.display();
    width = DisplayWidth(display, m_connection.screen());
    height = DisplayHeight(display, m_connection.screen());
}

void X11Frame::destroy_window()
{
    if (m_window == None)
    {
        return;
    }

    XDestroyWindow(m_connection.display(), m_window);
    XFlush(m_connection.display());
    m_window = None;
    m_mapped = false;
}

void X11Frame::handle_event(const XEvent &event)
{
    if (event.type == ClientMessage && event.xclient.window == m_window &&
        static_cast<Atom>(event.xclient.data.l[0]) == m_connection.wm_delete_window())
    {
        ui::goodbye();
    }
    if (event.type == DestroyNotify && event.xdestroywindow.window == m_window)
    {
        m_window = None;
        m_mapped = false;
    }
    if (event.type == ConfigureNotify && event.xconfigure.window == m_window)
    {
        m_width = event.xconfigure.width;
        m_height = event.xconfigure.height;
    }
}

void X11Frame::set_fixed_size(const int width, const int height)
{
    XSizeHints hints{};
    hints.flags = PMinSize | PMaxSize | PBaseSize;
    hints.min_width = width;
    hints.min_height = height;
    hints.max_width = width;
    hints.max_height = height;
    hints.base_width = width;
    hints.base_height = height;
    XSetWMNormalHints(m_connection.display(), m_window, &hints);
}

} // namespace id::misc
