// SPDX-License-Identifier: GPL-3.0-only
//
#include "port.h"
#include "prototyp.h"

#include "lsys.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "file_gets.h"
#include "file_item.h"
#include "fixed_pt.h"
#include "fpu087.h"
#include "fractals.h"
#include "id.h"
#include "id_data.h"
#include "lsys_fns.h"
#include "stack_avail.h"
#include "stop_msg.h"
#include "thinking.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace
{

struct LSysCmd
{
    void (*f)(LSysTurtleStateI *);
    long n;
    char ch;
};

} // namespace

static bool readLSystemFile(char const *str);
static void free_rules_mem();
static int rule_present(char symbol);
static bool save_axiom(char const *axiom);
static bool save_rule(char const *rule, int index);
static bool append_rule(char const *rule, int index);
static void free_lcmds();
static LSysCmd *findsize(LSysCmd *, LSysTurtleStateI *, LSysCmd **, int);
static LSysCmd *drawLSysI(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth);
static bool lsysi_findscale(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth);
static LSysCmd *LSysISizeTransform(char const *s, LSysTurtleStateI *ts);
static LSysCmd *LSysIDrawTransform(char const *s, LSysTurtleStateI *ts);
static void lsysi_dosincos();
static void lsysi_doslash(LSysTurtleStateI *cmd);
static void lsysi_dobslash(LSysTurtleStateI *cmd);
static void lsysi_doat(LSysTurtleStateI *cmd);
static void lsysi_dopipe(LSysTurtleStateI *cmd);
static void lsysi_dosizedm(LSysTurtleStateI *cmd);
static void lsysi_dosizegf(LSysTurtleStateI *cmd);
static void lsysi_dodrawd(LSysTurtleStateI *cmd);
static void lsysi_dodrawm(LSysTurtleStateI *cmd);
static void lsysi_dodrawg(LSysTurtleStateI *cmd);
static void lsysi_dodrawf(LSysTurtleStateI *cmd);
static void lsysi_dodrawc(LSysTurtleStateI *cmd);
static void lsysi_dodrawgt(LSysTurtleStateI *cmd);
static void lsysi_dodrawlt(LSysTurtleStateI *cmd);

static std::vector<long> s_sin_table;
static std::vector<long> r_cos_table;
constexpr long PI_DIV_180_L{11930465L};
static std::string s_axiom;
static std::vector<std::string> s_rules;
static std::vector<LSysCmd *> s_rule_cmds;
static std::vector<LSysFCmd *> s_rulef_cmds;
static bool s_loaded{};

char g_max_angle{};

LDBL get_number(char const **str)
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
    char numstr[30];
    std::strcpy(numstr, "");
    int i = 0;
    while ((**str <= '9' && **str >= '0') || **str == '.')
    {
        numstr[i++] = **str;
        (*str)++;
    }
    (*str)--;
    numstr[i] = 0;
    LDBL ret = std::atof(numstr);
    if (ret <= 0.0)   // this is a sanity check
    {
        return 0;
    }
    if (root)
    {
        ret = sqrtl(ret);
    }
    if (inverse)
    {
        ret = 1.0/ret;
    }
    return ret;
}

