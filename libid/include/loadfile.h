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
#define FRACTAL_INFO_VERSION 17  // file version, independent of system
// increment this EVERY time the fractal_info structure changes

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
    std::int16_t iterationsold;    // Pre version 18.24
    std::int16_t fractal_type;     // 0=Mandelbrot 1=Julia 2= ...
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double creal;
    double cimag;
    std::int16_t videomodeax;
    std::int16_t videomodebx;
    std::int16_t videomodecx;
    std::int16_t videomodedx;
    std::int16_t dotmode;
    std::int16_t xdots;
    std::int16_t ydots;
    std::int16_t colors;
    std::int16_t version;          // used to be 'future[0]'
    float parm3;
    float parm4;
    float potential[3];
    std::int16_t rseed;
    std::int16_t rflag;
    std::int16_t biomorph;
    std::int16_t inside;
    std::int16_t logmapold;
    float invert[3];
    std::int16_t decomp[2];
    std::int16_t symmetry;
    // version 2 stuff
    std::int16_t init3d[16];
    std::int16_t previewfactor;
    std::int16_t xtrans;
    std::int16_t ytrans;
    std::int16_t red_crop_left;
    std::int16_t red_crop_right;
    std::int16_t blue_crop_left;
    std::int16_t blue_crop_right;
    std::int16_t red_bright;
    std::int16_t blue_bright;
    std::int16_t xadjust;
    std::int16_t eyeseparation;
    std::int16_t glassestype;
    // version 3 stuff, release 13
    std::int16_t outside;
    // version 4 stuff, release 14
    double x3rd;          // 3rd corner
    double y3rd;
    char stdcalcmode;     // 1/2/g/b
    char useinitorbit;    // init Mandelbrot orbit flag
    std::int16_t calc_status;    // resumable, finished, etc
    std::int32_t tot_extend_len;  // total length of extension blocks in .gif file
    std::int16_t distestold;
    std::int16_t floatflag;
    std::int16_t bailoutold;
    std::int32_t calctime;
    std::uint8_t trigndx[4];      // which trig functions selected
    std::int16_t finattract;
    double initorbit[2];  // init Mandelbrot orbit values
    std::int16_t periodicity;    // periodicity checking
    // version 5 stuff, release 15
    std::int16_t pot16bit;       // save 16 bit continuous potential info
    float faspectratio;   // finalaspectratio, y/x
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
    std::int16_t distestwidth;
    // version 7 stuff, release 16
    double dparm3;
    double dparm4;
    // version 8 stuff, release 17
    std::int16_t fillcolor;
    // version 9 stuff, release 18
    double mxmaxfp;
    double mxminfp;
    double mymaxfp;
    double myminfp;
    std::int16_t zdots;
    float originfp;
    float depthfp;
    float heightfp;
    float widthfp;
    float distfp;
    float eyesfp;
    std::int16_t orbittype;
    std::int16_t juli3Dmode;
    std::int16_t maxfn;
    std::int16_t inversejulia;
    double dparm5;
    double dparm6;
    double dparm7;
    double dparm8;
    double dparm9;
    double dparm10;
    // version 10 stuff, release 19
    std::int32_t bailout;
    std::int16_t bailoutest;
    std::int32_t iterations;
    std::int16_t bf_math;
    std::int16_t g_bf_length;
    std::int16_t yadjust;        // yikes! we left this out ages ago!
    std::int16_t old_demm_colors;
    std::int32_t logmap;
    std::int32_t distest;
    double dinvert[3];
    std::int16_t logcalc;
    std::int16_t stoppass;
    std::int16_t quick_calc;
    double closeprox;
    std::int16_t nobof;
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
    double oxmin;
    double oxmax;
    double oymin;
    double oymax;
    double ox3rd;
    double oy3rd;
    std::int16_t keep_scrn_coords;
    char drawmode;
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
extern bool                  g_ld_check;
extern bool                  g_loaded_3d;
extern short                 g_skip_x_dots;
extern short                 g_skip_y_dots;

int read_overlay();
void set_if_old_bif();
void set_function_parm_defaults();
int fgetwindow();
void backwards_v18();
void backwards_v19();
void backwards_v20();
