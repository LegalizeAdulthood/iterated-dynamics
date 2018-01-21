#pragma once
#if !defined(JIIM_H)
#define JIIM_H

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
