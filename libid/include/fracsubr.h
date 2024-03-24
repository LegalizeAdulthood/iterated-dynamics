#pragma once

extern int                   g_resume_len;
extern bool                  g_tab_or_help;
extern bool                  g_use_grid;

void wait_until(int index, uclock_t wait_time);
void calcfracinit();
void adjust_corner();
int put_resume(int, ...);
int get_resume(int, ...);
int alloc_resume(int, int);
int start_resume();
void end_resume();
void sleepms(long);
void reset_clock();
void iplot_orbit(long, long, int);
void plot_orbit(double, double, int);
void scrub_orbit();
int add_worklist(int, int, int, int, int, int, int, int);
void tidy_worklist();
void get_julia_attractor(double, double);
int ssg_blocksize();
void symPIplot(int, int, int);
void symPIplot2J(int, int, int);
void symPIplot4J(int, int, int);
void symplot2(int, int, int);
void symplot2Y(int, int, int);
void symplot2J(int, int, int);
void symplot4(int, int, int);
void symplot2basin(int, int, int);
void symplot4basin(int, int, int);
void noplot(int, int, int);
void fractal_floattobf();
void adjust_cornerbf();
