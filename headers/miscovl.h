#pragma once

extern bool g_is_true_color;

extern int select_video_mode(int);
extern void make_mig(unsigned int, unsigned int);
extern int getprecdbl(int);
extern int getprecbf(int);
extern int getprecbf_mag();
extern void parse_comments(char *value);
extern void init_comments();
