// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/lsys.h"

#include "engine/id_data.h"
#include "fractals/lsys_fns.h"
#include "io/file_gets.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"
#include "misc/Driver.h"
#include "misc/stack_avail.h"
#include "ui/cmdfiles.h"
#include "ui/file_item.h"
#include "ui/stop_msg.h"
#include "ui/thinking.h"

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

/* Macro to take an FP number and turn it into a
 * 16/16-bit fixed-point number.
 */
#define FIXED_MUL        524288L
#define FIXED_PT(x)      ((long) (FIXED_MUL * (x)))
/* The number by which to multiply sines, cosines and other
 * values with magnitudes less than or equal to 1.
 * sins and coss are a 3/29 bit fixed-point scheme (so the
 * range is +/- 2), with good accuracy.  The range is to
 * avoid overflowing when the aspect ratio is taken into
 * account.
 */
#define FIXED_LT1       536870912.0
#define ANGLE_TO_DOUBLE (2.0*PI / 4294967296.0)

namespace
{

struct LSysCmd
{
    void (*f)(LSysTurtleStateI *);
    long n;
    char ch;
};

} // namespace

static bool read_lsystem_file(char const *str);
static void free_rules_mem();
static int rule_present(char symbol);
static bool save_axiom(char const *text);
static bool save_rule(char const *rule, int index);
static bool append_rule(char const *rule, int index);
static void free_l_cmds();
static LSysCmd *find_size(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth);
static LSysCmd *draw_lsysi(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth);
static bool lsysi_find_scale(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth);
static LSysCmd *lsysi_size_transform(char const *s, LSysTurtleStateI *ts);
static LSysCmd *lsysi_draw_transform(char const *s, LSysTurtleStateI *ts);
static void lsysi_do_sin_cos();
static void lsysi_do_slash(LSysTurtleStateI *cmd);
static void lsysi_do_backslash(LSysTurtleStateI *cmd);
static void lsysi_do_at(LSysTurtleStateI *cmd);
static void lsysi_do_pipe(LSysTurtleStateI *cmd);
static void lsysi_do_size_dm(LSysTurtleStateI *cmd);
static void lsysi_do_size_gf(LSysTurtleStateI *cmd);
static void lsysi_do_draw_d(LSysTurtleStateI *cmd);
static void lsysi_do_draw_m(LSysTurtleStateI *cmd);
static void lsysi_do_draw_g(LSysTurtleStateI *cmd);
static void lsysi_do_draw_f(LSysTurtleStateI *cmd);
static void lsysi_do_draw_c(LSysTurtleStateI *cmd);
static void lsysi_do_draw_gt(LSysTurtleStateI *cmd);
static void lsysi_do_draw_lt(LSysTurtleStateI *cmd);

static std::vector<long> s_sin_table;
static std::vector<long> s_cos_table;
constexpr long PI_DIV_180_L{11930465L};
static std::string s_axiom;
static std::vector<std::string> s_rules;
static std::vector<LSysCmd *> s_rule_cmds;
static std::vector<LSysFCmd *> s_rule_f_cmds;
static bool s_loaded{};

char g_max_angle{};

LDouble get_number(char const **str)
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

static bool read_lsystem_file(char const *str)
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
        while ((c = fgetc(infile)) != '{')
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
                char const *temp = std::strtok(nullptr, " =\t\n");
                int const index = rule_present(*word);
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

