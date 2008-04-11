#if !defined(RESUME_H)
#define RESUME_H

extern char *g_resume_info;
extern int g_resume_length;
extern bool g_resuming;

extern int put_resume(int count, void const *bytes);
extern int get_resume(int count, void *bytes);
extern int alloc_resume(int, int);
extern int start_resume();
extern void end_resume();

#endif
