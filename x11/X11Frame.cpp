// SPDX-License-Identifier: GPL-3.0-only
//
#include <X11Driver/X11Frame.h>

#include "misc/Driver.h"
#include "ui/goodbye.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"

#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <optional>
#include <poll.h>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace id::misc
{

namespace
{

using namespace id::ui;

enum X11MouseFlag
{
    X11_MOUSE_LEFT = 0x0001,
    X11_MOUSE_RIGHT = 0x0002,
    X11_MOUSE_SHIFT = 0x0004,
    X11_MOUSE_CONTROL = 0x0008,
    X11_MOUSE_MIDDLE = 0x0010,
};

struct X11KeyMap
{
    KeySym key_symbol;
    int id_key;
};

struct X11WindowPosition
{
    int x{};
    int y{};
};

#if defined(__APPLE__)
constexpr const char *const MACOS_WINDOW_POSITION_FILE{"com.legalizeadulthood.iterated-dynamics.plist"};
#else
constexpr const char *const WINDOW_POSITION_DIR{"iterated-dynamics"};
constexpr const char *const WINDOW_POSITION_FILE{"x11-window.ini"};
#endif

long window_event_mask()
{
    return ExposureMask | StructureNotifyMask | KeyPressMask | PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
        FocusChangeMask;
}

std::filesystem::path window_position_path()
{
#if defined(__APPLE__)
    if (const char *home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    {
        return std::filesystem::path{home} / "Library" / "Preferences" / MACOS_WINDOW_POSITION_FILE;
    }
#else
    if (const char *config_home = std::getenv("XDG_CONFIG_HOME"); config_home != nullptr && config_home[0] != '\0')
    {
        return std::filesystem::path{config_home} / WINDOW_POSITION_DIR / WINDOW_POSITION_FILE;
    }

    if (const char *home = std::getenv("HOME"); home != nullptr && home[0] != '\0')
    {
        return std::filesystem::path{home} / ".config" / WINDOW_POSITION_DIR / WINDOW_POSITION_FILE;
    }
#endif

    return {};
}

#if !defined(__APPLE__)
bool read_int_value(const std::string &line, const char *key, int &value)
{
    const std::string prefix{std::string{key} + "="};
    if (line.rfind(prefix, 0) != 0)
    {
        return false;
    }

    try
    {
        value = std::stoi(line.substr(prefix.size()));
    }
    catch (...)
    {
        return false;
    }
    return true;
}
#endif

#if defined(__APPLE__)
bool read_expected_line(std::istream &in, const char *expected)
{
    std::string line;
    return std::getline(in, line) && line == expected;
}

std::optional<int> read_plist_integer(std::istream &in)
{
    std::string line;
    if (!std::getline(in, line))
    {
        return {};
    }

    constexpr std::string_view PREFIX{"    <integer>"};
    constexpr std::string_view SUFFIX{"</integer>"};
    const std::string_view text{line};
    if (text.size() <= PREFIX.size() + SUFFIX.size() || text.substr(0, PREFIX.size()) != PREFIX ||
        text.substr(text.size() - SUFFIX.size()) != SUFFIX)
    {
        return {};
    }

    const std::string value{text.substr(PREFIX.size(), text.size() - PREFIX.size() - SUFFIX.size())};
    try
    {
        std::size_t pos{};
        const int result{std::stoi(value, &pos)};
        if (pos != value.size())
        {
            return {};
        }
        return result;
    }
    catch (...)
    {
        return {};
    }
}

std::optional<X11WindowPosition> read_window_position(std::istream &in)
{
    if (!read_expected_line(in, R"(<?xml version="1.0" encoding="UTF-8"?>)") ||
        !read_expected_line(in,
            R"(<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" )"
            R"("http://www.apple.com/DTDs/PropertyList-1.0.dtd">)") ||
        !read_expected_line(in, R"(<plist version="1.0">)") || !read_expected_line(in, "<dict>") ||
        !read_expected_line(in, "    <key>X11WindowLeft</key>"))
    {
        return {};
    }

    const std::optional<int> left{read_plist_integer(in)};
    if (!left.has_value() || !read_expected_line(in, "    <key>X11WindowTop</key>"))
    {
        return {};
    }

    const std::optional<int> top{read_plist_integer(in)};
    std::string extra;
    if (!top.has_value() || !read_expected_line(in, "</dict>") || !read_expected_line(in, "</plist>") ||
        std::getline(in, extra))
    {
        return {};
    }

    return X11WindowPosition{*left, *top};
}

void write_window_position(std::ostream &out, const X11WindowPosition &position, Display *, int)
{
    out << R"(<?xml version="1.0" encoding="UTF-8"?>)" << '\n';
    out << R"(<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" )"
           R"("http://www.apple.com/DTDs/PropertyList-1.0.dtd">)"
        << '\n';
    out << R"(<plist version="1.0">)" << '\n';
    out << "<dict>" << '\n';
    out << "    <key>X11WindowLeft</key>" << '\n';
    out << "    <integer>" << position.x << "</integer>" << '\n';
    out << "    <key>X11WindowTop</key>" << '\n';
    out << "    <integer>" << position.y << "</integer>" << '\n';
    out << "</dict>" << '\n';
    out << "</plist>" << '\n';
}
#else
std::optional<X11WindowPosition> read_window_position(std::istream &in)
{
    X11WindowPosition position{};
    bool have_x{};
    bool have_y{};
    for (std::string line; std::getline(in, line);)
    {
        have_x = read_int_value(line, "left", position.x) || have_x;
        have_y = read_int_value(line, "top", position.y) || have_y;
    }
    if (!have_x || !have_y)
    {
        return {};
    }
    return position;
}

void write_window_position(std::ostream &out, const X11WindowPosition &position, Display *display, const int screen)
{
    out << "left=" << position.x << '\n';
    out << "top=" << position.y << '\n';
    out << "screen_width=" << DisplayWidth(display, screen) << '\n';
    out << "screen_height=" << DisplayHeight(display, screen) << '\n';
}
#endif

std::optional<X11WindowPosition> load_window_position(Display *display, const int screen)
{
    const std::filesystem::path path{window_position_path()};
    if (path.empty())
    {
        return {};
    }

    std::ifstream in{path};
    if (!in)
    {
        return {};
    }

    const std::optional<X11WindowPosition> position{read_window_position(in)};
    if (!position.has_value())
    {
        return {};
    }

    const int screen_width{DisplayWidth(display, screen)};
    const int screen_height{DisplayHeight(display, screen)};
    if (position->x < 0 || position->y < 0 || position->x >= screen_width || position->y >= screen_height)
    {
        return {};
    }
    return position;
}

std::optional<X11WindowPosition> parse_geometry_position(
    const std::string &geometry, Display *display, const int screen, const int width, const int height)
{
    if (geometry.empty())
    {
        return {};
    }

    int x{};
    int y{};
    unsigned int geometry_width{};
    unsigned int geometry_height{};
    const int mask{XParseGeometry(geometry.c_str(), &x, &y, &geometry_width, &geometry_height)};
    if ((mask & (XValue | YValue)) == 0)
    {
        return {};
    }

    X11WindowPosition position{};
    if ((mask & XValue) != 0)
    {
        position.x = (mask & XNegative) != 0 ? DisplayWidth(display, screen) - width + x : x;
    }
    if ((mask & YValue) != 0)
    {
        position.y = (mask & YNegative) != 0 ? DisplayHeight(display, screen) - height + y : y;
    }
    return position;
}

void set_size_hints(Display *display, const Window window, const int width, const int height,
    const std::optional<X11WindowPosition> &position)
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
    if (position)
    {
        hints.flags |= USPosition | PPosition;
        hints.x = position->x;
        hints.y = position->y;
    }
    XSetWMNormalHints(display, window, &hints);
}

int button_flag(const unsigned int button)
{
    switch (button)
    {
    case Button1:
        return X11_MOUSE_LEFT;
    case Button2:
        return X11_MOUSE_MIDDLE;
    case Button3:
        return X11_MOUSE_RIGHT;
    default:
        return 0;
    }
}

int button_state_flags(const unsigned int state)
{
    int result{};
    if ((state & Button1Mask) != 0)
    {
        result |= X11_MOUSE_LEFT;
    }
    if ((state & Button2Mask) != 0)
    {
        result |= X11_MOUSE_MIDDLE;
    }
    if ((state & Button3Mask) != 0)
    {
        result |= X11_MOUSE_RIGHT;
    }
    if ((state & ShiftMask) != 0)
    {
        result |= X11_MOUSE_SHIFT;
    }
    if ((state & ControlMask) != 0)
    {
        result |= X11_MOUSE_CONTROL;
    }
    return result;
}

int press_key_flags(const XButtonEvent &event)
{
    return button_state_flags(event.state) | button_flag(event.button);
}

int release_key_flags(const XButtonEvent &event)
{
    return button_state_flags(event.state) & ~button_flag(event.button);
}

std::string key_flags_string(const int value)
{
    std::string result;
    const auto append_flag = [&](const int flag, const char *label)
    {
        if ((value & flag) != 0)
        {
            if (!result.empty())
            {
                result += " | ";
            }
            result += label;
        }
    };
    append_flag(X11_MOUSE_LEFT, "LEFT");
    append_flag(X11_MOUSE_RIGHT, "RIGHT");
    append_flag(X11_MOUSE_SHIFT, "SHIFT");
    append_flag(X11_MOUSE_CONTROL, "CONTROL");
    append_flag(X11_MOUSE_MIDDLE, "MIDDLE");
    return result.empty() ? "NONE" : result;
}

int get_mouse_look_key()
{
    const int key{+g_look_at_mouse};
    assert(key < 0);
    return -key;
}

#undef ID_DEBUG_MOUSE
#ifdef ID_DEBUG_MOUSE
void debug_mouse(const std::string &text)
{
    driver_debug_line(text);
}
#else
void debug_mouse(const std::string & /*text*/)
{
}
#endif

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

bool X11Frame::init(std::string title)
{
    m_title = std::move(title);
    return m_connection.open();
}

void X11Frame::terminate()
{
    destroy_window();
    m_connection.close();
}

void X11Frame::set_geometry(std::string geometry)
{
    m_geometry = std::move(geometry);
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

    std::optional<X11WindowPosition> position{
        parse_geometry_position(m_geometry, display, m_connection.screen(), width, height)};
    if (!position)
    {
        position = load_window_position(display, m_connection.screen());
    }
    const int x{position ? position->x : 0};
    const int y{position ? position->y : 0};
    m_window = XCreateWindow(display, m_connection.root_window(), x, y, width, height, 0, m_connection.depth(),
        InputOutput, m_connection.visual(), CWBackPixel | CWBorderPixel | CWColormap | CWEventMask, &attributes);
    if (m_window == None)
    {
        return;
    }

    m_width = width;
    m_height = height;
    m_input_windows.clear();
    add_input_window(m_window);
    XStoreName(display, m_window, m_title.c_str());
    Atom delete_window = m_connection.wm_delete_window();
    XSetWMProtocols(display, m_window, &delete_window, 1);
    set_size_hints(display, m_window, width, height, position);
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

X11Connection &X11Frame::connection()
{
    return m_connection;
}

const X11Connection &X11Frame::connection() const
{
    return m_connection;
}

Window X11Frame::window() const
{
    return m_window;
}

void X11Frame::set_event_handler(EventHandler handler)
{
    m_event_handler = std::move(handler);
}

void X11Frame::add_input_window(const Window window)
{
    if (window == None || is_input_window(window))
    {
        return;
    }

    m_input_windows.push_back(window);
}

void X11Frame::remove_input_window(const Window window)
{
    m_input_windows.erase(std::remove(m_input_windows.begin(), m_input_windows.end(), window), m_input_windows.end());
}

void X11Frame::get_cursor_pos(int &x, int &y) const
{
    x = m_cursor_x;
    y = m_cursor_y;
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

    save_window_position();
    XDestroyWindow(m_connection.display(), m_window);
    XFlush(m_connection.display());
    m_window = None;
    m_mapped = false;
    m_input_windows.clear();
}

void X11Frame::handle_event(const XEvent &event)
{
    if (event.type == ClientMessage && event.xclient.window == m_window &&
        static_cast<Atom>(event.xclient.data.l[0]) == m_connection.wm_delete_window())
    {
        save_window_position();
        ui::goodbye();
    }
    if (event.type == KeyPress && is_input_window(event.xkey.window))
    {
        handle_key_press(event.xkey);
    }
    if (event.type == MotionNotify && is_input_window(event.xmotion.window))
    {
        handle_mouse_move(event.xmotion);
    }
    if (event.type == ButtonPress && is_input_window(event.xbutton.window))
    {
        handle_button_press(event.xbutton);
    }
    if (event.type == ButtonRelease && is_input_window(event.xbutton.window))
    {
        handle_button_release(event.xbutton);
    }
    if (event.type == DestroyNotify && event.xdestroywindow.window == m_window)
    {
        m_window = None;
        m_mapped = false;
        m_input_windows.clear();
    }
    if (m_event_handler)
    {
        m_event_handler(event);
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

void X11Frame::handle_button_press(XButtonEvent event)
{
    update_cursor_pos(event.x, event.y);
    const int key_flags{press_key_flags(event)};
    const bool double_click{is_double_click(event)};
    grab_pointer(event.time);

    switch (event.button)
    {
    case Button1:
        mouse_notify_primary_down(double_click, event.x, event.y, key_flags);
        debug_mouse((double_click ? "left down: (double) " : "left down: ") + std::to_string(event.x) + "," +
            std::to_string(event.y) + ", flags: " + key_flags_string(key_flags));
        break;
    case Button2:
        mouse_notify_middle_down(double_click, event.x, event.y, key_flags);
        debug_mouse((double_click ? "middle down: (double) " : "middle down: ") + std::to_string(event.x) + "," +
            std::to_string(event.y) + ", flags: " + key_flags_string(key_flags));
        break;
    case Button3:
        mouse_notify_secondary_down(double_click, event.x, event.y, key_flags);
        debug_mouse((double_click ? "right down: (double) " : "right down: ") + std::to_string(event.x) + "," +
            std::to_string(event.y) + ", flags: " + key_flags_string(key_flags));
        break;
    default:
        break;
    }

    m_last_button_window = event.window;
    m_last_button = event.button;
    m_last_button_time = event.time;
    m_last_button_x = event.x;
    m_last_button_y = event.y;
}

void X11Frame::handle_button_release(XButtonEvent event)
{
    update_cursor_pos(event.x, event.y);
    const int key_flags{release_key_flags(event)};
    switch (event.button)
    {
    case Button1:
        mouse_notify_primary_up(event.x, event.y, key_flags);
        if (+g_look_at_mouse < 0)
        {
            add_key_press(get_mouse_look_key());
        }
        else
        {
            debug_mouse("left up: " + std::to_string(event.x) + "," + std::to_string(event.y) +
                ", flags: " + key_flags_string(key_flags));
        }
        break;
    case Button2:
        mouse_notify_middle_up(event.x, event.y, key_flags);
        debug_mouse("middle up: " + std::to_string(event.x) + "," + std::to_string(event.y) +
            ", flags: " + key_flags_string(key_flags));
        break;
    case Button3:
        mouse_notify_secondary_up(event.x, event.y, key_flags);
        debug_mouse("right up: " + std::to_string(event.x) + "," + std::to_string(event.y) +
            ", flags: " + key_flags_string(key_flags));
        break;
    default:
        break;
    }
    if ((key_flags & (X11_MOUSE_LEFT | X11_MOUSE_RIGHT | X11_MOUSE_MIDDLE)) == 0)
    {
        ungrab_pointer(event.time);
    }
}

void X11Frame::handle_mouse_move(XMotionEvent event)
{
    update_cursor_pos(event.x, event.y);
    const int key_flags{button_state_flags(event.state)};
    mouse_notify_move(event.x, event.y, key_flags);
    debug_mouse("movement: " + std::to_string(event.x) + "," + std::to_string(event.y) +
        ", flags: " + key_flags_string(key_flags));
}

void X11Frame::update_cursor_pos(const int x, const int y)
{
    m_cursor_x = x;
    m_cursor_y = y;
}

bool X11Frame::is_double_click(const XButtonEvent &event) const
{
    constexpr Time DOUBLE_CLICK_MS{500};
    constexpr int DOUBLE_CLICK_DISTANCE{4};
    return m_last_button_window == event.window && m_last_button == event.button &&
        event.time - m_last_button_time <= DOUBLE_CLICK_MS &&
        std::abs(event.x - m_last_button_x) <= DOUBLE_CLICK_DISTANCE &&
        std::abs(event.y - m_last_button_y) <= DOUBLE_CLICK_DISTANCE;
}

void X11Frame::grab_pointer(const Time time)
{
    if (m_window == None || m_pointer_grabbed)
    {
        return;
    }

    const int result = XGrabPointer(m_connection.display(), m_window, True,
        PointerMotionMask | ButtonPressMask | ButtonReleaseMask, GrabModeAsync, GrabModeAsync, None, None, time);
    m_pointer_grabbed = result == GrabSuccess;
}

void X11Frame::ungrab_pointer(const Time time)
{
    if (!m_pointer_grabbed)
    {
        return;
    }

    XUngrabPointer(m_connection.display(), time);
    XFlush(m_connection.display());
    m_pointer_grabbed = false;
}

bool X11Frame::is_input_window(const Window window) const
{
    return std::find(m_input_windows.begin(), m_input_windows.end(), window) != m_input_windows.end();
}

void X11Frame::set_fixed_size(const int width, const int height)
{
    set_size_hints(m_connection.display(), m_window, width, height, {});
}

void X11Frame::save_window_position() const
{
    if (m_window == None)
    {
        return;
    }

    const std::filesystem::path path{window_position_path()};
    if (path.empty())
    {
        return;
    }

    Display *display{m_connection.display()};
    int x{};
    int y{};
    Window child{};
    if (!XTranslateCoordinates(display, m_window, m_connection.root_window(), 0, 0, &x, &y, &child))
    {
        return;
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
    {
        return;
    }

    std::ofstream out{path};
    if (!out)
    {
        return;
    }
    write_window_position(out, X11WindowPosition{x, y}, display, m_connection.screen());
}

} // namespace id::misc
