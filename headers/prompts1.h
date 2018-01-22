#pragma once
#if !defined(PROMPTS1_H)
#define PROMPTS1_H

#include <string>

extern std::string const     g_glasses1_map;
extern std::string const     g_jiim_left_right[];
extern std::string const     g_jiim_method[];
extern bool                  g_julibrot;
extern std::string const     g_julibrot_3d_options[];
extern std::string           g_map_name;
extern bool                  g_map_set;
extern const int             g_num_trig_functions;
extern trig_funct_lst        g_trig_fn[];

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