static bool readLSystemFile(char const *str)
{
    int err = 0;
    char inline1[MAX_LSYS_LINE_LEN+1];
    std::FILE *infile;

    if (find_file_item(g_l_system_filename, str, &infile, gfe_type::L_SYSTEM))
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
    int rulind = 0;
    char msgbuf[481] = { 0 }; // enough for 6 full lines

    int linenum = 0;
    while (file_gets(inline1, MAX_LSYS_LINE_LEN, infile) > -1)  // Max line length chars
    {
        linenum++;
        char *word = std::strchr(inline1, ';');
        if (word != nullptr)   // strip comment
        {
            *word = 0;
        }
        strlwr(inline1);

        if ((int)std::strspn(inline1, " \t\n") < (int)std::strlen(inline1)) // not a blank line
        {
            bool check = false;
            word = std::strtok(inline1, " =\t\n");
            if (!std::strcmp(word, "axiom"))
            {
                if (save_axiom(std::strtok(nullptr, " \t\n")))
                {
                    std::strcat(msgbuf, "Error:  out of memory\n");
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
                bool memerr = false;

                if (std::strchr("+-/\\@|!c<>][", *word))
                {
                    std::sprintf(&msgbuf[std::strlen(msgbuf)],
                            "Syntax error line %d: Redefined reserved symbol %s\n", linenum, word);
                    ++err;
                    break;
                }
                char const *temp = std::strtok(nullptr, " =\t\n");
                int const index = rule_present(*word);
                char fixed[MAX_LSYS_LINE_LEN+1];
                if (!index)
                {
                    std::strcpy(fixed, word);
                    if (temp)
                    {
                        std::strcat(fixed, temp);
                    }
                    memerr = save_rule(fixed, rulind++);
                }
                else if (temp)
                {
                    memerr = append_rule(temp, index);
                }
                if (memerr)
                {
                    std::strcat(msgbuf, "Error:  out of memory\n");
                    ++err;
                    break;
                }
                check = true;
            }
            else if (err < 6)
            {
                std::sprintf(&msgbuf[std::strlen(msgbuf)],
                        "Syntax error line %d: %s\n", linenum, word);
                ++err;
            }
            if (check)
            {
                word = std::strtok(nullptr, " \t\n");
                if (word != nullptr)
                {
                    if (err < 6)
                    {
                        std::sprintf(&msgbuf[std::strlen(msgbuf)],
                                "Extra text after command line %d: %s\n", linenum, word);
                        ++err;
                    }
                }
            }
        }
    }
    std::fclose(infile);
    if (s_axiom.empty() && err < 6)
    {
        std::strcat(msgbuf, "Error:  no axiom\n");
        ++err;
    }
    if ((g_max_angle < 3||g_max_angle > 50) && err < 6)
    {
        std::strcat(msgbuf, "Error:  illegal or missing angle\n");
        ++err;
    }
    if (err)
    {
        msgbuf[std::strlen(msgbuf)-1] = 0; // strip trailing \n
        stop_msg(msgbuf);
        return true;
    }
    return false;
}

int lsystem()
{
    int order;
    bool stackoflow = false;

    if (!s_loaded && lsystem_load())
    {
        return -1;
    }

    g_overflow = false;           // reset integer math overflow flag

    order = (int)g_params[0];
    if (order <= 0)
    {
        order = 0;
    }
    if (g_user_float_flag)
    {
        g_overflow = true;
    }
    else
    {
        LSysTurtleStateI ts;

        ts.stackoflow = false;
        ts.maxangle = g_max_angle;
        ts.dmaxangle = (char)(g_max_angle - 1);

        s_rule_cmds.push_back(LSysISizeTransform(s_axiom.c_str(), &ts));
        for (auto const &rule : s_rules)
        {
            s_rule_cmds.push_back(LSysISizeTransform(rule.c_str(), &ts));
        }
        s_rule_cmds.push_back(nullptr);

        lsysi_dosincos();
        if (lsysi_findscale(s_rule_cmds[0], &ts, &s_rule_cmds[1], order))
        {
            ts.reverse = 0;
            ts.angle = ts.reverse;
            ts.realangle = ts.angle;

            free_lcmds();
            s_rule_cmds.push_back(LSysIDrawTransform(s_axiom.c_str(), &ts));
            for (auto const &rule : s_rules)
            {
                s_rule_cmds.push_back(LSysIDrawTransform(rule.c_str(), &ts));
            }
            s_rule_cmds.push_back(nullptr);

            // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
            ts.curcolor = 15;
            if (ts.curcolor > g_colors)
            {
                ts.curcolor = (char)(g_colors-1);
            }
            drawLSysI(s_rule_cmds[0], &ts, &s_rule_cmds[1], order);
        }
        stackoflow = ts.stackoflow;
    }

    if (stackoflow)
    {
        stop_msg("insufficient memory, try a lower order");
    }
    else if (g_overflow)
    {
        LSysTurtleStateF ts;

        g_overflow = false;

        ts.stackoflow = false;
        ts.maxangle = g_max_angle;
        ts.dmaxangle = (char)(g_max_angle - 1);

        s_rulef_cmds.push_back(lsysf_size_transform(s_axiom.c_str(), &ts));
        for (auto const &rule : s_rules)
        {
            s_rulef_cmds.push_back(lsysf_size_transform(rule.c_str(), &ts));
        }
        s_rulef_cmds.push_back(nullptr);

        lsysf_do_sin_cos();
        if (lsysf_find_scale(s_rulef_cmds[0], &ts, &s_rulef_cmds[1], order))
        {
            ts.reverse = 0;
            ts.angle = ts.reverse;
            ts.realangle = ts.angle;

            free_lcmds();
            s_rulef_cmds.push_back(lsysf_draw_transform(s_axiom.c_str(), &ts));
            for (auto const &rule : s_rules)
            {
                s_rulef_cmds.push_back(lsysf_draw_transform(rule.c_str(), &ts));
            }
            s_rulef_cmds.push_back(nullptr);

            // !! HOW ABOUT A BETTER WAY OF PICKING THE DEFAULT DRAWING COLOR
            ts.curcolor = 15;
            if (ts.curcolor > g_colors)
            {
                ts.curcolor = (char)(g_colors-1);
            }
            draw_lsysf(s_rulef_cmds[0], &ts, &s_rulef_cmds[1], order);
        }
        g_overflow = false;
    }
    free_rules_mem();
    free_lcmds();
    s_loaded = false;
    return 0;
}

bool lsystem_load()
{
    if (readLSystemFile(g_l_system_name.c_str()))
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
    catch (std::bad_alloc const&)
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
    catch (std::bad_alloc const&)
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
    catch (std::bad_alloc const &)
    {
        return true;
    }
}

static void free_lcmds()
{
    for (auto cmd : s_rule_cmds)
    {
        if (cmd)
        {
            free(cmd);
        }
    }
    s_rule_cmds.clear();
    for (auto cmd : s_rulef_cmds)
    {
        if (cmd != nullptr)
        {
            free(cmd);
        }
    }
    s_rulef_cmds.clear();
}

// integer specific routines

static void lsysi_doplus(LSysTurtleStateI *cmd)
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
void lsysi_doplus_pow2(LSysTurtleStateI *cmd)
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

void lsysi_dominus(LSysTurtleStateI *cmd)
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

void lsysi_dominus_pow2(LSysTurtleStateI *cmd)
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

static void lsysi_doslash(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        cmd->realangle -= cmd->num;
    }
    else
    {
        cmd->realangle += cmd->num;
    }
}

