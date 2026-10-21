// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/inverse_julia.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/ImageRegion.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel.h"
#include "engine/pixel_grid.h"
#include "engine/random_seed.h"
#include "engine/jiim.h"
#include "engine/resume.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "io/disk_storage.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/memory.h"
#include "misc/ValueSaver.h"
#include "ui/editpal.h"
#include "ui/find_special_colors.h"
#include "ui/frothy_basin.h"
#include "ui/get_a_number.h"
#include "ui/help.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/standard_orbit_plot.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/video.h"

#include <fmt/format.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

namespace
{

enum
{
    MAX_RECT = 1024 // largest width of SaveRect/RestoreRect
};

enum class JuliaWindowStyle
{
    LARGE = 0,       // full screen or large view window
    NO_OVERLAP = 1,  // Julia/orbit and fractal don't overlap
    FULL_SCREEN = 2, // Julia/orbit takes whole screen
    HIDDEN = 3,
};

constexpr int NUMBER_FONT_HEIGHT{8};
static int s_secret_mode{};

static void fill_rect(const int x, int y, const int width, int depth, const int color)
{
    // fast version of fillrect
    if (!g_has_inverse)
    {
        return;
    }
    const std::vector row(width, static_cast<char>(color % g_colors));
    while (depth-- > 0)
    {
        put_row(x, y++, width, row.data());
    }
}

enum class OrbitFlags
{
    POINT = 0,
    CIRCLE = 1,
    LINE = 2
};

int operator+(const OrbitFlags value)
{
    return static_cast<int>(value);
}

OrbitFlags operator&(const OrbitFlags lhs, const OrbitFlags rhs)
{
    return static_cast<OrbitFlags>(+lhs & +rhs);
}

bool bit_set(const OrbitFlags value, const OrbitFlags bit)
{
    return (value & bit) == bit;
}

OrbitFlags operator^(const OrbitFlags lhs, const OrbitFlags rhs)
{
    return static_cast<OrbitFlags>(+lhs ^ +rhs);
}

OrbitFlags &operator^=(OrbitFlags &lhs, const OrbitFlags rhs)
{
    lhs = lhs ^ rhs;
    return lhs;
}

class InverseJuliaController : public InverseJuliaKeyboardContext
{
public:
    explicit InverseJuliaController(JIIMType which);

    void process();
    JIIMType which() const override;
    void begin_key_batch() override;
    bool leaving_now() const override;
    void set_last_key(int key) override;
    void leave_now() override;
    void leave_after_refresh() override;
    bool continuing() const override;
    void reset_julia_selection() override;
    void select_julia_from_cursor() override;
    void reset_zoom() override;
    void zoom(float factor) override;
    void toggle_circle_mode() override;
    void toggle_line_mode() override;
    void toggle_numbers() override;
    void set_exact_point(DComplex point) override;
    void toggle_hidden_fractal() override;
    void set_secret_mode(int mode) override;
    void move_cursor(int d_col, int d_row) override;

private:
    void save_rect(int x, int y, int width, int depth);
    void restore_rect(int x, int y, int width, int depth);
    void start();
    bool iterate();
    bool process_keyboard(bool wait_for_key);
    void finish();

    JIIMType m_which;
    InverseJulia m_calculation;
    CrossHairCursor m_cursor;
    bool m_show_numbers{};
    std::vector<char> m_screen_rect;
    JuliaWindowStyle m_window_style{};
    int m_corner_x{};
    int m_corner_y{};
    int m_win_width{};
    int m_win_height{};
    bool m_ok_to_miim{};
    Affine m_cvt{};
    bool m_exact{};
    DComplex m_c{};
    int m_x_factor{};
    int m_y_factor{}; // aspect ratio
    int m_x_off{};
    int m_y_off{};    // center of the window
    int m_key{-1};
    float m_zoom{};
    double m_aspect{};
    bool m_first_time{true};
    int m_old_screen_x_offset{g_logical_screen.x_offset};
    int m_old_screen_y_offset{g_logical_screen.y_offset};
    bool m_still{true};
    bool m_leave_now{};

