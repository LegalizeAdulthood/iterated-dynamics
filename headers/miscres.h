#pragma once
#if !defined(MISCRES_H)
#define MISCRES_H

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

extern void restore_active_ovly();
extern void findpath(char const *filename, char *fullpathname);
extern void notdiskmsg();
extern void cvtcentermag(double *, double *, LDBL *, double *, double *, double *);
extern void cvtcorners(double, double, LDBL, double, double, double);
extern void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void cvtcornersbf(bf_t, bf_t, LDBL, double, double, double);
extern void updatesavename(char *filename);
extern void updatesavename(std::string &filename);
extern int check_writefile(char *name, char const *ext);
extern int check_writefile(std::string &name, char const *ext);
extern std::string showtrig();
extern int set_trig_array(int k, char const *name);
extern void set_trig_pointers(int);
extern int tab_display();
extern int endswithslash(char const *fl);
extern int ifsload();
extern bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, int itemtype);
extern bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, int itemtype);
extern int file_gets(char *buf, int maxlen, std::FILE *infile);
extern void roundfloatd(double *);
extern void fix_inversion(double *);
extern int ungetakey(int);
extern std::string get_calculation_time(long ctime);

#endif
