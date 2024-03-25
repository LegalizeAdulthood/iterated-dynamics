#pragma once

#include <string>
enum
{
    MAX_COMMENT_LEN = 57 // length of par comments
};

extern char g_par_comment[4][MAX_COMMENT_LEN];
extern std::string g_command_comment[4];          // comments for command set

inline void clear_command_comments()
{
    for (std::string &comment : g_command_comment)
    {
        comment.clear();
    }
}

std::string expand_command_comment(int i);
void init_comments();
void parse_comments(char *value);
