#include "load_entry_text.h"

void load_entry_text(
    std::FILE *entfile,
    char *buf,
    int maxlines,
    int startrow,
    int startcol)
{
    int linelen;
    bool comment = false;
    int c = 0;
    int tabpos = 7 - (startcol % 8);

    if (maxlines <= 0)
    {
        // no lines to get!
        *buf = (char) 0;
        return;
    }

    //move down to starting row
    for (int i = 0; i < startrow; i++)
    {
        while ((c = fgetc(entfile)) != '\n' && c != EOF && c != '\032')
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
            *buf = (char) 0;
            return;
        }
    }

    // write maxlines of entry
    while (maxlines-- > 0)
    {
        comment = false;
        c = 0;
        linelen = c;

        // skip line up to startcol
        int i = 0;
        while (i++ < startcol && (c = fgetc(entfile)) != EOF && c != '\032')
        {
            if (c == ';')
            {
                comment = true;
            }
            if (c == '}' && !comment)
            {
                //reached end of entry
                *buf = (char) 0;
                return;
            }
            if (c == '\r')
            {
                i--;
                continue;
            }
            if (c == '\t')
            {
                i += 7 - (i % 8);
            }
            if (c == '\n')
            {
                //need to insert '\n', even for short lines
                *(buf++) = (char)c;
                break;
            }
        }
        if (c == EOF || c == '\032')
        {
            // unexpected end of file
            *buf = (char) 0;
            return;
        }
        if (c == '\n')         // line is already completed
        {
            continue;
        }

        if (i > startcol)
        {
            // can happen because of <tab> character
            while (i-- > startcol)
            {
                *(buf++) = ' ';
                linelen++;
            }
        }

        //process rest of line into buf
        while ((c = fgetc(entfile)) != EOF && c != '\032')
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
                    while ((linelen % 8) != tabpos && linelen < 75)
                    {
                        // 76 wide max
                        *(buf++) = ' ';
                        ++linelen;
                    }
                    c = ' ';
                }
                if (c == '\n')
                {
                    *(buf++) = '\n';
                    break;
                }
                if (++linelen > 75)
                {
                    if (linelen == 76)
                    {
                        *(buf++) = '\021';

                    }
                }
                else
                {
                    *(buf++) = (char)c;
                }
                if (c == '}' && !comment)
                {
                    //reached end of entry
                    *(buf) = (char) 0;
                    return;
                }
            }
        }
        if (c == EOF || c == '\032')
        {
            // unexpected end of file
            *buf = (char) 0;
            return;
        }
    }
    if (*(buf-1) == '\n')   // specified that buf will not end with a '\n'
    {
        buf--;
    }
    *buf = (char) 0;
}
