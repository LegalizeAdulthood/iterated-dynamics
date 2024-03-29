#pragma once

enum class jiim_types
{
    JIIM = 0,
    ORBIT
};

extern double                g_julia_c_x;
extern double                g_julia_c_y;
extern DComplex              g_save_c;

void Jiim(jiim_types which);
LComplex PopLong();
DComplex PopFloat();
LComplex DeQueueLong();
DComplex DeQueueFloat();
LComplex ComplexSqrtLong(long, long);
DComplex ComplexSqrtFloat(double, double);
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
