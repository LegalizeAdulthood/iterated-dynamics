#pragma once

extern bool g_is_true_color;

int select_video_mode(int);
void make_mig(unsigned int, unsigned int);
int getprecdbl(int);
int getprecbf(int);
int getprecbf_mag();
void parse_comments(char *value);
void init_comments();
