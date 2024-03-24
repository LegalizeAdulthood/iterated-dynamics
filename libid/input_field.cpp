#include "input_field.h"

#include "port.h"

#include "drivers.h"
#include "id.h"
#include "id_data.h"
#include "miscres.h"
#include "round_float_double.h"

#include <cmath>
#include <cstdio>
#include <cstring>

int input_field(
    int options,          // &1 numeric, &2 integer, &4 double
    int attr,             // display attribute
    char *fld,            // the field itself
    int len,              // field length (declare as 1 larger for \0)
    int row,              // display row
    int col,              // display column
    int (*checkkey)(int curkey)  // routine to check non data keys, or nullptr
)
{
    char savefld[81];
    char buf[81];
    int curkey;
    int i, j;
    int old_look_at_mouse = g_look_at_mouse;
    g_look_at_mouse = 0;
    int ret = -1;
    std::strcpy(savefld, fld);
    int insert = 0;
    bool started = false;
    int offset = 0;
    bool display = true;
    while (true)
    {
        std::strcpy(buf, fld);
        i = (int) std::strlen(buf);
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
        curkey = driver_key_cursor(row+insert, col+offset);  // get a keystroke
        if (curkey == 1047)
        {
            curkey = 47; // numeric slash
        }
        switch (curkey)
        {
        case FIK_ENTER:
        case FIK_ENTER_2:
            ret = 0;
            goto inpfld_end;
        case FIK_ESC:
            goto inpfld_end;
        case FIK_RIGHT_ARROW:
            if (offset < len-1)
            {
                ++offset;
            }
            started = true;
            break;
        case FIK_LEFT_ARROW:
            if (offset > 0)
            {
                --offset;
            }
            started = true;
            break;
        case FIK_HOME:
            offset = 0;
            started = true;
            break;
        case FIK_END:
            offset = (int) std::strlen(fld);
            started = true;
            break;
        case FIK_BACKSPACE:
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
        case FIK_DELETE:                           // delete
            j = (int) std::strlen(fld);
            for (int k = offset; k < j; ++k)
            {
                fld[k] = fld[k+1];
            }
            started = true;
            display = true;
            break;
        case FIK_INSERT:                           // insert
            insert ^= 0x8000;
            started = true;
            break;
        case FIK_F5:
            std::strcpy(fld, savefld);
            offset = 0;
            insert = offset;
            started = false;
            display = true;
            break;
        default:
            if (nonalpha(curkey))
            {
                if (checkkey && (ret = (*checkkey)(curkey)) != 0)
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
            if ((options & INPUTFIELD_NUMERIC)
                && (curkey < '0' || curkey > '9')
                && curkey != '+' && curkey != '-')
            {
                if (options & INPUTFIELD_INTEGER)
                {
                    break;
                }
                // allow scientific notation, and specials "e" and "p"
                if (((curkey != 'e' && curkey != 'E') || offset >= 18)
                    && ((curkey != 'p' && curkey != 'P') || offset != 0)
                    && curkey != '.')
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
            fld[offset++] = (char)curkey;
            // if "e" or "p" in first col make number e or pi
            if ((options & (INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER)) == INPUTFIELD_NUMERIC)
            {
                // floating point
                double tmpd;
                char tmpfld[30];
                bool specialv = false;
                if (*fld == 'e' || *fld == 'E')
                {
                    tmpd = std::exp(1.0);
                    specialv = true;
                }
                if (*fld == 'p' || *fld == 'P')
                {
                    tmpd = std::atan(1.0) * 4;
                    specialv = true;
                }
                if (specialv)
                {
                    if ((options & INPUTFIELD_DOUBLE) == 0)
                    {
                        roundfloatd(&tmpd);
                    }
                    std::sprintf(tmpfld, "%.15g", tmpd);
                    tmpfld[len-1] = 0; // safety, field should be long enough
                    std::strcpy(fld, tmpfld);
                    offset = 0;
                }
            }
            started = true;
            display = true;
        }
    }
inpfld_end:
    g_look_at_mouse = old_look_at_mouse;
    return ret;
}
