#include <float.h>
#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#include "port.h"
#include "prototyp.h"
#include "lsys.h"
#include "drivers.h"

struct lsys_cmd
{
    void (*f)(lsys_turtlestatei *);
    long n;
    char ch;
};

static bool readLSystemFile(char *str);
static void free_rules_mem();
static int rule_present(char symbol);
static bool save_rule(char *rule, char **saveptr);
static bool append_rule(char *rule, int index);
static void free_lcmds();
static lsys_cmd *findsize(lsys_cmd *, lsys_turtlestatei *, lsys_cmd **, int);
static lsys_cmd *drawLSysI(lsys_cmd *command, lsys_turtlestatei *ts, lsys_cmd **rules, int depth);
static bool lsysi_findscale(lsys_cmd *command, lsys_turtlestatei *ts, lsys_cmd **rules, int depth);
static lsys_cmd *LSysISizeTransform(char *s, lsys_turtlestatei *ts);
static lsys_cmd *LSysIDrawTransform(char *s, lsys_turtlestatei *ts);
static void lsysi_dosincos();

static void lsysi_doslash(lsys_turtlestatei *cmd);
static void lsysi_dobslash(lsys_turtlestatei *cmd);
static void lsysi_doat(lsys_turtlestatei *cmd);
static void lsysi_dopipe(lsys_turtlestatei *cmd);
static void lsysi_dosizedm(lsys_turtlestatei *cmd);
static void lsysi_dosizegf(lsys_turtlestatei *cmd);
static void lsysi_dodrawd(lsys_turtlestatei *cmd);
static void lsysi_dodrawm(lsys_turtlestatei *cmd);
static void lsysi_dodrawg(lsys_turtlestatei *cmd);
static void lsysi_dodrawf(lsys_turtlestatei *cmd);
static void lsysi_dodrawc(lsys_turtlestatei *cmd);
static void lsysi_dodrawgt(lsys_turtlestatei *cmd);
static void lsysi_dodrawlt(lsys_turtlestatei *cmd);

#define sins ((long *)(boxy))
#define coss (((long *)(boxy)+50)) // 50 after the start of sins
static char *ruleptrs[MAXRULES];
static lsys_cmd *rules2[MAXRULES];
char maxangle;
static bool loaded = false;


bool ispow2(int n)
{
    return (n == (n & -n));
}

LDBL getnumber(char **str)
{
    char numstr[30];
    LDBL ret;
    int i, root, inverse;

    root = 0;
    inverse = 0;
    strcpy(numstr, "");
    (*str)++;
    switch (**str)
    {
    case 'q':
        root = 1;
        (*str)++;
        break;
    case 'i':
        inverse = 1;
        (*str)++;
        break;
    }
    switch (**str)
    {
    case 'q':
        root = 1;
        (*str)++;
        break;
    case 'i':
        inverse = 1;
        (*str)++;
        break;
    }
    i = 0;
    while ((**str <= '9' && **str >= '0') || **str == '.')
    {
        numstr[i++] = **str;
        (*str)++;
    }
    (*str)--;
    numstr[i] = 0;
    ret = atof(numstr);
    if (ret <= 0.0) // this is a sanity check
        return 0;
    if (root)
        ret = sqrtl(ret);
    if (inverse)
        ret = 1.0/ret;
    return ret;
}

