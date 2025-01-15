// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/load_entry_text.h"

static bool skip_starting_rows(std::FILE *entry_file, int start_row)
{
    bool comment = false;
    int c;
    for (int i = 0; i < start_row; i++)
    {
        while ((c = fgetc(entry_file)) != '\n' && c != EOF)
        {
            if (c == ';')
            {
                comment = true;
            }
            if (c == '}' && !comment)    // end of entry before start line
            {
                break;                 // this should never happen
            }
        }
        if (c == '\n')
        {
            comment = false;
        }
        else
        {
            // reached end of file or end of entry
            return true;
        }
    }
    return false;
}

void load_entry_text(
    std::FILE *entry_file,
    char *buf,
    int max_lines,
    int start_row,
    int start_col)
{
    if (max_lines <= 0)
    {
        // no lines to get!
        *buf = 0;
        return;
    }

    // move down to starting row
    if (skip_starting_rows(entry_file, start_row))
    {
        // reached end of file or end of entry
        *buf = 0;
        return;
    }

    // write max_lines of entry
    const int tab_pos = 7 - start_col % 8;
    while (max_lines-- > 0)
    {
        bool comment = false;
        int c = 0;
        int line_len = 0;

        // skip line up to start_col
        if (start_col > 0)
        {
            int i = 0;
            while (i++ < start_col && (c = fgetc(entry_file)) != EOF)
            {
                if (c == ';')
                {
                    comment = true;
                }
                if (c == '}' && !comment)
                {
                    // reached end of entry
                    *buf = 0;
                    return;
                }
                if (c == '\r')
                {
                    i--;
                    continue;
                }
                if (c == '\t')
                {
                    i += 7 - i % 8;
                }
                if (c == '\n')
                {
                    // need to insert '\n', even for short lines
                    *buf++ = (char)c;
                    break;
                }
            }
            if (c == EOF)
            {
                // unexpected end of file
                *buf = 0;
                return;
            }
            if (c == '\n')         // line is already completed
            {
                continue;
            }

            if (i > start_col)
            {
                // can happen because of <tab> character
                while (i-- > start_col)
                {
                    *buf++ = ' ';
                    line_len++;
                }
            }
        }

        // process rest of line into buf
        while ((c = fgetc(entry_file)) != EOF)
        {
            if (c == ';')
            {
                comment = true;
            }
            else if (c == '\n' || c == '\r')
            {
                comment = false;
            }
            if (c != '\r')
            {
                if (c == '\t')
                {
                    while (line_len % 8 != tab_pos && line_len < 75)
                    {
                        // 76 wide max
                        *buf++ = ' ';
                        ++line_len;
                    }
                    c = ' ';
                }
                if (c == '\n')
                {
                    *buf++ = '\n';
                    break;
                }
                if (++line_len > 75)
                {
                    if (line_len == 76)
                    {
                        *buf++ = SCROLL_MARKER;
                    }
                }
                else
                {
                    *buf++ = (char)c;
                }
                if (c == '}' && !comment)
                {
                    // reached end of entry
                    *buf = 0;
                    return;
                }
            }
        }
        if (c == EOF)
        {
            // unexpected end of file
            *buf = 0;
            return;
        }
    }
    if (*(buf-1) == '\n')   // specified that buf will not end with a '\n'
    {
        buf--;
    }
    *buf = 0;
}
