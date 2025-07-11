// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/lsys.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/lsys_fns.h"
#include "io/file_gets.h"
#include "math/fixed_pt.h"
#include "misc/Driver.h"
#include "ui/file_item.h"
#include "ui/stop_msg.h"

#include <config/string_lower.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <vector>

static bool read_lsystem_file(const char *str);
static void free_rules_mem();
static int rule_present(char symbol);
static bool save_axiom(const char *text);
static bool save_rule(const char *rule, int index);
static bool append_rule(const char *rule, int index);
static void free_l_cmds();

static std::string s_axiom;
static std::vector<std::string> s_rules;
static std::vector<LSysCmd *> s_rule_f_cmds;
static bool s_loaded{};

char g_max_angle{};

LDouble get_number(const char **str)
{
    bool root = false;
    bool inverse = false;

    (*str)++;
    switch (**str)
    {
    case 'q':
        root = true;
        (*str)++;
        break;
    case 'i':
        inverse = true;
        (*str)++;
        break;
    }
    switch (**str)
    {
    case 'q':
        root = true;
        (*str)++;
        break;
    case 'i':
        inverse = true;
        (*str)++;
        break;
    }
    char num_str[30];
    std::strcpy(num_str, "");
    int i = 0;
    while ((**str <= '9' && **str >= '0') || **str == '.')
    {
        num_str[i++] = **str;
        (*str)++;
    }
    (*str)--;
    num_str[i] = 0;
    LDouble ret = std::atof(num_str);
    if (ret <= 0.0)   // this is a sanity check
    {
        return 0;
    }
    if (root)
    {
        ret = std::sqrt(ret);
    }
    if (inverse)
    {
        ret = 1.0/ret;
    }
    return ret;
}

static bool read_lsystem_file(const char *str)
{
    int err = 0;
    char inline1[MAX_LSYS_LINE_LEN+1];
    std::FILE *infile;

    if (find_file_item(g_l_system_filename, str, &infile, ItemType::L_SYSTEM))
    {
        return true;
    }
    {
        int c;
        while ((c = std::fgetc(infile)) != '{')
        {
            if (c == EOF)
            {
                return true;
            }
        }
    }

    g_max_angle = 0;
    int rul_ind = 0;
    char msg_buff[481]{}; // enough for 6 full lines

    int line_num = 0;
    while (file_gets(inline1, MAX_LSYS_LINE_LEN, infile) > -1)  // Max line length chars
    {
        line_num++;
        char *word = std::strchr(inline1, ';');
        if (word != nullptr)   // strip comment
        {
            *word = 0;
        }
        string_lower(inline1);

        if ((int)std::strspn(inline1, " \t\n") < (int)std::strlen(inline1)) // not a blank line
        {
            bool check = false;
            word = std::strtok(inline1, " =\t\n");
            if (!std::strcmp(word, "axiom"))
            {
                if (save_axiom(std::strtok(nullptr, " \t\n")))
                {
                    std::strcat(msg_buff, "Error:  out of memory\n");
                    ++err;
                    break;
                }
                check = true;
            }
            else if (!std::strcmp(word, "angle"))
            {
                g_max_angle = (char)std::atoi(std::strtok(nullptr, " \t\n"));
                check = true;
            }
            else if (!std::strcmp(word, "}"))
            {
                break;
            }
            else if (!word[1])
            {
                bool mem_err = false;

                if (std::strchr("+-/\\@|!c<>][", *word))
                {
                    std::sprintf(&msg_buff[std::strlen(msg_buff)],
                        "Syntax error line %d: Redefined reserved symbol %s\n", line_num, word);
                    ++err;
                    break;
                }
                const char *temp = std::strtok(nullptr, " =\t\n");
                const int index = rule_present(*word);
                if (!index)
                {
                    char fixed[MAX_LSYS_LINE_LEN + 1];
                    std::strcpy(fixed, word);
                    if (temp)
                    {
                        std::strcat(fixed, temp);
                    }
                    mem_err = save_rule(fixed, rul_ind++);
                }
                else if (temp)
                {
                    mem_err = append_rule(temp, index);
                }
                if (mem_err)
                {
                    std::strcat(msg_buff, "Error:  out of memory\n");
                    ++err;
                    break;
                }
                check = true;
            }
            else if (err < 6)
            {
                std::sprintf(&msg_buff[std::strlen(msg_buff)],
                        "Syntax error line %d: %s\n", line_num, word);
                ++err;
            }
            if (check)
            {
                word = std::strtok(nullptr, " \t\n");
                if (word != nullptr)
                {
                    if (err < 6)
                    {
                        std::sprintf(&msg_buff[std::strlen(msg_buff)],
                                "Extra text after command line %d: %s\n", line_num, word);
                        ++err;
                    }
                }
            }
        }
    }
    std::fclose(infile);
    if (s_axiom.empty() && err < 6)
    {
        std::strcat(msg_buff, "Error:  no axiom\n");
        ++err;
    }
    if ((g_max_angle < 3||g_max_angle > 50) && err < 6)
    {
        std::strcat(msg_buff, "Error:  illegal or missing angle\n");
        ++err;
    }
    if (err)
    {
        msg_buff[std::strlen(msg_buff)-1] = 0; // strip trailing \n
        stop_msg(msg_buff);
        return true;
    }
    return false;
}