    static OrbitFlags s_mode; // point, circle, ...
};

OrbitFlags InverseJuliaController::s_mode{};

void InverseJuliaController::save_rect(const int x, const int y, const int width, const int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    m_screen_rect.clear();
    const std::vector background(width, static_cast<char>(g_color_dark));
    m_screen_rect.resize(width * depth);
    m_cursor.hide();
    for (int y_off = 0; y_off < depth; y_off++)
    {
        get_row(x, y + y_off, width, &m_screen_rect[width * y_off]);
        put_row(x, y + y_off, width, background.data());
    }
    m_cursor.show();
}

void InverseJuliaController::restore_rect(const int x, const int y, const int width, const int depth)
{
    if (!g_has_inverse)
    {
        return;
    }

    m_cursor.hide();
    for (int y_off = 0; y_off < depth; y_off++)
    {
        put_row(x, y + y_off, width, &m_screen_rect[width * y_off]);
    }
    m_cursor.show();
}

InverseJuliaController::InverseJuliaController(const JIIMType which) :
    m_which{which},
    m_calculation{which}
{
    find_special_colors();
}

void InverseJuliaController::start()
{
    if (m_which == JIIMType::ORBIT)
    {
        g_has_inverse = true;
    }

    m_show_numbers = false;
    g_using_jiim = true;
    g_line_buff.resize(std::max(g_screen_x_dots, g_screen_y_dots));
    m_aspect = static_cast<double>(g_logical_screen.x_dots) * 3 /
        (static_cast<double>(g_logical_screen.y_dots) * 4); // assumes 4:3

    if (m_which == JIIMType::ORBIT)
    {
        per_image();
    }

    m_cursor = CrossHairCursor();

    // MIIM code: Grab memory for Queue/Stack before SaveRect gets it.
    m_ok_to_miim = false;
    if (m_which == JIIMType::JIIM && g_debug_flag != DebugFlags::PREVENT_MIIM)
    {
        m_ok_to_miim = prepare_inverse_julia_queue(8 * 1024UL);
    }
    if (m_which == JIIMType::JIIM && !m_ok_to_miim)
    {
        set_random_seed();
    }

    g_vesa_x_res = g_screen_x_dots;
    g_vesa_y_res = g_screen_y_dots;

    if (g_logical_screen.x_offset != 0 || g_logical_screen.y_offset != 0) // we're in view windows
    {
        ValueSaver saved_has_inverse(g_has_inverse, true);
        save_rect(0, 0, g_logical_screen.x_dots, g_logical_screen.y_dots);
        g_logical_screen.x_offset = g_video_start_x;
        g_logical_screen.y_offset = g_video_start_y;
        restore_rect(0, 0, g_logical_screen.x_dots, g_logical_screen.y_dots);
    }

    if (g_logical_screen.x_dots == g_vesa_x_res || g_logical_screen.y_dots == g_vesa_y_res ||
        g_vesa_x_res - g_logical_screen.x_dots < g_vesa_x_res / 3 ||
        g_vesa_y_res - g_logical_screen.y_dots < g_vesa_y_res / 3 || g_logical_screen.x_dots >= MAX_RECT)
    {
        // this mode puts orbit/julia in an overlapping window 1/3 the size of the physical screen
        m_window_style = JuliaWindowStyle::LARGE;
        m_win_width = g_vesa_x_res / 3;
        m_win_height = g_vesa_y_res / 3;
        m_corner_x = g_video_start_x + m_win_width * 2;
        m_corner_y = g_video_start_y + m_win_height * 2;
        m_x_off = g_video_start_x + m_win_width * 5 / 2;
        m_y_off = g_video_start_y + m_win_height * 5 / 2;
    }
    else if (g_logical_screen.x_dots > g_vesa_x_res / 3 && g_logical_screen.y_dots > g_vesa_y_res / 3)
    {
        m_window_style = JuliaWindowStyle::NO_OVERLAP;
        m_win_width = g_vesa_x_res - g_logical_screen.x_dots;
        m_win_height = g_vesa_y_res - g_logical_screen.y_dots;
        m_corner_x = g_video_start_x + g_logical_screen.x_dots;
        m_corner_y = g_video_start_y + g_logical_screen.y_dots;
        m_x_off = m_corner_x + m_win_width / 2;
        m_y_off = m_corner_y + m_win_height / 2;
    }
    else
    {
        m_window_style = JuliaWindowStyle::FULL_SCREEN;
        m_win_width = g_vesa_x_res;
        m_win_height = g_vesa_y_res;
        m_corner_x = g_video_start_x;
        m_corner_y = g_video_start_y;
        m_x_off = g_video_start_x + m_win_width / 2;
        m_y_off = g_video_start_y + m_win_height / 2;
    }

    m_x_factor = static_cast<int>(m_win_width / 5.33);
    m_y_factor = -m_win_height / 4;

    if (m_window_style == JuliaWindowStyle::LARGE)
    {
        save_rect(m_corner_x, m_corner_y, m_win_width, m_win_height);
    }
    else if (m_window_style == JuliaWindowStyle::FULL_SCREEN) // leave the fractal
    {
        fill_rect(
            g_logical_screen.x_dots, m_corner_y, m_win_width - g_logical_screen.x_dots, m_win_height, g_color_dark);
        fill_rect(m_corner_x, g_logical_screen.y_dots, g_logical_screen.x_dots, m_win_height - g_logical_screen.y_dots,
            g_color_dark);
    }
    else // blank whole window
    {
        fill_rect(m_corner_x, m_corner_y, m_win_width, m_win_height, g_color_dark);
    }

    setup_convert_to_screen(&m_cvt);

    // reuse last location if inside window
    g_col = static_cast<int>(std::lround(m_cvt.a * g_save_c.x + m_cvt.b * g_save_c.y + m_cvt.e));
    g_row = static_cast<int>(std::lround(m_cvt.c * g_save_c.x + m_cvt.d * g_save_c.y + m_cvt.f));
    if (g_col < 0 || g_col >= g_logical_screen.x_dots || g_row < 0 || g_row >= g_logical_screen.y_dots)
    {
        m_c = g_image_region.center();
    }
    else
    {
        m_c = g_save_c;
    }

    g_col = static_cast<int>(std::lround(m_cvt.a * m_c.x + m_cvt.b * m_c.y + m_cvt.e));
    g_row = static_cast<int>(std::lround(m_cvt.c * m_c.x + m_cvt.d * m_c.y + m_cvt.f));

    fill_pixel_grid();

    m_cursor.set_pos(g_col, g_row);
    m_cursor.show();

    m_zoom = 1.0f;
}

JIIMType InverseJuliaController::which() const
{
    return m_which;
}

void InverseJuliaController::begin_key_batch()
{
    m_still = true;
    m_leave_now = false;
}

bool InverseJuliaController::leaving_now() const
{
    return m_leave_now;
}

void InverseJuliaController::set_last_key(const int key)
{
    m_key = key;
}

void InverseJuliaController::leave_now()
{
    m_leave_now = true;
}

void InverseJuliaController::leave_after_refresh()
{
    m_still = false;
}

bool InverseJuliaController::continuing() const
{
    return m_still;
}

void InverseJuliaController::reset_julia_selection()
{
    g_julia_c.x = JULIA_C_NOT_SET;
    g_julia_c.y = JULIA_C_NOT_SET;
}

void InverseJuliaController::select_julia_from_cursor()
{
    g_julia_c = m_c;
}

void InverseJuliaController::reset_zoom()
{
    m_zoom = 1.0F;
}

void InverseJuliaController::zoom(const float factor)
{
    m_zoom *= factor;
}

void InverseJuliaController::toggle_circle_mode()
{
    s_mode ^= OrbitFlags::CIRCLE;
}

void InverseJuliaController::toggle_line_mode()
{
    s_mode ^= OrbitFlags::LINE;
}

void InverseJuliaController::toggle_numbers()
{
    m_show_numbers = !m_show_numbers;
    if (m_window_style == JuliaWindowStyle::LARGE && !m_show_numbers)
    {
        m_cursor.hide();
        clear_temp_msg();
        m_cursor.show();
    }
}

void InverseJuliaController::set_exact_point(const DComplex point)
{
    m_c = point;
    m_exact = true;
    g_col = static_cast<int>(std::lround(m_cvt.a * m_c.x + m_cvt.b * m_c.y + m_cvt.e));
    g_row = static_cast<int>(std::lround(m_cvt.c * m_c.x + m_cvt.d * m_c.y + m_cvt.f));
}

void InverseJuliaController::toggle_hidden_fractal()
{
    if (m_window_style == JuliaWindowStyle::FULL_SCREEN)
    {
        m_window_style = JuliaWindowStyle::HIDDEN;
    }
    else if (m_window_style == JuliaWindowStyle::HIDDEN && m_win_width == g_vesa_x_res)
    {
        restore_rect(g_video_start_x, g_video_start_y, g_logical_screen.x_dots, g_logical_screen.y_dots);
        m_window_style = JuliaWindowStyle::FULL_SCREEN;
    }
}

void InverseJuliaController::set_secret_mode(const int mode)
{
    s_secret_mode = mode;
}

void InverseJuliaController::move_cursor(const int d_col, const int d_row)
{
    if (d_col > 0 || d_row > 0)
    {
        m_exact = false;
    }
    g_col += d_col;
    g_row += d_row;

    // keep cursor in logical screen
    if (g_col >= g_logical_screen.x_dots)
    {
        g_col = g_logical_screen.x_dots - 1;
        m_exact = false;
    }
    if (g_row >= g_logical_screen.y_dots)
    {
        g_row = g_logical_screen.y_dots - 1;
        m_exact = false;
    }
    if (g_col < 0)
    {
        g_col = 0;
        m_exact = false;
    }
    if (g_row < 0)
    {
        g_row = 0;
        m_exact = false;
    }

    m_cursor.set_pos(g_col, g_row);
}

class InverseJuliaMouseNotification : public NullMouseNotification
{
public:
    explicit InverseJuliaMouseNotification(CrossHairCursor &cursor) :
        m_cursor{cursor}
    {
    }

