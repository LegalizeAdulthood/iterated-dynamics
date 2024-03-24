#pragma once

#include <string>

enum class bailouts;

extern std::string const     g_jiim_left_right[];
extern std::string const     g_jiim_method[];
extern bool                  g_julibrot;
extern std::string const     g_julibrot_3d_options[];

int get_fracttype();
int get_fract_params(int);
