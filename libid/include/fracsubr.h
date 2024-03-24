#pragma once

void wait_until(int index, uclock_t wait_time);
void sleepms(long);
void reset_clock();
void iplot_orbit(long, long, int);
void plot_orbit(double, double, int);
void scrub_orbit();
void get_julia_attractor(double, double);
int ssg_blocksize();
