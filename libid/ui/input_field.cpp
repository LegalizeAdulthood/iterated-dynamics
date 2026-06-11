// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/input_field.h"

#include "math/round_float_double.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"

#include <fmt/format.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace id::math;
using namespace id::misc;

namespace id::ui
{

int input_field(const InputFieldFlags options, //
    const int attr,                            // display attribute
    char *fld,                                 // the field itself
    const int len,                             // field length (declare as 1 larger for \0)
    const int row,                             // display row
    const int col,                             // display column
    int (*check_key)(int key)                  // routine to check non data keys, or nullptr
)
{
    return input_field(options, attr, fld, len, len, row, col, check_key);
}

int input_field(const InputFieldFlags options, //
    const int attr,                            // display attribute
    char *fld,                                 // the field itself
    const int len,                             // field length (declare as 1 larger for \0)
    const int display_len,                     // display width
    const int row,                             // display row
    const int col,                             // display column
    int (*check_key)(int key)                  // routine to check non data keys, or nullptr
)
{
    const std::string save_fld{fld};
    int j;
    ValueSaver saved_look_at_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    int ret = -1;
    int insert = 0;
    bool started = false;
    int offset = 0;
    int window_start = 0;
    bool display = true;
    while (true)
    {
        int visible_len = display_len > 0 && display_len < len ? display_len : len;
        if (visible_len < 0)
        {
            visible_len = 0;
        }
        const int old_window_start = window_start;
        if (offset < window_start)
        {
            window_start = offset;
        }
        if (offset > window_start + visible_len)
        {
            window_start = offset - visible_len;
        }
        if (old_window_start != window_start)
        {
            display = true;
        }
        std::string buf{fld + window_start};
        buf.resize(visible_len, ' ');
        if (display)
        {
            // display current value
            driver_put_string(row, col, attr, buf.c_str());
            display = false;
        }
        int key = driver_key_cursor(row + insert, col + offset - window_start);  // get a keystroke
        if (key == 1047) // numeric keypad /  TODO: map scan code to ID_KEY value
        {
            key = 47; // numeric slash
        }
        switch (key)
        {
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            ret = 0;
            goto end;
        case ID_KEY_ESC:
            goto end;
        case ID_KEY_RIGHT_ARROW:
            if (offset < len-1)
            {
                ++offset;
            }
            started = true;
            break;
        case ID_KEY_LEFT_ARROW:
            if (offset > 0)
            {
                --offset;
            }
            started = true;
            break;
        case ID_KEY_HOME:
            offset = 0;
            started = true;
            break;
        case ID_KEY_END:
            offset = static_cast<int>(std::strlen(fld));
            started = true;
            break;
        case ID_KEY_BACKSPACE:
        case 127:                              // backspace
            if (offset > 0)
            {
                j = static_cast<int>(std::strlen(fld));
                for (int k = offset-1; k < j; ++k)
                {
                    fld[k] = fld[k+1];
                }
                --offset;
            }
            started = true;
            display = true;
            break;
        case ID_KEY_DELETE:                           // delete
            j = static_cast<int>(std::strlen(fld));
            for (int k = offset; k < j; ++k)
            {
                fld[k] = fld[k+1];
            }
            started = true;
            display = true;
            break;
        case ID_KEY_INSERT:                           // insert
            insert ^= 0x8000;
            started = true;
            break;
        case ID_KEY_F5:
            std::strcpy(fld, save_fld.c_str());
            offset = 0;
            insert = 0;
            started = false;
            display = true;
            break;
        default:
            if (non_alpha(key))
            {
                if (check_key && (ret = (*check_key)(key)) != 0)
                {
                    goto end;
                }
                break;                                // non alphanum char
            }
            if (offset >= len)
            {
                break;                // at end of field
            }
            if (insert && started && std::strlen(fld) >= static_cast<size_t>(len))
            {
                break;                                // insert & full
            }
            if (bit_set(options , InputFieldFlags::NUMERIC)
                && (key < '0' || key > '9')
                && key != '+' && key != '-')
            {
                if (bit_set(options, InputFieldFlags::INTEGER))
                {
                    break;
                }
                // allow scientific notation, and specials "e" and "p"
                if (((key != 'e' && key != 'E') || offset >= 18)
                    && ((key != 'p' && key != 'P') || offset != 0)
                    && key != '.')
                {
                    break;
                }
            }
            if (!started)   // first char is data, zap field
            {
                fld[0] = 0;
            }
            if (insert)
            {
                j = static_cast<int>(std::strlen(fld));
                while (j >= offset)
                {
                    fld[j+1] = fld[j];
                    --j;
                }
            }
            if (static_cast<size_t>(offset) >= std::strlen(fld))
            {
                fld[offset+1] = 0;
            }
            fld[offset++] = static_cast<char>(key);
            // if "e" or "p" in first col make number e or pi
            if ((options & (InputFieldFlags::NUMERIC | InputFieldFlags::INTEGER)) == InputFieldFlags::NUMERIC)
            {
                // floating point
                double tmp_d;
                bool special_val = false;
                if (*fld == 'e' || *fld == 'E')
                {
                    tmp_d = std::exp(1.0);
                    special_val = true;
                }
                if (*fld == 'p' || *fld == 'P')
                {
                    tmp_d = std::atan(1.0) * 4;
                    special_val = true;
                }
                if (special_val)
                {
                    if (!bit_set(options, InputFieldFlags::DOUBLE))
                    {
                        round_float_double(&tmp_d);
                    }
                    std::string tmp_fld{fmt::format("{:.15g}", tmp_d)};
                    if (len > 0 && tmp_fld.size() >= static_cast<size_t>(len))
                    {
                        tmp_fld.resize(static_cast<size_t>(len - 1));
                    }
                    std::strcpy(fld, tmp_fld.c_str());
                    offset = 0;
                }
            }
            started = true;
            display = true;
        }
    }
end:
    return ret;
}

} // namespace id::ui
