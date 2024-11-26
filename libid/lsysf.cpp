// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"
#include "prototyp.h"

#include "lsys.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "lsys_fns.h"
#include "stack_avail.h"
#include "thinking.h"

#include <cmath>
#include <cstring>

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
        LDBL nf;
    } parm;
    char ch;
};

static std::vector<LDBL> s_sin_table_f;
static std::vector<LDBL> s_cos_table_f;
static constexpr LDBL PI_DIV_180{PI / 180.0L};

static LSysFCmd *find_size(LSysFCmd *, LSysTurtleStateF *, LSysFCmd **, int);

static void lsysf_do_plus(LSysTurtleStateF *cmd)
{
    if (cmd->reverse)
    {
        if (++cmd->angle == cmd->maxangle)
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
            cmd->angle = cmd->dmaxangle;
        }
    }
}

// This is the same as lsys_doplus, except maxangle is a power of 2.
static void lsysf_do_plus_pow2(LSysTurtleStateF *cmd)
{
    if (cmd->reverse)
    {
        cmd->angle++;
        cmd->angle &= cmd->dmaxangle;
    }
    else
    {
        cmd->angle--;
        cmd->angle &= cmd->dmaxangle;
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
            cmd->angle = cmd->dmaxangle;
        }
    }
    else
    {
        if (++cmd->angle == cmd->maxangle)
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
        cmd->angle &= cmd->dmaxangle;
    }
    else
    {
        cmd->angle++;
        cmd->angle &= cmd->dmaxangle;
    }
}

static void lsysf_do_slash(LSysTurtleStateF *cmd)
{
    if (cmd->reverse)
    {
        cmd->realangle -= cmd->parm.nf;
    }
    else
    {
        cmd->realangle += cmd->parm.nf;
    }
}

static void lsysf_do_bslash(LSysTurtleStateF *cmd)
{
    if (cmd->reverse)
    {
        cmd->realangle += cmd->parm.nf;
    }
    else
    {
        cmd->realangle -= cmd->parm.nf;
    }
}

static void lsysf_do_at(LSysTurtleStateF *cmd)
{
    cmd->size *= cmd->parm.nf;
}

static void
lsysf_do_pipe(LSysTurtleStateF *cmd)
{
    cmd->angle = (char)(cmd->angle + cmd->maxangle / 2);
    cmd->angle %= cmd->maxangle;
}

static void lsysf_do_pipe_pow2(LSysTurtleStateF *cmd)
{
    cmd->angle += cmd->maxangle >> 1;
    cmd->angle &= cmd->dmaxangle;
}

static void lsysf_do_bang(LSysTurtleStateF *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsysf_do_size_dm(LSysTurtleStateF *cmd)
{
    double angle = (double) cmd->realangle;
    double s;
    double c;

    s = std::sin(angle);
    c = std::cos(angle);

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;

    if (cmd->xpos > cmd->xmax)
    {
        cmd->xmax = cmd->xpos;
    }
    if (cmd->ypos > cmd->ymax)
    {
        cmd->ymax = cmd->ypos;
    }
    if (cmd->xpos < cmd->xmin)
    {
        cmd->xmin = cmd->xpos;
    }
    if (cmd->ypos < cmd->ymin)
    {
        cmd->ymin = cmd->ypos;
    }
}

static void lsysf_do_size_gf(LSysTurtleStateF *cmd)
{
    cmd->xpos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->ypos += cmd->size * s_sin_table_f[(int)cmd->angle];

    if (cmd->xpos > cmd->xmax)
    {
        cmd->xmax = cmd->xpos;
    }
    if (cmd->ypos > cmd->ymax)
    {
        cmd->ymax = cmd->ypos;
    }
    if (cmd->xpos < cmd->xmin)
    {
        cmd->xmin = cmd->xpos;
    }
    if (cmd->ypos < cmd->ymin)
    {
        cmd->ymin = cmd->ypos;
    }
}

static void lsysf_do_draw_d(LSysTurtleStateF *cmd)
{
    double angle = (double) cmd->realangle;
    double s;
    double c;
    int lastx;
    int lasty;
    s = std::sin(angle);
    c = std::cos(angle);

    lastx = (int) cmd->xpos;
    lasty = (int) cmd->ypos;

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;

    driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}

static void lsysf_do_draw_m(LSysTurtleStateF *cmd)
{
    double angle = (double) cmd->realangle;
    double s;
    double c;

    s = std::sin(angle);
    c = std::cos(angle);

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;
}

static void lsysf_do_draw_g(LSysTurtleStateF *cmd)
{
    cmd->xpos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->ypos += cmd->size * s_sin_table_f[(int)cmd->angle];
}

static void lsysf_do_draw_f(LSysTurtleStateF *cmd)
{
    int lastx = (int) cmd->xpos;
    int lasty = (int) cmd->ypos;
    cmd->xpos += cmd->size * s_cos_table_f[(int)cmd->angle];
    cmd->ypos += cmd->size * s_sin_table_f[(int)cmd->angle];
    driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}

static void lsysf_do_draw_c(LSysTurtleStateF *cmd)
{
    cmd->curcolor = (char)(((int) cmd->parm.n) % g_colors);
}

static void lsysf_do_draw_gt(LSysTurtleStateF *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor - cmd->parm.n);
    cmd->curcolor %= g_colors;
    if (cmd->curcolor == 0)
    {
        cmd->curcolor = (char)(g_colors-1);
    }
}