static bool readLSystemFile(char *str)
{
    int c;
    char **rulind;
    int err = 0;
    bool check = false;
    char inline1[MAX_LSYS_LINE_LEN+1], fixed[MAX_LSYS_LINE_LEN+1], *word;
    FILE *infile;
    char msgbuf[481]; // enough for 6 full lines

    if (find_file_item(LFileName, str, &infile, 2))
        return true;
    while ((c = fgetc(infile)) != '{')
        if (c == EOF)
            return true;
    maxangle = 0;
    for (int linenum = 0; linenum < MAXRULES; ++linenum)
        ruleptrs[linenum] = nullptr;
    rulind = &ruleptrs[1];
    msgbuf[0] = 0;;

    int linenum = 0;
    while (file_gets(inline1, MAX_LSYS_LINE_LEN, infile) > -1)  // Max line length chars
    {
        linenum++;
        word = strchr(inline1, ';');
        if (word != nullptr) // strip comment
            *word = 0;
        strlwr(inline1);

        if ((int)strspn(inline1, " \t\n") < (int)strlen(inline1)) // not a blank line
        {
            word = strtok(inline1, " =\t\n");
            if (!strcmp(word, "axiom"))
            {
                if (save_rule(strtok(nullptr, " \t\n"), &ruleptrs[0]))
                {
                    strcat(msgbuf, "Error:  out of memory\n");
                    ++err;
                    break;
                }
                check = true;
            }
            else if (!strcmp(word, "angle"))
            {
                maxangle = (char)atoi(strtok(nullptr, " \t\n"));
                check = true;
            }
            else if (!strcmp(word, "}"))
                break;
            else if (!word[1])
            {
                char *temp;
                int index;
                bool memerr = false;

                if (strchr("+-/\\@|!c<>][", *word))
                {
                    sprintf(&msgbuf[strlen(msgbuf)],
                            "Syntax error line %d: Redefined reserved symbol %s\n", linenum, word);
                    ++err;
                    break;
                }
                temp = strtok(nullptr, " =\t\n");
                index = rule_present(*word);

                if (!index)
                {
                    strcpy(fixed, word);
                    if (temp)
                        strcat(fixed, temp);
                    memerr = save_rule(fixed, rulind++);
                }
                else if (temp)
                {
                    strcpy(fixed, temp);
                    memerr = append_rule(fixed, index);
                }
                if (memerr)
                {
                    strcat(msgbuf, "Error:  out of memory\n");
                    ++err;
                    break;
                }
                check = true;
            }
            else if (err < 6)
            {
                sprintf(&msgbuf[strlen(msgbuf)],
                        "Syntax error line %d: %s\n", linenum, word);
                ++err;
            }
            if (check)
            {
                check = false;
                word = strtok(nullptr, " \t\n");
                if (word != nullptr)
                    if (err < 6)
                    {
                        sprintf(&msgbuf[strlen(msgbuf)],
                                "Extra text after command line %d: %s\n", linenum, word);
                        ++err;
                    }
            }
        }
    }
    fclose(infile);
    if (!ruleptrs[0] && err < 6)
    {
        strcat(msgbuf, "Error:  no axiom\n");
        ++err;
    }
    if ((maxangle < 3||maxangle > 50) && err < 6)
    {
        strcat(msgbuf, "Error:  illegal or missing angle\n");
        ++err;
    }
    if (err)
    {
        msgbuf[strlen(msgbuf)-1] = 0; // strip trailing \n
        stopmsg(STOPMSG_NONE, msgbuf);
        return true;
    }
    *rulind = nullptr;
    return false;
}

int Lsystem()
{
    int order;
    lsys_cmd **sc;
    bool stackoflow = false;

    if (!loaded && LLoad())
        return -1;

    overflow = false;           // reset integer math overflow flag

    order = (int)param[0];
    if (order <= 0)
        order = 0;
    if (usr_floatflag)
        overflow = true;
    else
    {
        lsys_turtlestatei ts;

        ts.stackoflow = false;
        ts.maxangle = maxangle;
        ts.dmaxangle = (char)(maxangle - 1);

        sc = rules2;
        for (char **rulesc = ruleptrs; *rulesc; rulesc++)
            *sc++ = LSysISizeTransform(*rulesc, &ts);
        *sc = nullptr;

        lsysi_dosincos();
        if (lsysi_findscale(rules2[0], &ts, &rules2[1], order))
        {
            ts.reverse = 0;
            ts.angle = ts.reverse;
            ts.realangle = ts.angle;

            free_lcmds();
            sc = rules2;
            for (char **rulesc = ruleptrs; *rulesc; rulesc++)
                *sc++ = LSysIDrawTransform(*rulesc, &ts);
            *sc = nullptr;

            // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
            ts.curcolor = 15;
            if (ts.curcolor > colors)
                ts.curcolor = (char)(colors-1);
            drawLSysI(rules2[0], &ts, &rules2[1], order);
        }
        stackoflow = ts.stackoflow;
    }

    if (stackoflow)
    {
        stopmsg(STOPMSG_NONE, "insufficient memory, try a lower order");
    }
    else if (overflow)
    {
        lsys_turtlestatef ts;

        overflow = false;

        ts.stackoflow = false;
        ts.maxangle = maxangle;
        ts.dmaxangle = (char)(maxangle - 1);

        sc = rules2;
        for (char **rulesc = ruleptrs; *rulesc; rulesc++)
            *sc++ = LSysFSizeTransform(*rulesc, &ts);
        *sc = nullptr;

        lsysf_dosincos();
        if (lsysf_findscale(rules2[0], &ts, &rules2[1], order))
        {
            ts.reverse = 0;
            ts.angle = ts.reverse;
            ts.realangle = ts.angle;

            free_lcmds();
            sc = rules2;
            for (char **rulesc = ruleptrs; *rulesc; rulesc++)
                *sc++ = LSysFDrawTransform(*rulesc, &ts);
            *sc = nullptr;

            // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
            ts.curcolor = 15;
            if (ts.curcolor > colors)
                ts.curcolor = (char)(colors-1);
            lsys_prepfpu(&ts);
            drawLSysF(rules2[0], &ts, &rules2[1], order);
            lsys_donefpu(&ts);
        }
        overflow = false;
    }
    free_rules_mem();
    free_lcmds();
    loaded = false;
    return 0;
}

