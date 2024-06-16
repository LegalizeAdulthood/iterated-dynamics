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

}
