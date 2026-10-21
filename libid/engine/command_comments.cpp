// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/command_comments.h"

#include <algorithm>

namespace id::engine
{

std::array<std::string, 4> g_par_comment;
std::string g_command_comment[4];

void clear_command_comments()
{
    for (std::string &comment : g_command_comment)
    {
        comment.clear();
    }
}

void init_comments()
{
    for (std::string &comment : g_par_comment)
    {
        comment.clear();
    }
}

void parse_comments(std::string_view value)
{
    for (std::string &comment : g_par_comment)
    {
        if (value.empty())
        {
            break;
        }
        const std::size_t next{value.find('/')};
        if (value.front() != '/')
        {
            comment = value.substr(0, std::min(next, static_cast<std::size_t>(MAX_COMMENT_LEN - 1)));
        }
        if (next == std::string_view::npos)
        {
            break;
        }
        value.remove_prefix(next + 1);
    }
}

} // namespace id::engine