    void move(int x, int y, int key_flags) override
    {
        m_cursor.set_pos(x, y);
        NullMouseNotification::move(x, y, key_flags);
    }

private:
    CrossHairCursor &m_cursor;
};

class InverseJuliaKeyboardHandler : public KeyboardHandler
{
public:
    explicit InverseJuliaKeyboardHandler(InverseJuliaKeyboardContext &context) :
        m_context{context}
    {
    }

    bool handle_key(int key) override
    {
        int d_col = 0;
        int d_row = 0;
        constexpr int BIG_DELTA{4};
        constexpr int SMALL_DELTA{1};
        constexpr float ZOOM_INCREMENT{1.15f};

        m_context.set_last_key(key);
        m_context.reset_julia_selection();
        switch (key)
        {
        case ID_KEY_CTL_KEYPAD_5:
        case ID_KEY_KEYPAD_5:
            break;

        case ID_KEY_CTL_PAGE_UP:
            d_col = BIG_DELTA;
            d_row = -BIG_DELTA;
            break;

        case ID_KEY_CTL_PAGE_DOWN:
            d_col = BIG_DELTA;
            d_row = BIG_DELTA;
            break;

        case ID_KEY_CTL_HOME:
            d_col = -BIG_DELTA;
            d_row = -BIG_DELTA;
            break;

        case ID_KEY_CTL_END:
            d_col = -BIG_DELTA;
            d_row = BIG_DELTA;
            break;

        case ID_KEY_PAGE_UP:
            d_col = SMALL_DELTA;
            d_row = -SMALL_DELTA;
            break;

        case ID_KEY_PAGE_DOWN:
            d_col = SMALL_DELTA;
            d_row = SMALL_DELTA;
            break;

        case ID_KEY_HOME:
            d_col = -SMALL_DELTA;
            d_row = -SMALL_DELTA;
            break;

        case ID_KEY_END:
            d_col = SMALL_DELTA;
            d_row = SMALL_DELTA;
            break;

        case ID_KEY_UP_ARROW:
            d_row = -SMALL_DELTA;
            break;

        case ID_KEY_DOWN_ARROW:
            d_row = SMALL_DELTA;
            break;

        case ID_KEY_LEFT_ARROW:
            d_col = -SMALL_DELTA;
            break;

        case ID_KEY_RIGHT_ARROW:
            d_col = SMALL_DELTA;
            break;

        case ID_KEY_CTL_UP_ARROW:
            d_row = -BIG_DELTA;
            break;

        case ID_KEY_CTL_DOWN_ARROW:
            d_row = BIG_DELTA;
            break;

        case ID_KEY_CTL_LEFT_ARROW:
            d_col = -BIG_DELTA;
            break;

        case ID_KEY_CTL_RIGHT_ARROW:
            d_col = BIG_DELTA;
            break;

        case 'z':
        case 'Z':
            m_context.reset_zoom();
            break;

        case '<':
        case ',':
            m_context.zoom(1.0F / ZOOM_INCREMENT);
            break;

        case '>':
        case '.':
            m_context.zoom(ZOOM_INCREMENT);
            break;

        case ID_KEY_SPACE:
            m_context.select_julia_from_cursor();
            m_context.leave_now();
            return true;

        case 'c':
        case 'C':
            m_context.toggle_circle_mode();
            break;

        case 'l':
        case 'L':
            m_context.toggle_line_mode();
            break;

        case 'n':
        case 'N':
            m_context.toggle_numbers();
            break;

        case 'p':
        case 'P':
            set_exact_point();
            break;

        case 'h':
        case 'H':
            m_context.toggle_hidden_fractal();
            break;

        case '0':
        case '1':
        case '2':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            if (m_context.which() == JIIMType::JIIM)
            {
                m_context.set_secret_mode(key - '0');
            }
            else
            {
                m_context.leave_after_refresh();
            }
            break;

        case 's':
        case 'S':
            m_context.leave_now();
            return true;

        default:
            m_context.leave_after_refresh();
            break;
        }

        m_context.move_cursor(d_col, d_row);
        return true;
    }

private:
    void set_exact_point()
    {
        DComplex point;
        get_a_number(&point.x, &point.y);
        m_context.set_exact_point(point);
    }

