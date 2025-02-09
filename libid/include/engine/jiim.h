// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

enum class JIIMType
{
    JIIM = 0,
    ORBIT
};

extern double                g_julia_c_x;
extern double                g_julia_c_y;
extern DComplex              g_save_c;
constexpr double             JULIA_C_NOT_SET{100000.0};

void jiim(JIIMType which);
DComplex pop_float();
DComplex dequeue_float();
bool init_queue(unsigned long request);
void free_queue();
void   clear_queue();
int    queue_empty();
int    queue_full();
int    queue_full_almost();
int    push_float(float x, float y);
int    enqueue_float(float x, float y);
