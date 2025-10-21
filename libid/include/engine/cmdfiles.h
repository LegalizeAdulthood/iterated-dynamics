// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>

namespace id::fractals
{
struct FractalSpecific;
}

namespace id::engine
{

enum class InitOrbitMode
{
    NORMAL = 0,
    VALUE = 1,
    PIXEL = 2
};

enum class RecordColorsMode
{
    NONE = 0,
    AUTOMATIC = 'a',
    COMMENT = 'c',
    YES = 'y'
};

enum class CmdFile
{
    AT_CMD_LINE = 0,         // command line @filename
    SSTOOLS_INI = 1,         // sstools.ini
    AT_AFTER_STARTUP = 2,    // <@> command after startup
    AT_CMD_LINE_SET_NAME = 3 // command line @filename/setname
};

enum class CmdArgFlags
{
    BAD_ARG          = -1,
    NONE             = 0,
    FRACTAL_PARAM    = 1,
    PARAM_3D         = 2,
    YES_3D           = 4,
    RESET            = 8,
    GOODBYE          = 16,
};
inline int operator+(const CmdArgFlags value)
{
    return static_cast<int>(value);
}
inline CmdArgFlags operator|(const CmdArgFlags lhs, const CmdArgFlags rhs)
{
    return static_cast<CmdArgFlags>(+lhs | +rhs);
}
inline CmdArgFlags &operator|=(CmdArgFlags &lhs, const CmdArgFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}
inline bool bit_set(const CmdArgFlags flags, const CmdArgFlags bit)
{
    return (+flags & +bit) == +bit;
}

// for init_batch
enum class BatchMode
{
    FINISH_CALC_BEFORE_SAVE = -1,
    NONE,
    NORMAL,
    SAVE,
    BAILOUT_ERROR_NO_SAVE,
    BAILOUT_INTERRUPTED_TRY_SAVE,
    BAILOUT_INTERRUPTED_SAVE
};

enum class Display3DMode
{
    MINUS_ONE = -1,
    NONE = 0,
    YES = 1,
    B_COMMAND = 2
};

enum class ShowFile
{
    REQUEST_IMAGE = -1,
    LOAD_IMAGE = 0,
    IMAGE_LOADED = 1,
};

extern std::string           g_last_map_name;
extern bool                  g_colors_preloaded;
extern std::filesystem::path g_parameter_file;
extern std::string           g_parameter_set_name;
extern int                   g_bf_digits;
extern Display3DMode         g_display_3d;
extern bool                  g_dither_flag;
extern bool                  g_escape_exit;
extern bool                  g_fast_restore;
extern bool                  g_finite_attractor;
extern bool                  g_first_init;
extern std::string           g_image_filename_mask;
extern BatchMode             g_init_batch;
extern bool                  g_overlay_3d;
extern bool                  g_overwrite_file;
extern bool                  g_read_color;
extern RecordColorsMode      g_record_colors;
extern ShowFile              g_show_file;
extern int                   g_stop_pass;
extern bool                  g_targa_out;
extern int                   g_transparent_color_3d[];
extern bool                  g_use_center_mag;
extern InitOrbitMode         g_use_init_orbit;

void cmd_files(int argc, const char *const *argv);
CmdArgFlags load_commands(std::FILE *infile);
void set_3d_defaults();
int init_msg(const char *cmd_str, const char *bad_filename, CmdFile mode);
CmdArgFlags cmd_arg(char *cur_arg, CmdFile mode);

} // namespace id::engine
