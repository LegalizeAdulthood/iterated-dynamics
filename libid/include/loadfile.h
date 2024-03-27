#pragma once

#include <string>

#include "port.h"

#define INFO_ID         "Fractal"

/*
 * Note: because big endian machines store structures differently, we have
 * to do special processing of the FRACTAL_INFO structure in decode_info.cpp.
 * Make sure changes to the structure here get reflected there.
 */
#define FRACTAL_INFO_VERSION 17  // file version, independent of system
// increment this EVERY time the fractal_info structure changes

// TODO: instead of hacking the padding here, adjust the code that reads this structure
#if defined(_WIN32)
#pragma pack(push, 1)
#endif
struct FRACTAL_INFO         // for saving data in GIF file
{
    char  info_id[8];       // Unique identifier for info block
    short iterationsold;    // Pre version 18.24
    short fractal_type;     // 0=Mandelbrot 1=Julia 2= ...
    double xmin;
    double xmax;
    double ymin;
    double ymax;
    double creal;
    double cimag;
    short videomodeax;
    short videomodebx;
    short videomodecx;
    short videomodedx;
    short dotmode;
    short xdots;
    short ydots;
    short colors;
    short version;          // used to be 'future[0]'
    float parm3;
    float parm4;
    float potential[3];
    short rseed;
    short rflag;
    short biomorph;
    short inside;
    short logmapold;
    float invert[3];
    short decomp[2];
    short symmetry;
    // version 2 stuff
    short init3d[16];
    short previewfactor;
    short xtrans;
    short ytrans;
    short red_crop_left;
    short red_crop_right;
    short blue_crop_left;
    short blue_crop_right;
    short red_bright;
    short blue_bright;
    short xadjust;
    short eyeseparation;
    short glassestype;
    // version 3 stuff, release 13
    short outside;
    // version 4 stuff, release 14
    double x3rd;          // 3rd corner
    double y3rd;
    char stdcalcmode;     // 1/2/g/b
    char useinitorbit;    // init Mandelbrot orbit flag
    short calc_status;    // resumable, finished, etc
    long tot_extend_len;  // total length of extension blocks in .gif file
    short distestold;
    short floatflag;
    short bailoutold;
    long calctime;
    BYTE trigndx[4];      // which trig functions selected
    short finattract;
    double initorbit[2];  // init Mandelbrot orbit values
    short periodicity;    // periodicity checking
    // version 5 stuff, release 15
    short pot16bit;       // save 16 bit continuous potential info
    float faspectratio;   // finalaspectratio, y/x
    short system;         // 0 for dos, 1 for windows
    short release;        // release number, with 2 decimals implied
    short display_3d;     // stored only for now, for future use
    short transparent[2];
    short ambient;
    short haze;
    short randomize;
    // version 6 stuff, release 15.x
    short rotate_lo;
    short rotate_hi;
    short distestwidth;
    // version 7 stuff, release 16
    double dparm3;
    double dparm4;
    // version 8 stuff, release 17
    short fillcolor;
    // version 9 stuff, release 18
    double mxmaxfp;
    double mxminfp;
    double mymaxfp;
    double myminfp;
    short zdots;
    float originfp;
    float depthfp;
    float heightfp;
    float widthfp;
    float distfp;
    float eyesfp;
    short orbittype;
    short juli3Dmode;
    short maxfn;
    short inversejulia;
    double dparm5;
    double dparm6;
    double dparm7;
    double dparm8;
    double dparm9;
    double dparm10;
    // version 10 stuff, release 19
    long bailout;
    short bailoutest;
    long iterations;
    short bf_math;
    short bflength;
    short yadjust;        // yikes! we left this out ages ago!
    short old_demm_colors;
    long logmap;
    long distest;
    double dinvert[3];
    short logcalc;
    short stoppass;
    short quick_calc;
    double closeprox;
    short nobof;
    long orbit_interval;
    short orbit_delay;
    double math_tol[2];
    short future[7];     // for stuff we haven't thought of yet
};

struct formula_info         // for saving formula data in GIF file
{
    char  form_name[40];
    short uses_p1;
    short uses_p2;
    short uses_p3;
    short uses_ismand;
    short ismand;
    short uses_p4;
    short uses_p5;
    short future[6];       // for stuff we haven't thought of, yet
};

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

/*
 * Note: because big endian machines store structures differently, we have
 * to do special processing of the ORBITS_INFO structure in decode_info.cpp.
 * Make sure changes to the structure here get reflected there.
 */
struct ORBITS_INFO      // for saving orbits data in a GIF file
{
    double oxmin;
    double oxmax;
    double oymin;
    double oymax;
    double ox3rd;
    double oy3rd;
    short keep_scrn_coords;
    char drawmode;
    char dummy; // need an even number of bytes
    short future[74];      // total of 200 bytes
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

int read_overlay();
void set_if_old_bif();
void set_function_parm_defaults();
int fgetwindow();
void backwards_v18();
void backwards_v19();
void backwards_v20();
