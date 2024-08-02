/*
 * Common help code
 *
 */
#include "port.h"
#include "helpcom.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>

bool is_hyphen(char const *ptr)   /* true if ptr points to a real hyphen */
{
    /* checkes for "--" and " -" */
    if (*ptr != '-')
    {
        return false;    /* that was easy! */
    }

    --ptr;

    return *ptr != ' ' && *ptr != '-';
}


static token_types find_token_length(char const *curr, unsigned len, int *ret_size, int *ret_width)
{
    int size{};
    int width{};
    token_types tok;

    if (len == 0)
    {
        tok = token_types::TOK_DONE;
    }
    else
    {
        switch (*curr)
        {
        case ' ':    /* it's a run of spaces */
            tok = token_types::TOK_SPACE;
            while (*curr == ' ' && size < (int)len)
            {
                ++curr;
                ++size;
                ++width;
            }
            break;

        case CMD_SPACE:
            tok = token_types::TOK_SPACE;
            ++curr;
            ++size;
            width = *curr;
            ++size;
            break;

        case CMD_LINK:
            tok = token_types::TOK_LINK;
            size += 1+3*sizeof(int); /* skip CMD_LINK + topic_num + topic_off + page_num */
            curr += 1+3*sizeof(int);

            while (*curr != CMD_LINK)
            {
                if (*curr == CMD_LITERAL)
                {
                    ++curr;
                    ++size;
                }
                ++curr;
                ++size;
                ++width;
                assert((unsigned) size < len);
            }

            ++size;   /* skip ending CMD_LINK */
            break;

        case CMD_PARA:
            tok = token_types::TOK_PARA;
            size += 3;     /* skip CMD_PARA + indent + margin */
            break;

        case CMD_XONLINE:
            tok = token_types::TOK_XONLINE;
            ++size;
            break;

        case CMD_XDOC:
            tok = token_types::TOK_XDOC;
            ++size;
            break;

        case CMD_CENTER:
            tok = token_types::TOK_CENTER;
            ++size;
            break;

        case '\n':
            tok = token_types::TOK_NL;
            ++size;
            break;

        case CMD_FF:
            tok = token_types::TOK_FF;
            ++size;
            break;

        default:   /* it must be a word */
            tok = token_types::TOK_WORD;
            while (true)
            {
                if (size >= (int)len)
                {
                    break;
                }
                if (*curr == CMD_LITERAL)
                {
                    curr += 2;
                    size += 2;
                    width += 1;
                }
                else if (*curr == '\0')
                {
                    assert(0);
                }
                else if ((unsigned)*curr <= MAX_CMD || *curr == ' ' || *curr == '\n')
                {
                    break;
                }
                else if (*curr == '-')
                {
                    ++curr;
                    ++size;
                    ++width;
                    if (is_hyphen(curr-1))
                    {
                        break;
                    }
                }
                else
                {
                    ++curr;
                    ++size;
                    ++width;
                }
            }
            break;
        } /* switch */
    }

    if (ret_size  != nullptr)
    {
        *ret_size  = size;
    }
    if (ret_width != nullptr)
    {
        *ret_width = width;
    }

    return tok;
}


token_types find_token_length(
    token_modes mode, char const *curr, unsigned int len, int *ret_size, int *ret_width)
{
    int t;
    token_types tok = find_token_length(curr, len, &t, ret_width);

    int size;
    if ((tok == token_types::TOK_XONLINE && mode == token_modes::ONLINE)
        || (tok == token_types::TOK_XDOC && mode == token_modes::DOC))
    {
        size = 0;

        while (true)
        {
            curr  += t;
            len   -= t;
            size += t;

            tok = find_token_length(curr, len, &t, nullptr);

            if ((tok == token_types::TOK_XONLINE && mode == token_modes::ONLINE)
                || (tok == token_types::TOK_XDOC && mode == token_modes::DOC)
                || (tok == token_types::TOK_DONE))
            {
                break;
            }
        }

        size += t;
    }
    else
    {
        size = t;
    }

    if (ret_size != nullptr)
    {
        *ret_size = size;
    }

    return tok;
}


