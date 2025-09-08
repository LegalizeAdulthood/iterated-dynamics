// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/Point.h"

namespace id::engine
{

enum
{
    MAX_CALC_WORK = 12
};

// work list entry for std escape time engines
struct WorkList
{
    math::Point2i start; // screen window for this entry
    math::Point2i stop;  //
    math::Point2i begin; // start point within window, x=0 except on resume, y for 2pass/ssg
    int symmetry;        // if symmetry in window, prevents bad combines
    int pass;            // for 2pass and solid guessing
};

extern int g_num_work_list;
extern WorkList g_work_list[MAX_CALC_WORK];

bool add_work_list(math::Point2i start, math::Point2i stop, math::Point2i begin, int pass, int symmetry);
bool add_work_list(int x_from, int y_from, //
    int x_to, int y_to,                    //
    int x_begin, int y_begin,              //
    int pass, int symmetry);
void tidy_work_list();

} // namespace id::engine
