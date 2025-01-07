// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum
{
    MAX_CALC_WORK = 12
};

// work list entry for std escape time engines
struct WorkList
{
    int xx_start; // screen window for this entry
    int xx_stop;  //
    int yy_start; //
    int yy_stop;  //
    int yy_begin; // start row within window, for 2pass/ssg resume
    int symmetry; // if symmetry in window, prevents bad combines
    int pass;     // for 2pass and solid guessing
    int xx_begin; // start col within window, =0 except on resume
};

extern int                   g_num_work_list;
extern WorkList              g_work_list[MAX_CALC_WORK];

int add_work_list(int x_from, int x_to, int x_begin, //
    int y_from, int y_to, int y_begin,               //
    int pass, int symmetry);
void tidy_work_list();
