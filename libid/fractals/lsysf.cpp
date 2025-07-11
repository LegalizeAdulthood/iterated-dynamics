// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/lsys.h"

#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/lsys_fns.h"
#include "math/fixed_pt.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/stack_avail.h"
#include "ui/thinking.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#ifdef max
#undef max
#endif

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

static std::vector<LDouble> s_sin_table_f;
static std::vector<LDouble> s_cos_table_f;
static constexpr LDouble PI_DIV_180{PI / 180.0L};

static LSysCmd *find_size(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth);

static void lsysf_do_plus(LSysTurtleState *cmd)
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
static void lsysf_do_plus_pow2(LSysTurtleState *cmd)
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

static void lsysf_do_minus(LSysTurtleState *cmd)
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

static void lsysf_do_minus_pow2(LSysTurtleState *cmd)
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

static void lsysf_do_slash(LSysTurtleState *cmd)
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

static void lsysf_do_backslash(LSysTurtleState *cmd)
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

static void lsysf_do_at(LSysTurtleState *cmd)
{
    cmd->size *= cmd->param.nf;
}

static void
lsysf_do_pipe(LSysTurtleState *cmd)
{
    cmd->angle = (char)(cmd->angle + cmd->max_angle / 2);
    cmd->angle %= cmd->max_angle;
}

static void lsysf_do_pipe_pow2(LSysTurtleState *cmd)
{
    cmd->angle += cmd->max_angle >> 1;
    cmd->angle &= cmd->d_max_angle;
}

