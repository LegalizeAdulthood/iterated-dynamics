// SPDX-License-Identifier: GPL-3.0-only
//
#include "work_list.h"

int g_num_work_list{}; // resume work list for standard engine
WorkList g_work_list[MAX_CALC_WORK]{};

static int    combine_work_list();

int add_work_list(int xfrom, int xto, int xbegin,
                 int yfrom, int yto, int ybegin,
                 int pass, int sym)
{
    if (g_num_work_list >= MAX_CALC_WORK)
    {
        return -1;
    }
    g_work_list[g_num_work_list].xxstart = xfrom;
    g_work_list[g_num_work_list].xxstop  = xto;
    g_work_list[g_num_work_list].xxbegin = xbegin;
    g_work_list[g_num_work_list].yystart = yfrom;
    g_work_list[g_num_work_list].yystop  = yto;
    g_work_list[g_num_work_list].yybegin = ybegin;
    g_work_list[g_num_work_list].pass    = pass;
    g_work_list[g_num_work_list].sym     = sym;
    ++g_num_work_list;
    tidy_work_list();
    return 0;
}

static int combine_work_list() // look for 2 entries which can freely merge
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        if (g_work_list[i].yystart == g_work_list[i].yybegin)
        {
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                if (g_work_list[j].sym == g_work_list[i].sym
                    && g_work_list[j].yystart == g_work_list[j].yybegin
                    && g_work_list[j].xxstart == g_work_list[j].xxbegin
                    && g_work_list[i].pass == g_work_list[j].pass)
                {
                    if (g_work_list[i].xxstart == g_work_list[j].xxstart
                        && g_work_list[i].xxbegin == g_work_list[j].xxbegin
                        && g_work_list[i].xxstop  == g_work_list[j].xxstop)
                    {
                        if (g_work_list[i].yystop+1 == g_work_list[j].yystart)
                        {
                            g_work_list[i].yystop = g_work_list[j].yystop;
                            return j;
                        }
                        if (g_work_list[j].yystop+1 == g_work_list[i].yystart)
                        {
                            g_work_list[i].yystart = g_work_list[j].yystart;
                            g_work_list[i].yybegin = g_work_list[j].yybegin;
                            return j;
                        }
                    }
                    if (g_work_list[i].yystart == g_work_list[j].yystart
                        && g_work_list[i].yybegin == g_work_list[j].yybegin
                        && g_work_list[i].yystop  == g_work_list[j].yystop)
                    {
                        if (g_work_list[i].xxstop+1 == g_work_list[j].xxstart)
                        {
                            g_work_list[i].xxstop = g_work_list[j].xxstop;
                            return j;
                        }
                        if (g_work_list[j].xxstop+1 == g_work_list[i].xxstart)
                        {
                            g_work_list[i].xxstart = g_work_list[j].xxstart;
                            g_work_list[i].xxbegin = g_work_list[j].xxbegin;
                            return j;
                        }
                    }
                }
            }
        }
    }
    return 0; // nothing combined
}

// combine mergeable entries, resort
void tidy_work_list()
{
    {
        int i;
        while ((i = combine_work_list()) != 0)
        {
            // merged two, delete the gone one
            while (++i < g_num_work_list)
            {
                g_work_list[i-1] = g_work_list[i];
            }
            --g_num_work_list;
        }
    }
    for (int i = 0; i < g_num_work_list; ++i)
    {
        for (int j = i+1; j < g_num_work_list; ++j)
        {
            if (g_work_list[j].pass < g_work_list[i].pass
                || (g_work_list[j].pass == g_work_list[i].pass
                    && (g_work_list[j].yystart < g_work_list[i].yystart
                        || (g_work_list[j].yystart == g_work_list[i].yystart
                            && g_work_list[j].xxstart <  g_work_list[i].xxstart))))
            {
                // dumb sort, swap 2 entries to correct order
                WorkList tempwork = g_work_list[i];
                g_work_list[i] = g_work_list[j];
                g_work_list[j] = tempwork;
            }
        }
    }
}
