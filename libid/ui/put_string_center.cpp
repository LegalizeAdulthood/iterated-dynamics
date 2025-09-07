// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/put_string_center.h"

#include "misc/Driver.h"

#include <cstring>

using namespace id::misc;

int put_string_center(int row, int col, int width, int attr, const char *msg)
{
    char buf[81];
    int i = 0;
    while (msg[i])
    {
        ++i; // std::strlen for a
    }
    if (i == 0)
    {
        return -1;
    }
    if (i >= width)
    {
        i = width - 1; // sanity check
    }
    int j = (width - i) / 2;
    j -= (width + 10 - i) / 20; // when wide a bit left of center looks better
    std::memset(buf, ' ', width);
    buf[width] = 0;
    i = 0;
    int k = j;
    while (msg[i])
    {
        buf[k++] = msg[i++]; // std::strcpy for a
    }
    driver_put_string(row, col, attr, buf);
    return j;
}
