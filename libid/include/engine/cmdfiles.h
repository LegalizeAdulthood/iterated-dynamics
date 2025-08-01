// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

#include <config/port.h>

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

struct FractalSpecific;

enum TextColors
{
    BLACK = 0,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN, // dirty yellow on cga
    WHITE,

    // use values below this for foreground only, they don't work background
    GRAY, // don't use this much - is black on cga
    LT_BLUE,
    LT_GREEN,
    LT_CYAN,
    LT_RED,
    LT_MAGENTA,
    YELLOW,
    LT_WHITE,
    INVERSE = 0x8000,
    BRIGHT = 0x4000
};
// and their use:
#define C_TITLE           (g_text_color[0] | BRIGHT)
#define C_TITLE_DEV       (g_text_color[1])
#define C_HELP_HDG        (g_text_color[2] | BRIGHT)
#define C_HELP_BODY       (g_text_color[3])
#define C_HELP_INSTR      (g_text_color[4])
#define C_HELP_LINK       (g_text_color[5] | BRIGHT)
#define C_HELP_CURLINK    (g_text_color[6] | INVERSE)
#define C_PROMPT_BKGRD    (g_text_color[7])
#define C_PROMPT_TEXT     (g_text_color[8])
#define C_PROMPT_LO       (g_text_color[9])
#define C_PROMPT_MED      (g_text_color[10])
#define C_PROMPT_HI       (g_text_color[11] | BRIGHT)
#define C_PROMPT_INPUT    (g_text_color[12] | INVERSE)
#define C_PROMPT_CHOOSE   (g_text_color[13] | INVERSE)
#define C_CHOICE_CURRENT  (g_text_color[14] | INVERSE)
#define C_CHOICE_SP_INSTR (g_text_color[15])
#define C_CHOICE_SP_KEYIN (g_text_color[16] | BRIGHT)
#define C_GENERAL_HI      (g_text_color[17] | BRIGHT)
#define C_GENERAL_MED     (g_text_color[18])
#define C_GENERAL_LO      (g_text_color[19])
#define C_GENERAL_INPUT   (g_text_color[20] | INVERSE)
#define C_DVID_BKGRD      (g_text_color[21])
#define C_DVID_HI         (g_text_color[22] | BRIGHT)
#define C_DVID_LO         (g_text_color[23])
#define C_STOP_ERR        (g_text_color[24] | BRIGHT)
#define C_STOP_INFO       (g_text_color[25] | BRIGHT)
#define C_TITLE_LOW       (g_text_color[26])
#define C_AUTH_DIV1       (g_text_color[27] | INVERSE)
#define C_AUTH_DIV2       (g_text_color[28] | INVERSE)
#define C_PRIMARY         (g_text_color[29])
#define C_CONTRIB         (g_text_color[30])

enum class InitOrbitMode
{
    NORMAL = 0,
    VALUE = 1,
    PIXEL = 2
};

enum class TrueColorMode
{
    DEFAULT_COLOR = 0,
    ITERATE = 1
};

enum class RecordColorsMode
{
    NONE = 0,
    AUTOMATIC = 'a',
    COMMENT = 'c',
    YES = 'y'
};

enum OrbitSaveFlags
{
    OSF_RAW = 1,
    OSF_MIDI = 2
};

