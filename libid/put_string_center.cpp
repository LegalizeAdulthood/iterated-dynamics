// SPDX-License-Identifier: GPL-3.0-only
//
#include "put_string_center.h"

#include "drivers.h"

#include <cstring>

int put_string_center(int row, int col, int width, int attr, char const *msg)
{
    char buf[81];
    int i;
    int j;
    int k;
    i = 0;
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
    j = (width - i) / 2;
    j -= (width + 10 - i) / 20; // when wide a bit left of center looks better
    std::memset(buf, ' ', width);
    buf[width] = 0;
    i = 0;
    k = j;
    while (msg[i])
    {
        buf[k++] = msg[i++]; // std::strcpy for a
    }
    driver_put_string(row, col, attr, buf);
    return j;
}