int lsystem()
{
    bool stack_overflow = false;

    if (!s_loaded && lsystem_load())
    {
        return -1;
    }

    g_overflow = false;           // reset integer math overflow flag

    int order = (int) g_params[0];
    order = std::max(order, 0);
    if (g_user_float_flag)
    {
        g_overflow = true;
    }
    else
    {
        LSysTurtleStateI ts;

        ts.stack_overflow = false;
        ts.max_angle = g_max_angle;
        ts.d_max_angle = (char)(g_max_angle - 1);

        s_rule_cmds.push_back(lsysi_size_transform(s_axiom.c_str(), &ts));
        for (auto const &rule : s_rules)
        {
            s_rule_cmds.push_back(lsysi_size_transform(rule.c_str(), &ts));
        }
        s_rule_cmds.push_back(nullptr);

        lsysi_do_sin_cos();
        if (lsysi_find_scale(s_rule_cmds[0], &ts, &s_rule_cmds[1], order))
        {
            ts.reverse = 0;
            ts.angle = ts.reverse;
            ts.real_angle = ts.angle;

            free_l_cmds();
            s_rule_cmds.push_back(lsysi_draw_transform(s_axiom.c_str(), &ts));
            for (auto const &rule : s_rules)
            {
                s_rule_cmds.push_back(lsysi_draw_transform(rule.c_str(), &ts));
            }
            s_rule_cmds.push_back(nullptr);

            // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
            ts.curr_color = 15;
            if (ts.curr_color > g_colors)
            {
                ts.curr_color = (char)(g_colors-1);
            }
            draw_lsysi(s_rule_cmds[0], &ts, &s_rule_cmds[1], order);
        }
        stack_overflow = ts.stack_overflow;
    }

    if (stack_overflow)
    {
        stop_msg("insufficient memory, try a lower order");
    }
    else if (g_overflow)
    {
        LSysTurtleStateF ts;

        g_overflow = false;

        ts.stack_overflow = false;
        ts.max_angle = g_max_angle;
        ts.d_max_angle = (char)(g_max_angle - 1);

        s_rule_f_cmds.push_back(lsysf_size_transform(s_axiom.c_str(), &ts));
        for (auto const &rule : s_rules)
        {
            s_rule_f_cmds.push_back(lsysf_size_transform(rule.c_str(), &ts));
        }
        s_rule_f_cmds.push_back(nullptr);

        lsysf_do_sin_cos();
        if (lsysf_find_scale(s_rule_f_cmds[0], &ts, &s_rule_f_cmds[1], order))
        {
            ts.reverse = 0;
            ts.angle = ts.reverse;
            ts.real_angle = ts.angle;

            free_l_cmds();
            s_rule_f_cmds.push_back(lsysf_draw_transform(s_axiom.c_str(), &ts));
            for (auto const &rule : s_rules)
            {
                s_rule_f_cmds.push_back(lsysf_draw_transform(rule.c_str(), &ts));
            }
            s_rule_f_cmds.push_back(nullptr);

            // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
            ts.curr_color = 15;
            if (ts.curr_color > g_colors)
            {
                ts.curr_color = (char)(g_colors-1);
            }
            draw_lsysf(s_rule_f_cmds[0], &ts, &s_rule_f_cmds[1], order);
        }
        g_overflow = false;
    }
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

static bool save_axiom(char const *text)
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

static bool save_rule(char const *rule, int index)
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

static bool append_rule(char const *rule, int index)
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
    for (auto cmd : s_rule_cmds)
    {
        if (cmd)
        {
            free(cmd);
        }
    }
    s_rule_cmds.clear();
    for (auto cmd : s_rule_f_cmds)
    {
        if (cmd != nullptr)
        {
            free(cmd);
        }
    }
    s_rule_f_cmds.clear();
}

// integer specific routines

static void lsysi_do_plus(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        if (++cmd->angle == cmd->max_angle)
        {
            cmd->angle = 0;
        }
    }
    else
    {
        if (cmd->angle)
        {
            cmd->angle--;
        }
        else
        {
            cmd->angle = cmd->d_max_angle;
        }
    }
}

// This is the same as lsys_doplus, except maxangle is a power of 2.
static void lsysi_do_plus_pow2(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        cmd->angle++;
        cmd->angle &= cmd->d_max_angle;
    }
    else
    {
        cmd->angle--;
        cmd->angle &= cmd->d_max_angle;
    }
}

static void lsysi_do_minus(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        if (cmd->angle)
        {
            cmd->angle--;
        }
        else
        {
            cmd->angle = cmd->d_max_angle;
        }
    }
    else
    {
        if (++cmd->angle == cmd->max_angle)
        {
            cmd->angle = 0;
        }
    }
}