int find_line_width(token_modes mode, char const *curr, unsigned len)
{
    int size   = 0;
    int width  = 0;
    int lwidth = 0;
    bool done = false;

    do
    {
        switch (find_token_length(mode, curr, len, &size, &width))
        {
        case token_types::TOK_DONE:
        case token_types::TOK_PARA:
        case token_types::TOK_NL:
        case token_types::TOK_FF:
            done = true;
            break;

        case token_types::TOK_XONLINE:
        case token_types::TOK_XDOC:
        case token_types::TOK_CENTER:
            curr += size;
            len -= size;
            break;

        default:   /* SPACE, LINK or WORD */
            lwidth += width;
            curr += size;
            len -= size;
            break;
        }
    }
    while (!done);

    return lwidth;
}


bool process_document(PD_FUNC *get_info, PD_FUNC *output, void *info)
{
    PD_INFO pd{};
    pd.page_num = 1;

    auto const do_print = [=, &pd](char const *str, int n)
    {
        pd.s = str;
        pd.i = n;
        return output(PD_COMMANDS::PD_PRINT, &pd, info);
    };
    auto const do_print_n = [=, &pd](char ch, int n)
    {
        pd.s = &ch;
        pd.i = n;
        return output(PD_COMMANDS::PD_PRINTN, &pd, info);
    };

    output(PD_COMMANDS::PD_HEADING, &pd, info);

    bool first_section = true;
    char page_text[30]{};
    while (get_info(PD_COMMANDS::PD_GET_CONTENT, &pd, info))
    {
        if (!output(PD_COMMANDS::PD_START_SECTION, &pd, info))
        {
            return false;
        }

        if (pd.new_page && pd.line_num != 0)
        {
            if (!output(PD_COMMANDS::PD_FOOTING, &pd, info))
            {
                return false;
            }
            ++pd.page_num;
            pd.line_num = 0;
            if (!output(PD_COMMANDS::PD_HEADING, &pd, info))
            {
                return false;
            }
        }
        else
        {
            if (pd.line_num + 2 > PAGE_DEPTH-CONTENT_BREAK)
            {
                if (!output(PD_COMMANDS::PD_FOOTING, &pd, info))
                {
                    return false;
                }
                ++pd.page_num;
                pd.line_num = 0;
                if (!output(PD_COMMANDS::PD_HEADING, &pd, info))
                {
                    return false;
                }
            }
            else if (pd.line_num > 0)
            {
                if (!do_print_n('\n', 2))
                {
                    return false;
                }
                pd.line_num += 2;
            }
        }

        if (!output(PD_COMMANDS::PD_SET_SECTION_PAGE, &pd, info))
        {
            return false;
        }

        if (!first_section)
        {
            if (!output(PD_COMMANDS::PD_PRINT_SEC, &pd, info))
            {
                return false;
            }
            ++pd.line_num;
        }

        bool first_topic = true;
        while (get_info(PD_COMMANDS::PD_GET_TOPIC, &pd, info))
        {
            if (!output(PD_COMMANDS::PD_START_TOPIC, &pd, info))
            {
                return false;
            }

            if (!first_section)     /* do not skip blanks for DocContents */
            {
                while (pd.len > 0)
                {
                    int size = 0;
                    token_types tok = find_token_length(token_modes::DOC, pd.curr, pd.len, &size, nullptr);
                    if (tok != token_types::TOK_XDOC
                        && tok != token_types::TOK_XONLINE
                        && tok != token_types::TOK_NL
                        && tok != token_types::TOK_DONE)
                    {
                        break;
                    }
                    pd.curr += size;
                    pd.len  -= size;
                }
                if (first_topic && pd.len != 0)
                {
                    if (!do_print_n('\n', 1))
                    {
                        return false;
                    }
                    ++pd.line_num;
                }
            }

            if (pd.line_num > PAGE_DEPTH-TOPIC_BREAK)
            {
                if (!output(PD_COMMANDS::PD_FOOTING, &pd, info))
                {
                    return false;
                }
                ++pd.page_num;
                pd.line_num = 0;
                if (!output(PD_COMMANDS::PD_HEADING, &pd, info))
                {
                    return false;
                }
            }
            else if (!first_topic)
            {
                if (!do_print_n('\n', 1))
                {
                    return false;
                }
                ++pd.line_num;
            }

            if (!output(PD_COMMANDS::PD_SET_TOPIC_PAGE, &pd, info))
            {
                return false;
            }

            bool skip_blanks = false;
            int col = 0;
            do
            {
                if (!output(PD_COMMANDS::PD_PERIODIC, &pd, info))
                {
                    return false;
                }

                int size = 0;
                int width = 0;
                token_types tok = find_token_length(token_modes::DOC, pd.curr, pd.len, &size, &width);
                switch (tok)
                {
                case token_types::TOK_PARA:
                {
                    unsigned int holdlen = 0;
                    char const *holdcurr = nullptr;
                    int in_link = 0;

                    ++pd.curr;

                    int const indent = *pd.curr++;
                    int const margin = *pd.curr++;

                    pd.len -= 3;

                    if (!do_print_n(' ', indent))
                    {
                        return false;
                    }

                    col = indent;

                    while (true)
                    {
                        if (!output(PD_COMMANDS::PD_PERIODIC, &pd, info))
                        {
                            return false;
                        }

                        tok = find_token_length(token_modes::DOC, pd.curr, pd.len, &size, &width);

                        if (tok == token_types::TOK_NL || tok == token_types::TOK_FF)
                        {
                            break;
                        }

                        if (tok == token_types::TOK_DONE)
                        {
                            if (in_link == 0)
                            {
                                col = 0;
                                ++pd.line_num;
                                if (!do_print_n('\n', 1))
                                {
                                    return false;
                                }
                                break;
                            }

                            if (in_link == 1)
                            {
                                tok = token_types::TOK_SPACE;
                                width = 1;
                                size = 0;
                                ++in_link;
                            }
                            else if (in_link == 2)
                            {
                                tok = token_types::TOK_WORD;
                                width = (int) std::strlen(page_text);
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

                        if (tok == token_types::TOK_PARA)
                        {
                            col = 0;   /* fake a nl */
                            ++pd.line_num;
                            if (!do_print_n('\n', 1))
                            {
                                return false;
                            }
                            break;
                        }

                        if (tok == token_types::TOK_XONLINE || tok == token_types::TOK_XDOC)
                        {
                            pd.curr += size;
                            pd.len -= size;
                            continue;
                        }

                        if (tok == token_types::TOK_LINK)
                        {
                            pd.s = pd.curr+1;
                            if (get_info(PD_COMMANDS::PD_GET_LINK_PAGE, &pd, info))
                            {
                                in_link = 1;
                                std::snprintf(page_text, std::size(page_text), "(p. %d)", pd.i);
                            }
                            else
                            {
                                in_link = 3;
                            }
                            holdcurr = pd.curr + size;
                            holdlen = pd.len - size;
                            pd.len = size - 2 - 3*sizeof(int);
                            pd.curr += 1 + 3*sizeof(int);
                            continue;
                        }

                        // now tok is SPACE or WORD

                        if (col+width > PAGE_WIDTH)
                        {
                            /* go to next line... */
                            if (!do_print_n('\n', 1))
                            {
                                return false;
                            }
                            if (++pd.line_num >= PAGE_DEPTH)
                            {
                                if (!output(PD_COMMANDS::PD_FOOTING, &pd, info))
                                {
                                    return false;
                                }
                                ++pd.page_num;
                                pd.line_num = 0;
                                if (!output(PD_COMMANDS::PD_HEADING, &pd, info))
                                {
                                    return false;
                                }
                            }

                            if (tok == token_types::TOK_SPACE)
                            {
                                width = 0;    /* skip spaces at start of a line */
                            }

                            if (!do_print_n(' ', margin))
                            {
                                return false;
                            }
                            col = margin;
                        }

                        if (width > 0)
                        {
                            if (tok == token_types::TOK_SPACE)
                            {
                                if (!do_print_n(' ', width))
                                {
                                    return false;
                                }
                            }
                            else
                            {
                                if (!do_print(pd.curr, (size == 0) ? width : size))
                                {
                                    return false;
                                }
                            }
                        }

                        col += width;
                        pd.curr += size;
                        pd.len -= size;
                    }

                    skip_blanks = false;
                    size = 0;
                    width = size;
                    break;
                }

                case token_types::TOK_NL:
                    if (skip_blanks && col == 0)
                    {
                        break;
                    }

                    ++pd.line_num;

                    if (pd.line_num >= PAGE_DEPTH || (col == 0 && pd.line_num >= PAGE_DEPTH-BLANK_BREAK))
                    {
                        if (col != 0)      /* if last wasn't a blank line... */
                        {
                            if (!do_print_n('\n', 1))
                            {
                                return false;
                            }
                        }
                        if (!output(PD_COMMANDS::PD_FOOTING, &pd, info))
                        {
                            return false;
                        }
                        ++pd.page_num;
                        pd.line_num = 0;
                        skip_blanks = true;
                        if (!output(PD_COMMANDS::PD_HEADING, &pd, info))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!do_print_n('\n', 1))
                        {
                            return false;
                        }
                    }

                    col = 0;
                    break;

                case token_types::TOK_FF:
                    if (skip_blanks)
                    {
                        break;
                    }
                    if (!output(PD_COMMANDS::PD_FOOTING, &pd, info))
                    {
                        return false;
                    }
                    col = 0;
                    pd.line_num = 0;
                    ++pd.page_num;
                    if (!output(PD_COMMANDS::PD_HEADING, &pd, info))
                    {
                        return false;
                    }
                    break;

                case token_types::TOK_CENTER:
                    width = (PAGE_WIDTH - find_line_width(token_modes::DOC, pd.curr, pd.len)) / 2;
                    if (!do_print_n(' ', width))
                    {
                        return false;
                    }
                    break;

                case token_types::TOK_LINK:
                    skip_blanks = false;
                    if (!do_print(pd.curr+1+3*sizeof(int), size-3*sizeof(int)-2))
                    {
                        return false;
                    }
                    pd.s = pd.curr+1;
                    if (get_info(PD_COMMANDS::PD_GET_LINK_PAGE, &pd, info))
                    {
                        width += 9;
                        std::snprintf(page_text, std::size(page_text), " (p. %d)", pd.i);
                        if (!do_print(page_text, (int) std::strlen(page_text)))
                        {
                            return false;
                        }
                    }
                    break;

                case token_types::TOK_WORD:
                    skip_blanks = false;
                    if (!do_print(pd.curr, size))
                    {
                        return false;
                    }
                    break;

                case token_types::TOK_SPACE:
                    skip_blanks = false;
                    if (!do_print_n(' ', width))
                    {
                        return false;
                    }
                    break;

                case token_types::TOK_DONE:
                case token_types::TOK_XONLINE:   /* skip */
                case token_types::TOK_XDOC:      /* ignore */
                    break;

                } /* switch */

                pd.curr += size;
                pd.len  -= size;
                col     += width;
            }
            while (pd.len > 0);

            get_info(PD_COMMANDS::PD_RELEASE_TOPIC, &pd, info);

            first_topic = false;
        } /* while */

        first_section = false;
    } /* while */

    return output(PD_COMMANDS::PD_FOOTING, &pd, info);
}
