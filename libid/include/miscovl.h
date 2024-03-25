#pragma once

extern bool g_is_true_color;

int getprecbf(int);
int getprecbf_mag();
int getprecdbl(int);
void init_comments();
void make_mig(unsigned int, unsigned int);
void parse_comments(char *value);
int select_video_mode(int);
