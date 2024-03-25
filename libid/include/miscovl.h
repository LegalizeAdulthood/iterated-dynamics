#pragma once

extern bool g_is_true_color;

void init_comments();
void make_mig(unsigned int xmult, unsigned int ymult);
void parse_comments(char *value);
int select_video_mode(int curmode);
