// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <functional>
#include <string>
#include <ctime>

enum
{
    MAX_COMMENT_LEN = 57 // length of par comments
};

extern char g_par_comment[4][MAX_COMMENT_LEN];
extern std::string g_command_comment[4];          // comments for command set
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
void parse_comments(char *value);
