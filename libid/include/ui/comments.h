// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <array>
#include <ctime>
#include <functional>
#include <string>
#include <string_view>

namespace id::ui
{

enum
{
    MAX_COMMENT_LEN = 57 // length of par comments
};

extern std::array<std::string, 4> g_par_comment;
extern std::string g_command_comment[4]; // comments for command set
extern std::function<std::string()> g_get_cpu_id;

inline void clear_command_comments()
{
    for (std::string &comment : g_command_comment)
    {
        comment.clear();
    }
}

const std::string &expand_command_comment(int i, std::time_t local_time);
const std::string &expand_command_comment(int i);
void init_comments();
void parse_comments(std::string_view value);

} // namespace id::ui
