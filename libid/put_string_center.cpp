#include "put_string_center.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "find_path.h"
#include "memory.h"
#include "os.h"

#include <cstdio>
#include <cstring>

#ifdef XFRACT
#include <unistd.h>
#endif

int putstringcenter(int row, int col, int width, int attr, char const *msg)
{
    char buf[81];
    int i, j, k;
    i = 0;
#ifdef XFRACT
    if (width >= 80)
    {
        width = 79; // Some systems choke in column 80
    }
#endif
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
