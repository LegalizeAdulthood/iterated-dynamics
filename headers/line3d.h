#pragma once
#if !defined(LINE3D_H)
#define LINE3D_H

enum class raytrace_formats
{
    none = 0,
    povray = 1,
    vivid = 2,
    raw = 3,
    mtv = 4,
    rayshade = 5,
    acrospin = 6,
    dxf = 7
};

extern int                   g_ambient;             // Ambient= parameter value
extern int const             g_bad_value;
extern BYTE                  g_background_color[];
extern bool                  g_brief;
extern int                   g_converge_x_adjust;
extern int                   g_converge_y_adjust;
extern int                   g_haze;
extern std::string           g_light_name;
extern MATRIX                g_m;
extern bool                  g_preview;
extern int                   g_preview_factor;
extern int                   g_randomize_3d;
extern std::string           g_raytrace_filename;
extern raytrace_formats      g_raytrace_format;
extern bool                  g_show_box;
extern void                (*g_standard_plot)(int, int, int);
extern bool                  g_targa_overlay;
extern VECTOR                g_view;
extern int                   g_x_shift;
extern int                   g_xx_adjust;
extern int                   g_y_shift;
extern int                   g_yy_adjust;

extern int line3d(BYTE *, unsigned int);
extern int targa_color(int, int, int);
extern bool targa_validate(char const *File_Name);
bool startdisk1(char const *File_Name2, FILE *Source, bool overlay);

#endif
