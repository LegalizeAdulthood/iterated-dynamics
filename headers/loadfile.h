#pragma once
#if !defined(LOADFILE_H)
#define LOADFILE_H

#include <string>

#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct ext_blk_3
{
    bool got_data;
    int length;
    char form_name[40];
    short uses_p1;
    short uses_p2;
    short uses_p3;
    short uses_ismand;
    short ismand;
    short uses_p4;
    short uses_p5;
};
#if defined(_WIN32)
#pragma pack(pop)
#endif

extern bool                  g_bad_outside;
extern std::string           g_browse_name;
extern float                 g_file_aspect_ratio;
extern int                   g_file_colors;
extern int                   g_file_x_dots;
extern int                   g_file_y_dots;
extern bool                  g_ld_check;
extern bool                  g_loaded_3d;
extern short                 g_skip_x_dots;
extern short                 g_skip_y_dots;

extern int read_overlay();
extern void set_if_old_bif();
extern void set_function_parm_defaults();
extern int fgetwindow();
extern void backwards_v18();
extern void backwards_v19();
extern void backwards_v20();
extern bool check_back();

#endif
