#include "help_source.h"

#include <cstdio>

namespace hc
{

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

}
