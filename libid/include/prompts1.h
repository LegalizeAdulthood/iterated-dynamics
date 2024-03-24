#pragma once

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

int get_fracttype();
int get_fract_params(int);
