// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "fractals/fractype.h"
#include "misc/version.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace id::io
{

enum class ShowFile
{
    REQUEST_IMAGE = -1,
    LOAD_IMAGE = 0,
    IMAGE_LOADED = 1,
};

enum class SaveSystem
{
    DOS = 0,
    WINDOWS = 1, // We always save as Windows, never as DOS
};

inline int operator+(SaveSystem value)
{
    return static_cast<int>(value);
}

struct FractalInfo;                                 // for saving data in GIF file
struct FormulaInfo;                                 // for saving formula data in GIF file
struct ExtBlock2;                                   //
struct ExtBlock3;                                   //
struct ExtBlock4;                                   //
struct ExtBlock5;                                   //
struct ExtBlock6;                                   // parameter evolution stuff
struct ExtBlock7;                                   //
struct OrbitsInfo;                                  // for saving orbits data in a GIF file

extern bool                  g_bad_outside;
extern bool                  g_fast_restore;        // true - reset viewwindows prior to a restore and
                                                    // do not display warnings when video mode changes during restore
extern float                 g_file_aspect_ratio;
extern int                   g_file_colors;
extern int                   g_file_x_dots;
extern int                   g_file_y_dots;
extern bool                  g_loaded_3d;
extern bool                  g_overlay_3d;          // 3D overlay flag
extern bool                  g_new_bifurcation_functions_loaded;
extern short                 g_skip_x_dots;
extern short                 g_skip_y_dots;
extern misc::Version         g_file_version;
extern std::filesystem::path g_read_filename;
extern ShowFile              g_show_file;           // LOAD_IMAGE if file display pending

int read_overlay();
void set_if_old_bif();
void set_function_param_defaults();
void backwards_legacy_v18();
void backwards_legacy_v19();
void backwards_legacy_v20();
// return true on error, false on success
bool find_fractal_info(const std::string &gif_file, FractalInfo *info,   //
    ExtBlock2 *blk_2_info, ExtBlock3 *blk_3_info, ExtBlock4 *blk_4_info, //
    ExtBlock5 *blk_5_info, ExtBlock6 *blk_6_info, ExtBlock7 *blk_7_info);
fractals::FractalType migrate_integer_types(int read_type);

} // namespace id::io
