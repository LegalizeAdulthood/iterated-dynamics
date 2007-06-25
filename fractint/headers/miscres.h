#if !defined(MISC_RES_H)
#define MISC_RES_H

#include "big.h"

extern void restore_active_ovly();
extern void not_disk_message();
extern void convert_center_mag(double *, double *, LDBL *, double *, double *, double *);
extern void convert_corners(double, double, LDBL, double, double, double);
extern void convert_center_mag_bf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void convert_corners_bf(bf_t, bf_t, LDBL, double, double, double);
extern int check_key();
extern void show_function(char *message);
extern int set_function_array(int, const char *);
extern void set_trig_pointers(int);
extern int tab_display();
extern int ends_with_slash(char *);
extern int ifs_load();
extern int find_file_item(char *, const char *item_name, FILE **, int);
extern int file_gets(char *, int, FILE *);
extern void round_float_d(double *);
extern void fix_inversion(double *);
extern int unget_a_key(int);
extern void get_calculation_time(char *, long);

#endif
