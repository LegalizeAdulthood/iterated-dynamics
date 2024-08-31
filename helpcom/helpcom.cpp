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

namespace {

class DocumentProcessor
{
public:
    DocumentProcessor(PD_FUNC *get_info_, PD_FUNC *output_, void *info_) :
        get_info(get_info_),
        output(output_),
        info(info_)
    {
    }

    bool process();

private:
    bool content();
    bool topic();
    bool topic_paragraph();
    bool topic_newline();
    bool topic_formfeed();
    bool topic_center();
    bool topic_link();
    bool topic_word();
    bool topic_token();
    bool heading()
    {
        return output(PD_COMMANDS::PD_HEADING, &pd, info);
    }
    bool get_content()
    {
        return get_info(PD_COMMANDS::PD_GET_CONTENT, &pd, info);
    }
    bool start_section()
    {
        return output(PD_COMMANDS::PD_START_SECTION, &pd, info);
    }
    bool footing()
    {
        return output(PD_COMMANDS::PD_FOOTING, &pd, info);
    }
    bool get_link_page()
    {
        return get_info(PD_COMMANDS::PD_GET_LINK_PAGE, &pd, info);
    }
    bool set_section_page()
    {
        return output(PD_COMMANDS::PD_SET_SECTION_PAGE, &pd, info);
    }
    bool print_section()
    {
        return output(PD_COMMANDS::PD_PRINT_SEC, &pd, info);
    }
    bool get_topic()
    {
        return get_info(PD_COMMANDS::PD_GET_TOPIC, &pd, info);
    }
    bool start_topic()
    {
        return output(PD_COMMANDS::PD_START_TOPIC, &pd, info);
    }
    bool set_topic_page()
    {
        return output(PD_COMMANDS::PD_SET_TOPIC_PAGE, &pd, info);
    }
    bool periodic()
    {
        return output(PD_COMMANDS::PD_PERIODIC, &pd, info);
    }
    bool release_topic()
    {
        return get_info(PD_COMMANDS::PD_RELEASE_TOPIC, &pd, info);
    }
    bool print(char const *str, int n)
    {
        pd.s = str;
        pd.i = n;
        return output(PD_COMMANDS::PD_PRINT, &pd, info);
    }
    bool print_n(char ch, int n)
    {
        pd.s = &ch;
        pd.i = n;
        return output(PD_COMMANDS::PD_PRINTN, &pd, info);
    }

