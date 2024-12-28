// SPDX-License-Identifier: GPL-3.0-only
//
#include "HTMLProcessor.h"

#include "HelpSource.h"
#include "messages.h"

#include <helpcom.h>

#include <fstream>
#include <iostream>
#include <stdexcept>

namespace hc
{

std::string g_html_output_dir{"."};

std::string rst_name(std::string const &content_name)
{
    std::string name;
    name.reserve(content_name.length());
    bool underscore = true;     // can't start with an underscore
    for (unsigned char c : content_name)
    {
        if (std::isalnum(c) != 0)
        {
            name += static_cast<char>(std::tolower(c));
            underscore = false;
        }
        else if (!underscore)
        {
            name += '_';
            underscore = true;
        }
    }
    auto pos = name.find_last_not_of('_');
    if (pos != std::string::npos)
    {
        name.erase(pos + 1);
    }
    return name;
}

void HTMLProcessor::process()
{
    if (g_src.contents.empty())
    {
        throw std::runtime_error(".SRC has no DocContents.");
    }

    write_index_html();
    write_contents();
}

void HTMLProcessor::write_index_html()
{
    msg(("Writing " + m_fname).c_str());

    const Content &toc = g_src.contents[0];
    if (toc.num_topic != 1)
    {
        throw std::runtime_error("First content block contains multiple topics.");
    }
    if (toc.topic_name[0] != DOCCONTENTS_TITLE)
    {
        throw std::runtime_error("First content block doesn't contain DocContent.");
    }

    const Topic &toc_topic = g_src.topics[toc.topic_num[0]];
    std::ofstream str(g_html_output_dir + "/index.rst");
    str << ".. toctree::\n";
    char const *text = toc_topic.get_topic_text();
    char const *curr = text;
    unsigned int len = toc_topic.text_len;
    while (len > 0)
    {
        int size = 0;
        int width = 0;
        TokenType const tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

        switch (tok)
        {
        case TokenType::TOK_SPACE:
            break;

        case TokenType::TOK_NL:
            str << '\n';
            break;

        case TokenType::TOK_WORD:
            str << "   " << std::string(curr, width);
            break;

        default:
            throw std::runtime_error("Unexpected token in table of contents.");
        }
        len -= size;
        curr += size;
    }
}

void HTMLProcessor::write_contents()
{
    for (const Content &c : g_src.contents)
    {
        write_content(c);
    }
}

void HTMLProcessor::write_content(const Content &c)
{
    for (int i = 0; i < c.num_topic; ++i)
    {
        const Topic &t = g_src.topics[c.topic_num[i]];
        if (t.title == DOCCONTENTS_TITLE)
        {
            continue;
        }

        write_topic(t);
    }
}

void HTMLProcessor::write_topic(const Topic &t)
{
    std::string const filename = rst_name(t.title) + ".rst";
    msg("Writing %s", filename.c_str());
    std::ofstream str(g_html_output_dir + '/' + filename);
    char const *text = t.get_topic_text();
    char const *curr = text;
    unsigned int len = t.text_len;
    unsigned int column = 0;
    std::string spaces;
    auto const nl = [&str, &column](int width)
    {
        if (column + width > 70)
        {
            str << '\n';
            column = 0;
            return true;
        }
        return false;
    };
    while (len > 0)
    {
        int size = 0;
        int width = 0;
        TokenType const tok = find_token_length(token_modes::ONLINE, curr, len, &size, &width);

        switch (tok)
        {
        case TokenType::TOK_SPACE:
            if (!nl(width))
            {
                spaces = std::string(width, ' ');
                column += width;
            }
            else
            {
                spaces.clear();
            }
            break;

        case TokenType::TOK_NL:
            str << '\n';
            spaces.clear();
            column = 0;
            break;

        case TokenType::TOK_WORD:
            if (!nl(width) && !spaces.empty())
            {
                str << spaces;
                spaces.clear();
            }
            str << std::string(curr, width);
            column += width;
            break;

        case TokenType::TOK_PARA:
            if (column > 0)
            {
                str << '\n';
            }
            column = 0;
            spaces.clear();
            break;

        case TokenType::TOK_LINK:
            {
                char const *data = &curr[1];
                int const link_num = get_int(data);
                int const link_topic_num = g_src.all_links[link_num].topic_num;
                data += 3*sizeof(int);
                std::string const link_text(":doc:`" + std::string(data, width) +
                    " <" + rst_name(g_src.topics[link_topic_num].title) + ">`");
                if (!nl(static_cast<int>(link_text.length())) && !spaces.empty())
                {
                    str << spaces;
                    spaces.clear();
                }
                str << link_text;
                column += static_cast<unsigned int>(link_text.length());
            }
            break;

        case TokenType::TOK_FF:
        case TokenType::TOK_XONLINE:
        case TokenType::TOK_XDOC:
            break;

        default:
            throw std::runtime_error("Unexpected token in topic.");
        }
        len -= size;
        curr += size;
    }
}

} // namespace hc