bool LLoad()
{
    if (readLSystemFile(LName))
    { // error occurred
        free_rules_mem();
        loaded = false;
        return true;
    }
    loaded = true;
    return false;
}

static void free_rules_mem()
{
    for (int i = 0; i < MAXRULES; ++i)
        if (ruleptrs[i])
            free(ruleptrs[i]);
}

static int rule_present(char symbol)
{
    int i;
    for (i = 1; i < MAXRULES && ruleptrs[i] && *ruleptrs[i] != symbol ; i++)
        ;
    return (i < MAXRULES && ruleptrs[i]) ? i : 0;
}

static bool save_rule(char *rule, char **saveptr)
{
    int i;
    char *tmpfar;
    i = (int) strlen(rule)+1;
    tmpfar = (char *) malloc(i);
    if (tmpfar == nullptr)
    {
        return true;
    }
    *saveptr = tmpfar;
    while (--i >= 0)
        *(tmpfar++) = *(rule++);
    return false;
}

static bool append_rule(char *rule, int index)
{
    char *dst, *old, *sav;

    sav = ruleptrs[index];
    old = sav;
    int i;
    for (i = 0; *(old++); i++)
        ;
    int j = (int) strlen(rule) + 1;
    dst = (char *)malloc((long)(i + j));
    if (dst == nullptr)
        return true;

    old = sav;
    ruleptrs[index] = dst;
    while (i-- > 0)
        *(dst++) = *(old++);
    while (j-- > 0)
        *(dst++) = *(rule++);
    free(sav);
    return false;
}

static void free_lcmds()
{
    lsys_cmd **sc = rules2;

    while (*sc)
        free(*sc++);
}

#if defined(XFRACT) || defined(_WIN32)
#define lsysi_doslash_386 lsysi_doslash
#define lsysi_dobslash_386 lsysi_dobslash
#define lsys_doat lsysi_doat
#define lsys_dosizegf lsysi_dosizegf
#define lsys_dodrawg lsysi_dodrawg
void lsys_prepfpu(lsys_turtlestatef *x) { }
void lsys_donefpu(lsys_turtlestatef *x) { }
#endif