static void lsysi_do_minus_pow2(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        cmd->angle--;
        cmd->angle &= cmd->d_max_angle;
    }
    else
    {
        cmd->angle++;
        cmd->angle &= cmd->d_max_angle;
    }
}

static void lsysi_do_slash(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        cmd->real_angle -= cmd->num;
    }
    else
    {
        cmd->real_angle += cmd->num;
    }
}

static void lsysi_do_backslash(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        cmd->real_angle += cmd->num;
    }
    else
    {
        cmd->real_angle -= cmd->num;
    }
}

static void lsysi_do_at(LSysTurtleStateI *cmd)
{
    cmd->size = multiply(cmd->size, cmd->num, 19);
}

static void lsysi_do_pipe(LSysTurtleStateI *cmd)
{
    cmd->angle = (char)(cmd->angle + (char)(cmd->max_angle / 2));
    cmd->angle %= cmd->max_angle;
}

static void lsysi_do_pipe_pow2(LSysTurtleStateI *cmd)
{
    cmd->angle += cmd->max_angle >> 1;
    cmd->angle &= cmd->d_max_angle;
}

static void lsysi_do_bang(LSysTurtleStateI *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsysi_do_size_dm(LSysTurtleStateI *cmd)
{
    double angle = (double) cmd->real_angle * ANGLE_TO_DOUBLE;
    double s;
    double c;

    sin_cos(&angle, &s, &c);
    long fixed_sin = (long) (s * FIXED_LT1);
    long fixed_cos = (long) (c * FIXED_LT1);

    cmd->x_pos = cmd->x_pos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixed_cos, 29));
    cmd->y_pos = cmd->y_pos + (multiply(cmd->size, fixed_sin, 29));

    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsysi_do_size_gf(LSysTurtleStateI *cmd)
{
    cmd->x_pos = cmd->x_pos + (multiply(cmd->size, s_cos_table[(int)cmd->angle], 29));
    cmd->y_pos = cmd->y_pos + (multiply(cmd->size, s_sin_table[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsysi_do_draw_d(LSysTurtleStateI *cmd)
{
    double angle = (double) cmd->real_angle * ANGLE_TO_DOUBLE;
    double s;
    double c;

    sin_cos(&angle, &s, &c);
    long fixed_sin = (long) (s * FIXED_LT1);
    long fixed_cos = (long) (c * FIXED_LT1);

    int last_x = (int) (cmd->x_pos >> 19);
    int last_y = (int) (cmd->y_pos >> 19);
    cmd->x_pos = cmd->x_pos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixed_cos, 29));
    cmd->y_pos = cmd->y_pos + (multiply(cmd->size, fixed_sin, 29));
    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    driver_draw_line(last_x, last_y, (int)(cmd->x_pos >> 19), (int)(cmd->y_pos >> 19), cmd->curr_color);
}

static void lsysi_do_draw_m(LSysTurtleStateI *cmd)
{
    double angle = (double) cmd->real_angle * ANGLE_TO_DOUBLE;
    double s;
    double c;

    sin_cos(&angle, &s, &c);
    long fixed_sin = (long) (s * FIXED_LT1);
    long fixed_cos = (long) (c * FIXED_LT1);

    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    cmd->x_pos = cmd->x_pos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixed_cos, 29));
    cmd->y_pos = cmd->y_pos + (multiply(cmd->size, fixed_sin, 29));
}

