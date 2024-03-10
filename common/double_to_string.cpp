#include "double_to_string.h"

#include <cstdio>
#include <cstring>

void double_to_string(char *buf, double val)
{
    constexpr std::size_t length = 20;
    // cellular needs 16
    for (int i = 16; i > 0; --i)
    {
        std::sprintf(buf, "%.*g", i, val);
        if (std::strlen(buf) <= length)
        {
            return;
        }
    }
}
