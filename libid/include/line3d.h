// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "3d.h"
#include "port.h"

#include <cstdio>
#include <string>

enum class RayTraceFormat
{
    NONE = 0,
    POVRAY = 1,
    VIVID = 2,
    RAW = 3,
    MTV = 4,
    RAYSHADE = 5,
    ACROSPIN = 6,
    DXF = 7
};

extern int                   g_ambient;             // Ambient= parameter value
extern int const             g_bad_value;
extern Byte                  g_background_color[];
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
extern RayTraceFormat        g_raytrace_format;
extern bool                  g_show_box;
extern void                (*g_standard_plot)(int, int, int);
extern bool                  g_targa_overlay;
extern VECTOR                g_view;
extern int                   g_x_shift;
extern int                   g_xx_adjust;
extern int                   g_y_shift;
extern int                   g_yy_adjust;

int line3d(Byte * pixels, unsigned linelen);
int targa_color(int x, int y, int color);
bool targa_validate(char const *File_Name);
bool start_disk1(const std::string &filename, std::FILE *source, bool overlay);
