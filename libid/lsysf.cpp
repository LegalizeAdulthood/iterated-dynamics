// SPDX-License-Identifier: GPL-3.0-only
//
#include "lsys.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "id_data.h"
#include "lsys_fns.h"
#include "stack_avail.h"
#include "thinking.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#ifdef max
#undef max
#endif

struct LSysFCmd
{
    void (*f)(LSysTurtleStateF *);
    int ptype;
    union
    {
        long n;
        LDouble nf;
    } parm;
    char ch;
};

static std::vector<LDouble> s_sin_table_f;
static std::vector<LDouble> s_cos_table_f;
static constexpr LDouble PI_DIV_180{PI / 180.0L};

static LSysFCmd *find_size(LSysFCmd *, LSysTurtleStateF *, LSysFCmd **, int);

static void lsysf_do_plus(LSysTurtleStateF *cmd)
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
static void lsysf_do_plus_pow2(LSysTurtleStateF *cmd)
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

static void lsysf_do_minus(LSysTurtleStateF *cmd)
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

static void lsysf_do_minus_pow2(LSysTurtleStateF *cmd)
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

static void lsysf_do_slash(LSysTurtleStateF *cmd)
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

static void lsysf_do_bslash(LSysTurtleStateF *cmd)
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

static void lsysf_do_at(LSysTurtleStateF *cmd)
{
    cmd->size *= cmd->param.nf;
}

static void
lsysf_do_pipe(LSysTurtleStateF *cmd)
{
    cmd->angle = (char)(cmd->angle + cmd->max_angle / 2);
    cmd->angle %= cmd->max_angle;
}

static void lsysf_do_pipe_pow2(LSysTurtleStateF *cmd)
{
    cmd->angle += cmd->max_angle >> 1;
    cmd->angle &= cmd->d_max_angle;
}

static void lsysf_do_bang(LSysTurtleStateF *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsysf_do_size_dm(LSysTurtleStateF *cmd)
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

static void lsysf_do_size_gf(LSysTurtleStateF *cmd)
{
    cmd->x_pos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->y_pos += cmd->size * s_sin_table_f[(int)cmd->angle];

    cmd->x_max = std::max(cmd->x_pos, cmd->x_max);
    cmd->y_max = std::max(cmd->y_pos, cmd->y_max);
    cmd->x_min = std::min(cmd->x_pos, cmd->x_min);
    cmd->y_min = std::min(cmd->y_pos, cmd->y_min);
}

static void lsysf_do_draw_d(LSysTurtleStateF *cmd)
{
    double angle = (double) cmd->real_angle;
    double s = std::sin(angle);
    double c = std::cos(angle);

    int lastx = (int) cmd->x_pos;
    int lasty = (int) cmd->y_pos;

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;

    driver_draw_line(lastx, lasty, (int) cmd->x_pos, (int) cmd->y_pos, cmd->curr_color);
}

static void lsysf_do_draw_m(LSysTurtleStateF *cmd)
{
    double angle = (double) cmd->real_angle;

    double s = std::sin(angle);
    double c = std::cos(angle);

    cmd->x_pos += cmd->size * cmd->aspect * c;
    cmd->y_pos += cmd->size * s;
}

static void lsysf_do_draw_g(LSysTurtleStateF *cmd)
{
    cmd->x_pos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->y_pos += cmd->size * s_sin_table_f[(int)cmd->angle];
}

static void lsysf_do_draw_f(LSysTurtleStateF *cmd)
{
    int lastx = (int) cmd->x_pos;
    int lasty = (int) cmd->y_pos;
    cmd->x_pos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->y_pos += cmd->size * s_sin_table_f[(int)cmd->angle];
    driver_draw_line(lastx, lasty, (int) cmd->x_pos, (int) cmd->y_pos, cmd->curr_color);
}

static void lsysf_do_draw_c(LSysTurtleStateF *cmd)
{
    cmd->curr_color = (char)(((int) cmd->param.n) % g_colors);
}

static void lsysf_do_draw_gt(LSysTurtleStateF *cmd)
{
    cmd->curr_color = (char)(cmd->curr_color - cmd->param.n);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = (char)(g_colors-1);
    }
}

