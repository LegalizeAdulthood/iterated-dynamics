// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "cmplx.h"

enum class jiim_types
{
    JIIM = 0,
    ORBIT
};

extern double                g_julia_c_x;
extern double                g_julia_c_y;
extern DComplex              g_save_c;
constexpr double             JULIA_C_NOT_SET{100000.0};

void jiim(jiim_types which);
LComplex pop_long();
DComplex pop_float();
LComplex dequeue_long();
DComplex dequeue_float();
LComplex complex_sqrt_long(long, long);
DComplex complex_sqrt_float(double, double);
inline DComplex complex_sqrt_float(const DComplex &z)
{
    return complex_sqrt_float(z.x, z.y);
}
bool init_queue(unsigned long);
void   free_queue();
void   clear_queue();
int    queue_empty();
int    queue_full();
int    queue_full_almost();
int    push_long(long, long);
int    push_float(float, float);
int    enqueue_long(long, long);
int    enqueue_float(float, float);
