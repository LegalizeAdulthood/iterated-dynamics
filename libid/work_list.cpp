// SPDX-License-Identifier: GPL-3.0-only
//
#include "work_list.h"

int g_num_work_list{}; // resume work list for standard engine
WorkList g_work_list[MAX_CALC_WORK]{};

static int    combine_work_list();

int add_work_list(int x_from, int x_to, int x_begin, //
    int y_from, int y_to, int y_begin,               //
    int pass, int symmetry)
{
    if (g_num_work_list >= MAX_CALC_WORK)
    {
        return -1;
    }
    g_work_list[g_num_work_list].xx_start = x_from;
    g_work_list[g_num_work_list].xx_stop  = x_to;
    g_work_list[g_num_work_list].xx_begin = x_begin;
    g_work_list[g_num_work_list].yy_start = y_from;
    g_work_list[g_num_work_list].yy_stop  = y_to;
    g_work_list[g_num_work_list].yy_begin = y_begin;
    g_work_list[g_num_work_list].pass    = pass;
    g_work_list[g_num_work_list].symmetry     = symmetry;
    ++g_num_work_list;
    tidy_work_list();
    return 0;
}

static int combine_work_list() // look for 2 entries which can freely merge
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        if (g_work_list[i].yy_start == g_work_list[i].yy_begin)
        {
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                if (g_work_list[j].symmetry == g_work_list[i].symmetry
                    && g_work_list[j].yy_start == g_work_list[j].yy_begin
                    && g_work_list[j].xx_start == g_work_list[j].xx_begin
                    && g_work_list[i].pass == g_work_list[j].pass)
                {
                    if (g_work_list[i].xx_start == g_work_list[j].xx_start
                        && g_work_list[i].xx_begin == g_work_list[j].xx_begin
                        && g_work_list[i].xx_stop  == g_work_list[j].xx_stop)
                    {
                        if (g_work_list[i].yy_stop+1 == g_work_list[j].yy_start)
                        {
                            g_work_list[i].yy_stop = g_work_list[j].yy_stop;
                            return j;
                        }
                        if (g_work_list[j].yy_stop+1 == g_work_list[i].yy_start)
                        {
                            g_work_list[i].yy_start = g_work_list[j].yy_start;
                            g_work_list[i].yy_begin = g_work_list[j].yy_begin;
                            return j;
                        }
                    }
                    if (g_work_list[i].yy_start == g_work_list[j].yy_start
                        && g_work_list[i].yy_begin == g_work_list[j].yy_begin
                        && g_work_list[i].yy_stop  == g_work_list[j].yy_stop)
                    {
                        if (g_work_list[i].xx_stop+1 == g_work_list[j].xx_start)
                        {
                            g_work_list[i].xx_stop = g_work_list[j].xx_stop;
                            return j;
                        }
                        if (g_work_list[j].xx_stop+1 == g_work_list[i].xx_start)
                        {
                            g_work_list[i].xx_start = g_work_list[j].xx_start;
                            g_work_list[i].xx_begin = g_work_list[j].xx_begin;
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
                    && (g_work_list[j].yy_start < g_work_list[i].yy_start
                        || (g_work_list[j].yy_start == g_work_list[i].yy_start
                            && g_work_list[j].xx_start <  g_work_list[i].xx_start))))
            {
                // dumb sort, swap 2 entries to correct order
                WorkList tempwork = g_work_list[i];
                g_work_list[i] = g_work_list[j];
                g_work_list[j] = tempwork;
            }
        }
    }
}