static void lsysf_do_draw_lt(LSysTurtleStateF *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor + cmd->parm.n);
    cmd->curcolor %= g_colors;
    if (cmd->curcolor == 0)
    {
        cmd->curcolor = 1;
    }
}

static LSysFCmd *
find_size(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth)
{
    bool tran;

    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }

    if (stack_avail() < 400)
    {
        // leave some margin for calling subrtns
        ts->stackoflow = true;
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
        tran = false;
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
                    ts->parm.n = command->parm.n;
                    break;
                case 10:
                    ts->parm.nf = command->parm.nf;
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
                LDBL const savesize = ts->size;
                LDBL const saverang = ts->realangle;
                LDBL const savex = ts->xpos;
                LDBL const savey = ts->ypos;
                command = find_size(command+1, ts, rules, depth);
                if (command == nullptr)
                {
                    return nullptr;
                }
                ts->angle = saveang;
                ts->reverse = saverev;
                ts->size = savesize;
                ts->realangle = saverang;
                ts->xpos = savex;
                ts->ypos = savey;
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
    ts->ymin = 0;
    ts->ymax = ts->ymin;
    ts->xmax = ts->ymax;
    ts->xmin = ts->xmax;
    ts->ypos = ts->xmin;
    ts->xpos = ts->ypos;
    ts->counter = 0;
    ts->reverse = ts->counter;
    ts->angle = ts->reverse;
    ts->realangle = 0;
    ts->size = 1;
    LSysFCmd *fsret = find_size(command, ts, rules, depth);
    thinking(0, nullptr); // erase thinking message if any
    LDBL xmin = ts->xmin;
    LDBL xmax = ts->xmax;
    LDBL ymin = ts->ymin;
    LDBL ymax = ts->ymax;
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
    LDBL const locsize = (vert < horiz) ? vert : horiz;

    if (horiz == 1E37)
    {
        ts->xpos = g_logical_screen_x_dots/2;
    }
    else
    {
        ts->xpos = (g_logical_screen_x_dots-locsize*(xmax+xmin))/2;
    }
    if (vert == 1E37)
    {
        ts->ypos = g_logical_screen_y_dots/2;
    }
    else
    {
        ts->ypos = (g_logical_screen_y_dots-locsize*(ymax+ymin))/2;
    }
    ts->size = locsize;

    return true;
}

LSysFCmd *
draw_lsysf(LSysFCmd *command, LSysTurtleStateF *ts, LSysFCmd **rules, int depth)
{
    bool tran;

    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }

    if (stack_avail() < 400)
    {
        // leave some margin for calling subroutines
        ts->stackoflow = true;
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
        tran = false;
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
                    ts->parm.n = command->parm.n;
                    break;
                case 10:
                    ts->parm.nf = command->parm.nf;
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
                LDBL const savesize = ts->size;
                LDBL const saverang = ts->realangle;
                LDBL const savex = ts->xpos;
                LDBL const savey = ts->ypos;
                char const savecolor = ts->curcolor;
                command = draw_lsysf(command+1, ts, rules, depth);
                if (command == nullptr)
                {
                    return nullptr;
                }
                ts->angle = saveang;
                ts->reverse = saverev;
                ts->size = savesize;
                ts->realangle = saverang;
                ts->xpos = savex;
                ts->ypos = savey;
                ts->curcolor = savecolor;
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

    auto const plus = is_pow2(ts->maxangle) ? lsysf_do_plus_pow2 : lsysf_do_plus;
    auto const minus = is_pow2(ts->maxangle) ? lsysf_do_minus_pow2 : lsysf_do_minus;
    auto const pipe = is_pow2(ts->maxangle) ? lsysf_do_pipe_pow2 : lsysf_do_pipe;

    LSysFCmd *ret = (LSysFCmd *) malloc((long) max * sizeof(LSysFCmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
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
                ts->stackoflow = true;
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
        ts->stackoflow = true;
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

    auto const plus = is_pow2(ts->maxangle) ? lsysf_do_plus_pow2 : lsysf_do_plus;
    auto const minus = is_pow2(ts->maxangle) ? lsysf_do_minus_pow2 : lsysf_do_minus;
    auto const pipe = is_pow2(ts->maxangle) ? lsysf_do_pipe_pow2 : lsysf_do_pipe;

    LSysFCmd *ret = (LSysFCmd *) malloc((long) max * sizeof(LSysFCmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(LSysTurtleStateF *) = nullptr;
        LDBL num = 0;
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
                ts->stackoflow = true;
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
        ts->stackoflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(LSysFCmd));
    free(ret);
    return doub;
}

void lsysf_do_sin_cos()
{
    LDBL locaspect;
    LDBL TWOPI = 2.0 * PI;
    LDBL twopimax;
    LDBL twopimaxi;

    locaspect = g_screen_aspect*g_logical_screen_x_dots/g_logical_screen_y_dots;
    twopimax = TWOPI / g_max_angle;
    s_sin_table_f.resize(g_max_angle);
    s_cos_table_f.resize(g_max_angle);
    for (int i = 0; i < g_max_angle; i++)
    {
        twopimaxi = i * twopimax;
        s_sin_table_f[i] = sinl(twopimaxi);
        s_cos_table_f[i] = locaspect * cosl(twopimaxi);
    }
}
