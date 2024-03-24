#include "port.h"
#include "prototyp.h"

#include "lsys.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "fractals.h"
#include "id_data.h"
#include "lsys_fns.h"
#include "thinking.h"

#include <cmath>
#include <cstring>

#ifdef max
#undef max
#endif

struct lsysf_cmd
{
    void (*f)(lsys_turtlestatef *);
    int ptype;
    union
    {
        long n;
        LDBL nf;
    } parm;
    char ch;
};

static std::vector<LDBL> sins_f;
static std::vector<LDBL> coss_f;
static LDBL const PI_DIV_180 = PI/180.0L;

static lsysf_cmd *findsize(lsysf_cmd *, lsys_turtlestatef *, lsysf_cmd **, int);

static void lsysf_doplus(lsys_turtlestatef *cmd)
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
static void lsysf_doplus_pow2(lsys_turtlestatef *cmd)
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

static void lsysf_dominus(lsys_turtlestatef *cmd)
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

static void lsysf_dominus_pow2(lsys_turtlestatef *cmd)
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

static void lsysf_doslash(lsys_turtlestatef *cmd)
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

static void lsysf_dobslash(lsys_turtlestatef *cmd)
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

static void lsysf_doat(lsys_turtlestatef *cmd)
{
    cmd->size *= cmd->parm.nf;
}

static void
lsysf_dopipe(lsys_turtlestatef *cmd)
{
    cmd->angle = (char)(cmd->angle + cmd->maxangle / 2);
    cmd->angle %= cmd->maxangle;
}

static void lsysf_dopipe_pow2(lsys_turtlestatef *cmd)
{
    cmd->angle += cmd->maxangle >> 1;
    cmd->angle &= cmd->dmaxangle;
}