static void lsysf_do_draw_lt(LSysTurtleStateF *cmd)
{
    cmd->curr_color = (char)(cmd->curr_color + cmd->param.n);
    cmd->curr_color %= g_colors;
    if (cmd->curr_color == 0)
    {
        cmd->curr_color = 1;
    }
}

static LSysFCmd *
find_size(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth)
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
        if (depth)
        {
            for (LSysFCmd **rulind = rules; *rulind; rulind++)
            {
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (find_size((*rulind)+1, ts, rules, depth-1) == nullptr)
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
                switch (command->ptype)
                {
                case 4:
                    ts->param.n = command->parm.n;
                    break;
                case 10:
                    ts->param.nf = command->parm.nf;
                    break;
                default:
                    break;
                }
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                char const saveang = ts->angle;
                char const saverev = ts->reverse;
                LDouble const savesize = ts->size;
                LDouble const saverang = ts->real_angle;
                LDouble const savex = ts->x_pos;
                LDouble const savey = ts->y_pos;
                command = find_size(command+1, ts, rules, depth);
                if (command == nullptr)
                {
                    return nullptr;
                }
                ts->angle = saveang;
                ts->reverse = saverev;
                ts->size = savesize;
                ts->real_angle = saverang;
                ts->x_pos = savex;
                ts->y_pos = savey;
            }
        }
        command++;
    }
    return command;
}

bool
lsysf_find_scale(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth)
{
    ts->aspect = g_screen_aspect*g_logical_screen_x_dots/g_logical_screen_y_dots;
    ts->y_min = 0;
    ts->y_max = ts->y_min;
    ts->x_max = ts->y_max;
    ts->x_min = ts->x_max;
    ts->y_pos = ts->x_min;
    ts->x_pos = ts->y_pos;
    ts->counter = 0;
    ts->reverse = ts->counter;
    ts->angle = ts->reverse;
    ts->real_angle = 0;
    ts->size = 1;
    LSysFCmd *fsret = find_size(command, ts, rules, depth);
    thinking(0, nullptr); // erase thinking message if any
    LDouble xmin = ts->x_min;
    LDouble xmax = ts->x_max;
    LDouble ymin = ts->y_min;
    LDouble ymax = ts->y_max;
    if (fsret == nullptr)
    {
        return false;
    }
    float horiz;
    if (xmax == xmin)
    {
        horiz = (float)1E37;
    }
    else
    {
        horiz = (float)((g_logical_screen_x_dots-10)/(xmax-xmin));
    }
    float vert;
    if (ymax == ymin)
    {
        vert = (float)1E37;
    }
    else
    {
        vert = (float)((g_logical_screen_y_dots-6) /(ymax-ymin));
    }
    LDouble const locsize = (vert < horiz) ? vert : horiz;

    if (horiz == 1E37)
    {
        ts->x_pos = g_logical_screen_x_dots/2;
    }
    else
    {
        ts->x_pos = (g_logical_screen_x_dots-locsize*(xmax+xmin))/2;
    }
    if (vert == 1E37)
    {
        ts->y_pos = g_logical_screen_y_dots/2;
    }
    else
    {
        ts->y_pos = (g_logical_screen_y_dots-locsize*(ymax+ymin))/2;
    }
    ts->size = locsize;

    return true;
}

LSysFCmd *
draw_lsysf(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth)
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
            for (LSysFCmd **rulind = rules; *rulind; rulind++)
            {
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (draw_lsysf((*rulind)+1, ts, rules, depth-1) == nullptr)
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
                switch (command->ptype)
                {
                case 4:
                    ts->param.n = command->parm.n;
                    break;
                case 10:
                    ts->param.nf = command->parm.nf;
                    break;
                default:
                    break;
                }
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                char const saveang = ts->angle;
                char const saverev = ts->reverse;
                LDouble const savesize = ts->size;
                LDouble const saverang = ts->real_angle;
                LDouble const savex = ts->x_pos;
                LDouble const savey = ts->y_pos;
                char const savecolor = ts->curr_color;
                command = draw_lsysf(command+1, ts, rules, depth);
                if (command == nullptr)
                {
                    return nullptr;
                }
                ts->angle = saveang;
                ts->reverse = saverev;
                ts->size = savesize;
                ts->real_angle = saverang;
                ts->x_pos = savex;
                ts->y_pos = savey;
                ts->curr_color = savecolor;
            }
        }
        command++;
    }
    return command;
}

