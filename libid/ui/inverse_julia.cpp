// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/inverse_julia.h"

#include "engine/jiim.h"
#include "engine/resume.h"
#include "fractals/fractalp.h"
#include "misc/Driver.h"
#include "ui/check_key.h"
#include "ui/editpal.h"
#include "ui/get_a_number.h"
#include "ui/id_keys.h"

#include <memory>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

namespace
{

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

} // namespace

KeyboardHandlerPtr make_inverse_julia_keyboard_handler(InverseJuliaKeyboardContext &context)
{
    return std::make_shared<InverseJuliaKeyboardHandler>(context);
}

bool process_inverse_julia_keyboard(
    InverseJuliaKeyboardContext &context, CrossHairCursor &cursor, const bool wait_for_key)
{
    if (wait_for_key)
    {
        cursor.wait_key();
    }

    context.begin_key_batch();
    bool key_pressed = false;
    while (driver_key_pressed())
    {
        cursor.wait_key();
        key_pressed = true;
        dispatch_keyboard_key(driver_get_key());
        if (context.leaving_now())
        {
            break;
        }
    }
    return key_pressed;
}

void requeue_inverse_julia_key(const int key)
{
    driver_unget_key(key);
}

int inverse_julia_fractal_type()
{
    int color = 0;

    if (g_resuming)              // can't resume
    {
        return -1;
    }

    while (color >= 0)       // generate points
    {
        if (check_key())
        {
            free_queue();
            return -1;
        }
        color = orbit_calc();
        g_old_z = g_new_z;
    }
    free_queue();
    return 0;
}

} // namespace id::ui