static void lsysf_do_bang(LSysTurtleState *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsysf_do_size_dm(LSysTurtleState *cmd)
{
    double angle = (double) cmd->real_angle;

    double s = std::sin(angle);
    double c = std::cos(angle);

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;

    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsysf_do_size_gf(LSysTurtleState *cmd)
{
    cmd->x_pos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->y_pos += cmd->size * s_sin_table_f[(int)cmd->angle];

    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsysf_do_draw_d(LSysTurtleState *cmd)
{
    double angle = (double) cmd->real_angle;
    double s = std::sin(angle);
    double c = std::cos(angle);

    int last_x = (int) cmd->x_pos;
    int last_y = (int) cmd->y_pos;

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;

    driver_draw_line(last_x, last_y, (int) cmd->x_pos, (int) cmd->y_pos, cmd->curr_color);
}

static void lsysf_do_draw_m(LSysTurtleState *cmd)
{
    double angle = (double) cmd->real_angle;

    double s = std::sin(angle);
    double c = std::cos(angle);

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;
}

static void lsysf_do_draw_g(LSysTurtleState *cmd)
{
    cmd->x_pos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->y_pos += cmd->size * s_sin_table_f[(int)cmd->angle];
}

static void lsysf_do_draw_f(LSysTurtleState *cmd)
{
    int last_x = (int) cmd->x_pos;
    int last_y = (int) cmd->y_pos;
    cmd->x_pos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->y_pos += cmd->size * s_sin_table_f[(int)cmd->angle];
    driver_draw_line(last_x, last_y, (int) cmd->x_pos, (int) cmd->y_pos, cmd->curr_color);
}

static void lsysf_do_draw_c(LSysTurtleState *cmd)
{
    cmd->curr_color = (char)(((int) cmd->param.n) % g_colors);
}

static void lsysf_do_draw_gt(LSysTurtleState *cmd)
{
    cmd->curr_color = (char)(cmd->curr_color - cmd->param.n);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = (char)(g_colors-1);
    }
}

static void lsysf_do_draw_lt(LSysTurtleState *cmd)
{
    cmd->curr_color = (char)(cmd->curr_color + cmd->param.n);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = 1;
    }
}

static LSysCmd *
find_size(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth)
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
                    if (find_size((*rule_index)+1, ts, rules, depth-1) == nullptr)
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

bool
lsysf_find_scale(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth)
{
    ts->aspect = g_screen_aspect*g_logical_screen_x_dots/g_logical_screen_y_dots;
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
    LSysCmd *f_s_ret = find_size(command, ts, rules, depth);
    thinking_end(); // erase thinking message if any
    LDouble x_min = ts->x_min;
    LDouble x_max = ts->x_max;
    LDouble y_min = ts->y_min;
    LDouble y_max = ts->y_max;
    if (f_s_ret == nullptr)
    {
        return false;
    }
    float horiz;
    if (x_max == x_min)
    {
        horiz = (float)1E37;
    }
    else
    {
        horiz = (float)((g_logical_screen_x_dots-10)/(x_max-x_min));
    }
    float vert;
    if (y_max == y_min)
    {
        vert = (float)1E37;
    }
    else
    {
        vert = (float)((g_logical_screen_y_dots-6) /(y_max-y_min));
    }
    const LDouble local_size = (vert < horiz) ? vert : horiz;

    if (horiz == 1E37)
    {
        ts->x_pos = g_logical_screen_x_dots/2;
    }
    else
    {
        ts->x_pos = (g_logical_screen_x_dots-local_size*(x_max+x_min))/2;
    }
    if (vert == 1E37)
    {
        ts->y_pos = g_logical_screen_y_dots/2;
    }
    else
    {
        ts->y_pos = (g_logical_screen_y_dots-local_size*(y_max+y_min))/2;
    }
    ts->size = local_size;

    return true;
}

LSysCmd *
draw_lsysf(LSysCmd *command, LSysTurtleState *ts, LSysCmd **rules, int depth)
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
                    if (draw_lsysf((*rule_index)+1, ts, rules, depth-1) == nullptr)
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
                command = draw_lsysf(command+1, ts, rules, depth);
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

LSysCmd *lsysf_size_transform(const char *s, LSysTurtleState *ts)
{
    int max = 10;
    int n = 0;

    const auto plus = is_pow2(ts->max_angle) ? lsysf_do_plus_pow2 : lsysf_do_plus;
    const auto minus = is_pow2(ts->max_angle) ? lsysf_do_minus_pow2 : lsysf_do_minus;
    const auto pipe = is_pow2(ts->max_angle) ? lsysf_do_pipe_pow2 : lsysf_do_pipe;

    LSysCmd *ret = (LSysCmd *) malloc((long) max * sizeof(LSysCmd));
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
            f = lsysf_do_slash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsysf_do_backslash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsysf_do_at;
            param_type = 10;
            ret[n].param.nf = get_number(&s);
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysf_do_bang;
            break;
        case 'd':
        case 'm':
            f = lsysf_do_size_dm;
            break;
        case 'g':
        case 'f':
            f = lsysf_do_size_gf;
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
            LSysCmd *doubled = (LSysCmd *) malloc((long) max*2*sizeof(LSysCmd));
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

LSysCmd *lsysf_draw_transform(const char *s, LSysTurtleState *ts)
{
    int max = 10;
    int n = 0;

    const auto plus = is_pow2(ts->max_angle) ? lsysf_do_plus_pow2 : lsysf_do_plus;
    const auto minus = is_pow2(ts->max_angle) ? lsysf_do_minus_pow2 : lsysf_do_minus;
    const auto pipe = is_pow2(ts->max_angle) ? lsysf_do_pipe_pow2 : lsysf_do_pipe;

    LSysCmd *ret = (LSysCmd *) malloc((long) max * sizeof(LSysCmd));
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
            f = lsysf_do_slash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsysf_do_backslash;
            param_type = 10;
            ret[n].param.nf = get_number(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsysf_do_at;
            param_type = 10;
            ret[n].param.nf = get_number(&s);
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysf_do_bang;
            break;
        case 'd':
            f = lsysf_do_draw_d;
            break;
        case 'm':
            f = lsysf_do_draw_m;
            break;
        case 'g':
            f = lsysf_do_draw_g;
            break;
        case 'f':
            f = lsysf_do_draw_f;
            break;
        case 'c':
            f = lsysf_do_draw_c;
            num = get_number(&s);
            break;
        case '<':
            f = lsysf_do_draw_lt;
            num = get_number(&s);
            break;
        case '>':
            f = lsysf_do_draw_gt;
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
            ret[n].param.n = (long)num;
        }
        ret[n].param_type = param_type;
        if (++n == max)
        {
            LSysCmd *doubled = (LSysCmd *) malloc((long) max*2*sizeof(LSysCmd));
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

void lsysf_do_sin_cos()
{
    LDouble two_pi = 2.0 * PI;

    LDouble local_aspect = g_screen_aspect * g_logical_screen_x_dots / g_logical_screen_y_dots;
    LDouble two_pi_max = two_pi / g_max_angle;
    s_sin_table_f.resize(g_max_angle);
    s_cos_table_f.resize(g_max_angle);
    for (int i = 0; i < g_max_angle; i++)
    {
        LDouble two_pi_max_i = i * two_pi_max;
        s_sin_table_f[i] = sinl(two_pi_max_i);
        s_cos_table_f[i] = local_aspect * cosl(two_pi_max_i);
    }
}
