#if !defined(MISC_RES_H)
#define MISC_RES_H

#include "big.h"

extern const std::string g_insufficient_ifs_memory;

extern void restore_active_ovly();
extern void not_disk_message();
extern void convert_center_mag(double *, double *, LDBL *, double *, double *, double *);
extern void convert_corners(double, double, LDBL, double, double, double);
extern void convert_center_mag_bf(bf_t, bf_t, LDBL *, double *, double *, double *);
extern void convert_corners_bf(bf_t, bf_t, LDBL, double, double, double);
extern void show_function(char *message);
extern int set_function_array(int, const char *);
extern void set_trig_pointers(int);
extern int tab_display();
extern bool ends_with_slash(const char *text);
extern int ifs_load();
extern bool find_file_item(std::string &filename, const std::string &item_name, std::ifstream &stream, int item_type);
extern int file_gets(char *buffer, int buffer_length, std::ifstream &file);
extern void round_float_d(double *);
extern void fix_inversion(double *);
extern int unget_a_key(int);
extern void get_calculation_time(char *, long);

#endif
