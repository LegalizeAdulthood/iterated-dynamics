#pragma once

#include <cstdio>
#include <string>

enum class bailouts;

struct trig_funct_lst
{
    char const *name;
    void (*lfunct)();
    void (*dfunct)();
    void (*mfunct)();
};

// structure passed to fullscreen_prompts
struct fullscreenvalues
{
    int type;   // 'd' for double, 'f' for float, 's' for string,
    // 'D' for integer in double, '*' for comment
    // 'i' for integer, 'y' for yes=1 no=0
    // 0x100+n for string of length n
    // 'l' for one of a list of strings
    // 'L' for long
    union
    {
        double dval;        // when type 'd' or 'f'
        int    ival;        // when type is 'i'
        long   Lval;        // when type is 'L'
        char   sval[16];    // when type is 's'
        char  *sbuf;        // when type is 0x100+n
        struct              // when type is 'l'
        {
            int  val;       // selected choice
            int  vlen;      // char len per choice
            char const **list;  // list of values
            int  llen;      // number of values
        } ch;
    } uval;
};

extern std::string const     g_glasses1_map;
extern std::string const     g_jiim_left_right[];
extern std::string const     g_jiim_method[];
extern bool                  g_julibrot;
extern std::string const     g_julibrot_3d_options[];
extern std::string           g_map_name;
extern bool                  g_map_set;
extern const int             g_num_trig_functions;
extern trig_funct_lst        g_trig_fn[];

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
extern void setbailoutformula(bailouts);
extern int find_extra_param(fractal_type type);
extern void load_params(fractal_type fractype);
extern bool check_orbit_name(char const *orbitname);
struct entryinfo;
extern int scan_entries(std::FILE *infile, struct entryinfo *ch, char const *itemname);
