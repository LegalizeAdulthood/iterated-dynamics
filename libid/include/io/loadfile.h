// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdint>
#include <string>

#define INFO_ID         "Fractal"

/*
 * Note: because big endian machines store structures differently, we have
 * to do special processing of the FractalInfo structure in decode_info.cpp.
 * Make sure changes to the structure here get reflected there.
 */
enum
{
    FRACTAL_INFO_VERSION = 17
    // file version, independent of system
    // increment this EVERY time the fractal_info structure changes
};

// TODO: instead of hacking the padding here, adjust the code that reads this structure
#if defined(_WIN32)
#pragma pack(push, 1)
#define ID_PACKED
#else
#define ID_PACKED __attribute__((packed))
#endif
struct FractalInfo         // for saving data in GIF file
{
    char  info_id[8];       // Unique identifier for info block
    std::int16_t iterations_old;    // Pre version 18.24
    std::int16_t fractal_type;     // 0=Mandelbrot 1=Julia 2= ...
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double c_real;
    double c_imag;
    std::int16_t ax;
    std::int16_t bx;
    std::int16_t cx;
    std::int16_t dx;
    std::int16_t dot_mode;
    std::int16_t x_dots;
    std::int16_t y_dots;
    std::int16_t colors;
    std::int16_t version;          // used to be 'future[0]'
    float param3;
    float param4;
    float potential[3];
    std::int16_t random_seed;
    std::int16_t random_seed_flag;
    std::int16_t biomorph;
    std::int16_t inside;
    std::int16_t log_map_old;
    float invert[3];
    std::int16_t decomp[2];
    std::int16_t symmetry;
    // version 2 stuff
    std::int16_t init3d[16];
    std::int16_t preview_factor;
    std::int16_t x_trans;
    std::int16_t y_trans;
    std::int16_t red_crop_left;
    std::int16_t red_crop_right;
    std::int16_t blue_crop_left;
    std::int16_t blue_crop_right;
    std::int16_t red_bright;
    std::int16_t blue_bright;
    std::int16_t x_adjust;
    std::int16_t eye_separation;
    std::int16_t glasses_type;
    // version 3 stuff, release 13
    std::int16_t outside;
    // version 4 stuff, release 14
    double x3rd;          // 3rd corner
    double y3rd;
    char std_calc_mode;     // 1/2/g/b
    char use_init_orbit;    // init Mandelbrot orbit flag
    std::int16_t calc_status;    // resumable, finished, etc
    std::int32_t tot_extend_len;  // total length of extension blocks in .gif file
    std::int16_t dist_est_old;
    std::int16_t float_flag;
    std::int16_t bailout_old;
    std::int32_t calc_time;
    std::uint8_t trig_index[4];      // which trig functions selected
    std::int16_t finite_attractor;
    double init_orbit[2];  // init Mandelbrot orbit values
    std::int16_t periodicity;    // periodicity checking
    // version 5 stuff, release 15
    std::int16_t pot16bit;       // save 16 bit continuous potential info
    float final_aspect_ratio;   // finalaspectratio, y/x
    std::int16_t system;         // 0 for dos, 1 for windows
    std::int16_t release;        // release number, with 2 decimals implied
    std::int16_t display_3d;     // stored only for now, for future use
    std::int16_t transparent[2];
    std::int16_t ambient;
    std::int16_t haze;
    std::int16_t randomize;
    // version 6 stuff, release 15.x
    std::int16_t rotate_lo;
    std::int16_t rotate_hi;
    std::int16_t dist_est_width;
    // version 7 stuff, release 16
    double d_param3;
    double d_param4;
    // version 8 stuff, release 17
    std::int16_t fill_color;
    // version 9 stuff, release 18
    double julibrot_x_max;
    double julibrot_x_min;
    double julibrot_y_max;
    double julibrot_y_min;
    std::int16_t julibrot_z_dots;
    float julibrot_origin_fp;
    float julibrot_depth_fp;
    float julibrot_height_fp;
    float julibrot_width_fp;
    float julibrot_dist_fp;
    float eyes_fp;
    std::int16_t orbit_type;
    std::int16_t juli3d_mode;
    std::int16_t max_fn;
    std::int16_t inverse_julia;
    double d_param5;
    double d_param6;
    double d_param7;
    double d_param8;
    double d_param9;
    double d_param10;
    // version 10 stuff, release 19
    std::int32_t bailout;
    std::int16_t bailout_test;
    std::int32_t iterations;
    std::int16_t bf_math;
    std::int16_t bf_length;
    std::int16_t y_adjust;        // yikes! we left this out ages ago!
    std::int16_t old_demm_colors;
    std::int32_t log_map;
    std::int32_t dist_est;
    double d_invert[3];
    std::int16_t log_calc;
    std::int16_t stop_pass;
    std::int16_t quick_calc;
    double close_prox;
    std::int16_t no_bof;
    std::int32_t orbit_interval;
    std::int16_t orbit_delay;
    double math_tol[2];
    std::int16_t future[7];     // for stuff we haven't thought of yet
} ID_PACKED;

struct FormulaInfo         // for saving formula data in GIF file
{
    char  form_name[40];
    std::int16_t uses_p1;
    std::int16_t uses_p2;
    std::int16_t uses_p3;
    std::int16_t uses_ismand;
    std::int16_t ismand;
    std::int16_t uses_p4;
    std::int16_t uses_p5;
    std::int16_t future[6];       // for stuff we haven't thought of, yet
} ID_PACKED;

struct ExtBlock3
{
    bool got_data;
    int32_t length;
    char form_name[40];
    std::int16_t uses_p1;
    std::int16_t uses_p2;
    std::int16_t uses_p3;
    std::int16_t uses_ismand;
    std::int16_t ismand;
    std::int16_t uses_p4;
    std::int16_t uses_p5;
} ID_PACKED;

/*
 * Note: because big endian machines store structures differently, we have
 * to do special processing of the OrbitsInfo structure in decode_info.cpp.
 * Make sure changes to the structure here get reflected there.
 */
struct OrbitsInfo      // for saving orbits data in a GIF file
{
    double orbit_corner_min_x;
    double orbit_corner_max_x;
    double orbit_corner_min_y;
    double orbit_corner_max_y;
    double orbit_corner_3rd_x;
    double orbit_corner_3rd_y;
    std::int16_t keep_screen_coords;
    char draw_mode;
    char dummy; // need an even number of bytes
    std::int16_t future[74];      // total of 200 bytes
} ID_PACKED;
#if defined(_WIN32)
#pragma pack(pop)
#endif

bool operator==(const FractalInfo &lhs, const FractalInfo &rhs);
inline bool operator!=(const FractalInfo &lhs, const FractalInfo &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const FormulaInfo &lhs, const FormulaInfo &rhs);
inline bool operator!=(const FormulaInfo &lhs, const FormulaInfo &rhs)
{
    return !(lhs == rhs);
}

bool operator==(const OrbitsInfo &lhs, const OrbitsInfo &rhs);
inline bool operator!=(const OrbitsInfo &lhs, const OrbitsInfo &rhs)
{
    return !(lhs == rhs);
}

extern bool                  g_bad_outside;
extern std::string           g_browse_name;
extern float                 g_file_aspect_ratio;
extern int                   g_file_colors;
extern int                   g_file_x_dots;
extern int                   g_file_y_dots;
extern bool                  g_loaded_3d;
extern short                 g_skip_x_dots;
extern short                 g_skip_y_dots;

int read_overlay();
void set_if_old_bif();
void set_function_param_defaults();
int file_get_window();
void backwards_v18();
void backwards_v19();
void backwards_v20();
