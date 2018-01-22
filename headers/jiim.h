#pragma once
#if !defined(JIIM_H)
#define JIIM_H

enum class jiim_types
{
    JIIM = 0,
    ORBIT
};

extern double                g_julia_c_x;
extern double                g_julia_c_y;
extern DComplex              g_save_c;

extern void Jiim(jiim_types which);
extern LComplex PopLong();
extern DComplex PopFloat();
extern LComplex DeQueueLong();
extern DComplex DeQueueFloat();
extern LComplex ComplexSqrtLong(long, long);
extern DComplex ComplexSqrtFloat(double, double);
extern bool Init_Queue(unsigned long);
extern void   Free_Queue();
extern void   ClearQueue();
extern int    QueueEmpty();
extern int    QueueFull();
extern int    QueueFullAlmost();
extern int    PushLong(long, long);
extern int    PushFloat(float, float);
extern int    EnQueueLong(long, long);
extern int    EnQueueFloat(float, float);

#endif
