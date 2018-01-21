#pragma once
#if !defined(PROMPTS1_H)
#define PROMPTS1_H

struct fullscreenvalues;
extern int fullscreen_prompt(
    char const *hdg,
    int numprompts,
    char const **prompts,
    fullscreenvalues *values,
    int fkeymask,
    char *extrainfo);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    char *filename, char *entryname);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, char *entryname);
extern long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, std::string &entryname);
extern int get_fracttype();
extern int get_fract_params(int);
extern int get_fract3d_params();
extern int get_3d_params();
extern int prompt_valuestring(char *buf, fullscreenvalues const *val);
extern void setbailoutformula(bailouts);
extern int find_extra_param(fractal_type type);
extern void load_params(fractal_type fractype);
extern bool check_orbit_name(char const *orbitname);
struct entryinfo;
extern int scan_entries(FILE *infile, struct entryinfo *ch, char const *itemname);

#endif
