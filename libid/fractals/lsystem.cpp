// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/lsystem.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/LogicalScreen.h"
#include "engine/VideoInfo.h"
#include "io/file_gets.h"
#include "math/fixed_pt.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/stack_avail.h"
#include "ui/file_item.h"
#include "ui/stop_msg.h"
#include "ui/thinking.h"

#include <config/string_lower.h>

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <new>
#include <string>
#include <vector>

#ifdef max
#undef max
#endif

namespace fs = std::filesystem;

using namespace id::config;
using namespace id::engine;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::fractals
{

namespace
{

enum
{
    MAX_LSYS_LINE_LEN = 255 // this limits line length to 255
};

struct LSysTurtleState
{
    char counter, angle, reverse;
    bool stack_overflow;
    // dmaxangle is maxangle - 1
    char max_angle, d_max_angle, curr_color, dummy;  // dummy ensures longword alignment
    LDouble size;
    LDouble real_angle;
    LDouble x_pos, y_pos;
    LDouble x_min, y_min, x_max, y_max;
    LDouble aspect; // aspect ratio of each pixel, ysize/xsize
    union
    {
        long n;
        LDouble nf;
    } param;
};

struct LSysCmd
{
    void (*f)(LSysTurtleState *);
    int param_type;
    union
    {
        long n;
        LDouble nf;
    } param;
    char ch;
};

} // namespace

char g_max_angle{};
fs::path g_l_system_filename;                // file to find L-System's in
std::string g_l_system_name;                 // Name of L-System (if not empty)

static std::vector<double> s_sin_table;
static std::vector<double> s_cos_table;
static constexpr LDouble PI_DIV_180{PI / 180.0L};
static std::string s_axiom;
static std::vector<std::string> s_rules;
static std::vector<LSysCmd *> s_rule_cmds;
static bool s_loaded{};

static LSysCmd *find_size(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth);
static bool read_lsystem_file(const char *str);
static void free_rules_mem();
static int rule_present(char symbol);
static bool save_axiom(const char *text);
static bool save_rule(const char *rule, int index);
static bool append_rule(const char *rule, int index);
static void free_l_cmds();

inline bool is_pow2(const int n)
{
    return n == (n & -n);
}

static LDouble get_number(const char **str)
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
    char msg_buff[6 * 80 + 1]{};                                // enough for 6 full lines
    const auto append_error = [&](const std::string &s)
    {
        std::strcat(msg_buff, s.c_str());
        ++err;
    };

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

        if (static_cast<int>(std::strspn(inline1, " \t\n")) < static_cast<int>(std::strlen(inline1))) // not a blank line
        {
            bool check = false;
            word = std::strtok(inline1, " =\t\n");
            if (std::strcmp(word, "axiom") == 0)
            {
                if (save_axiom(std::strtok(nullptr, " \t\n")))
                {
                    append_error("Error:  out of memory\n");
                    break;
                }
                check = true;
            }
            else if (std::strcmp(word, "angle") == 0)
            {
                g_max_angle = static_cast<char>(std::atoi(std::strtok(nullptr, " \t\n")));
                check = true;
            }
            else if (std::strcmp(word, "}") == 0)
            {
                break;
            }
            else if (!word[1])
            {
                bool mem_err = false;

                if (std::strchr("+-/\\@|!c<>][", *word))
                {
                    append_error(fmt::format(
                        "Syntax error line {:d}: Redefined reserved symbol {:s}\n", line_num, word));
                    break;
                }
                const char *temp = std::strtok(nullptr, " =\t\n");
                if (const int index = rule_present(*word); !index)
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
                    append_error("Error:  out of memory\n");
                    break;
                }
                check = true;
            }
            else if (err < 6)
            {
                append_error(fmt::format("Syntax error line {:d}: {:s}\n", line_num, word));
            }
            if (check)
            {
                word = std::strtok(nullptr, " \t\n");
                if (word != nullptr)
                {
                    if (err < 6)
                    {
                        append_error(
                            fmt::format("Extra text after command line {:d}: {:s}\n", line_num, word));
                    }
                }
            }
        }
    }
    std::fclose(infile);
    if (s_axiom.empty() && err < 6)
    {
        append_error("Error:  no axiom\n");
    }
    if ((g_max_angle < 3||g_max_angle > 50) && err < 6)
    {
        append_error("Error:  illegal or missing angle\n");
    }
    if (err)
    {
        msg_buff[std::strlen(msg_buff)-1] = 0; // strip trailing \n
        stop_msg(msg_buff);
        return true;
    }
    return false;
}