static void lsysf_dobang(lsys_turtlestatef *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsysf_dosizedm(lsys_turtlestatef *cmd)
{
    double angle = (double) cmd->realangle;
    double s, c;

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

static void lsysf_dosizegf(lsys_turtlestatef *cmd)
{
    cmd->xpos += cmd->size * coss_f[(int)cmd->angle];
    cmd->ypos += cmd->size * sins_f[(int)cmd->angle];

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

static void lsysf_dodrawd(lsys_turtlestatef *cmd)
{
    double angle = (double) cmd->realangle;
    double s, c;
    int lastx, lasty;
    s = std::sin(angle);
    c = std::cos(angle);

    lastx = (int) cmd->xpos;
    lasty = (int) cmd->ypos;

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;

    driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}

static void lsysf_dodrawm(lsys_turtlestatef *cmd)
{
    double angle = (double) cmd->realangle;
    double s, c;

    s = std::sin(angle);
    c = std::cos(angle);

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;
}

static void lsysf_dodrawg(lsys_turtlestatef *cmd)
{
    cmd->xpos += cmd->size * coss_f[(int)cmd->angle];
    cmd->ypos += cmd->size * sins_f[(int)cmd->angle];
}

static void lsysf_dodrawf(lsys_turtlestatef *cmd)
{
    int lastx = (int) cmd->xpos;
    int lasty = (int) cmd->ypos;
    cmd->xpos += cmd->size * coss_f[(int)cmd->angle];
    cmd->ypos += cmd->size * sins_f[(int)cmd->angle];
    driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}

static void lsysf_dodrawc(lsys_turtlestatef *cmd)
{
    cmd->curcolor = (char)(((int) cmd->parm.n) % g_colors);
}

static void lsysf_dodrawgt(lsys_turtlestatef *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor - cmd->parm.n);
    cmd->curcolor %= g_colors;
    if (cmd->curcolor == 0)
    {
        cmd->curcolor = (char)(g_colors-1);
    }
}

static void lsysf_dodrawlt(lsys_turtlestatef *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor + cmd->parm.n);
    cmd->curcolor %= g_colors;
    if (cmd->curcolor == 0)
    {
        cmd->curcolor = 1;
    }
}

static lsysf_cmd *
findsize(lsysf_cmd *command, lsys_turtlestatef *ts, lsysf_cmd **rules, int depth)
{
    bool tran;

    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }

    if (stackavail() < 400)
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
            for (lsysf_cmd **rulind = rules; *rulind; rulind++)
            {
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (findsize((*rulind)+1, ts, rules, depth-1) == nullptr)
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
                command = findsize(command+1, ts, rules, depth);
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
lsysf_findscale(lsysf_cmd *command, lsys_turtlestatef *ts, lsysf_cmd **rules, int depth)
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
    lsysf_cmd *fsret = findsize(command, ts, rules, depth);
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

lsysf_cmd *
drawLSysF(lsysf_cmd *command, lsys_turtlestatef *ts, lsysf_cmd **rules, int depth)
{
    bool tran;

    if (g_overflow)       // integer math routines overflowed
    {
        return nullptr;
    }


    if (stackavail() < 400)
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
            for (lsysf_cmd **rulind = rules; *rulind; rulind++)
            {
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (drawLSysF((*rulind)+1, ts, rules, depth-1) == nullptr)
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
                command = drawLSysF(command+1, ts, rules, depth);
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

lsysf_cmd *LSysFSizeTransform(char const *s, lsys_turtlestatef *ts)
{
    int max = 10;
    int n = 0;

    auto const plus = ispow2(ts->maxangle) ? lsysf_doplus_pow2 : lsysf_doplus;
    auto const minus = ispow2(ts->maxangle) ? lsysf_dominus_pow2 : lsysf_dominus;
    auto const pipe = ispow2(ts->maxangle) ? lsysf_dopipe_pow2 : lsysf_dopipe;

    lsysf_cmd *ret = (lsysf_cmd *) malloc((long) max * sizeof(lsysf_cmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(lsys_turtlestatef *) = nullptr;
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
            f = lsysf_doslash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsysf_dobslash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsysf_doat;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s);
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysf_dobang;
            break;
        case 'd':
        case 'm':
            f = lsysf_dosizedm;
            break;
        case 'g':
        case 'f':
            f = lsysf_dosizegf;
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
            lsysf_cmd *doub = (lsysf_cmd *) malloc((long) max*2*sizeof(lsysf_cmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            std::memcpy(doub, ret, max*sizeof(lsysf_cmd));
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

    lsysf_cmd *doub = (lsysf_cmd *) malloc((long) n*sizeof(lsysf_cmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stackoflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(lsysf_cmd));
    free(ret);
    return doub;
}

lsysf_cmd *LSysFDrawTransform(char const *s, lsys_turtlestatef *ts)
{
    int max = 10;
    int n = 0;

    auto const plus = ispow2(ts->maxangle) ? lsysf_doplus_pow2 : lsysf_doplus;
    auto const minus = ispow2(ts->maxangle) ? lsysf_dominus_pow2 : lsysf_dominus;
    auto const pipe = ispow2(ts->maxangle) ? lsysf_dopipe_pow2 : lsysf_dopipe;

    lsysf_cmd *ret = (lsysf_cmd *) malloc((long) max * sizeof(lsysf_cmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        void (*f)(lsys_turtlestatef *) = nullptr;
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
            f = lsysf_doslash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI_DIV_180;
            break;
        case '\\':
            f = lsysf_dobslash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI_DIV_180;
            break;
        case '@':
            f = lsysf_doat;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s);
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysf_dobang;
            break;
        case 'd':
            f = lsysf_dodrawd;
            break;
        case 'm':
            f = lsysf_dodrawm;
            break;
        case 'g':
            f = lsysf_dodrawg;
            break;
        case 'f':
            f = lsysf_dodrawf;
            break;
        case 'c':
            f = lsysf_dodrawc;
            num = getnumber(&s);
            break;
        case '<':
            f = lsysf_dodrawlt;
            num = getnumber(&s);
            break;
        case '>':
            f = lsysf_dodrawgt;
            num = getnumber(&s);
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
            lsysf_cmd *doub = (lsysf_cmd *) malloc((long) max*2*sizeof(lsysf_cmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            std::memcpy(doub, ret, max*sizeof(lsysf_cmd));
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

    lsysf_cmd *doub = (lsysf_cmd *) malloc((long) n*sizeof(lsysf_cmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stackoflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(lsysf_cmd));
    free(ret);
    return doub;
}

void lsysf_dosincos()
{
    LDBL locaspect;
    LDBL TWOPI = 2.0 * PI;
    LDBL twopimax;
    LDBL twopimaxi;

    locaspect = g_screen_aspect*g_logical_screen_x_dots/g_logical_screen_y_dots;
    twopimax = TWOPI / maxangle;
    sins_f.resize(maxangle);
    coss_f.resize(maxangle);
    for (int i = 0; i < maxangle; i++)
    {
        twopimaxi = i * twopimax;
        sins_f[i] = sinl(twopimaxi);
        coss_f[i] = locaspect * cosl(twopimaxi);
    }
}