    InverseJuliaKeyboardContext &m_context;
};

class InverseJuliaOrbitKeyboardHandler : public KeyboardHandler
{
public:
    bool handle_key(const int key) override
    {
        if (g_show_orbit)
        {
            scrub_orbit_display();
        }
        if (key == 'o' || key == 'O')
        {
            if (!driver_is_disk())
            {
                g_show_orbit = !g_show_orbit;
            }
            return true;
        }

        driver_unget_key(key);
        set_calc_interrupted();
        m_interrupted = true;
        return true;
    }

    bool interrupted() const
    {
        return m_interrupted;
    }

private:
    bool m_interrupted{};
};

} // namespace

class InverseJuliaMouseScope::Impl
{
public:
    explicit Impl(CrossHairCursor &cursor) :
        m_saved_look_at_mouse{g_look_at_mouse, MouseLook::POSITION},
        m_saved_cursor_mouse_tracking{g_cursor_mouse_tracking, true},
        m_subscription{std::make_shared<InverseJuliaMouseNotification>(cursor)}
    {
    }

private:
    ValueSaver<MouseLook> m_saved_look_at_mouse;
    ValueSaver<bool> m_saved_cursor_mouse_tracking;
    MouseSubscription m_subscription;
};

InverseJuliaMouseScope::InverseJuliaMouseScope(CrossHairCursor &cursor) :
    m_impl{std::make_shared<Impl>(cursor)}
{
}

