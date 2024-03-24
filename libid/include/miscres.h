#pragma once

#include <cstdio>
#include <string>

extern int                   g_num_affine_transforms;

extern void (*ltrig0)();
extern void (*ltrig1)();
extern void (*ltrig2)();
extern void (*ltrig3)();
extern void (*dtrig0)();
extern void (*dtrig1)();
extern void (*dtrig2)();
extern void (*dtrig3)();
extern void (*mtrig0)();
extern void (*mtrig1)();
extern void (*mtrig2)();
extern void (*mtrig3)();

void restore_active_ovly();
void notdiskmsg();
void cvtcentermag(double *, double *, LDBL *, double *, double *, double *);
void cvtcorners(double, double, LDBL, double, double, double);
void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
void cvtcornersbf(bf_t, bf_t, LDBL, double, double, double);
void check_writefile(std::string &name, char const *ext);
std::string showtrig();
int set_trig_array(int k, char const *name);
void set_trig_pointers(int);
int tab_display();
int endswithslash(char const *fl);
int ifsload();
