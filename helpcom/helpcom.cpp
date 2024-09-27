// SPDX-License-Identifier: GPL-3.0-only
//
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

        case CMD_XADOC:
            tok = token_types::TOK_XADOC;
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
        || (tok == token_types::TOK_XDOC && mode == token_modes::DOC)
        || (tok == token_types::TOK_XADOC && mode == token_modes::ADOC))
    {
        size = 0;

        while (true)
        {
            curr  += t;
            len   -= t;
            size += t;

            tok = find_token_length(curr, len, &t, nullptr);

            if ((tok == token_types::TOK_XONLINE && mode == token_modes::ONLINE) ||
                (tok == token_types::TOK_XDOC && mode == token_modes::DOC) ||
                (tok == token_types::TOK_XADOC && mode == token_modes::ADOC) ||
                tok == token_types::TOK_DONE)
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

namespace
{

class DocumentProcessor
{
public:
    DocumentProcessor(token_modes mode, PD_FUNC *get_info, PD_FUNC *output, void *info) :
        m_token_mode(mode),
        m_get_info(get_info),
        m_output(output),
        m_info(info)
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
    bool topic_space();
    bool topic_token();
    bool heading()
    {
        return m_output(PD_COMMANDS::PD_HEADING, &m_pd, m_info);
    }
    bool get_content()
    {
        return m_get_info(PD_COMMANDS::PD_GET_CONTENT, &m_pd, m_info);
    }
    bool start_section()
    {
        return m_output(PD_COMMANDS::PD_START_SECTION, &m_pd, m_info);
    }
    bool footing()
    {
        return m_output(PD_COMMANDS::PD_FOOTING, &m_pd, m_info);
    }
    bool get_link_page()
    {
        return m_get_info(PD_COMMANDS::PD_GET_LINK_PAGE, &m_pd, m_info);
    }
    bool set_section_page()
    {
        return m_output(PD_COMMANDS::PD_SET_SECTION_PAGE, &m_pd, m_info);
    }
    bool print_section()
    {
        return m_output(PD_COMMANDS::PD_PRINT_SEC, &m_pd, m_info);
    }
    bool get_topic()
    {
        return m_get_info(PD_COMMANDS::PD_GET_TOPIC, &m_pd, m_info);
    }
    bool start_topic()
    {
        return m_output(PD_COMMANDS::PD_START_TOPIC, &m_pd, m_info);
    }
    bool set_topic_page()
    {
        return m_output(PD_COMMANDS::PD_SET_TOPIC_PAGE, &m_pd, m_info);
    }
    bool periodic()
    {
        return m_output(PD_COMMANDS::PD_PERIODIC, &m_pd, m_info);
    }
    bool release_topic()
    {
        return m_get_info(PD_COMMANDS::PD_RELEASE_TOPIC, &m_pd, m_info);
    }
    bool print(char const *str, int n)
    {
        m_pd.s = str;
        m_pd.i = n;
        return m_output(PD_COMMANDS::PD_PRINT, &m_pd, m_info);
    }
    bool print_n(char ch, int n)
    {
        m_pd.s = &ch;
        m_pd.i = n;
        return m_output(PD_COMMANDS::PD_PRINTN, &m_pd, m_info);
    }

    token_modes m_token_mode{};
    PD_FUNC *m_get_info;
    PD_FUNC *m_output;
    void *m_info;
    PD_INFO m_pd{};
    std::string m_page_text;
    bool m_first_section{};
    bool m_first_topic{};
    bool m_skip_blanks{};
    int m_col{};
    int m_size{};
    int m_width{};
};

bool DocumentProcessor::process()
{
    m_pd.page_num = 1;

    heading();

    m_first_section = true;
    while (get_content())
    {
        if (!content())
        {
            return false;
        }
        m_first_section = false;
    }

    return footing();
}

bool DocumentProcessor::content()
{
    if (!start_section())
    {
        return false;
    }

    if (m_pd.new_page && m_pd.line_num != 0)
    {
        if (!footing())
        {
            return false;
        }
        ++m_pd.page_num;
        m_pd.line_num = 0;
        if (!heading())
        {
            return false;
        }
    }
    else
    {
        if (m_pd.line_num + 2 > PAGE_DEPTH - CONTENT_BREAK)
        {
            if (!footing())
            {
                return false;
            }
            ++m_pd.page_num;
            m_pd.line_num = 0;
            if (!heading())
            {
                return false;
            }
        }
        else if (m_pd.line_num > 0)
        {
            if (!print_n('\n', 2))
            {
                return false;
            }
            m_pd.line_num += 2;
        }
    }

    if (!set_section_page())
    {
        return false;
    }

    if (!m_first_section)
    {
        if (!print_section())
        {
            return false;
        }
        ++m_pd.line_num;
    }

    m_first_topic = true;
    while (get_topic())
    {
        if (!topic())
        {
            return false;
        }
        m_first_topic = false;
    }

    return true;
}

bool DocumentProcessor::topic()
{
    if (!start_topic())
    {
        return false;
    }

    if (!m_first_section) /* do not skip blanks for DocContents */
    {
        while (m_pd.len > 0)
        {
            int size = 0;
            token_types tok = find_token_length(m_token_mode, m_pd.curr, m_pd.len, &size, nullptr);
            if (tok != token_types::TOK_XDOC && tok != token_types::TOK_XONLINE &&
                tok != token_types::TOK_NL && tok != token_types::TOK_DONE)
            {
                break;
            }
            m_pd.curr += size;
            m_pd.len -= size;
        }
        if (m_first_topic && m_pd.len != 0)
        {
            if (!print_n('\n', 1))
            {
                return false;
            }
            ++m_pd.line_num;
        }
    }

    if (m_pd.line_num > PAGE_DEPTH - TOPIC_BREAK)
    {
        if (!footing())
        {
            return false;
        }
        ++m_pd.page_num;
        m_pd.line_num = 0;
        if (!heading())
        {
            return false;
        }
    }
    else if (!m_first_topic)
    {
        if (!print_n('\n', 1))
        {
            return false;
        }
        ++m_pd.line_num;
    }

    if (!set_topic_page())
    {
        return false;
    }

    m_skip_blanks = false;
    m_col = 0;
    do
    {
        if (!topic_token())
        {
            return false;
        }
    } while (m_pd.len > 0);

    release_topic();

    return true;
}

bool DocumentProcessor::topic_paragraph()
{
    unsigned int holdlen = 0;
    char const *holdcurr = nullptr;
    int in_link = 0;

    ++m_pd.curr;

    int const indent = *m_pd.curr++;
    int const margin = *m_pd.curr++;

    m_pd.len -= 3;

    if (!print_n(' ', indent))
    {
        return false;
    }

    m_col = indent;

    while (true)
    {
        if (!periodic())
        {
            return false;
        }

        token_types tok = find_token_length(m_token_mode, m_pd.curr, m_pd.len, &m_size, &m_width);

        if (tok == token_types::TOK_NL || tok == token_types::TOK_FF)
        {
            break;
        }

        if (tok == token_types::TOK_DONE)
        {
            if (in_link == 0)
            {
                m_col = 0;
                ++m_pd.line_num;
                if (!print_n('\n', 1))
                {
                    return false;
                }
                break;
            }

            if (!m_page_text.empty())
            {
                if (in_link == 1)
                {
                    tok = token_types::TOK_SPACE;
                    m_width = 1;
                    m_size = 0;
                    ++in_link;
                }
                else if (in_link == 2)
                {
                    tok = token_types::TOK_WORD;
                    m_width = (int) m_page_text.length();
                    m_col += 8 - m_width;
                    m_size = 0;
                    m_pd.curr = m_page_text.c_str();
                    ++in_link;
                }
                else if (in_link == 3)
                {
                    m_pd.curr = holdcurr;
                    m_pd.len = holdlen;
                    in_link = 0;
                    continue;
                }
            }
            else
            {
                m_pd.curr = holdcurr;
                m_pd.len = holdlen;
                in_link = 0;
                continue;
            }
        }

        if (tok == token_types::TOK_PARA)
        {
            m_col = 0; /* fake a nl */
            ++m_pd.line_num;
            if (!print_n('\n', 1))
            {
                return false;
            }
            break;
        }

        if (tok == token_types::TOK_XONLINE || tok == token_types::TOK_XDOC)
        {
            m_pd.curr += m_size;
            m_pd.len -= m_size;
            continue;
        }

        if (tok == token_types::TOK_LINK)
        {
            m_pd.s = m_pd.curr + 1;
            m_pd.i = m_size;
            if (get_link_page())
            {
                in_link = 1;
                m_page_text = m_pd.link_page;
            }
            else
            {
                in_link = 3;
            }
            holdcurr = m_pd.curr + m_size;
            holdlen = m_pd.len - m_size;
            m_pd.len = m_size - 2 - 3 * sizeof(int);
            m_pd.curr += 1 + 3 * sizeof(int);
            continue;
        }

        // now tok is SPACE or WORD

        if (m_col + m_width > PAGE_WIDTH)
        {
            /* go to next line... */
            if (!print_n('\n', 1))
            {
                return false;
            }
            if (++m_pd.line_num >= PAGE_DEPTH)
            {
                if (!footing())
                {
                    return false;
                }
                ++m_pd.page_num;
                m_pd.line_num = 0;
                if (!heading())
                {
                    return false;
                }
            }

            if (tok == token_types::TOK_SPACE)
            {
                m_width = 0; /* skip spaces at start of a line */
            }

            if (!print_n(' ', margin))
            {
                return false;
            }
            m_col = margin;
        }

        if (m_width > 0)
        {
            if (tok == token_types::TOK_SPACE)
            {
                if (!print_n(' ', m_width))
                {
                    return false;
                }
            }
            else
            {
                if (!print(m_pd.curr, (m_size == 0) ? m_width : m_size))
                {
                    return false;
                }
            }
        }

        m_col += m_width;
        m_pd.curr += m_size;
        m_pd.len -= m_size;
    }

    m_skip_blanks = false;
    m_size = 0;
    m_width = m_size;

    return true;
}

bool DocumentProcessor::topic_newline()
{
    ++m_pd.line_num;

    if (m_pd.line_num >= PAGE_DEPTH || (m_col == 0 && m_pd.line_num >= PAGE_DEPTH - BLANK_BREAK))
    {
        if (m_col != 0) /* if last wasn't a blank line... */
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
        ++m_pd.page_num;
        m_pd.line_num = 0;
        m_skip_blanks = true;
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

    m_col = 0;

    return true;
}

bool DocumentProcessor::topic_formfeed()
{
    if (!footing())
    {
        return false;
    }
    m_col = 0;
    m_pd.line_num = 0;
    ++m_pd.page_num;
    return heading();
}

bool DocumentProcessor::topic_center()
{
    m_width = (PAGE_WIDTH - find_line_width(m_token_mode, m_pd.curr, m_pd.len)) / 2;
    return print_n(' ', m_width);
}

bool DocumentProcessor::topic_link()
{
    m_pd.s = m_pd.curr + 1;
    m_pd.i = m_size;
    if (get_link_page())
    {
        m_skip_blanks = false;
        if (!print(m_pd.curr + 1 + 3 * sizeof(int), m_size - 3 * sizeof(int) - 2))
        {
            return false;
        }

        m_width += static_cast<int>(m_pd.link_page.size());
        if (!print(m_page_text.c_str(), (int) m_page_text.size()))
        {
            return false;
        }
    }

    return true;
}

bool DocumentProcessor::topic_word()
{
    m_skip_blanks = false;
    return print(m_pd.curr, m_size);
}

bool DocumentProcessor::topic_space()
{
    m_skip_blanks = false;
    return print_n(' ', m_width);
}

bool DocumentProcessor::topic_token()
{
    if (!periodic())
    {
        return false;
    }

    m_size = 0;
    m_width = 0;
    token_types tok = find_token_length(m_token_mode, m_pd.curr, m_pd.len, &m_size, &m_width);
    switch (tok)
    {
    case token_types::TOK_PARA:
        if (!topic_paragraph())
        {
            return false;
        }
        break;

    case token_types::TOK_NL:
        if (m_skip_blanks && m_col == 0)
        {
            break;
        }

        if (!topic_newline())
        {
            return false;
        }
        break;

    case token_types::TOK_FF:
        if (m_skip_blanks)
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
        if (!topic_space())
        {
            return false;
        }
        break;

    case token_types::TOK_DONE:
    case token_types::TOK_XONLINE: /* skip */
    case token_types::TOK_XDOC:    /* ignore */
        break;
    } /* switch */

    m_pd.curr += m_size;
    m_pd.len -= m_size;
    m_col += m_width;

    return true;
}

} // namespace

bool process_document(token_modes mode, PD_FUNC *get_info, PD_FUNC *output, void *info)
{
    return DocumentProcessor(mode, get_info, output, info).process();
}
