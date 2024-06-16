#include "help_source.h"

#include <cstdio>
#include <stdexcept>
#include <system_error>

namespace hc
{

std::vector<CONTENT> g_contents;     // the table-of-contents
std::vector<LINK> g_all_links;       //
std::vector<TOPIC> g_topics;         //
std::vector<LABEL> g_labels;         //
std::vector<LABEL> g_private_labels; //
std::FILE *g_swap_file{};            //
long g_swap_pos{};                   //
std::vector<char> g_buffer;          // buffer to/from swap file
char *g_curr{};                      // current position in the buffer
int g_max_links{};                   // max. links on any page

void TOPIC::alloc_topic_text(unsigned size)
{
    text_len = size;
    text = g_swap_pos;
    g_swap_pos += size;
    std::fseek(g_swap_file, text, SEEK_SET);
    std::fwrite(&g_buffer[0], 1, text_len, g_swap_file);
}

int TOPIC::add_page(const PAGE &p)
{
    page.push_back(p);
    return num_page++;
}

void TOPIC::add_page_break(int margin, char const *text, char const *start, char const *curr, int num_links)
{
    PAGE p;
    p.offset = (unsigned)(start - text);
    p.length = (unsigned)(curr - start);
    p.margin = margin;
    add_page(p);

    if (g_max_links < num_links)
    {
        g_max_links = num_links;
    }
}

char *TOPIC::get_topic_text()
{
    read_topic_text();
    return g_buffer.data();
}

const char *TOPIC::get_topic_text() const
{
    read_topic_text();
    return g_buffer.data();
}

void TOPIC::release_topic_text(bool save) const
{
    if (save)
    {
        std::fseek(g_swap_file, text, SEEK_SET);
        std::fwrite(&g_buffer[0], 1, text_len, g_swap_file);
    }
}

void TOPIC::start(char const *text, int len)
{
    flags = 0;
    title_len = len;
    title.assign(text, len);
    doc_page = -1;
    num_page = 0;
    g_curr = &g_buffer[0];
}

void TOPIC::read_topic_text() const
{
    std::fseek(g_swap_file, text, SEEK_SET);
    if (std::fread(&g_buffer[0], 1, text_len, g_swap_file) != text_len)
    {
        throw std::system_error(errno, std::system_category(), "get_topic_text failed fread");
    }
}

LABEL *find_label(char const *name)
{
    if (*name == '@')
    {
        for (LABEL &pl : g_private_labels)
        {
            if (name == pl.name)
            {
                return &pl;
            }
        }
    }
    else
    {
        for (LABEL &l : g_labels)
        {
            if (name == l.name)
            {
                return &l;
            }
        }
    }

    return nullptr;
}

int find_topic_title(char const *title)
{
    while (*title == ' ')
    {
        ++title;
    }

    int len = (int) std::strlen(title) - 1;
    while (title[len] == ' ' && len > 0)
    {
        --len;
    }

    ++len;

    if (len > 2 && title[0] == '\"' && title[len-1] == '\"')
    {
        ++title;
        len -= 2;
    }

    for (int t = 0; t < static_cast<int>(g_topics.size()); t++)
    {
        if ((int) g_topics[t].title.length() == len
            && strnicmp(title, g_topics[t].title.c_str(), len) == 0)
        {
            return t;
        }
    }

    return -1;   // not found
}

}
