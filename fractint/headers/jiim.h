#if !defined(JIIM_H)
#define JIIM_H

extern void Jiim(int);
extern ComplexL PopLong();
extern ComplexD PopFloat();
extern ComplexL DeQueueLong();
extern ComplexD DeQueueFloat();
extern ComplexL ComplexSqrtLong(long,  long);
extern ComplexD ComplexSqrtFloat(double, double);
extern int    Init_Queue(unsigned long);
extern void   Free_Queue();
extern void   ClearQueue();
extern int    QueueEmpty();
extern int    QueueFull();
extern int    QueueFullAlmost();
extern int    PushLong(long,  long);
extern int    PushFloat(float,  float);
extern int    EnQueueLong(long,  long);
extern int    EnQueueFloat(float,  float);

#endif
