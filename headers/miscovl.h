#pragma once
#if !defined(MISCOVL_H)
#define MISCOVL_H

#include <string>

extern void make_batch_file();
extern void edit_text_colors();
extern int select_video_mode(int);
extern void format_vid_table(int choice, char *buf);
extern void make_mig(unsigned int, unsigned int);
extern int getprecdbl(int);
extern int getprecbf(int);
extern int getprecbf_mag();
extern void parse_comments(char *value);
extern void init_comments();
extern void write_batch_parms(char const *colorinf, bool colorsonly, int maxcolor, int i, int j);
extern std::string expand_comments(char const *source);

#endif
