// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/file_gets.h"

#include <cstdio>

int file_gets(char *buf, int max_len, std::FILE *infile)
{
    // similar to 'fgets', but file may be in either text or binary mode
    // returns -1 at eof, length of string otherwise
    if (std::feof(infile))
    {
        return -1;
    }
    int len = 0;
    while (len < max_len)
    {
        int c = getc(infile);
        if (c == EOF)
        {
            if (len)
            {
                break;
            }
            return -1;
        }
        if (c == '\n')
        {
            break;             // linefeed is end of line
        }
        if (c != '\r')
        {
            buf[len++] = (char)c;    // ignore c/r
        }
    }
    buf[len] = 0;
    return len;
}
