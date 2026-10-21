// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "geometry/3d.h"
#include "io/disk_storage.h"

#include <config/port.h>

#include <cstdio>
#include <string>
#include <vector>

namespace id::geometry
{

enum class RayTraceFormat
{
    NONE = 0,
    DKB_POVRAY = 1,
    VIVID = 2,
    RAW = 3,
    MTV = 4,
    RAYSHADE = 5,
    ACROSPIN = 6,
    DXF = 7
};

constexpr int BAD_VALUE{-10000}; // set bad values to this

extern int                   g_ambient;                 // Ambient= parameter value
extern Byte                  g_background_color[];
extern bool                  g_brief;
extern int                   g_converge_x_adjust;
extern int                   g_converge_y_adjust;
extern int                   g_haze;
extern std::string           g_light_name;
extern Matrix      g_m;
extern bool                  g_preview;
extern int                   g_preview_factor;
extern int                   g_randomize_3d;
extern std::string           g_raytrace_filename;       // just the filename
extern RayTraceFormat        g_raytrace_format;
extern bool                  g_show_box;
extern void                (*g_standard_plot)(int, int, int);
extern bool                  g_targa_out;               // 3D full color flag
extern bool                  g_targa_overlay;
extern int                   g_transparent_color_3d[];  // transparency min/max values
extern Vector                g_view;
extern int                   g_x_shift;
extern int                   g_xx_adjust;
extern int                   g_y_shift;
extern int                   g_yy_adjust;

struct Line3DResult
{
    int status{};
    std::vector<std::string> warnings;
};

io::TargaStorageRequest prepare_line3d();
Line3DResult line3d(Byte *pixels, unsigned line_len);
int targa_color(int x, int y, int color);
// Empty diagnostic means success; source remains owned by the caller.
std::string start_targa_overlay(const std::string &path, std::FILE *source, io::TargaStorageRequest &storage);
std::string start_targa(const std::string &path, io::TargaStorageRequest &storage);

} // namespace id::geometry