InverseJuliaMouseScope::~InverseJuliaMouseScope() = default;

KeyboardHandlerPtr make_inverse_julia_keyboard_handler(InverseJuliaKeyboardContext &context)
{
    return std::make_shared<InverseJuliaKeyboardHandler>(context);
}

namespace
{

bool InverseJuliaController::process_keyboard(const bool wait_for_key)
{
    if (wait_for_key)
    {
        m_cursor.wait_key();
    }

    begin_key_batch();
    bool key_pressed = false;
    while (driver_key_pressed())
    {
        m_cursor.wait_key();
        key_pressed = true;
        dispatch_keyboard_key(driver_get_key());
        if (leaving_now())
        {
            break;
        }
    }
    return key_pressed;
}

bool InverseJuliaController::iterate()
{
    bool mouse_updated{};
    if (m_cursor.get_x() != g_col || m_cursor.get_y() != g_row)
    {
        g_col = m_cursor.get_x();
        g_row = m_cursor.get_y();
        mouse_updated = true;
    }
    if (m_calculation.active() || mouse_updated)
    {
        m_cursor.check_blink();
    }
    const bool key_pressed{process_keyboard(!m_calculation.active() && !mouse_updated)};
    if (leaving_now())
    {
        return false;
    }
    if (key_pressed || m_first_time || mouse_updated)
    {
        m_first_time = false;
        const bool cursor_was_visible{!m_cursor.hidden()};
        if (cursor_was_visible)
        {
            m_cursor.hide();
        }

        if (!m_exact)
        {
            m_c.x = dx_pixel();
            m_c.y = dy_pixel();
        }

        if (m_show_numbers) // write coordinates on screen
        {
            std::string msg{fmt::format("{:16.14f} {:16.14f} {:3d}", m_c.x, m_c.y, get_color(g_col, g_row))};
            if (m_window_style == JuliaWindowStyle::LARGE)
            {
                // show temp msg will clear self if new msg is a different length - pad to length 40
                msg += std::string(40 - msg.length(), ' ');
                m_cursor.hide();
                show_temp_msg(msg);
                m_cursor.show();
            }
            else
            {
                driver_display_string(5, g_vesa_y_res - NUMBER_FONT_HEIGHT, g_color_bright, g_color_dark, msg.c_str());
            }
        }
        g_save_c = m_c;
        m_calculation.reset(m_c, m_ok_to_miim);
        // move window if bumped
        if (m_window_style == JuliaWindowStyle::LARGE && g_col > m_corner_x && g_col < m_corner_x + m_win_width &&
            g_row > m_corner_y && g_row < m_corner_y + m_win_height)
        {
            restore_rect(m_corner_x, m_corner_y, m_win_width, m_win_height);
            if (m_corner_x == g_video_start_x + m_win_width * 2)
            {
                m_corner_x = g_video_start_x + 2;
            }
            else
            {
                m_corner_x = g_video_start_x + m_win_width * 2;
            }
            m_x_off = m_corner_x + m_win_width / 2;
            save_rect(m_corner_x, m_corner_y, m_win_width, m_win_height);
        }
        if (m_window_style == JuliaWindowStyle::FULL_SCREEN)
        {
            fill_rect(g_logical_screen.x_dots, m_corner_y, m_win_width - g_logical_screen.x_dots,
                m_win_height - (m_show_numbers ? NUMBER_FONT_HEIGHT : 0), g_color_dark);
            fill_rect(m_corner_x, g_logical_screen.y_dots, g_logical_screen.x_dots,
                m_win_height - g_logical_screen.y_dots - (m_show_numbers ? NUMBER_FONT_HEIGHT : 0), g_color_dark);
        }
        else
        {
            fill_rect(m_corner_x, m_corner_y, m_win_width, m_win_height, g_color_dark);
        }
        if (cursor_was_visible)
        {
            m_cursor.show();
        }
    }

    const InverseJuliaPlot plot{m_corner_x, m_corner_y, m_win_width, m_win_height, m_x_factor, m_y_factor, m_x_off,
        m_y_off, g_screen_y_dots - (m_show_numbers ? NUMBER_FONT_HEIGHT : 0),
        m_window_style == JuliaWindowStyle::FULL_SCREEN, m_zoom, m_aspect, bit_set(s_mode, OrbitFlags::CIRCLE),
        bit_set(s_mode, OrbitFlags::LINE), static_cast<int>(s_secret_mode)};
    if (!m_calculation.iterate(plot))
    {
        return true;
    }

    return continuing();
}

void InverseJuliaController::finish()
{
    if (m_ok_to_miim)
    {
        free_queue();
    }

    if (m_key != 's' && m_key != 'S')
    {
        m_cursor.hide();
        if (m_window_style == JuliaWindowStyle::LARGE)
        {
            restore_rect(m_corner_x, m_corner_y, m_win_width, m_win_height);
        }
        else if (m_window_style >= JuliaWindowStyle::FULL_SCREEN)
        {
            if (m_window_style == JuliaWindowStyle::FULL_SCREEN)
            {
                fill_rect(g_logical_screen.x_dots, m_corner_y, m_win_width - g_logical_screen.x_dots, m_win_height,
                    g_color_dark);
                fill_rect(m_corner_x, g_logical_screen.y_dots, g_logical_screen.x_dots,
                    m_win_height - g_logical_screen.y_dots, g_color_dark);
            }
            else
            {
                fill_rect(m_corner_x, m_corner_y, m_win_width, m_win_height, g_color_dark);
            }
            if (m_window_style == JuliaWindowStyle::HIDDEN && m_win_width == g_vesa_x_res) // unhide
            {
                restore_rect(0, 0, g_logical_screen.x_dots, g_logical_screen.y_dots);
                m_window_style = JuliaWindowStyle::FULL_SCREEN;
            }
            m_cursor.hide();
            ValueSaver saved_has_inverse{g_has_inverse, true};
            save_rect(0, 0, g_logical_screen.x_dots, g_logical_screen.y_dots);
            g_logical_screen.x_offset = m_old_screen_x_offset;
            g_logical_screen.y_offset = m_old_screen_y_offset;
            restore_rect(0, 0, g_logical_screen.x_dots, g_logical_screen.y_dots);
        }
    }
    g_line_buff.clear();
    m_screen_rect.clear();
    g_using_jiim = false;
    if (m_key == 's' || m_key == 'S')
    {
        g_viewport.enabled = false;
        g_viewport.x_dots = 0;
        g_viewport.y_dots = 0;
        g_viewport.reduction = 4.2F;
        g_viewport.crop = true;
        g_viewport.final_aspect_ratio = g_screen_aspect;
        g_logical_screen.x_dots = g_screen_x_dots;
        g_logical_screen.y_dots = g_screen_y_dots;
        g_logical_screen.x_size_dots = g_logical_screen.x_dots - 1;
        g_logical_screen.y_size_dots = g_logical_screen.y_dots - 1;
        g_logical_screen.x_offset = 0;
        g_logical_screen.y_offset = 0;
        free_temp_msg();
    }
    else
    {
        clear_temp_msg();
    }
    m_show_numbers = false;
    driver_unget_key(m_key);
}

void InverseJuliaController::process()
{
    ValueSaver saved_debug_flag{g_debug_flag};
    ValueSaver saved_help_mode{
        g_help_mode, m_which == JIIMType::JIIM ? HelpLabels::HELP_JIIM : HelpLabels::HELP_ORBITS};
    ValueSaver saved_dispatch{g_dispatch};
    KeyboardHandlerPtr keyboard_handler{make_inverse_julia_keyboard_handler(*this)};
    ScopedKeyboardHandler keyboard_scope{keyboard_handler};

    start();
    {
        InverseJuliaMouseScope mouse_scope{m_cursor};
        while (iterate())
        {
        }
    }
    finish();
}

} // namespace

