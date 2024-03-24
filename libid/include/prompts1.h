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

extern std::string const     g_jiim_left_right[];
extern std::string const     g_jiim_method[];
extern bool                  g_julibrot;
extern std::string const     g_julibrot_3d_options[];
extern const int             g_num_trig_functions;
extern trig_funct_lst        g_trig_fn[];

long get_file_entry(int type, char const *title, char const *fmask,
                    char *filename, char *entryname);
long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, char *entryname);
long get_file_entry(int type, char const *title, char const *fmask,
                    std::string &filename, std::string &entryname);
int get_fracttype();
int get_fract_params(int);
bool search_for_entry(std::FILE *infile, char const *itemname);