    PD_FUNC *get_info;
    PD_FUNC *output;
    void *info;
    PD_INFO pd{};
    std::string page_text;
    bool first_section{};
    bool first_topic{};
    bool skip_blanks{};
    int col{};
    int size{};
    int width{};
};

bool DocumentProcessor::process()
{
    pd.page_num = 1;

    heading();

    first_section = true;
    while (get_content())
    {
        if (!content())
        {
            return false;
        }
    }

    return footing();
}

bool DocumentProcessor::content()
{
    if (!start_section())
    {
        return false;
    }

    if (pd.new_page && pd.line_num != 0)
    {
        if (!footing())
        {
            return false;
        }
        ++pd.page_num;
        pd.line_num = 0;
        if (!heading())
        {
            return false;
        }
    }
    else
    {
        if (pd.line_num + 2 > PAGE_DEPTH - CONTENT_BREAK)
        {
            if (!footing())
            {
                return false;
            }
            ++pd.page_num;
            pd.line_num = 0;
            if (!heading())
            {
                return false;
            }
        }
        else if (pd.line_num > 0)
        {
            if (!print_n('\n', 2))
            {
                return false;
            }
            pd.line_num += 2;
        }
    }

    if (!set_section_page())
    {
        return false;
    }

    if (!first_section)
    {
        if (!print_section())
        {
            return false;
        }
        ++pd.line_num;
    }

    first_topic = true;
    while (get_topic())
    {
        if (!topic())
        {
            return false;
        }
    }

    first_section = false;
    return true;
}

bool DocumentProcessor::topic()
{
    if (!start_topic())
    {
        return false;
    }

    if (!first_section) /* do not skip blanks for DocContents */
    {
        while (pd.len > 0)
        {
            int size = 0;
            token_types tok = find_token_length(token_modes::DOC, pd.curr, pd.len, &size, nullptr);
            if (tok != token_types::TOK_XDOC && tok != token_types::TOK_XONLINE &&
                tok != token_types::TOK_NL && tok != token_types::TOK_DONE)
            {
                break;
            }
            pd.curr += size;
            pd.len -= size;
        }
        if (first_topic && pd.len != 0)
        {
            if (!print_n('\n', 1))
            {
                return false;
            }
            ++pd.line_num;
        }
    }

    if (pd.line_num > PAGE_DEPTH - TOPIC_BREAK)
    {
        if (!footing())
        {
            return false;
        }
        ++pd.page_num;
        pd.line_num = 0;
        if (!heading())
        {
            return false;
        }
    }
    else if (!first_topic)
    {
        if (!print_n('\n', 1))
        {
            return false;
        }
        ++pd.line_num;
    }

    if (!set_topic_page())
    {
        return false;
    }

    skip_blanks = false;
    col = 0;
    do
    {
        if (!topic_token())
        {
            return false;
        }
    } while (pd.len > 0);

    release_topic();

    first_topic = false;
    return true;
}

bool DocumentProcessor::topic_paragraph()
{
    unsigned int holdlen = 0;
    char const *holdcurr = nullptr;
    int in_link = 0;

    ++pd.curr;

    int const indent = *pd.curr++;
    int const margin = *pd.curr++;

    pd.len -= 3;

    if (!print_n(' ', indent))
    {
        return false;
    }

    col = indent;

    while (true)
    {
        if (!periodic())
        {
            return false;
        }

        token_types tok = find_token_length(token_modes::DOC, pd.curr, pd.len, &size, &width);

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
                if (!print_n('\n', 1))
                {
                    return false;
                }
                break;
            }

            if (!page_text.empty())
            {
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
                    width = (int) page_text.length();
                    col += 8 - width;
                    size = 0;
                    pd.curr = page_text.c_str();
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
            else
            {
                pd.curr = holdcurr;
                pd.len = holdlen;
                in_link = 0;
                continue;
            }
        }

        if (tok == token_types::TOK_PARA)
        {
            col = 0; /* fake a nl */
            ++pd.line_num;
            if (!print_n('\n', 1))
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
            pd.s = pd.curr + 1;
            pd.i = size;
            if (get_link_page())
            {
                in_link = 1;
                page_text = pd.link_page;
            }
            else
            {
                in_link = 3;
            }
            holdcurr = pd.curr + size;
            holdlen = pd.len - size;
            pd.len = size - 2 - 3 * sizeof(int);
            pd.curr += 1 + 3 * sizeof(int);
            continue;
        }

        // now tok is SPACE or WORD

        if (col + width > PAGE_WIDTH)
        {
            /* go to next line... */
            if (!print_n('\n', 1))
            {
                return false;
            }
            if (++pd.line_num >= PAGE_DEPTH)
            {
                if (!footing())
                {
                    return false;
                }
                ++pd.page_num;
                pd.line_num = 0;
                if (!heading())
                {
                    return false;
                }
            }

            if (tok == token_types::TOK_SPACE)
            {
                width = 0; /* skip spaces at start of a line */
            }

            if (!print_n(' ', margin))
            {
                return false;
            }
            col = margin;
        }

        if (width > 0)
        {
            if (tok == token_types::TOK_SPACE)
            {
                if (!print_n(' ', width))
                {
                    return false;
                }
            }
            else
            {
                if (!print(pd.curr, (size == 0) ? width : size))
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

    return true;
}

bool DocumentProcessor::topic_newline()
{
    ++pd.line_num;

    if (pd.line_num >= PAGE_DEPTH || (col == 0 && pd.line_num >= PAGE_DEPTH - BLANK_BREAK))
    {
        if (col != 0) /* if last wasn't a blank line... */
        {
            if (!print_n('\n', 1))
            {
                return false;
            }
        }
        if (!footing())
        {
            return false;
        }
        ++pd.page_num;
        pd.line_num = 0;
        skip_blanks = true;
        if (!heading())
        {
            return false;
        }
    }
    else
    {
        if (!print_n('\n', 1))
        {
            return false;
        }
    }

    col = 0;

    return true;
}

bool DocumentProcessor::topic_formfeed()
{
    if (!footing())
    {
        return false;
    }
    col = 0;
    pd.line_num = 0;
    ++pd.page_num;
    return heading();
}

bool DocumentProcessor::topic_center()
{
    width = (PAGE_WIDTH - find_line_width(token_modes::DOC, pd.curr, pd.len)) / 2;
    return print_n(' ', width);
}

bool DocumentProcessor::topic_link()
{
    pd.s = pd.curr + 1;
    pd.i = size;
    if (get_link_page())
    {
        skip_blanks = false;
        if (!print(pd.curr + 1 + 3 * sizeof(int), size - 3 * sizeof(int) - 2))
        {
            return false;
        }

        width += static_cast<int>(pd.link_page.size());
        if (!print(page_text.c_str(), (int) page_text.size()))
        {
            return false;
        }
    }
    return true;
}

bool DocumentProcessor::topic_word()
{
    skip_blanks = false;
    return print(pd.curr, size);
}

bool DocumentProcessor::topic_token()
{
    if (!periodic())
    {
        return false;
    }

    size = 0;
    width = 0;
    token_types tok = find_token_length(token_modes::DOC, pd.curr, pd.len, &size, &width);
    switch (tok)
    {
    case token_types::TOK_PARA:
        if (!topic_paragraph())
        {
            return false;
        }
        break;

    case token_types::TOK_NL:
        if (skip_blanks && col == 0)
        {
            break;
        }

        if (!topic_newline())
        {
            return false;
        }
        break;

    case token_types::TOK_FF:
        if (skip_blanks)
        {
            break;
        }

        if (!topic_formfeed())
        {
            return false;
        }
        break;

    case token_types::TOK_CENTER:
        if (!topic_center())
        {
            return false;
        }
        break;

    case token_types::TOK_LINK:
        if (!topic_link())
        {
            return false;
        }
        break;

    case token_types::TOK_WORD:
        if (!topic_word())
        {
            return false;
        }
        break;

    case token_types::TOK_SPACE:
        skip_blanks = false;
        if (!print_n(' ', width))
        {
            return false;
        }
        break;

    case token_types::TOK_DONE:
    case token_types::TOK_XONLINE: /* skip */
    case token_types::TOK_XDOC:    /* ignore */
        break;

    } /* switch */

    pd.curr += size;
    pd.len -= size;
    col += width;
    
    return true;
}

} // namespace

bool process_document(PD_FUNC *get_info, PD_FUNC *output, void *info)
{
    return DocumentProcessor(get_info, output, info).process();
}