static void lsys_build_trig_table()
{
    constexpr LDouble two_pi = 2.0 * PI;

    const double local_aspect = g_screen_aspect * g_logical_screen.x_dots / g_logical_screen.y_dots;
    const double increment = two_pi / g_max_angle;
    s_sin_table.resize(g_max_angle);
    s_cos_table.resize(g_max_angle);
    for (int i = 0; i < g_max_angle; i++)
    {
        const double angle = i * increment;
        s_sin_table[i] = std::sin(angle);
        s_cos_table[i] = local_aspect * std::cos(angle);
    }
}

// This is the same as lsys_plus, except maxangle is a power of 2.
static void lsys_plus_pow2(LSysTurtleState *cmd)
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

static void lsys_minus_pow2(LSysTurtleState *cmd)
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

static void lsys_plus(LSysTurtleState *cmd)
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

static void lsys_minus(LSysTurtleState *cmd)
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

static void lsys_pipe(LSysTurtleState *cmd)
{
    cmd->angle = static_cast<char>(cmd->angle + cmd->max_angle / 2);
    cmd->angle %= cmd->max_angle;
}

static void lsys_pipe_pow2(LSysTurtleState *cmd)
{
    cmd->angle += cmd->max_angle >> 1;
    cmd->angle &= cmd->d_max_angle;
}

static void lsys_slash(LSysTurtleState *cmd)
{
    if (cmd->reverse)
    {
        cmd->real_angle -= cmd->param.nf;
    }
    else
    {
        cmd->real_angle += cmd->param.nf;
    }
}

static void lsys_backslash(LSysTurtleState *cmd)
{
    if (cmd->reverse)
    {
        cmd->real_angle += cmd->param.nf;
    }
    else
    {
        cmd->real_angle -= cmd->param.nf;
    }
}

static void lsys_at(LSysTurtleState *cmd)
{
    cmd->size *= cmd->param.nf;
}

