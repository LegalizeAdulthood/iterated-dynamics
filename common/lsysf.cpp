#include <float.h>
#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#include "port.h"
#include "prototyp.h"
#include "lsys.h"
#include "drivers.h"

#ifdef max
#undef max
#endif

struct lsys_cmd
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

#define sins_f ((LDBL *)(boxy))
#define coss_f (((LDBL *)(boxy)+50))

static lsys_cmd *findsize(lsys_cmd *, lsys_turtlestatef *, lsys_cmd **, int);

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_doplus(lsys_turtlestatef *cmd)
{
    if (cmd->reverse)
    {
        if (++cmd->angle == cmd->maxangle)
            cmd->angle = 0;
    }
    else
    {
        if (cmd->angle)
            cmd->angle--;
        else
            cmd->angle = cmd->dmaxangle;
    }
}
#else
extern void lsysf_doplus(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
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
#else
extern void lsysf_doplus_pow2(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dominus(lsys_turtlestatef *cmd)
{
    if (cmd->reverse)
    {
        if (cmd->angle)
            cmd->angle--;
        else
            cmd->angle = cmd->dmaxangle;
    }
    else
    {
        if (++cmd->angle == cmd->maxangle)
            cmd->angle = 0;
    }
}
#else
extern void lsysf_dominus(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
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
#else
extern void lsysf_dominus_pow2(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_doslash(lsys_turtlestatef *cmd)
{
    if (cmd->reverse)
        cmd->realangle -= cmd->parm.nf;
    else
        cmd->realangle += cmd->parm.nf;
}
#else
extern void lsysf_doslash(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dobslash(lsys_turtlestatef *cmd)
{
    if (cmd->reverse)
        cmd->realangle += cmd->parm.nf;
    else
        cmd->realangle -= cmd->parm.nf;
}
#else
extern void lsysf_dobslash(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_doat(lsys_turtlestatef *cmd)
{
    cmd->size *= cmd->parm.nf;
}
#else
extern void lsysf_doat(lsys_turtlestatef *cmd, long n);
#endif

static void
lsysf_dopipe(lsys_turtlestatef *cmd)
{
    cmd->angle = (char)(cmd->angle + cmd->maxangle / 2);
    cmd->angle %= cmd->maxangle;
}

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dopipe_pow2(lsys_turtlestatef *cmd)
{
    cmd->angle += cmd->maxangle >> 1;
    cmd->angle &= cmd->dmaxangle;
}
#else
extern void lsysf_dopipe_pow2(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dobang(lsys_turtlestatef *cmd)
{
    cmd->reverse = ! cmd->reverse;
}
#else
extern void lsysf_dobang(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dosizedm(lsys_turtlestatef *cmd)
{
    double angle = (double) cmd->realangle;
    double s, c;

    s = sin(angle);
    c = cos(angle);

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;

    if (cmd->xpos > cmd->xmax)
        cmd->xmax = cmd->xpos;
    if (cmd->ypos > cmd->ymax)
        cmd->ymax = cmd->ypos;
    if (cmd->xpos < cmd->xmin)
        cmd->xmin = cmd->xpos;
    if (cmd->ypos < cmd->ymin)
        cmd->ymin = cmd->ypos;
}
#else
extern void lsysf_dosizedm(lsys_turtlestatef *cmd, long n);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dosizegf(lsys_turtlestatef *cmd)
{
    cmd->xpos += cmd->size * coss_f[(int)cmd->angle];
    cmd->ypos += cmd->size * sins_f[(int)cmd->angle];

    if (cmd->xpos > cmd->xmax)
        cmd->xmax = cmd->xpos;
    if (cmd->ypos > cmd->ymax)
        cmd->ymax = cmd->ypos;
    if (cmd->xpos < cmd->xmin)
        cmd->xmin = cmd->xpos;
    if (cmd->ypos < cmd->ymin)
        cmd->ymin = cmd->ypos;
}
#else
extern void lsysf_dosizegf(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawd(lsys_turtlestatef *cmd)
{
    double angle = (double) cmd->realangle;
    double s, c;
    int lastx, lasty;
    s = sin(angle);
    c = cos(angle);

    lastx = (int) cmd->xpos;
    lasty = (int) cmd->ypos;

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;

    driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}
#else
extern void lsysf_dodrawd(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawm(lsys_turtlestatef *cmd)
{
    double angle = (double) cmd->realangle;
    double s, c;

    s = sin(angle);
    c = cos(angle);

    cmd->xpos += cmd->size * cmd->aspect * c;
    cmd->ypos += cmd->size * s;
}
#else
extern void lsysf_dodrawm(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawg(lsys_turtlestatef *cmd)
{
    cmd->xpos += cmd->size * coss_f[(int)cmd->angle];
    cmd->ypos += cmd->size * sins_f[(int)cmd->angle];
}
#else
extern void lsysf_dodrawg(lsys_turtlestatef *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysf_dodrawf(lsys_turtlestatef *cmd)
{
    int lastx = (int) cmd->xpos;
    int lasty = (int) cmd->ypos;
    cmd->xpos += cmd->size * coss_f[(int)cmd->angle];
    cmd->ypos += cmd->size * sins_f[(int)cmd->angle];
    driver_draw_line(lastx, lasty, (int) cmd->xpos, (int) cmd->ypos, cmd->curcolor);
}
#else
extern void lsysf_dodrawf(lsys_turtlestatef *cmd);
#endif

static void lsysf_dodrawc(lsys_turtlestatef *cmd)
{
    cmd->curcolor = (char)(((int) cmd->parm.n) % colors);
}

static void lsysf_dodrawgt(lsys_turtlestatef *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor - cmd->parm.n);
    cmd->curcolor %= colors;
    if (cmd->curcolor == 0)
        cmd->curcolor = (char)(colors-1);
}

static void lsysf_dodrawlt(lsys_turtlestatef *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor + cmd->parm.n);
    cmd->curcolor %= colors;
    if (cmd->curcolor == 0)
        cmd->curcolor = 1;
}

static lsys_cmd *
findsize(lsys_cmd *command, lsys_turtlestatef *ts, lsys_cmd **rules, int depth)
{
    bool tran;

    if (overflow)     // integer math routines overflowed
        return nullptr;

    if (stackavail() < 400)
    { // leave some margin for calling subrtns
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
            for (lsys_cmd **rulind = rules; *rulind; rulind++)
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (findsize((*rulind)+1, ts, rules, depth-1) == nullptr)
                        return (nullptr);
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
                char saveang, saverev;
                LDBL savesize, savex, savey, saverang;

                lsys_donefpu(ts);
                saveang = ts->angle;
                saverev = ts->reverse;
                savesize = ts->size;
                saverang = ts->realangle;
                savex = ts->xpos;
                savey = ts->ypos;
                lsys_prepfpu(ts);
                command = findsize(command+1, ts, rules, depth);
                if (command == nullptr)
                    return (nullptr);
                lsys_donefpu(ts);
                ts->angle = saveang;
                ts->reverse = saverev;
                ts->size = savesize;
                ts->realangle = saverang;
                ts->xpos = savex;
                ts->ypos = savey;
                lsys_prepfpu(ts);
            }
        }
        command++;
    }
    return command;
}

bool
lsysf_findscale(lsys_cmd *command, lsys_turtlestatef *ts, lsys_cmd **rules, int depth)
{
    float horiz, vert;
    LDBL xmin, xmax, ymin, ymax;
    LDBL locsize;
    LDBL locaspect;
    lsys_cmd *fsret;

    locaspect = screenaspect*xdots/ydots;
    ts->aspect = locaspect;
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
    lsys_prepfpu(ts);
    fsret = findsize(command, ts, rules, depth);
    lsys_donefpu(ts);
    thinking(0, nullptr); // erase thinking message if any
    xmin = ts->xmin;
    xmax = ts->xmax;
    ymin = ts->ymin;
    ymax = ts->ymax;
    if (fsret == nullptr)
        return false;
    if (xmax == xmin)
        horiz = (float)1E37;
    else
        horiz = (float)((xdots-10)/(xmax-xmin));
    if (ymax == ymin)
        vert = (float)1E37;
    else
        vert = (float)((ydots-6) /(ymax-ymin));
    locsize = (vert < horiz) ? vert : horiz;

    if (horiz == 1E37)
        ts->xpos = xdots/2;
    else
        ts->xpos = (xdots-locsize*(xmax+xmin))/2;
    if (vert == 1E37)
        ts->ypos = ydots/2;
    else
        ts->ypos = (ydots-locsize*(ymax+ymin))/2;
    ts->size = locsize;

    return true;
}

lsys_cmd *
drawLSysF(lsys_cmd *command, lsys_turtlestatef *ts, lsys_cmd **rules, int depth)
{
    bool tran;

    if (overflow)     // integer math routines overflowed
        return nullptr;


    if (stackavail() < 400)
    { // leave some margin for calling subroutines
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
            for (lsys_cmd **rulind = rules; *rulind; rulind++)
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (drawLSysF((*rulind)+1, ts, rules, depth-1) == nullptr)
                        return nullptr;
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
                char saveang, saverev, savecolor;
                LDBL savesize, savex, savey, saverang;

                lsys_donefpu(ts);
                saveang = ts->angle;
                saverev = ts->reverse;
                savesize = ts->size;
                saverang = ts->realangle;
                savex = ts->xpos;
                savey = ts->ypos;
                savecolor = ts->curcolor;
                lsys_prepfpu(ts);
                command = drawLSysF(command+1, ts, rules, depth);
                if (command == nullptr)
                    return (nullptr);
                lsys_donefpu(ts);
                ts->angle = saveang;
                ts->reverse = saverev;
                ts->size = savesize;
                ts->realangle = saverang;
                ts->xpos = savex;
                ts->ypos = savey;
                ts->curcolor = savecolor;
                lsys_prepfpu(ts);
            }
        }
        command++;
    }
    return command;
}

lsys_cmd *
LSysFSizeTransform(char *s, lsys_turtlestatef *ts)
{
    lsys_cmd *ret;
    lsys_cmd *doub;
    int max = 10;
    int n = 0;
    void (*f)(lsys_turtlestatef *);
    long num;
    int ptype;
    double PI180 = PI / 180.0;

    void (*plus)(lsys_turtlestatef *) = (ispow2(ts->maxangle)) ? lsysf_doplus_pow2 : lsysf_doplus;
    void (*minus)(lsys_turtlestatef *) = (ispow2(ts->maxangle)) ? lsysf_dominus_pow2 : lsysf_dominus;
    void (*pipe)(lsys_turtlestatef *) = (ispow2(ts->maxangle)) ? lsysf_dopipe_pow2 : lsysf_dopipe;

    void (*slash)(lsys_turtlestatef *) =  lsysf_doslash;
    void (*bslash)(lsys_turtlestatef *) = lsysf_dobslash;
    void (*at)(lsys_turtlestatef *) =     lsysf_doat;
    void (*dogf)(lsys_turtlestatef *) =   lsysf_dosizegf;

    ret = (lsys_cmd *) malloc((long) max * sizeof(lsys_cmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        f = nullptr;
        num = 0;
        ptype = 4;
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
            f = slash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI180;
            break;
        case '\\':
            f = bslash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI180;
            break;
        case '@':
            f = at;
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
            f = dogf;
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
#if defined(XFRACT)
        ret[n].f = (void (*)(lsys_turtlestatef *))f;
#else
        ret[n].f = (void (*)(lsys_turtlestatef *))f;
#endif
        if (ptype == 4)
            ret[n].parm.n = num;
        ret[n].ptype = ptype;
        if (++n == max)
        {
            doub = (lsys_cmd *) malloc((long) max*2*sizeof(lsys_cmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            memcpy(doub, ret, max*sizeof(lsys_cmd));
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

    doub = (lsys_cmd *) malloc((long) n*sizeof(lsys_cmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stackoflow = true;
        return nullptr;
    }
    memcpy(doub, ret, n*sizeof(lsys_cmd));
    free(ret);
    return doub;
}

lsys_cmd *
LSysFDrawTransform(char *s, lsys_turtlestatef *ts)
{
    lsys_cmd *ret;
    lsys_cmd *doub;
    int max = 10;
    int n = 0;
    void (*f)(lsys_turtlestatef *);
    LDBL num;
    int ptype;
    LDBL PI180 = PI / 180.0;

    void (*plus)(lsys_turtlestatef *) = (ispow2(ts->maxangle)) ? lsysf_doplus_pow2 : lsysf_doplus;
    void (*minus)(lsys_turtlestatef *) = (ispow2(ts->maxangle)) ? lsysf_dominus_pow2 : lsysf_dominus;
    void (*pipe)(lsys_turtlestatef *) = (ispow2(ts->maxangle)) ? lsysf_dopipe_pow2 : lsysf_dopipe;

    void (*slash)(lsys_turtlestatef *) =  lsysf_doslash;
    void (*bslash)(lsys_turtlestatef *) = lsysf_dobslash;
    void (*at)(lsys_turtlestatef *) =     lsysf_doat;
    void (*drawg)(lsys_turtlestatef *) =  lsysf_dodrawg;

    ret = (lsys_cmd *) malloc((long) max * sizeof(lsys_cmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        f = nullptr;
        num = 0;
        ptype = 4;
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
            f = slash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI180;
            break;
        case '\\':
            f = bslash;
            ptype = 10;
            ret[n].parm.nf = getnumber(&s) * PI180;
            break;
        case '@':
            f = at;
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
            f = drawg;
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
#if defined(XFRACT)
        ret[n].f = (void (*)(lsys_turtlestatef *))f;
#else
        ret[n].f = (void (*)(lsys_turtlestatef *))f;
#endif
        if (ptype == 4)
            ret[n].parm.n = (long)num;
        ret[n].ptype = ptype;
        if (++n == max)
        {
            doub = (lsys_cmd *) malloc((long) max*2*sizeof(lsys_cmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            memcpy(doub, ret, max*sizeof(lsys_cmd));
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

    doub = (lsys_cmd *) malloc((long) n*sizeof(lsys_cmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stackoflow = true;
        return nullptr;
    }
    memcpy(doub, ret, n*sizeof(lsys_cmd));
    free(ret);
    return doub;
}

void lsysf_dosincos()
{
    LDBL locaspect;
    LDBL TWOPI = 2.0 * PI;
    LDBL twopimax;
    LDBL twopimaxi;

    locaspect = screenaspect*xdots/ydots;
    twopimax = TWOPI / maxangle;
    for (int i = 0; i < maxangle; i++)
    {
        twopimaxi = i * twopimax;
        sins_f[i] = sinl(twopimaxi);
        coss_f[i] = locaspect * cosl(twopimaxi);
    }
}
