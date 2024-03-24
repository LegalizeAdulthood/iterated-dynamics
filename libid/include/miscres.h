#pragma once

#include <string>

void restore_active_ovly();
void notdiskmsg();
void cvtcentermag(double *, double *, LDBL *, double *, double *, double *);
void cvtcorners(double, double, LDBL, double, double, double);
void cvtcentermagbf(bf_t, bf_t, LDBL *, double *, double *, double *);
void cvtcornersbf(bf_t, bf_t, LDBL, double, double, double);
void check_writefile(std::string &name, char const *ext);
int tab_display();