static void lsys_bang(LSysTurtleState *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsys_size_dm(LSysTurtleState *cmd)
{
    const double angle = static_cast<double>(cmd->real_angle);
    const double s = std::sin(angle);
    const double c = std::cos(angle);

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;

    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsys_size_gf(LSysTurtleState *cmd)
{
    cmd->x_pos += cmd->size * s_cos_table[static_cast<int>(cmd->angle)];
    cmd->y_pos += cmd->size * s_sin_table[static_cast<int>(cmd->angle)];

    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsys_draw_d(LSysTurtleState *cmd)
{
    const double angle = static_cast<double>(cmd->real_angle);
    const double s = std::sin(angle);
    const double c = std::cos(angle);
    const int last_x = static_cast<int>(cmd->x_pos);
    const int last_y = static_cast<int>(cmd->y_pos);

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;

    driver_draw_line(last_x, last_y, static_cast<int>(cmd->x_pos), static_cast<int>(cmd->y_pos), cmd->curr_color);
}

static void lsys_draw_m(LSysTurtleState *cmd)
{
    const double angle = static_cast<double>(cmd->real_angle);
    const double s = std::sin(angle);
    const double c = std::cos(angle);

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;
}

static void lsys_draw_g(LSysTurtleState *cmd)
{
    cmd->x_pos += cmd->size * s_cos_table[static_cast<int>(cmd->angle)];
    cmd->y_pos += cmd->size * s_sin_table[static_cast<int>(cmd->angle)];
}

static void lsys_draw_f(LSysTurtleState *cmd)
{
    const int last_x = static_cast<int>(cmd->x_pos);
    const int last_y = static_cast<int>(cmd->y_pos);
    cmd->x_pos += cmd->size * s_cos_table[static_cast<int>(cmd->angle)];
    cmd->y_pos += cmd->size * s_sin_table[static_cast<int>(cmd->angle)];
    driver_draw_line(last_x, last_y, static_cast<int>(cmd->x_pos), static_cast<int>(cmd->y_pos), cmd->curr_color);
}

static void lsys_draw_c(LSysTurtleState *cmd)
{
    cmd->curr_color = static_cast<char>(static_cast<int>(cmd->param.n) % g_colors);
}

static void lsys_draw_gt(LSysTurtleState *cmd)
{
    cmd->curr_color = static_cast<char>(cmd->curr_color - cmd->param.n);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = static_cast<char>(g_colors - 1);
    }
}

static void lsys_draw_lt(LSysTurtleState *cmd)
{
    cmd->curr_color = static_cast<char>(cmd->curr_color + cmd->param.n);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = 1;
    }
}

static LSysCmd *lsys_draw_transform(const char *s, LSysTurtleState *ts)
{
    int max = 10;
    int n = 0;

    const auto plus = is_pow2(ts->max_angle) ? lsys_plus_pow2 : lsys_plus;
    const auto minus = is_pow2(ts->max_angle) ? lsys_minus_pow2 : lsys_minus;
    const auto pipe = is_pow2(ts->max_angle) ? lsys_pipe_pow2 : lsys_pipe;

    LSysCmd *ret = static_cast<LSysCmd *>(malloc(static_cast<long>(max) * sizeof(LSysCmd)));
    if (ret == nullptr)
    {
        ts->stack_overflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleState *) = nullptr;
        LDouble num = 0;
        int param_type = 4;
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
            f = lsys_slash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsys_backslash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsys_at;
            param_type = 10;
            ret[n].param.nf = get_number(&s);
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsys_bang;
            break;
        case 'd':
            f = lsys_draw_d;
            break;
        case 'm':
            f = lsys_draw_m;
            break;
        case 'g':
            f = lsys_draw_g;
            break;
        case 'f':
            f = lsys_draw_f;
            break;
        case 'c':
            f = lsys_draw_c;
            num = get_number(&s);
            break;
        case '<':
            f = lsys_draw_lt;
            num = get_number(&s);
            break;
        case '>':
            f = lsys_draw_gt;
            num = get_number(&s);
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
        if (param_type == 4)
        {
            ret[n].param.n = static_cast<long>(num);
        }
        ret[n].param_type = param_type;
        if (++n == max)
        {
            LSysCmd *doubled = static_cast<LSysCmd *>(malloc(static_cast<long>(max) * 2 * sizeof(LSysCmd)));
            if (doubled == nullptr)
            {
                free(ret);
                ts->stack_overflow = true;
                return nullptr;
            }
            std::memcpy(doubled, ret, max*sizeof(LSysCmd));
            free(ret);
            ret = doubled;
            max <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].param.n = 0;
    n++;

    LSysCmd *doubled = static_cast<LSysCmd *>(malloc(static_cast<long>(n) * sizeof(LSysCmd)));
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

static LSysCmd *lsys_size_transform(const char *s, LSysTurtleState *ts)
{
    int max = 10;
    int n = 0;

    const auto plus = is_pow2(ts->max_angle) ? lsys_plus_pow2 : lsys_plus;
    const auto minus = is_pow2(ts->max_angle) ? lsys_minus_pow2 : lsys_minus;
    const auto pipe = is_pow2(ts->max_angle) ? lsys_pipe_pow2 : lsys_pipe;

    LSysCmd *ret = static_cast<LSysCmd *>(malloc(static_cast<long>(max) * sizeof(LSysCmd)));
    if (ret == nullptr)
    {
        ts->stack_overflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleState *) = nullptr;
        long num = 0;
        int param_type = 4;
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
            f = lsys_slash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsys_backslash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsys_at;
            param_type = 10;
            ret[n].param.nf = get_number(&s);
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsys_bang;
            break;
        case 'd':
        case 'm':
            f = lsys_size_dm;
            break;
        case 'g':
        case 'f':
            f = lsys_size_gf;
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
        if (param_type == 4)
        {
            ret[n].param.n = num;
        }
        ret[n].param_type = param_type;
        if (++n == max)
        {
            LSysCmd *doubled = static_cast<LSysCmd *>(malloc(static_cast<long>(max) * 2 * sizeof(LSysCmd)));
            if (doubled == nullptr)
            {
                free(ret);
                ts->stack_overflow = true;
                return nullptr;
            }
            std::memcpy(doubled, ret, max*sizeof(LSysCmd));
            free(ret);
            ret = doubled;
            max <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].param.n = 0;
    n++;

    LSysCmd *doubled = static_cast<LSysCmd *>(malloc(static_cast<long>(n) * sizeof(LSysCmd)));
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

static bool lsys_find_scale(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, const int depth)
{
    ts->aspect = g_screen_aspect*g_logical_screen.x_dots/g_logical_screen.y_dots;
    ts->y_min = 0;
    ts->y_max = 0;
    ts->x_max = 0;
    ts->x_min = 0;
    ts->y_pos = 0;
    ts->x_pos = 0;
    ts->counter = 0;
    ts->reverse = 0;
    ts->angle = 0;
    ts->real_angle = 0;
    ts->size = 1;
    const LSysCmd *f_s_ret = find_size(command, ts, rules, depth);
    thinking_end(); // erase thinking message if any
    const LDouble x_min = ts->x_min;
    const LDouble x_max = ts->x_max;
    const LDouble y_min = ts->y_min;
    const LDouble y_max = ts->y_max;
    if (f_s_ret == nullptr)
    {
        return false;
    }
    float horiz;
    if (x_max == x_min)
    {
        horiz = static_cast<float>(1E37);
    }
    else
    {
        horiz = static_cast<float>((g_logical_screen.x_dots - 10) / (x_max - x_min));
    }
    float vert;
    if (y_max == y_min)
    {
        vert = static_cast<float>(1E37);
    }
    else
    {
        vert = static_cast<float>((g_logical_screen.y_dots - 6) / (y_max - y_min));
    }
    const LDouble local_size = vert < horiz ? vert : horiz;

    if (horiz == 1E37)
    {
        ts->x_pos = g_logical_screen.x_dots/2;
    }
    else
    {
        ts->x_pos = (g_logical_screen.x_dots-local_size*(x_max+x_min))/2;
    }
    if (vert == 1E37)
    {
        ts->y_pos = g_logical_screen.y_dots/2;
    }
    else
    {
        ts->y_pos = (g_logical_screen.y_dots-local_size*(y_max+y_min))/2;
    }
    ts->size = local_size;

    return true;
}

static LSysCmd *draw_lsys(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, const int depth)
{
    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }

    if (stack_avail() < 400)
    {
        // leave some margin for calling subroutines
        ts->stack_overflow = true;
        return nullptr;
    }

    while (command->ch && command->ch != ']')
    {
        if (!ts->counter++)
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
                    if (draw_lsys(*rule_index +1, ts, rules, depth-1) == nullptr)
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
                switch (command->param_type)
                {
                case 4:
                    ts->param.n = command->param.n;
                    break;
                case 10:
                    ts->param.nf = command->param.nf;
                    break;
                default:
                    break;
                }
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                const char save_angle = ts->angle;
                const char save_reverse = ts->reverse;
                const LDouble save_size = ts->size;
                const LDouble save_real_angle = ts->real_angle;
                const LDouble save_x = ts->x_pos;
                const LDouble save_y = ts->y_pos;
                const char save_color = ts->curr_color;
                command = draw_lsys(command+1, ts, rules, depth);
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

int lsystem_type()
{
    if (!s_loaded && lsystem_load())
    {
        return -1;
    }

    int order = static_cast<int>(g_params[0]);
    order = std::max(order, 0);

    LSysTurtleState ts;

    ts.stack_overflow = false;
    ts.max_angle = g_max_angle;
    ts.d_max_angle = static_cast<char>(g_max_angle - 1);

    s_rule_cmds.push_back(lsys_size_transform(s_axiom.c_str(), &ts));
    for (const std::string &rule : s_rules)
    {
        s_rule_cmds.push_back(lsys_size_transform(rule.c_str(), &ts));
    }
    s_rule_cmds.push_back(nullptr);

    lsys_build_trig_table();
    if (lsys_find_scale(s_rule_cmds[0], &ts, &s_rule_cmds[1], order))
    {
        ts.reverse = 0;
        ts.angle = 0;
        ts.real_angle = 0;

        free_l_cmds();
        s_rule_cmds.push_back(lsys_draw_transform(s_axiom.c_str(), &ts));
        for (const std::string &rule : s_rules)
        {
            s_rule_cmds.push_back(lsys_draw_transform(rule.c_str(), &ts));
        }
        s_rule_cmds.push_back(nullptr);

        // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
        ts.curr_color = 15;
        if (ts.curr_color > g_colors)
        {
            ts.curr_color = static_cast<char>(g_colors - 1);
        }
        draw_lsys(s_rule_cmds[0], &ts, &s_rule_cmds[1], order);
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
    for (std::string &rule : s_rules)
    {
        rule.clear();
        rule.shrink_to_fit();
    }
    s_rules.clear();
    s_rules.shrink_to_fit();
}

static int rule_present(const char symbol)
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

static bool save_rule(const char *rule, const int index)
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

static bool append_rule(const char *rule, const int index)
{
    try
    {
        assert(index > 0 && static_cast<unsigned>(index) < s_rules.size());
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
    for (LSysCmd *cmd : s_rule_cmds)
    {
        if (cmd != nullptr)
        {
            free(cmd);
        }
    }
    s_rule_cmds.clear();
}

static LSysCmd *find_size(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, const int depth)
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
        if (!ts->counter++)
        {
            // let user know we're not dead
            if (thinking("L-System thinking (higher orders take longer)"))
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
                    if (find_size(*rule_index +1, ts, rules, depth-1) == nullptr)
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
                switch (command->param_type)
                {
                case 4:
                    ts->param.n = command->param.n;
                    break;
                case 10:
                    ts->param.nf = command->param.nf;
                    break;
                default:
                    break;
                }
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                const char save_angle = ts->angle;
                const char save_reverse = ts->reverse;
                const LDouble save_size = ts->size;
                const LDouble save_real_angle = ts->real_angle;
                const LDouble save_x = ts->x_pos;
                const LDouble save_y = ts->y_pos;
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

} // namespace id::fractals
