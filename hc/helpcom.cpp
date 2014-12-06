/*
 * helpcom.cpp
 *
 * Common code for hc.cpp and help.cpp
 *
 */
#include <cassert>

#include <string.h>

#include "port.h"
#include "helpcom.h"

#ifndef XFRACT
#define getint(ptr) (*(int *)(ptr))
#define setint(ptr,n) (*(int *)(ptr)) = n
#else
/* Get an int from an unaligned pointer
 * This routine is needed because this program uses unaligned 2 byte
 * pointers all over the place.
 */
int
getint(char *ptr)
{
    int s;
    bcopy(ptr,&s,sizeof(int));
    return s;
}

/* Set an int to an unaligned pointer */
void setint(char *ptr, int n)
{
    bcopy(&n,ptr,sizeof(int));
}
#endif


int is_hyphen(const char *ptr)   /* true if ptr points to a real hyphen */
{   /* checkes for "--" and " -" */
    if (*ptr != '-')
        return (0);    /* that was easy! */

    --ptr;

    return (*ptr!=' ' && *ptr!='-');
}


int _find_token_length(char *curr, unsigned len, int *size, int *width)
{
    int _size  = 0;
    int _width = 0;
    int tok;

    if (len == 0)
        tok = TOK_DONE;

    else
    {
        switch (*curr)
        {
        case ' ':    /* it's a run of spaces */
            tok = TOK_SPACE;
            while (*curr == ' ' && _size < (int)len)
            {
                ++curr;
                ++_size;
                ++_width;
            }
            break;

        case CMD_SPACE:
            tok = TOK_SPACE;
            ++curr;
            ++_size;
            _width = *curr;
            ++curr;
            ++_size;
            break;

        case CMD_LINK:
            tok = TOK_LINK;
            _size += 1+3*sizeof(int); /* skip CMD_LINK + topic_num + topic_off + page_num */
            curr += 1+3*sizeof(int);

            while (*curr != CMD_LINK)
            {
                if (*curr == CMD_LITERAL)
                {
                    ++curr;
                    ++_size;
                }
                ++curr;
                ++_size;
                ++_width;
                assert((unsigned) _size < len);
            }

            ++_size;   /* skip ending CMD_LINK */
            break;

        case CMD_PARA:
            tok = TOK_PARA;
            _size += 3;     /* skip CMD_PARA + indent + margin */
            break;

        case CMD_XONLINE:
            tok = TOK_XONLINE;
            ++_size;
            break;

        case CMD_XDOC:
            tok = TOK_XDOC;
            ++_size;
            break;

        case CMD_CENTER:
            tok = TOK_CENTER;
            ++_size;
            break;

        case '\n':
            tok = TOK_NL;
            ++_size;
            break;

        case CMD_FF:
            tok = TOK_FF;
            ++_size;
            break;

        default:   /* it must be a word */
            tok = TOK_WORD;
            for (;;)
            {
                if (_size >= (int)len)
                    break;

                else if (*curr == CMD_LITERAL)
                {
                    curr += 2;
                    _size += 2;
                    _width += 1;
                }

                else if (*curr == '\0')
                {
                    assert(0);
                }

                else if ((unsigned)*curr <= MAX_CMD || *curr == ' ' ||
                         *curr == '\n')
                    break;

                else if (*curr == '-')
                {
                    ++curr;
                    ++_size;
                    ++_width;
                    if (is_hyphen(curr-1))
                        break;
                }

                else
                {
                    ++curr;
                    ++_size;
                    ++_width;
                }
            }
            break;
        } /* switch */
    }

    if (size  != nullptr)   *size  = _size;
    if (width != nullptr)   *width = _width;

    return (tok);
}


int find_token_length(int mode, char *curr, unsigned len, int *size, int *width)
{
    int tok;
    int t;
    int _size;

    tok = _find_token_length(curr, len, &t, width);

    if ((tok == TOK_XONLINE && mode == ONLINE) ||
            (tok == TOK_XDOC    && mode == DOC))
    {
        _size = 0;

        for (;;)
        {
            curr  += t;
            len   -= t;
            _size += t;

            tok = _find_token_length(curr, len, &t, nullptr);

            if ((tok == TOK_XONLINE && mode == ONLINE) ||
                    (tok == TOK_XDOC    && mode == DOC)    ||
                    (tok == TOK_DONE))
                break;
        }

        _size += t;
    }
    else
        _size = t;

    if (size != nullptr)
        *size = _size;

    return (tok);
}


int find_line_width(int mode, char *curr, unsigned len)
{
    int size   = 0,
        width  = 0,
        lwidth = 0,
        done   = 0,
        tok;

    do
    {
        tok = find_token_length(mode, curr, len, &size, &width);

        switch (tok)
        {
        case TOK_DONE:
        case TOK_PARA:
        case TOK_NL:
        case TOK_FF:
            done = 1;
            break;

        case TOK_XONLINE:
        case TOK_XDOC:
        case TOK_CENTER:
            curr += size;
            len -= size;
            break;

        default:   /* TOK_SPACE, TOK_LINK or TOK_WORD */
            lwidth += width;
            curr += size;
            len -= size;
            break;
        }
    }
    while (!done);

    return (lwidth);
}