int lsystem_type()
{
    if (!s_loaded && lsystem_load())
    {
        return -1;
    }

    int order = (int) g_params[0];
    order = std::max(order, 0);

    LSysTurtleState ts;

    ts.stack_overflow = false;
    ts.max_angle = g_max_angle;
    ts.d_max_angle = (char) (g_max_angle - 1);

    s_rule_f_cmds.push_back(lsysf_size_transform(s_axiom.c_str(), &ts));
    for (const auto &rule : s_rules)
    {
        s_rule_f_cmds.push_back(lsysf_size_transform(rule.c_str(), &ts));
    }
    s_rule_f_cmds.push_back(nullptr);

    lsysf_do_sin_cos();
    if (lsysf_find_scale(s_rule_f_cmds[0], &ts, &s_rule_f_cmds[1], order))
    {
        ts.reverse = 0;
        ts.angle = 0;
        ts.real_angle = 0;

        free_l_cmds();
        s_rule_f_cmds.push_back(lsysf_draw_transform(s_axiom.c_str(), &ts));
        for (const auto &rule : s_rules)
        {
            s_rule_f_cmds.push_back(lsysf_draw_transform(rule.c_str(), &ts));
        }
        s_rule_f_cmds.push_back(nullptr);

        // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
        ts.curr_color = 15;
        if (ts.curr_color > g_colors)
        {
            ts.curr_color = (char) (g_colors - 1);
        }
        draw_lsysf(s_rule_f_cmds[0], &ts, &s_rule_f_cmds[1], order);
    }
    g_overflow = false;
    free_rules_mem();
    free_l_cmds();
    s_loaded = false;
    return 0;
}

bool lsystem_load()
{
    if (read_lsystem_file(g_l_system_name.c_str()))
    {
        // error occurred
        free_rules_mem();
        s_loaded = false;
        return true;
    }
    s_loaded = true;
    return false;
}

static void free_rules_mem()
{
    for (auto &rule : s_rules)
    {
        rule.clear();
        rule.shrink_to_fit();
    }
    s_rules.clear();
    s_rules.shrink_to_fit();
}

static int rule_present(char symbol)
{
    for (std::size_t i = 1; i < s_rules.size(); ++i)
    {
        if (s_rules[i][0] == symbol)
        {
            return static_cast<int>(i);
        }
    }
    return 0;
}

static bool save_axiom(const char *text)
{
    try
    {
        s_axiom = text;
        return false;
    }
    catch (const std::bad_alloc &)
    {
        return true;
    }
}

static bool save_rule(const char *rule, int index)
{
    try
    {
        assert(index >= 0);
        s_rules.resize(index + 1);
        s_rules[index] = rule;
        return false;
    }
    catch (const std::bad_alloc &)
    {
        return true;
    }
}

static bool append_rule(const char *rule, int index)
{
    try
    {
        assert(index > 0 && unsigned(index) < s_rules.size());
        s_rules[index] += rule;
        return false;
    }
    catch (const std::bad_alloc &)
    {
        return true;
    }
}

static void free_l_cmds()
{
    for (auto cmd : s_rule_f_cmds)
    {
        if (cmd != nullptr)
        {
            free(cmd);
        }
    }
    s_rule_f_cmds.clear();
}
