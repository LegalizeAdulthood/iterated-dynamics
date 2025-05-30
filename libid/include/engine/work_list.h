// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum
{
    MAX_CALC_WORK = 12
};

template <typename T>
struct Point2
{
    T x;
    T y;
};
using Point2i = Point2<int>;

// work list entry for std escape time engines
struct WorkList
{
    Point2i start; // screen window for this entry
    Point2i stop;  //
    Point2i begin; // start point within window, x=0 except on resume, y for 2pass/ssg
    int symmetry;  // if symmetry in window, prevents bad combines
    int pass;      // for 2pass and solid guessing
};

extern int                   g_num_work_list;
extern WorkList              g_work_list[MAX_CALC_WORK];

int add_work_list(int x_from, int x_to, int x_begin, //
    int y_from, int y_to, int y_begin,               //
    int pass, int symmetry);
void tidy_work_list();