static void lsysi_dobslash(LSysTurtleStateI *cmd)
{
    if (cmd->reverse)
    {
        cmd->realangle += cmd->num;
    }
    else
    {
        cmd->realangle -= cmd->num;
    }
}

static void lsysi_doat(LSysTurtleStateI *cmd)
{
    cmd->size = multiply(cmd->size, cmd->num, 19);
}

static void lsysi_dopipe(LSysTurtleStateI *cmd)
{
    cmd->angle = (char)(cmd->angle + (char)(cmd->maxangle / 2));
    cmd->angle %= cmd->maxangle;
}

static void lsysi_dopipe_pow2(LSysTurtleStateI *cmd)
{
    cmd->angle += cmd->maxangle >> 1;
    cmd->angle &= cmd->dmaxangle;
}

static void lsysi_dobang(LSysTurtleStateI *cmd)
{
    cmd->reverse = ! cmd->reverse;
}

static void lsysi_dosizedm(LSysTurtleStateI *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s;
    double c;
    long fixedsin;
    long fixedcos;

    sin_cos(&angle, &s, &c);
    fixedsin = (long)(s * FIXEDLT1);
    fixedcos = (long)(c * FIXEDLT1);

    cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));

    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
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

static void lsysi_dosizegf(LSysTurtleStateI *cmd)
{
    cmd->xpos = cmd->xpos + (multiply(cmd->size, r_cos_table[(int)cmd->angle], 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, s_sin_table[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
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

static void lsysi_dodrawd(LSysTurtleStateI *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s;
    double c;
    long fixedsin;
    long fixedcos;
    int lastx;
    int lasty;

    sin_cos(&angle, &s, &c);
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

static void lsysi_dodrawm(LSysTurtleStateI *cmd)
{
    double angle = (double) cmd->realangle * ANGLE2DOUBLE;
    double s;
    double c;
    long fixedsin;
    long fixedcos;

    sin_cos(&angle, &s, &c);
    fixedsin = (long)(s * FIXEDLT1);
    fixedcos = (long)(c * FIXEDLT1);

    // xpos+=size*aspect*cos(realangle*PI/180);
    // ypos+=size*sin(realangle*PI/180);
    cmd->xpos = cmd->xpos + (multiply(multiply(cmd->size, cmd->aspect, 19), fixedcos, 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, fixedsin, 29));
}

static void lsysi_dodrawg(LSysTurtleStateI *cmd)
{
    cmd->xpos = cmd->xpos + (multiply(cmd->size, r_cos_table[(int)cmd->angle], 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, s_sin_table[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
}

static void lsysi_dodrawf(LSysTurtleStateI *cmd)
{
    int lastx = (int)(cmd->xpos >> 19);
    int lasty = (int)(cmd->ypos >> 19);
    cmd->xpos = cmd->xpos + (multiply(cmd->size, r_cos_table[(int)cmd->angle], 29));
    cmd->ypos = cmd->ypos + (multiply(cmd->size, s_sin_table[(int)cmd->angle], 29));
    // xpos+=size*coss[angle];
    // ypos+=size*sins[angle];
    driver_draw_line(lastx, lasty, (int)(cmd->xpos >> 19), (int)(cmd->ypos >> 19), cmd->curcolor);
}

static void lsysi_dodrawc(LSysTurtleStateI *cmd)
{
    cmd->curcolor = (char)(((int) cmd->num) % g_colors);
}

static void lsysi_dodrawgt(LSysTurtleStateI *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor - (char)cmd->num);
    cmd->curcolor %= g_colors;
    if (cmd->curcolor == 0)
    {
        cmd->curcolor = (char)(g_colors-1);
    }
}

static void lsysi_dodrawlt(LSysTurtleStateI *cmd)
{
    cmd->curcolor = (char)(cmd->curcolor + (char)cmd->num);
    cmd->curcolor %= g_colors;
    if (cmd->curcolor == 0)
    {
        cmd->curcolor = 1;
    }
}

static LSysCmd *findsize(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth)
{
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
        bool tran = false;
        if (depth > 0)
        {
            for (LSysCmd **rulind = rules; *rulind; rulind++)
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
        if (depth == 0 || !tran)
        {
            if (command->f)
            {
                ts->num = command->n;
                (*command->f)(ts);
            }
            else if (command->ch == '[')
            {
                char const saveang = ts->angle;
                char const saverev = ts->reverse;
                long const savesize = ts->size;
                unsigned long const saverang = ts->realangle;
                long const savex = ts->xpos;
                long const savey = ts->ypos;
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

static bool lsysi_findscale(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth)
{
    float horiz;
    float vert;
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double locsize;
    double locaspect;
    LSysCmd *fsret;

    locaspect = g_screen_aspect*g_logical_screen_x_dots/g_logical_screen_y_dots;
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
    {
        return false;
    }
    if (xmax == xmin)
    {
        horiz = (float)1E37;
    }
    else
    {
        horiz = (float)((g_logical_screen_x_dots-10)/(xmax-xmin));
    }
    if (ymax == ymin)
    {
        vert = (float)1E37;
    }
    else
    {
        vert = (float)((g_logical_screen_y_dots-6) /(ymax-ymin));
    }
    locsize = (vert < horiz) ? vert : horiz;

    if (horiz == 1E37)
    {
        ts->xpos = FIXEDPT(g_logical_screen_x_dots/2);
    }
    else
    {
        //    ts->xpos = FIXEDPT(-xmin*(locsize)+5+((xdots-10)-(locsize)*(xmax-xmin))/2);
        ts->xpos = FIXEDPT((g_logical_screen_x_dots-locsize*(xmax+xmin))/2);
    }
    if (vert == 1E37)
    {
        ts->ypos = FIXEDPT(g_logical_screen_y_dots/2);
    }
    else
    {
        //    ts->ypos = FIXEDPT(-ymin*(locsize)+3+((ydots-6)-(locsize)*(ymax-ymin))/2);
        ts->ypos = FIXEDPT((g_logical_screen_y_dots-locsize*(ymax+ymin))/2);
    }
    ts->size = FIXEDPT(locsize);

    return true;
}

static LSysCmd *drawLSysI(LSysCmd *command, LSysTurtleStateI *ts, LSysCmd **rules, int depth)
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
            if (driver_key_pressed())
            {
                ts->counter--;
                return nullptr;
            }
        }
        tran = false;
        if (depth)
        {
            for (LSysCmd **rulind = rules; *rulind; rulind++)
            {
                if ((*rulind)->ch == command->ch)
                {
                    tran = true;
                    if (drawLSysI((*rulind)+1, ts, rules, depth-1) == nullptr)
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
                char const saveang = ts->angle;
                char const saverev = ts->reverse;
                long const savesize = ts->size;
                unsigned long const saverang = ts->realangle;
                long const savex = ts->xpos;
                long const savey = ts->ypos;
                char const savecolor = ts->curcolor;
                command = drawLSysI(command+1, ts, rules, depth);
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

static LSysCmd *LSysISizeTransform(char const *s, LSysTurtleStateI *ts)
{
    int maxval = 10;
    int n = 0;

    auto const plus = is_pow2(ts->maxangle) ? lsysi_doplus_pow2 : lsysi_doplus;
    auto const minus = is_pow2(ts->maxangle) ? lsysi_dominus_pow2 : lsysi_dominus;
    auto const pipe = is_pow2(ts->maxangle) ? lsysi_dopipe_pow2 : lsysi_dopipe;

    LSysCmd *ret = (LSysCmd *) malloc((long) maxval * sizeof(LSysCmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
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
            f = lsysi_doslash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '\\':
            f = lsysi_dobslash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '@':
            f = lsysi_doat;
            num = FIXEDPT(get_number(&s));
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
            f = lsysi_dosizegf;
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
            LSysCmd *doub = (LSysCmd *) malloc((long) maxval*2*sizeof(LSysCmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            std::memcpy(doub, ret, maxval*sizeof(LSysCmd));
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

    LSysCmd *doub = (LSysCmd *) malloc((long) n*sizeof(LSysCmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stackoflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(LSysCmd));
    free(ret);
    return doub;
}

static LSysCmd *LSysIDrawTransform(char const *s, LSysTurtleStateI *ts)
{
    int maxval = 10;
    int n = 0;

    auto const plus = is_pow2(ts->maxangle) ? lsysi_doplus_pow2 : lsysi_doplus;
    auto const minus = is_pow2(ts->maxangle) ? lsysi_dominus_pow2 : lsysi_dominus;
    auto const pipe = is_pow2(ts->maxangle) ? lsysi_dopipe_pow2 : lsysi_dopipe;

    LSysCmd *ret = (LSysCmd *) malloc((long) maxval * sizeof(LSysCmd));
    if (ret == nullptr)
    {
        ts->stackoflow = true;
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
            f = lsysi_doslash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '\\':
            f = lsysi_dobslash;
            num = (long)(get_number(&s) * PI_DIV_180_L);
            break;
        case '@':
            f = lsysi_doat;
            num = FIXEDPT(get_number(&s));
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
            f = lsysi_dodrawg;
            break;
        case 'f':
            f = lsysi_dodrawf;
            break;
        case 'c':
            f = lsysi_dodrawc;
            num = (long) get_number(&s);
            break;
        case '<':
            f = lsysi_dodrawlt;
            num = (long) get_number(&s);
            break;
        case '>':
            f = lsysi_dodrawgt;
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
        if (++n == maxval)
        {
            LSysCmd *doub = (LSysCmd *) malloc((long) maxval*2*sizeof(LSysCmd));
            if (doub == nullptr)
            {
                free(ret);
                ts->stackoflow = true;
                return nullptr;
            }
            std::memcpy(doub, ret, maxval*sizeof(LSysCmd));
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

    LSysCmd *doub = (LSysCmd *) malloc((long) n*sizeof(LSysCmd));
    if (doub == nullptr)
    {
        free(ret);
        ts->stackoflow = true;
        return nullptr;
    }
    std::memcpy(doub, ret, n*sizeof(LSysCmd));
    free(ret);
    return doub;
}

static void lsysi_dosincos()
{
    double locaspect;
    double TWOPI = 2.0 * PI;
    double twopimax;
    double twopimaxi;
    double s;
    double c;

    locaspect = g_screen_aspect*g_logical_screen_x_dots/g_logical_screen_y_dots;
    twopimax = TWOPI / g_max_angle;
    s_sin_table.resize(g_max_angle);
    r_cos_table.resize(g_max_angle);
    for (int i = 0; i < g_max_angle; i++)
    {
        twopimaxi = i * twopimax;
        sin_cos(&twopimaxi, &s, &c);
        s_sin_table[i] = (long)(s * FIXEDLT1);
        r_cos_table[i] = (long)((locaspect * c) * FIXEDLT1);
    }
}