enum SoundFlags
{
    SOUNDFLAG_OFF        = 0,
    SOUNDFLAG_BEEP       = 1,
    SOUNDFLAG_X          = 2,
    SOUNDFLAG_Y          = 3,
    SOUNDFLAG_Z          = 4,
    SOUNDFLAG_ORBIT_MASK = 0x07,
    SOUNDFLAG_SPEAKER    = 8,
    SOUNDFLAG_OPL3_FM    = 16,
    SOUNDFLAG_MIDI       = 32,
    SOUNDFLAG_QUANTIZED  = 64,
    SOUNDFLAG_MASK       = 0x7F
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
inline int operator+(CmdArgFlags value)
{
    return static_cast<int>(value);
}
inline CmdArgFlags operator|(CmdArgFlags lhs, CmdArgFlags rhs)
{
    return static_cast<CmdArgFlags>(+lhs | +rhs);
}
inline CmdArgFlags &operator|=(CmdArgFlags &lhs, CmdArgFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}
inline bool bit_set(CmdArgFlags flags, CmdArgFlags bit)
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

enum class IFSDimension
{
    TWO = 2,
    THREE = 3,
};

extern bool                  g_ask_video;
extern float                 g_aspect_drift;
extern long                  g_bailout;
extern int                   g_base_hertz;
extern int                   g_biomorph;
extern bool                  g_bof_match_book_images;
extern bool                  g_check_cur_dir;
extern int                   g_color_cycle_range_hi;
extern int                   g_color_cycle_range_lo;
extern std::string           g_last_map_name;
extern bool                  g_colors_preloaded;
extern std::filesystem::path g_parameter_file;
extern std::string           g_parameter_set_name;
extern int                   g_cycle_limit;
extern int                   g_decomp[];
extern int                   g_bf_digits;
extern Display3DMode         g_display_3d;
extern long                  g_distance_estimator;
extern int                   g_distance_estimator_width_factor;
extern int                   g_distance_estimator_x_dots;
extern int                   g_distance_estimator_y_dots;
extern bool                  g_dither_flag;
extern bool                  g_escape_exit;
extern bool                  g_fast_restore;
extern int                   g_fill_color;
extern bool                  g_finite_attractor;
extern bool                  g_first_init;
extern std::filesystem::path g_formula_filename;
extern std::string           g_formula_name;
extern std::string           g_image_filename_mask;
extern std::vector<float>    g_ifs_definition;
extern std::filesystem::path g_ifs_filename;
extern std::string           g_ifs_name;
extern IFSDimension          g_ifs_dim;
extern BatchMode             g_init_batch;
extern int                   g_init_cycle_limit;
extern int                   g_init_mode;
extern DComplex              g_init_orbit;
extern double                g_inversion[];
extern std::vector<int>      g_iteration_ranges;
extern int                   g_iteration_ranges_len;
extern std::filesystem::path g_l_system_filename;
extern std::string           g_l_system_name;
extern Byte                  g_map_clut[256][3];
extern bool                  g_map_specified;
extern double                g_math_tol[2];
extern bool                  g_new_bifurcation_functions_loaded;
extern int                   g_orbit_delay;
extern int                   g_orbit_save_flags;
extern std::string           g_orbit_save_name;
extern bool                  g_overlay_3d;
extern bool                  g_overwrite_file;
extern bool                  g_potential_16bit;
extern bool                  g_potential_flag;
extern bool                  g_read_color;
extern std::filesystem::path g_read_filename;
extern RecordColorsMode      g_record_colors;
extern std::filesystem::path g_save_filename;
extern float                 g_screen_aspect;
extern ShowFile              g_show_file;
extern int                   g_sound_flag;
extern bool                  g_start_show_orbit;
extern int                   g_stop_pass;
extern bool                  g_targa_out;
extern std::filesystem::path g_temp_dir;
extern Byte                  g_text_color[];
extern int                   g_transparent_color_3d[];
extern TrueColorMode         g_true_mode;
extern bool                  g_true_color;
extern bool                  g_use_center_mag;
extern InitOrbitMode         g_use_init_orbit;
extern int                   g_user_biomorph_value;
extern std::filesystem::path g_working_dir;

void cmd_files(int argc, const char *const *argv);
CmdArgFlags load_commands(std::FILE *infile);
void set_3d_defaults();
int init_msg(const char *cmd_str, const char *bad_filename, CmdFile mode);
CmdArgFlags cmd_arg(char *cur_arg, CmdFile mode);