static bool static_calc_type_allows_jiim()
{
    const CalcType table_calc_type{g_cur_fractal_specific->calc_type};
    return table_calc_type == standard_fractal_type //
        || table_calc_type == calc_mandelbrot_type  //
        || table_calc_type == froth_type;
}

void jiim(const JIIMType which)
{
    // must use standard fractal or be calcfroth
    if (!static_calc_type_allows_jiim())
    {
        return;
    }

    InverseJuliaController tool(which);
    tool.process();
}

bool prepare_inverse_julia_queue(const unsigned long request)
{
    if (request > static_cast<unsigned long>(std::numeric_limits<int>::max() / 8))
    {
        throw std::logic_error("Invalid inverse-Julia queue capacity");
    }
    if (driver_is_disk())
    {
        stop_msg("Don't try this in disk video mode, kids...\n");
        return false;
    }
    free_queue();
    for (unsigned long capacity = request; capacity > 1024; capacity /= 2)
    {
        std::uint64_t blocks{};
        const std::int64_t bytes{static_cast<std::int64_t>(capacity) * 8};
        if (!disk_video_memory_blocks(bytes, 1, 256, 0, blocks))
        {
            throw std::logic_error("Invalid inverse-Julia queue storage size");
        }
        std::string error;
        try
        {
            init_queue(capacity, memory_alloc(DISK_STORAGE_BLOCK_SIZE, blocks, MemoryLocation::MEMORY));
            return true;
        }
        catch (const MemoryError &failure)
        {
            error = failure.what();
        }
        catch (const std::bad_alloc &)
        {
            error = "Insufficient free memory for inverse-Julia queue";
        }
        if (stop_msg(StopMsgFlags::CANCEL, error + "\nPress a key to retry with a smaller queue."))
        {
            return false;
        }
    }
    return false;
}

int inverse_julia_fractal_type()
{
    int color = 0;

    if (g_resuming)              // can't resume
    {
        return -1;
    }

    const bool needs_queue{g_major_method == Major::BREADTH_FIRST || g_major_method == Major::DEPTH_FIRST};
    const bool has_queue{needs_queue && prepare_inverse_julia_queue(32 * 1024UL)};
    if (needs_queue && !has_queue)
    {
        stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, "Not enough memory: switching to random walk.\n");
        g_major_method = Major::RANDOM_WALK;
    }
    init_inverse_julia();

    reset_calc_interrupted();
    std::shared_ptr<InverseJuliaOrbitKeyboardHandler> keyboard_handler{
        std::make_shared<InverseJuliaOrbitKeyboardHandler>()};
    ScopedKeyboardHandler keyboard_scope{keyboard_handler};

    while (color >= 0)       // generate points
    {
        dispatch_pending_keyboard_input();
        if (keyboard_handler->interrupted())
        {
            if (has_queue)
            {
                free_queue();
            }
            return -1;
        }
        color = orbit_calc();
        g_old_z = g_new_z;
    }
    if (has_queue)
    {
        free_queue();
    }
    return 0;
}

} // namespace id::ui
