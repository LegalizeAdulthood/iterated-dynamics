// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/input_field.h"

#include "drivers.h"
#include "id_keys.h"
#include "mouse.h"
#include "round_float_double.h"
#include "ValueSaver.h"

#include <cmath>
#include <cstdio>
#include <cstring>

int input_field(InputFieldFlags options, //
    int attr,                            // display attribute
    char *fld,                           // the field itself
    int len,                             // field length (declare as 1 larger for \0)
    int row,                             // display row
    int col,                             // display column
    int (*check_key)(int key)            // routine to check non data keys, or nullptr
)
{
    char save_fld[81];
    char buf[81];
    int j;
    ValueSaver saved_look_at_mouse{g_look_at_mouse, +MouseLook::IGNORE_MOUSE};
    int ret = -1;
    std::strcpy(save_fld, fld);
    int insert = 0;
    bool started = false;
    int offset = 0;
    bool display = true;
    while (true)
    {
        std::strcpy(buf, fld);
        int i = (int) std::strlen(buf);
        while (i < len)
        {
            buf[i++] = ' ';
        }
        buf[len] = 0;
        if (display)
        {
            // display current value
            driver_put_string(row, col, attr, buf);
            display = false;
        }
        int key = driver_key_cursor(row + insert, col + offset);  // get a keystroke
        if (key == 1047) // numeric keypad /  TODO: map scan code to ID_KEY value
        {
            key = 47; // numeric slash
        }
        switch (key)
        {
        case ID_KEY_ENTER:
        case ID_KEY_ENTER_2:
            ret = 0;
            goto inpfld_end;
        case ID_KEY_ESC:
            goto inpfld_end;
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
            offset = (int) std::strlen(fld);
            started = true;
            break;
        case ID_KEY_BACKSPACE:
        case 127:                              // backspace
            if (offset > 0)
            {
                j = (int) std::strlen(fld);
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
            j = (int) std::strlen(fld);
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
            std::strcpy(fld, save_fld);
            offset = 0;
            insert = offset;
            started = false;
            display = true;
            break;
        default:
            if (non_alpha(key))
            {
                if (check_key && (ret = (*check_key)(key)) != 0)
                {
                    goto inpfld_end;
                }
                break;                                // non alphanum char
            }
            if (offset >= len)
            {
                break;                // at end of field
            }
            if (insert && started && std::strlen(fld) >= (size_t)len)
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
                j = (int) std::strlen(fld);
                while (j >= offset)
                {
                    fld[j+1] = fld[j];
                    --j;
                }
            }
            if ((size_t)offset >= std::strlen(fld))
            {
                fld[offset+1] = 0;
            }
            fld[offset++] = (char)key;
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
                    char tmp_fld[30];
                    if (!bit_set(options, InputFieldFlags::DOUBLE))
                    {
                        round_float_double(&tmp_d);
                    }
                    std::sprintf(tmp_fld, "%.15g", tmp_d);
                    tmp_fld[len-1] = 0; // safety, field should be long enough
                    std::strcpy(fld, tmp_fld);
                    offset = 0;
                }
            }
            started = true;
            display = true;
        }
    }
inpfld_end:
    return ret;
}