static void lsysi_do_draw_g(LSysTurtleStateI *cmd)
{
    cmd->x_pos = cmd->x_pos + (multiply(cmd->size, s_cos_table[(int)cmd->angle], 29));
    cmd->y_pos = cmd->y_pos + (multiply(cmd->size, s_sin_table[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
}

static void lsysi_do_draw_f(LSysTurtleStateI *cmd)
{
    int last_x = (int)(cmd->x_pos >> 19);
    int last_y = (int)(cmd->y_pos >> 19);
    cmd->x_pos = cmd->x_pos + (multiply(cmd->size, s_cos_table[(int)cmd->angle], 29));
    cmd->y_pos = cmd->y_pos + (multiply(cmd->size, s_sin_table[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
    driver_draw_line(last_x, last_y, (int)(cmd->x_pos >> 19), (int)(cmd->y_pos >> 19), cmd->curr_color);
}

static void lsysi_do_draw_c(LSysTurtleStateI *cmd)
{
    cmd->curr_color = (char)(((int) cmd->num) % g_colors);
}

static void lsysi_do_draw_gt(LSysTurtleStateI *cmd)
{
    cmd->curr_color = (char)(cmd->curr_color - (char)cmd->num);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = (char)(g_colors-1);
    }
}

static void lsysi_do_draw_lt(LSysTurtleStateI *cmd)
{
    cmd->curr_color = (char)(cmd->curr_color + (char)cmd->num);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = 1;
    }
}

static LSysCmd *find_size(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth)
{
    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }

    if (stack_avail() < 400)
    {
        // leave some margin for calling subrtns
        ts->stack_overflow = true;
        return nullptr;
    }

    while (command->ch && command->ch != ']')
    {
        if (!(ts->counter++))
        {
            // let user know we're not dead
            if (thinking(1, "L-System thinking (higher orders take longer)"))
            {
                ts->counter--;
                return nullptr;
            }
        }
        bool tran = false;
        if (depth > 0)
        {
            for (LSysCmd **rule_index = rules; *rule_index; rule_index++)
            {
                if ((*rule_index)->ch == command->ch)
                {
                    tran = true;
                    if (find_size((*rule_index)+1, ts, rules, depth-1) == nullptr)
                    {
                        return nullptr;
                    }
                }
            }
        }
        if (depth == 0 || !tran)
        {
            if (command->f)
            {
                ts->num = command->n;
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                char const save_angle = ts->angle;
                char const save_reverse = ts->reverse;
                long const save_size = ts->size;
                unsigned long const save_real_angle = ts->real_angle;
                long const save_x = ts->x_pos;
                long const save_y = ts->y_pos;
                command = find_size(command+1, ts, rules, depth);
                if (command == nullptr)
                {
                    return nullptr;
                }
                ts->angle = save_angle;
                ts->reverse = save_reverse;
                ts->size = save_size;
                ts->real_angle = save_real_angle;
                ts->x_pos = save_x;
                ts->y_pos = save_y;
            }
        }
        command++;
    }
    return command;
}

static bool lsysi_find_scale(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth)
{
    float horiz;
    float vert;

    double local_aspect = g_screen_aspect * g_logical_screen_x_dots / g_logical_screen_y_dots;
    ts->aspect = FIXED_PT(local_aspect);
    ts->counter = 0;
    ts->reverse = ts->counter;
    ts->angle = ts->reverse;
    ts->real_angle = ts->angle;
    ts->y_min = ts->real_angle;
    ts->y_max = ts->y_min;
    ts->x_max = ts->y_max;
    ts->x_min = ts->x_max;
    ts->y_pos = ts->x_min;
    ts->x_pos = ts->y_pos;
    ts->size = FIXED_PT(1L);
    LSysCmd *f_s_ret = find_size(command, ts, rules, depth);
    thinking(0, nullptr); // erase thinking message if any
    double x_min = (double) ts->x_min / FIXED_MUL;
    double x_max = (double) ts->x_max / FIXED_MUL;
    double y_min = (double) ts->y_min / FIXED_MUL;
    double y_max = (double) ts->y_max / FIXED_MUL;
    if (f_s_ret == nullptr)
    {
        return false;
    }
    if (x_max == x_min)
    {
        horiz = (float)1E37;
    }
    else
    {
        horiz = (float)((g_logical_screen_x_dots-10)/(x_max-x_min));
    }
    if (y_max == y_min)
    {
        vert = (float)1E37;
    }
    else
    {
        vert = (float)((g_logical_screen_y_dots-6) /(y_max-y_min));
    }
    double local_size = (vert < horiz) ? vert : horiz;

    if (horiz == 1E37)
    {
        ts->x_pos = FIXED_PT(g_logical_screen_x_dots/2);
    }
    else
    {
        //    ts->xpos = FIXEDPT(-xmin*(locsize)+5+((xdots-10)-(locsize)*(xmax-xmin))/2);
        ts->x_pos = FIXED_PT((g_logical_screen_x_dots-local_size*(x_max+x_min))/2);
    }
    if (vert == 1E37)
    {
        ts->y_pos = FIXED_PT(g_logical_screen_y_dots/2);
    }
    else
    {
        //    ts->ypos = FIXEDPT(-ymin*(locsize)+3+((ydots-6)-(locsize)*(ymax-ymin))/2);
        ts->y_pos = FIXED_PT((g_logical_screen_y_dots-local_size*(y_max+y_min))/2);
    }
    ts->size = FIXED_PT(local_size);

    return true;
}

static LSysCmd *draw_lsysi(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth)
{
    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }

    if (stack_avail() < 400)
    {
        // leave some margin for calling subrtns
        ts->stack_overflow = true;
        return nullptr;
    }

    while (command->ch && command->ch != ']')
    {
        if (!(ts->counter++))
        {
            if (driver_key_pressed())
            {
                ts->counter--;
                return nullptr;
            }
        }
        bool tran = false;
        if (depth)
        {
            for (LSysCmd **rule_index = rules; *rule_index; rule_index++)
            {
                if ((*rule_index)->ch == command->ch)
                {
                    tran = true;
                    if (draw_lsysi((*rule_index)+1, ts, rules, depth-1) == nullptr)
                    {
                        return nullptr;
                    }
                }
            }
        }
        if (!depth || !tran)
        {
            if (command->f)
            {
                ts->num = command->n;
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                char const save_angle = ts->angle;
                char const save_reverse = ts->reverse;
                long const save_size = ts->size;
                unsigned long const save_real_angle = ts->real_angle;
                long const save_x = ts->x_pos;
                long const save_y = ts->y_pos;
                char const save_color = ts->curr_color;
                command = draw_lsysi(command+1, ts, rules, depth);
                if (command == nullptr)
                {
                    return nullptr;
                }
                ts->angle = save_angle;
                ts->reverse = save_reverse;
                ts->size = save_size;
                ts->real_angle = save_real_angle;
                ts->x_pos = save_x;
                ts->y_pos = save_y;
                ts->curr_color = save_color;
            }
        }
        command++;
    }
    return command;
}

static LSysCmd *lsysi_size_transform(char const *s, LSysTurtleStateI *ts)
{
    int max_val = 10;
    int n = 0;

    auto const plus = is_pow2(ts->max_angle) ? lsysi_do_plus_pow2 : lsysi_do_plus;
    auto const minus = is_pow2(ts->max_angle) ? lsysi_do_minus_pow2 : lsysi_do_minus;
    auto const pipe = is_pow2(ts->max_angle) ? lsysi_do_pipe_pow2 : lsysi_do_pipe;

    LSysCmd *ret = (LSysCmd *) malloc((long) max_val * sizeof(LSysCmd));
    if (ret == nullptr)
    {
        ts->stack_overflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleStateI *) = nullptr;
        long num = 0;
        ret[n].ch = *s;
        switch (*s)
        {
        case '+':
            f = plus;
            break;
        case '-':
            f = minus;
            break;
        case '/':
            f = lsysi_do_slash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '\\':
            f = lsysi_do_backslash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '@':
            f = lsysi_do_at;
            num = FIXED_PT(get_number(&s));
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysi_do_bang;
            break;
        case 'd':
        case 'm':
            f = lsysi_do_size_dm;
            break;
        case 'g':
        case 'f':
            f = lsysi_do_size_gf;
            break;
        case '[':
            num = 1;
            break;
        case ']':
            num = 2;
            break;
        default:
            num = 3;
            break;
        }
        ret[n].f = f;
        ret[n].n = num;
        if (++n == max_val)
        {
            LSysCmd *doubled = (LSysCmd *) malloc((long) max_val*2*sizeof(LSysCmd));
            if (doubled == nullptr)
            {
                free(ret);
                ts->stack_overflow = true;
                return nullptr;
            }
            std::memcpy(doubled, ret, max_val*sizeof(LSysCmd));
            free(ret);
            ret = doubled;
            max_val <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].n = 0;
    n++;

    LSysCmd *doubled = (LSysCmd *) malloc((long) n*sizeof(LSysCmd));
    if (doubled == nullptr)
    {
        free(ret);
        ts->stack_overflow = true;
        return nullptr;
    }
    std::memcpy(doubled, ret, n*sizeof(LSysCmd));
    free(ret);
    return doubled;
}

static LSysCmd *lsysi_draw_transform(char const *s, LSysTurtleStateI *ts)
{
    int max_val = 10;
    int n = 0;

    auto const plus = is_pow2(ts->max_angle) ? lsysi_do_plus_pow2 : lsysi_do_plus;
    auto const minus = is_pow2(ts->max_angle) ? lsysi_do_minus_pow2 : lsysi_do_minus;
    auto const pipe = is_pow2(ts->max_angle) ? lsysi_do_pipe_pow2 : lsysi_do_pipe;

    LSysCmd *ret = (LSysCmd *) malloc((long) max_val * sizeof(LSysCmd));
    if (ret == nullptr)
    {
        ts->stack_overflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleStateI *) = nullptr;
        long num = 0;
        ret[n].ch = *s;
        switch (*s)
        {
        case '+':
            f = plus;
            break;
        case '-':
            f = minus;
            break;
        case '/':
            f = lsysi_do_slash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '\\':
            f = lsysi_do_backslash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '@':
            f = lsysi_do_at;
            num = FIXED_PT(get_number(&s));
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysi_do_bang;
            break;
        case 'd':
            f = lsysi_do_draw_d;
            break;
        case 'm':
            f = lsysi_do_draw_m;
            break;
        case 'g':
            f = lsysi_do_draw_g;
            break;
        case 'f':
            f = lsysi_do_draw_f;
            break;
        case 'c':
            f = lsysi_do_draw_c;
            num = (long) get_number(&s);
            break;
        case '<':
            f = lsysi_do_draw_lt;
            num = (long) get_number(&s);
            break;
        case '>':
            f = lsysi_do_draw_gt;
            num = (long) get_number(&s);
            break;
        case '[':
            num = 1;
            break;
        case ']':
            num = 2;
            break;
        default:
            num = 3;
            break;
        }
        ret[n].f = f;
        ret[n].n = num;
        if (++n == max_val)
        {
            LSysCmd *doubled = (LSysCmd *) malloc((long) max_val*2*sizeof(LSysCmd));
            if (doubled == nullptr)
            {
                free(ret);
                ts->stack_overflow = true;
                return nullptr;
            }
            std::memcpy(doubled, ret, max_val*sizeof(LSysCmd));
            free(ret);
            ret = doubled;
            max_val <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].n = 0;
    n++;

    LSysCmd *doubled = (LSysCmd *) malloc((long) n*sizeof(LSysCmd));
    if (doubled == nullptr)
    {
        free(ret);
        ts->stack_overflow = true;
        return nullptr;
    }
    std::memcpy(doubled, ret, n*sizeof(LSysCmd));
    free(ret);
    return doubled;
}

static void lsysi_do_sin_cos()
{
    double two_pi = 2.0 * PI;
    double two_pi_max_i;
    double s;
    double c;

    double local_aspect = g_screen_aspect * g_logical_screen_x_dots / g_logical_screen_y_dots;
    double two_pi_max = two_pi / g_max_angle;
    s_sin_table.resize(g_max_angle);
    s_cos_table.resize(g_max_angle);
    for (int i = 0; i < g_max_angle; i++)
    {
        two_pi_max_i = i * two_pi_max;
        sin_cos(&two_pi_max_i, &s, &c);
        s_sin_table[i] = (long)(s * FIXED_LT1);
        s_cos_table[i] = (long)((local_aspect * c) * FIXED_LT1);
    }
}
