// SPDX-License-Identifier: GPL-3.0-only
//
#include "X11Frame.h"

#include "ui/goodbye.h"
#include "ui/id_keys.h"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <limits>
#include <poll.h>

namespace id::misc
{

namespace
{

using namespace id::ui;

struct X11KeyMap
{
    KeySym key_symbol;
    int id_key;
};

long window_event_mask()
{
    return ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
        FocusChangeMask;
}

int clamp_timeout_ms(const std::chrono::steady_clock::duration duration)
{
    const auto ms{std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()};
    return static_cast<int>(std::clamp<long long>(ms, 0, std::numeric_limits<int>::max()));
}

bool has_modifier(const XKeyEvent &event, const unsigned int modifier)
{
    return (event.state & modifier) != 0;
}

template <size_t N>
int lookup_key(const X11KeyMap (&map)[N], const KeySym key_symbol)
{
    const auto it = std::find_if(std::begin(map), std::end(map),
        [key_symbol](const X11KeyMap &entry) { return entry.key_symbol == key_symbol; });
    return it == std::end(map) ? 0 : it->id_key;
}

int lookup_alt_key(const KeySym key_symbol)
{
    KeySym lower{};
    KeySym upper{};
    XConvertCase(key_symbol, &lower, &upper);
    if (lower == XK_a)
    {
        return ID_KEY_ALT_A;
    }
    if (lower == XK_s)
    {
        return ID_KEY_ALT_S;
    }
    if (key_symbol >= XK_1 && key_symbol <= XK_7)
    {
        return ID_KEY_ALT_1 + static_cast<int>(key_symbol - XK_1);
    }
    if (key_symbol >= XK_KP_1 && key_symbol <= XK_KP_7)
    {
        return ID_KEY_ALT_1 + static_cast<int>(key_symbol - XK_KP_1);
    }
    return 0;
}

int lookup_function_key(const KeySym key_symbol, const bool shift, const bool control, const bool alt)
{
    if (key_symbol < XK_F1 || key_symbol > XK_F10)
    {
        return 0;
    }

    const int offset{static_cast<int>(key_symbol - XK_F1)};
    if (shift)
    {
        return ID_KEY_SHF_F1 + offset;
    }
    if (control)
    {
        return ID_KEY_CTL_F1 + offset;
    }
    if (alt)
    {
        return ID_KEY_ALT_F1 + offset;
    }
    return ID_KEY_F1 + offset;
}

const X11KeyMap KEY_MAP[]{
    {XK_KP_Enter, ID_KEY_ENTER_2},
    {XK_Return, ID_KEY_ENTER},
    {XK_KP_Home, ID_KEY_HOME},
    {XK_Home, ID_KEY_HOME},
    {XK_KP_Up, ID_KEY_UP_ARROW},
    {XK_Up, ID_KEY_UP_ARROW},
    {XK_KP_Page_Up, ID_KEY_PAGE_UP},
    {XK_Page_Up, ID_KEY_PAGE_UP},
    {XK_KP_Left, ID_KEY_LEFT_ARROW},
    {XK_Left, ID_KEY_LEFT_ARROW},
    {XK_KP_Begin, ID_KEY_KEYPAD_5},
    {XK_KP_5, ID_KEY_KEYPAD_5},
    {XK_KP_Right, ID_KEY_RIGHT_ARROW},
    {XK_Right, ID_KEY_RIGHT_ARROW},
    {XK_KP_End, ID_KEY_END},
    {XK_End, ID_KEY_END},
    {XK_KP_Down, ID_KEY_DOWN_ARROW},
    {XK_Down, ID_KEY_DOWN_ARROW},
    {XK_KP_Page_Down, ID_KEY_PAGE_DOWN},
    {XK_Page_Down, ID_KEY_PAGE_DOWN},
    {XK_KP_Insert, ID_KEY_INSERT},
    {XK_Insert, ID_KEY_INSERT},
    {XK_KP_Delete, ID_KEY_DELETE},
    {XK_Delete, ID_KEY_DELETE},
    {XK_ISO_Left_Tab, ID_KEY_SHF_TAB},
    {XK_KP_Tab, ID_KEY_TAB},
    {XK_Tab, ID_KEY_TAB},
    {XK_KP_Add, '+'},
    {XK_KP_Subtract, '-'},
    {XK_BackSpace, ID_KEY_BACKSPACE},
    {XK_Escape, ID_KEY_ESC},
};

const X11KeyMap CONTROL_KEY_MAP[]{
    {XK_KP_Enter, ID_KEY_CTL_ENTER_2},
    {XK_Return, ID_KEY_CTL_ENTER},
    {XK_KP_Left, ID_KEY_CTL_LEFT_ARROW},
    {XK_Left, ID_KEY_CTL_LEFT_ARROW},
    {XK_KP_Right, ID_KEY_CTL_RIGHT_ARROW},
    {XK_Right, ID_KEY_CTL_RIGHT_ARROW},
    {XK_KP_End, ID_KEY_CTL_END},
    {XK_End, ID_KEY_CTL_END},
    {XK_KP_Page_Down, ID_KEY_CTL_PAGE_DOWN},
    {XK_Page_Down, ID_KEY_CTL_PAGE_DOWN},
    {XK_KP_Home, ID_KEY_CTL_HOME},
    {XK_Home, ID_KEY_CTL_HOME},
    {XK_KP_Page_Up, ID_KEY_CTL_PAGE_UP},
    {XK_Page_Up, ID_KEY_CTL_PAGE_UP},
    {XK_KP_Up, ID_KEY_CTL_UP_ARROW},
    {XK_Up, ID_KEY_CTL_UP_ARROW},
    {XK_KP_Subtract, ID_KEY_CTL_MINUS},
    {XK_minus, ID_KEY_CTL_MINUS},
    {XK_KP_Begin, ID_KEY_CTL_KEYPAD_5},
    {XK_KP_5, ID_KEY_CTL_KEYPAD_5},
    {XK_KP_Add, ID_KEY_CTL_PLUS},
    {XK_plus, ID_KEY_CTL_PLUS},
    {XK_equal, ID_KEY_CTL_PLUS},
    {XK_KP_Down, ID_KEY_CTL_DOWN_ARROW},
    {XK_Down, ID_KEY_CTL_DOWN_ARROW},
    {XK_KP_Insert, ID_KEY_CTL_INSERT},
    {XK_Insert, ID_KEY_CTL_INSERT},
    {XK_KP_Delete, ID_KEY_CTL_DEL},
    {XK_Delete, ID_KEY_CTL_DEL},
    {XK_KP_Tab, ID_KEY_CTL_TAB},
    {XK_Tab, ID_KEY_CTL_TAB},
    {XK_ISO_Left_Tab, ID_KEY_CTL_TAB},
};

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

void X11Frame::pump_messages(const bool wait)
{
    if (!m_connection.is_open())
    {
        return;
    }

    m_timed_out = false;
    Display *display = m_connection.display();
    while (true)
    {
        while (XPending(display) > 0)
        {
            XEvent event{};
            XNextEvent(display, &event);
            handle_event(event);
        }

        if (!wait || m_key_press_count != 0 || m_timed_out)
        {
            return;
        }

        int timeout_ms = -1;
        if (m_keyboard_timeout_active)
        {
            const auto now{std::chrono::steady_clock::now()};
            if (now >= m_keyboard_deadline)
            {
                m_timed_out = true;
                m_keyboard_deadline = now + m_keyboard_timeout_interval;
                return;
            }
            timeout_ms = clamp_timeout_ms(m_keyboard_deadline - now);
        }

        pollfd descriptor{ConnectionNumber(display), POLLIN, 0};
        const int result = poll(&descriptor, 1, timeout_ms);
        if (result < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return;
        }
        if (result == 0)
        {
            m_timed_out = true;
            m_keyboard_deadline = std::chrono::steady_clock::now() + m_keyboard_timeout_interval;
            return;
        }
        if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
        {
            ui::goodbye();
            return;
        }
    }
}

int X11Frame::get_key_press(const bool wait)
{
    pump_messages(wait);
    if (wait && m_timed_out)
    {
        return 0;
    }

    if (m_key_press_count == 0)
    {
        assert(!wait);
        return 0;
    }

    const int key{m_key_press_buffer[m_key_press_tail]};
    if (++m_key_press_tail >= KEY_BUF_MAX)
    {
        m_key_press_tail = 0;
    }
    --m_key_press_count;
    return key;
}

void X11Frame::set_keyboard_timeout(const int ms)
{
    m_timed_out = false;
    m_keyboard_timeout_active = true;
    m_keyboard_timeout_interval = std::chrono::milliseconds(std::max(ms, 1));
    m_keyboard_deadline = std::chrono::steady_clock::now() + m_keyboard_timeout_interval;
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

void X11Frame::add_key_press(const unsigned int key)
{
    if (key_buffer_full())
    {
        assert(m_key_press_count < KEY_BUF_MAX);
        return;
    }

    m_key_press_buffer[m_key_press_head] = key;
    if (++m_key_press_head >= KEY_BUF_MAX)
    {
        m_key_press_head = 0;
    }
    ++m_key_press_count;
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
    if (event.type == KeyPress && event.xkey.window == m_window)
    {
        handle_key_press(event.xkey);
    }
    if (event.type == DestroyNotify && event.xdestroywindow.window == m_window)
    {
        m_window = None;
        m_mapped = false;
    }
    if (event.type == ConfigureNotify && event.xconfigure.window == m_window)
    {
        if (event.xconfigure.width != m_width || event.xconfigure.height != m_height)
        {
            XResizeWindow(m_connection.display(), m_window, m_width, m_height);
            XFlush(m_connection.display());
        }
    }
}

void X11Frame::handle_key_press(XKeyEvent event)
{
    char text[8]{};
    KeySym key_symbol{};
    const int text_length = XLookupString(&event, text, sizeof(text), &key_symbol, nullptr);
    const bool shift{has_modifier(event, ShiftMask)};
    const bool control{has_modifier(event, ControlMask)};
    const bool alt{has_modifier(event, Mod1Mask)};

    int key{lookup_function_key(key_symbol, shift, control, alt)};
    if (key == 0 && control)
    {
        key = lookup_key(CONTROL_KEY_MAP, key_symbol);
    }
    if (key == 0 && alt)
    {
        key = lookup_alt_key(key_symbol);
    }
    if (key == 0)
    {
        key = lookup_key(KEY_MAP, key_symbol);
    }
    if (key != 0)
    {
        add_key_press(key);
        return;
    }
    if (text_length == 1)
    {
        add_key_press(static_cast<unsigned char>(text[0]));
    }
}

void X11Frame::set_fixed_size(const int width, const int height)
{
    XSizeHints hints{};
    hints.flags = USSize | PSize | PMinSize | PMaxSize | PBaseSize;
    hints.width = width;
    hints.height = height;
    hints.min_width = width;
    hints.min_height = height;
    hints.max_width = width;
    hints.max_height = height;
    hints.base_width = width;
    hints.base_height = height;
    XSetWMNormalHints(m_connection.display(), m_window, &hints);
}

} // namespace id::misc