#define DO_PRINTN(ch,n)  ( pd.s = &(ch), pd.i = (n), output(PD_PRINTN, &pd, info) )
#define DO_PRINT(str,n)  ( pd.s = (str), pd.i = (n), output(PD_PRINT, &pd, info) )


int process_document(PD_FUNC get_info, PD_FUNC output, VOIDPTR info)
{
    int       skip_blanks;
    int       tok;
    int       size,
              width;
    int       col;
    char      page_text[10];
    PD_INFO   pd;
    char      nl = '\n',
              sp = ' ';
    int       first_section,
              first_topic;

    pd.pnum = 1;
    pd.lnum = 0;

    output(PD_HEADING, &pd, info);

    first_section = 1;

    while (get_info(PD_GET_CONTENT, &pd, info))
    {
        if (!output(PD_START_SECTION, &pd, info))
            return (0);

        if (pd.new_page && pd.lnum != 0)
        {
            if (!output(PD_FOOTING, &pd, info))
                return (0);
            ++pd.pnum;
            pd.lnum = 0;
            if (!output(PD_HEADING, &pd, info))
                return (0);
        }

        else
        {
            if (pd.lnum+2 > PAGE_DEPTH-CONTENT_BREAK)
            {
                if (!output(PD_FOOTING, &pd, info))
                    return (0);
                ++pd.pnum;
                pd.lnum = 0;
                if (!output(PD_HEADING, &pd, info))
                    return (0);
            }
            else if (pd.lnum > 0)
            {
                if (!DO_PRINTN(nl, 2))
                    return (0);
                pd.lnum += 2;
            }
        }

        if (!output(PD_SET_SECTION_PAGE, &pd, info))
            return (0);

        if (!first_section)
        {
            if (!output(PD_PRINT_SEC, &pd, info))
                return (0);
            ++pd.lnum;
        }

        first_topic = 1;

        while (get_info(PD_GET_TOPIC, &pd, info))
        {
            if (!output(PD_START_TOPIC, &pd, info))
                return (0);

            skip_blanks = 0;
            col = 0;

            if (!first_section)     /* do not skip blanks for DocContents */
            {
                while (pd.len > 0)
                {
                    tok = find_token_length(DOC, pd.curr, pd.len, &size, nullptr);
                    if (tok != TOK_XDOC && tok != TOK_XONLINE &&
                            tok != TOK_NL   && tok != TOK_DONE)
                        break;
                    pd.curr += size;
                    pd.len  -= size;
                }
                if (first_topic && pd.len != 0)
                {
                    if (!DO_PRINTN(nl, 1))
                        return (0);
                    ++pd.lnum;
                }
            }

            if (pd.lnum > PAGE_DEPTH-TOPIC_BREAK)
            {
                if (!output(PD_FOOTING, &pd, info))
                    return (0);
                ++pd.pnum;
                pd.lnum = 0;
                if (!output(PD_HEADING, &pd, info))
                    return (0);
            }
            else if (!first_topic)
            {
                if (!DO_PRINTN(nl, 1))
                    return (0);
                pd.lnum++;
            }

            if (!output(PD_SET_TOPIC_PAGE, &pd, info))
                return (0);

            do
            {
                if (!output(PD_PERIODIC, &pd, info))
                    return (0);

                tok = find_token_length(DOC, pd.curr, pd.len, &size, &width);

                switch (tok)
                {
                case TOK_PARA:
                {
                    int       indent,
                              margin;
                    unsigned  holdlen = 0;
                    char *holdcurr = 0;
                    int       in_link = 0;

                    ++pd.curr;

                    indent = *pd.curr++;
                    margin = *pd.curr++;

                    pd.len -= 3;

                    if (!DO_PRINTN(sp, indent))
                        return (0);

                    col = indent;

                    for (;;)
                    {
                        if (!output(PD_PERIODIC, &pd, info))
                            return (0);

                        tok = find_token_length(DOC, pd.curr, pd.len, &size, &width);

                        if (tok == TOK_NL || tok == TOK_FF)
                            break;

                        if (tok == TOK_DONE)
                        {
                            if (in_link == 0)
                            {
                                col = 0;
                                ++pd.lnum;
                                if (!DO_PRINTN(nl, 1))
                                    return (0);
                                break;
                            }

                            else if (in_link == 1)
                            {
                                tok = TOK_SPACE;
                                width = 1;
                                size = 0;
                                ++in_link;
                            }

                            else if (in_link == 2)
                            {
                                tok = TOK_WORD;
                                width = (int) strlen(page_text);
                                col += 8 - width;
                                size = 0;
                                pd.curr = page_text;
                                ++in_link;
                            }

                            else if (in_link == 3)
                            {
                                pd.curr = holdcurr;
                                pd.len = holdlen;
                                in_link = 0;
                                continue;
                            }
                        }

                        if (tok == TOK_PARA)
                        {
                            col = 0;   /* fake a nl */
                            ++pd.lnum;
                            if (!DO_PRINTN(nl, 1))
                                return (0);
                            break;
                        }

                        if (tok == TOK_XONLINE || tok == TOK_XDOC)
                        {
                            pd.curr += size;
                            pd.len -= size;
                            continue;
                        }

                        if (tok == TOK_LINK)
                        {
                            pd.s = pd.curr+1;
                            if (get_info(PD_GET_LINK_PAGE, &pd, info))
                            {
                                in_link = 1;
                                sprintf(page_text, "(p. %d)", pd.i);
                            }
                            else
                                in_link = 3;
                            holdcurr = pd.curr + size;
                            holdlen = pd.len - size;
                            pd.len = size - 2 - 3*sizeof(int);
                            pd.curr += 1 + 3*sizeof(int);
                            continue;
                        }

                        /* now tok is TOK_SPACE or TOK_WORD */

                        if (col+width > PAGE_WIDTH)
                        {   /* go to next line... */
                            if (!DO_PRINTN(nl, 1))
                                return (0);
                            if (++pd.lnum >= PAGE_DEPTH)
                            {
                                if (!output(PD_FOOTING, &pd, info))
                                    return (0);
                                ++pd.pnum;
                                pd.lnum = 0;
                                if (!output(PD_HEADING, &pd, info))
                                    return (0);
                            }

                            if (tok == TOK_SPACE)
                                width = 0;   /* skip spaces at start of a line */

                            if (!DO_PRINTN(sp, margin))
                                return (0);
                            col = margin;
                        }

                        if (width > 0)
                        {
                            if (tok == TOK_SPACE)
                            {
                                if (!DO_PRINTN(sp, width))
                                    return (0);
                            }
                            else
                            {
                                if (!DO_PRINT(pd.curr, (size==0) ? width : size))
                                    return (0);
                            }
                        }

                        col += width;
                        pd.curr += size;
                        pd.len -= size;
                    }

                    skip_blanks = 0;
                    width = size = 0;
                    break;
                }

                case TOK_NL:
                    if (skip_blanks && col == 0)
                        break;

                    ++pd.lnum;

                    if (pd.lnum >= PAGE_DEPTH || (col == 0 && pd.lnum >= PAGE_DEPTH-BLANK_BREAK))
                    {
                        if (col != 0)      /* if last wasn't a blank line... */
                        {
                            if (!DO_PRINTN(nl, 1))
                                return (0);
                        }
                        if (!output(PD_FOOTING, &pd, info))
                            return (0);
                        ++pd.pnum;
                        pd.lnum = 0;
                        skip_blanks = 1;
                        if (!output(PD_HEADING, &pd, info))
                            return (0);
                    }
                    else
                    {
                        if (!DO_PRINTN(nl, 1))
                            return (0);
                    }

                    col = 0;
                    break;

                case TOK_FF:
                    if (skip_blanks)
                        break;
                    if (!output(PD_FOOTING, &pd, info))
                        return (0);
                    col = 0;
                    pd.lnum = 0;
                    ++pd.pnum;
                    if (!output(PD_HEADING, &pd, info))
                        return (0);
                    break;

                case TOK_CENTER:
                    width = (PAGE_WIDTH - find_line_width(DOC,pd.curr,pd.len)) / 2;
                    if (!DO_PRINTN(sp, width))
                        return (0);
                    break;

                case TOK_LINK:
                    skip_blanks = 0;
                    if (!DO_PRINT(pd.curr+1+3*sizeof(int),
                                  size-3*sizeof(int)-2))
                        return (0);
                    pd.s = pd.curr+1;
                    if (get_info(PD_GET_LINK_PAGE, &pd, info))
                    {
                        width += 9;
                        sprintf(page_text, " (p. %d)", pd.i);
                        if (!DO_PRINT(page_text, (int) strlen(page_text)))
                            return (0);
                    }
                    break;

                case TOK_WORD:
                    skip_blanks = 0;
                    if (!DO_PRINT(pd.curr, size))
                        return (0);
                    break;

                case TOK_SPACE:
                    skip_blanks = 0;
                    if (!DO_PRINTN(sp, width))
                        return (0);
                    break;

                case TOK_DONE:
                case TOK_XONLINE:   /* skip */
                case TOK_XDOC:      /* ignore */
                    break;

                } /* switch */

                pd.curr += size;
                pd.len  -= size;
                col     += width;
            }
            while (pd.len > 0);

            get_info(PD_RELEASE_TOPIC, &pd, info);

            first_topic = 0;
        } /* while */

        first_section = 0;
    } /* while */

    if (!output(PD_FOOTING, &pd, info))
        return (0);

    return (1);
}

#undef DO_PRINT
#undef DO_PRINTN
