// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/work_list.h"

int g_num_work_list{}; // resume work list for standard engine
WorkList g_work_list[MAX_CALC_WORK]{};

static int combine_work_list();

bool add_work_list(Point2i start, Point2i stop, Point2i begin, int pass, int symmetry)
{
    if (g_num_work_list >= MAX_CALC_WORK)
    {
        return true;
    }
    g_work_list[g_num_work_list].start = start;
    g_work_list[g_num_work_list].stop = stop;
    g_work_list[g_num_work_list].begin = begin;
    g_work_list[g_num_work_list].pass = pass;
    g_work_list[g_num_work_list].symmetry = symmetry;
    ++g_num_work_list;
    if (g_num_work_list > 1)
    {
        tidy_work_list();
    }
    return false;
}

bool add_work_list(int x_from, int y_from, //
    int x_to, int y_to,                    //
    int x_begin, int y_begin,              //
    int pass, int symmetry)
{
    if (g_num_work_list >= MAX_CALC_WORK)
    {
        return true;
    }
    g_work_list[g_num_work_list].start.x = x_from;
    g_work_list[g_num_work_list].stop.x  = x_to;
    g_work_list[g_num_work_list].begin.x = x_begin;
    g_work_list[g_num_work_list].start.y = y_from;
    g_work_list[g_num_work_list].stop.y  = y_to;
    g_work_list[g_num_work_list].begin.y = y_begin;
    g_work_list[g_num_work_list].pass    = pass;
    g_work_list[g_num_work_list].symmetry     = symmetry;
    ++g_num_work_list;
    tidy_work_list();
    return false;
}

static int combine_work_list() // look for 2 entries which can freely merge
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        if (g_work_list[i].start.y == g_work_list[i].begin.y)
        {
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                if (g_work_list[j].symmetry == g_work_list[i].symmetry
                    && g_work_list[j].start.y == g_work_list[j].begin.y
                    && g_work_list[j].start.x == g_work_list[j].begin.x
                    && g_work_list[i].pass == g_work_list[j].pass)
                {
                    if (g_work_list[i].start.x == g_work_list[j].start.x
                        && g_work_list[i].begin.x == g_work_list[j].begin.x
                        && g_work_list[i].stop.x  == g_work_list[j].stop.x)
                    {
                        if (g_work_list[i].stop.y+1 == g_work_list[j].start.y)
                        {
                            g_work_list[i].stop.y = g_work_list[j].stop.y;
                            return j;
                        }
                        if (g_work_list[j].stop.y+1 == g_work_list[i].start.y)
                        {
                            g_work_list[i].start.y = g_work_list[j].start.y;
                            g_work_list[i].begin.y = g_work_list[j].begin.y;
                            return j;
                        }
                    }
                    if (g_work_list[i].start.y == g_work_list[j].start.y
                        && g_work_list[i].begin.y == g_work_list[j].begin.y
                        && g_work_list[i].stop.y  == g_work_list[j].stop.y)
                    {
                        if (g_work_list[i].stop.x+1 == g_work_list[j].start.x)
                        {
                            g_work_list[i].stop.x = g_work_list[j].stop.x;
                            return j;
                        }
                        if (g_work_list[j].stop.x+1 == g_work_list[i].start.x)
                        {
                            g_work_list[i].start.x = g_work_list[j].start.x;
                            g_work_list[i].begin.x = g_work_list[j].begin.x;
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
                    && (g_work_list[j].start.y < g_work_list[i].start.y
                        || (g_work_list[j].start.y == g_work_list[i].start.y
                            && g_work_list[j].start.x <  g_work_list[i].start.x))))
            {
                // dumb sort, swap 2 entries to correct order
                WorkList tmp = g_work_list[i];
                g_work_list[i] = g_work_list[j];
                g_work_list[j] = tmp;
            }
        }
    }
}