// integer specific routines

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_doplus(lsys_turtlestatei *cmd)
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
extern void lsysi_doplus(lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
// This is the same as lsys_doplus, except maxangle is a power of 2.
static void lsysi_doplus_pow2(lsys_turtlestatei *cmd)
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
extern void lsysi_doplus_pow2(lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dominus(lsys_turtlestatei *cmd)
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
extern void lsysi_dominus(lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dominus_pow2(lsys_turtlestatei *cmd)
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
extern void lsysi_dominus_pow2(lsys_turtlestatei *cmd);
#endif

static void lsysi_doslash(lsys_turtlestatei *cmd)
{
    if (cmd->reverse)
        cmd->realangle -= cmd->num;
    else
        cmd->realangle += cmd->num;
}

#if !defined(XFRACT) && !defined(_WIN32)
extern void lsysi_doslash_386(lsys_turtlestatei *cmd);
#endif

static void lsysi_dobslash(lsys_turtlestatei *cmd)
{
    if (cmd->reverse)
        cmd->realangle += cmd->num;
    else
        cmd->realangle -= cmd->num;
}

#if !defined(XFRACT) && !defined(_WIN32)
extern void lsysi_dobslash_386(lsys_turtlestatei *cmd);
#endif

static void lsysi_doat(lsys_turtlestatei *cmd)
{
    cmd->size = multiply(cmd->size, cmd->num, 19);
}

static void lsysi_dopipe(lsys_turtlestatei *cmd)
{
    cmd->angle = (char)(cmd->angle + (char)(cmd->maxangle / 2));
    cmd->angle %= cmd->maxangle;
}

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dopipe_pow2(lsys_turtlestatei *cmd)
{
    cmd->angle += cmd->maxangle >> 1;
    cmd->angle &= cmd->dmaxangle;
}
#else
extern void lsysi_dopipe_pow2(lsys_turtlestatei *cmd);
#endif

#if defined(XFRACT) || defined(_WIN32)
static void lsysi_dobang(lsys_turtlestatei *cmd)
{
    cmd->reverse = ! cmd->reverse;
}
#else
extern void lsysi_dobang(lsys_turtlestatei *cmd);
#endif

static void lsysi_dosizedm(lsys_turtlestatei *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s, c;
    long fixedsin, fixedcos;

    FPUsincos(&angle, &s, &c);
    fixedsin = (long)(s * FIXEDLT1);
    fixedcos = (long)(c * FIXEDLT1);

    cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));

    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    if (cmd->xpos > cmd->xmax)
        cmd->xmax = cmd->xpos;
    if (cmd->ypos > cmd->ymax)
        cmd->ymax = cmd->ypos;
    if (cmd->xpos < cmd->xmin)
        cmd->xmin = cmd->xpos;
    if (cmd->ypos < cmd->ymin)
        cmd->ymin = cmd->ypos;
}

static void lsysi_dosizegf(lsys_turtlestatei *cmd)
{
    cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
    if (cmd->xpos > cmd->xmax)
        cmd->xmax = cmd->xpos;
    if (cmd->ypos > cmd->ymax)
        cmd->ymax = cmd->ypos;
    if (cmd->xpos < cmd->xmin)
        cmd->xmin = cmd->xpos;
    if (cmd->ypos < cmd->ymin)
        cmd->ymin = cmd->ypos;
}

static void lsysi_dodrawd(lsys_turtlestatei *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s, c;
    long fixedsin, fixedcos;
    int lastx, lasty;

    FPUsincos(&angle, &s, &c);
    fixedsin = (long)(s * FIXEDLT1);
    fixedcos = (long)(c * FIXEDLT1);

    lastx = (int)(cmd->xpos >> 19);
    lasty = (int)(cmd->ypos >> 19);
    cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    driver_draw_line(lastx, lasty, (int)(cmd->xpos >> 19), (int)(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_dodrawm(lsys_turtlestatei *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s, c;
    long fixedsin, fixedcos;

    FPUsincos(&angle, &s, &c);
    fixedsin = (long)(s * FIXEDLT1);
    fixedcos = (long)(c * FIXEDLT1);

    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
}

static void lsysi_dodrawg(lsys_turtlestatei *cmd)
{
    cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
}

static void lsysi_dodrawf(lsys_turtlestatei *cmd)
{
    int lastx = (int)(cmd->xpos >> 19);
    int lasty = (int)(cmd->ypos >> 19);
    cmd->xpos = cmd->xpos + (multiply(cmd->size, coss[(int)cmd->angle], 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, sins[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
    driver_draw_line(lastx, lasty, (int)(cmd->xpos >> 19), (int)(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_dodrawc(lsys_turtlestatei *cmd)
{
    cmd->curcolor = (char)(((int) cmd->num) % colors);
}

static void lsysi_dodrawgt(lsys_turtlestatei *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor - (char)cmd->num);
    cmd->curcolor %= colors;
    if (cmd->curcolor == 0)
        cmd->curcolor = (char)(colors-1);
}

static void lsysi_dodrawlt(lsys_turtlestatei *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor + (char)cmd->num);
    cmd->curcolor %= colors;
    if (cmd->curcolor == 0)
        cmd->curcolor = 1;
}

static lsys_cmd *
findsize(lsys_cmd *command, lsys_turtlestatei *ts, lsys_cmd **rules, int depth)
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
                ts->num = command->n;
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                char saveang, saverev;
                long savesize, savex, savey;
                unsigned long saverang;

                saveang = ts->angle;
                saverev = ts->reverse;
                savesize = ts->size;
                saverang = ts->realangle;
                savex = ts->xpos;
                savey = ts->ypos;
                command = findsize(command+1, ts, rules, depth);
                if (command == nullptr)
                    return (nullptr);
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

static bool
lsysi_findscale(lsys_cmd *command, lsys_turtlestatei *ts, lsys_cmd **rules, int depth)
{
    float horiz, vert;
    double xmin, xmax, ymin, ymax;
    double locsize;
    double locaspect;
    lsys_cmd *fsret;

    locaspect = screenaspect*xdots/ydots;
    ts->aspect = FIXEDPT(locaspect);
    ts->counter = 0;
    ts->reverse = ts->counter;
    ts->angle = ts->reverse;
    ts->realangle = ts->angle;
    ts->ymin = ts->realangle;
    ts->ymax = ts->ymin;
    ts->xmax = ts->ymax;
    ts->xmin = ts->xmax;
    ts->ypos = ts->xmin;
    ts->xpos = ts->ypos;
    ts->size = FIXEDPT(1L);
    fsret = findsize(command, ts, rules, depth);
    thinking(0, nullptr); // erase thinking message if any
    xmin = (double) ts->xmin / FIXEDMUL;
    xmax = (double) ts->xmax / FIXEDMUL;
    ymin = (double) ts->ymin / FIXEDMUL;
    ymax = (double) ts->ymax / FIXEDMUL;
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
        ts->xpos = FIXEDPT(xdots/2);
    else
        //    ts->xpos = FIXEDPT(-xmin*(locsize)+5+((xdots-10)-(locsize)*(xmax-xmin))/2);
        ts->xpos = FIXEDPT((xdots-locsize*(xmax+xmin))/2);
    if (vert == 1E37)
        ts->ypos = FIXEDPT(ydots/2);
    else
        //    ts->ypos = FIXEDPT(-ymin*(locsize)+3+((ydots-6)-(locsize)*(ymax-ymin))/2);
        ts->ypos = FIXEDPT((ydots-locsize*(ymax+ymin))/2);
    ts->size = FIXEDPT(locsize);

    return true;
}

static lsys_cmd *
drawLSysI(lsys_cmd *command, lsys_turtlestatei *ts, lsys_cmd **rules, int depth)
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
                    if (drawLSysI((*rulind)+1, ts, rules, depth-1) == nullptr)
                        return nullptr;
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
                char saveang, saverev, savecolor;
                long savesize, savex, savey;
                unsigned long saverang;

                saveang = ts->angle;
                saverev = ts->reverse;
                savesize = ts->size;
                saverang = ts->realangle;
                savex = ts->xpos;
                savey = ts->ypos;
                savecolor = ts->curcolor;
                command = drawLSysI(command+1, ts, rules, depth);
                if (command == nullptr)
                    return (nullptr);
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

static lsys_cmd *
LSysISizeTransform(char *s, lsys_turtlestatei *ts)
{
    lsys_cmd *ret;
    lsys_cmd *doub;
    int maxval = 10;
    int n = 0;
    void (*f)(lsys_turtlestatei *);
    long num;

    void (*plus)(lsys_turtlestatei *) = (ispow2(ts->maxangle)) ? lsysi_doplus_pow2 : lsysi_doplus;
    void (*minus)(lsys_turtlestatei *) = (ispow2(ts->maxangle)) ? lsysi_dominus_pow2 : lsysi_dominus;
    void (*pipe)(lsys_turtlestatei *) = (ispow2(ts->maxangle)) ? lsysi_dopipe_pow2 : lsysi_dopipe;

    void (*slash)(lsys_turtlestatei *) = lsysi_doslash_386;
    void (*bslash)(lsys_turtlestatei *) = lsysi_dobslash_386;
    void (*at)(lsys_turtlestatei *) = lsysi_doat_386;
    void (*dogf)(lsys_turtlestatei *) = lsysi_dosizegf_386;

    ret = (lsys_cmd *) malloc((long) maxval * sizeof(lsys_cmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        f = nullptr;
        num = 0;
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
            num = (long)(getnumber(&s) * 11930465L);
            break;
        case '\\':
            f = bslash;
            num = (long)(getnumber(&s) * 11930465L);
            break;
        case '@':
            f = at;
            num = FIXEDPT(getnumber(&s));
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysi_dobang;
            break;
        case 'd':
        case 'm':
            f = lsysi_dosizedm;
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
        ret[n].f = f;
        ret[n].n = num;
        if (++n == maxval)
        {
            doub = (lsys_cmd *) malloc((long) maxval*2*sizeof(lsys_cmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            memcpy(doub, ret, maxval*sizeof(lsys_cmd));
            free(ret);
            ret = doub;
            maxval <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].n = 0;
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

static lsys_cmd *
LSysIDrawTransform(char *s, lsys_turtlestatei *ts)
{
    lsys_cmd *ret;
    lsys_cmd *doub;
    int maxval = 10;
    int n = 0;
    void (*f)(lsys_turtlestatei *);
    long num;

    void (*plus)(lsys_turtlestatei *) = (ispow2(ts->maxangle)) ? lsysi_doplus_pow2 : lsysi_doplus;
    void (*minus)(lsys_turtlestatei *) = (ispow2(ts->maxangle)) ? lsysi_dominus_pow2 : lsysi_dominus;
    void (*pipe)(lsys_turtlestatei *) = (ispow2(ts->maxangle)) ? lsysi_dopipe_pow2 : lsysi_dopipe;

    void (*slash)(lsys_turtlestatei *) = lsysi_doslash_386;
    void (*bslash)(lsys_turtlestatei *) = lsysi_dobslash_386;
    void (*at)(lsys_turtlestatei *) = lsysi_doat_386;
    void (*drawg)(lsys_turtlestatei *) = lsysi_dodrawg_386;

    ret = (lsys_cmd *) malloc((long) maxval * sizeof(lsys_cmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
        return nullptr;
    }
    while (*s)
    {
        f = nullptr;
        num = 0;
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
            num = (long)(getnumber(&s) * 11930465L);
            break;
        case '\\':
            f = bslash;
            num = (long)(getnumber(&s) * 11930465L);
            break;
        case '@':
            f = at;
            num = FIXEDPT(getnumber(&s));
            break;
        case '|':
            f = pipe;
            break;
        case '!':
            f = lsysi_dobang;
            break;
        case 'd':
            f = lsysi_dodrawd;
            break;
        case 'm':
            f = lsysi_dodrawm;
            break;
        case 'g':
            f = drawg;
            break;
        case 'f':
            f = lsysi_dodrawf;
            break;
        case 'c':
            f = lsysi_dodrawc;
            num = (long) getnumber(&s);
            break;
        case '<':
            f = lsysi_dodrawlt;
            num = (long) getnumber(&s);
            break;
        case '>':
            f = lsysi_dodrawgt;
            num = (long) getnumber(&s);
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
        if (++n == maxval)
        {
            doub = (lsys_cmd *) malloc((long) maxval*2*sizeof(lsys_cmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            memcpy(doub, ret, maxval*sizeof(lsys_cmd));
            free(ret);
            ret = doub;
            maxval <<= 1;
        }
        s++;
    }
    ret[n].ch = 0;
    ret[n].f = nullptr;
    ret[n].n = 0;
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

static void lsysi_dosincos()
{
    double locaspect;
    double TWOPI = 2.0 * PI;
    double twopimax;
    double twopimaxi;
    double s, c;

    locaspect = screenaspect*xdots/ydots;
    twopimax = TWOPI / maxangle;
    for (int i = 0; i < maxangle; i++)
    {
        twopimaxi = i * twopimax;
        FPUsincos(&twopimaxi, &s, &c);
        sins[i] = (long)(s * FIXEDLT1);
        coss[i] = (long)((locaspect * c) * FIXEDLT1);
    }
}
