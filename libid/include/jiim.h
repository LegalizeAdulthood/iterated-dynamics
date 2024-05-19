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

void Jiim(jiim_types which);
LComplex PopLong();
DComplex PopFloat();
LComplex DeQueueLong();
DComplex DeQueueFloat();
LComplex ComplexSqrtLong(long, long);
DComplex ComplexSqrtFloat(double, double);
inline DComplex ComplexSqrtFloat(const DComplex &z)
{
    return ComplexSqrtFloat(z.x, z.y);
}
bool Init_Queue(unsigned long);
void   Free_Queue();
void   ClearQueue();
int    QueueEmpty();
int    QueueFull();
int    QueueFullAlmost();
int    PushLong(long, long);
int    PushFloat(float, float);
int    EnQueueLong(long, long);
int    EnQueueFloat(float, float);