LSysFCmd *lsysf_size_transform(char const *s, LSysTurtleStateF *ts)
{
    int max = 10;
    int n = 0;

    auto const plus = is_pow2(ts->max_angle) ? lsysf_do_plus_pow2 : lsysf_do_plus;
    auto const minus = is_pow2(ts->max_angle) ? lsysf_do_minus_pow2 : lsysf_do_minus;
    auto const pipe = is_pow2(ts->max_angle) ? lsysf_do_pipe_pow2 : lsysf_do_pipe;

    LSysFCmd *ret = (LSysFCmd *) malloc((long) max * sizeof(LSysFCmd));
    if (ret == nullptr)
    {
        ts->stack_overflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleStateF *) = nullptr;
        long num = 0;
        int ptype = 4;
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
            ptype = 10;
            ret[n].parm.nf = get_number(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsysf_do_bslash;
            ptype = 10;
            ret[n].parm.nf = get_number(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsysf_do_at;
            ptype = 10;
            ret[n].parm.nf = get_number(&s);
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
        if (ptype == 4)
        {
            ret[n].parm.n = num;
        }
        ret[n].ptype = ptype;
        if (++n == max)
        {
            LSysFCmd *doub = (LSysFCmd *) malloc((long) max*2*sizeof(LSysFCmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stack_overflow = true;
                return nullptr;
            }
            std::memcpy(doub, ret, max*sizeof(LSysFCmd));
            free(ret);
            ret = doub;
            max <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].parm.n = 0;
    n++;

    LSysFCmd *doub = (LSysFCmd *) malloc((long) n*sizeof(LSysFCmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stack_overflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(LSysFCmd));
    free(ret);
    return doub;
}

LSysFCmd *lsysf_draw_transform(char const *s, LSysTurtleStateF *ts)
{
    int max = 10;
    int n = 0;

    auto const plus = is_pow2(ts->max_angle) ? lsysf_do_plus_pow2 : lsysf_do_plus;
    auto const minus = is_pow2(ts->max_angle) ? lsysf_do_minus_pow2 : lsysf_do_minus;
    auto const pipe = is_pow2(ts->max_angle) ? lsysf_do_pipe_pow2 : lsysf_do_pipe;

    LSysFCmd *ret = (LSysFCmd *) malloc((long) max * sizeof(LSysFCmd));
    if (ret == nullptr)
    {
        ts->stack_overflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleStateF *) = nullptr;
        LDouble num = 0;
        int ptype = 4;
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
            ptype = 10;
            ret[n].parm.nf = get_number(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsysf_do_bslash;
            ptype = 10;
            ret[n].parm.nf = get_number(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsysf_do_at;
            ptype = 10;
            ret[n].parm.nf = get_number(&s);
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
        if (ptype == 4)
        {
            ret[n].parm.n = (long)num;
        }
        ret[n].ptype = ptype;
        if (++n == max)
        {
            LSysFCmd *doub = (LSysFCmd *) malloc((long) max*2*sizeof(LSysFCmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stack_overflow = true;
                return nullptr;
            }
            std::memcpy(doub, ret, max*sizeof(LSysFCmd));
            free(ret);
            ret = doub;
            max <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].parm.n = 0;
    n++;

    LSysFCmd *doub = (LSysFCmd *) malloc((long) n*sizeof(LSysFCmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stack_overflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(LSysFCmd));
    free(ret);
    return doub;
}

void lsysf_do_sin_cos()
{
    LDouble TWOPI = 2.0 * PI;

    LDouble locaspect = g_screen_aspect * g_logical_screen_x_dots / g_logical_screen_y_dots;
    LDouble twopimax = TWOPI / g_max_angle;
    s_sin_table_f.resize(g_max_angle);
    s_cos_table_f.resize(g_max_angle);
    for (int i = 0; i < g_max_angle; i++)
    {
        LDouble twopimaxi = i * twopimax;
        s_sin_table_f[i] = sinl(twopimaxi);
        s_cos_table_f[i] = locaspect * cosl(twopimaxi);
    }
}
