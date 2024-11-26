// SPDX-License-Identifier: GPL-3.0-only
//
//**********************************************************************
// This file contains a 3D replacement for the out_line function called
// by the decoder. The purpose is to apply various 3D transformations
// before displaying points. Called once per line of the input file.
//**********************************************************************
#include "port.h"
#include "prototyp.h"

#include "line3d.h"

#include "3d.h"
#include "calcfrac.h"
#include "check_write_file.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "dir_file.h"
#include "diskvid.h"
#include "drivers.h"
#include "fixed_pt.h"
#include "fractals.h"
#include "framain2.h"
#include "gifview.h"
#include "id.h"
#include "id_data.h"
#include "loadfile.h"
#include "os.h"
#include "pixel_limits.h"
#include "plot3d.h"
#include "rotate.h"
#include "save_file.h"
#include "stereo.h"
#include "stop_msg.h"
#include "version.h"
#include "video.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <vector>

struct point
{
    int x;
    int y;
    int color;
};

struct f_point
{
    float x;
    float y;
    float color;
};

struct minmax
{
    int minx;
    int maxx;
};

// routines in this module
static int first_time(int, VECTOR);
static void hsv_to_rgb(
    BYTE *red, BYTE *green, BYTE *blue, unsigned long hue, unsigned long sat, unsigned long val);
static int line3dmem();
static int rgb_to_hsv(
    BYTE red, BYTE green, BYTE blue, unsigned long *hue, unsigned long *sat, unsigned long *val);
static bool set_pixel_buff(BYTE *pixels, BYTE *fraction, unsigned linelen);
static void set_upr_lwr();
static int end_object(bool triout);
static int offscreen(point);
static int out_triangle(f_point, f_point, f_point, int, int, int);
static int RAY_Header();
static int start_object();
static void draw_light_box(double *, double *, MATRIX);
static void draw_rect(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color, bool rect);
static void line3d_cleanup();
static void clipcolor(int, int, int);
static void interpcolor(int, int, int);
static void putatriangle(point, point, point, int);
static void putminmax(int, int, int);
static void triangle_bounds(float pt_t[3][3]);
static void T_clipcolor(int, int, int);
static void vdraw_line(double *, double *, int color);
static void File_Error(char const *File_Name1, int ERROR);

// static variables
static void (*s_fill_plot)(int x, int y, int color){};   //
static void (*s_normal_plot)(int x, int y, int color){}; //
static float s_delta_phi{};                              // increment of latitude, longitude
static double s_r_scale{};                               // surface roughness factor
static long s_x_center{}, s_y_center{};                  // circle center
static double s_scale_x{}, s_scale_y{}, s_scale_z{};     // scale factors
static double s_radius{};                                // radius values
static double s_radius_factor{};                         // for intermediate calculation
static LMATRIX s_llm{};                                  // ""
static LVECTOR s_l_view{};                               // for perspective views
static double s_z_cutoff{};                              // perspective backside cutoff value
static float s_two_cos_delta_phi{};                      //
static float s_cos_phi{}, s_sin_phi{};                   // precalculated sin/cos of longitude
static float s_old_cos_phi1{}, s_old_sin_phi1{};         //
static float s_old_cos_phi2{}, s_old_sin_phi2{};         //
static std::vector<BYTE> s_fraction;                     // float version of pixels array
static float s_min_xyz[3]{}, s_max_xyz[3]{};             // For Raytrace output
static int s_line_length1{};                             //
static int s_targa_header_24 = 18;                       // Size of current Targa-24 header
static std::FILE *s_file_ptr1{};                         //
static unsigned int s_i_ambient{};                       //
static int s_rand_factor{};                              //
static int s_haze_mult{};                                //
static BYTE s_t24 = 24;                                  //
static BYTE s_t32 = 32;                                  //
static BYTE s_upr_lwr[4]{};                              //
static bool s_t_safe{};                          // Original Targa Image successfully copied to targa_temp
static VECTOR s_light_direction{};               //
static BYTE s_real_color{};                      // Actual color of cur pixel
static int s_ro{}, s_co{}, s_co_max{};           // For use in Acrospin support
static int s_local_preview_factor{};             //
static int s_z_coord = 256;                      //
static double s_aspect{};                        // aspect ratio
static int s_even_odd_row{};                     //
static std::vector<float> s_sin_theta_array;     // all sine thetas go here
static std::vector<float> s_cos_theta_array;     // all cosine thetas go here
static double s_rXr_scale{};                     // precalculation factor
static bool s_persp{};                           // flag for indicating perspective transformations
static point s_p1{}, s_p2{}, s_p3{};             //
static f_point s_f_bad{};                        // out of range value
static point s_bad{};                            // out of range value
static long s_num_tris{};                        // number of triangles output to ray trace file
static std::vector<f_point> s_f_last_row;        //
static int s_real_v{};                           // Actual value of V for fillytpe>4 monochrome images
static int s_error{};                            //
static std::string s_targa_temp("fractemp.tga"); //
static int s_p = 250;                            // Perspective dist used when viewing light vector
static int const s_bad_check = -3000;            // check values against this to determine if good
static std::vector<point> s_last_row;            // this array remembers the previous line
static std::vector<minmax> s_min_max_x;          // array of min and max x values used in triangle fill
static VECTOR s_cross{};                         //
static VECTOR s_tmp_cross{};                     //
static point s_old_last{};                       // old pixels

// global variables defined here
void (*g_standard_plot)(int, int, int){};
MATRIX g_m{}; // transformation matrix
int g_ambient{};
int g_randomize_3d{};
int g_haze{};
std::string g_light_name{"fract001"};
bool g_targa_overlay{};
BYTE g_background_color[3]{};
std::string g_raytrace_filename{"fract001"};
bool g_preview{};
bool g_show_box{};
int g_preview_factor{20};
int g_converge_x_adjust{};
int g_converge_y_adjust{};
int g_xx_adjust{};
int g_yy_adjust{};
int g_x_shift{};
int g_y_shift{};
int const g_bad_value{-10000};         // set bad values to this
raytrace_formats g_raytrace_format{};  // Flag to generate Ray trace compatible files in 3d
bool g_brief{};                        // 1 = short ray trace files
VECTOR g_view{};                       // position of observer for perspective

int line3d(BYTE * pixels, unsigned linelen)
{
    int RND;
    float f_water = 0.0F;       // transformed WATERLINE for ray trace files
    double r0;
    int xcenter0 = 0;
    int ycenter0 = 0;      // Unfudged versions
    double r;                    // sphere radius
    float costheta;
    float sintheta; // precalculated sin/cos of latitude
    int next;       // used by preview and grid
    int col;        // current column (original GIF)
    point cur;      // current pixels
    point old;      // old pixels
    f_point f_cur = { 0.0 };
    f_point f_old;
    VECTOR v;                    // double vector
    VECTOR v1;
    VECTOR v2;
    VECTOR crossavg;
    bool crossnotinit;           // flag for crossavg init indication
    LVECTOR lv;                  // long equivalent of v
    LVECTOR lv0;                 // long equivalent of v
    int lastdot;
    long fudge;

    fudge = 1L << 16;

    if (g_transparent_color_3d[0] || g_transparent_color_3d[1])
    {
        s_normal_plot = T_clipcolor;
        g_plot = s_normal_plot;          // Use transparent plot function
    }
    else                            // Use the usual plot function with clipping
    {
        s_normal_plot = clipcolor;
        g_plot = s_normal_plot;
    }

    g_current_row = g_row_count;           // use separate variable to allow for pot16bit files
    if (g_potential_16bit)
    {
        g_current_row >>= 1;
    }

    //**********************************************************************
    // This IF clause is executed ONCE per image. All precalculations are
    // done here, with out any special concern about speed. DANGER -
    // communication with the rest of the program is generally via static
    // or global variables.
    //**********************************************************************
    if (g_row_count++ == 0)
    {
        int err = first_time(linelen, v);
        if (err != 0)
        {
            return err;
        }
        if (g_logical_screen_x_dots > OLD_MAX_PIXELS)
        {
            return -1;
        }
        crossavg[0] = 0;
        crossavg[1] = 0;
        crossavg[2] = 0;
        s_x_center = g_logical_screen_x_dots / 2 + g_x_shift;
        xcenter0 = (int) s_x_center;
        s_y_center = g_logical_screen_y_dots / 2 - g_y_shift;
        ycenter0 = (int) s_y_center;
    }
    // make sure these pixel coordinates are out of range
    old = s_bad;
    f_old = s_f_bad;

    // copies pixels buffer to float type fraction buffer for fill purposes
    if (g_potential_16bit)
    {
        if (set_pixel_buff(pixels, &s_fraction[0], linelen))
        {
            return 0;
        }
    }
    else if (g_gray_flag)             // convert color numbers to grayscale values
    {
        for (col = 0; col < (int) linelen; col++)
        {
            int pal;
            int colornum;
            colornum = pixels[col];
            // effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255
            pal = ((int) g_dac_box[colornum][0] * 77 +
                   (int) g_dac_box[colornum][1] * 151 +
                   (int) g_dac_box[colornum][2] * 28);
            pal >>= 6;
            pixels[col] = (BYTE) pal;
        }
    }
    crossnotinit = true;
    col = 0;

    s_co = 0;

    //***********************************************************************
    // This section of code allows the operation of a preview mode when the
    // preview flag is set. Enabled, it allows the drawing of only the first
    // line of the source image, then every 10th line, until and including
    // the last line. For the undrawn lines, only necessary calculations are
    // made. As a bonus, in non-sphere mode a box is drawn to help visualize
    // the effects of 3D transformations. Thanks to Marc Reinig for this idea
    // and code.
    //***********************************************************************
    lastdot = std::min(g_logical_screen_x_dots - 1, (int) linelen - 1);
    if (g_fill_type >= fill_type::LIGHT_SOURCE_BEFORE)
    {
        if (g_haze && g_targa_out)
        {
            s_haze_mult = (int)(g_haze * (
                                  (float)((long)(g_logical_screen_y_dots - 1 - g_current_row) *
                                          (long)(g_logical_screen_y_dots - 1 - g_current_row)) /
                                  (float)((long)(g_logical_screen_y_dots - 1) * (long)(g_logical_screen_y_dots - 1))));
            s_haze_mult = 100 - s_haze_mult;
        }
    }

    if (g_preview_factor >= g_logical_screen_y_dots || g_preview_factor > lastdot)
    {
        g_preview_factor = std::min(g_logical_screen_y_dots - 1, lastdot);
    }

    s_local_preview_factor = g_logical_screen_y_dots / g_preview_factor;

    bool tout = false;          // triangle has been sent to ray trace file
    // Insure last line is drawn in preview and filltypes <0
    // Draw mod preview lines
    if ((g_raytrace_format != raytrace_formats::none || g_preview || g_fill_type < fill_type::POINTS) //
        && g_current_row != g_logical_screen_y_dots - 1                                               //
        && g_current_row % s_local_preview_factor                                                         //
        && !(g_raytrace_format == raytrace_formats::none                                              //
               && g_fill_type > fill_type::SOLID_FILL                                                 //
               && g_current_row == 1))
    {
        // Get init geometry in lightsource modes
        goto reallythebottom;     // skip over most of the line3d calcs
    }
    if (driver_diskp())
    {
        char s[40];
        std::snprintf(s, std::size(s), "mapping to 3d, reading line %d", g_current_row);
        dvid_status(1, s);
    }

    if (!col && g_raytrace_format != raytrace_formats::none && g_current_row != 0)
    {
        start_object();
    }
    // PROCESS ROW LOOP BEGINS HERE
    while (col < (int) linelen)
    {
        if ((g_raytrace_format != raytrace_formats::none || g_preview || g_fill_type < fill_type::POINTS)
            && (col != lastdot)             // if this is not the last col
                                            // if not the 1st or mod factor col
            && (col % (int)(s_aspect * s_local_preview_factor))
            && !(g_raytrace_format == raytrace_formats::none && g_fill_type > fill_type::SOLID_FILL && col == 1))
        {
            goto loopbottom;
        }

        s_real_color = pixels[col];
        cur.color = s_real_color;
        f_cur.color = (float) cur.color;

        if (g_raytrace_format != raytrace_formats::none|| g_preview || g_fill_type < fill_type::POINTS)
        {
            next = (int)(col + s_aspect * s_local_preview_factor);
            if (next == col)
            {
                next = col + 1;
            }
        }
        else
        {
            next = col + 1;
        }
        if (next >= lastdot)
        {
            next = lastdot;
        }

        if (cur.color > 0 && cur.color < g_water_line)
        {
            s_real_color = (BYTE) g_water_line;
            cur.color = s_real_color;
            f_cur.color = (float) cur.color;  // "lake"
        }
        else if (g_potential_16bit)
        {
            f_cur.color += ((float) s_fraction[col]) / (float)(1 << 8);
        }

        if (g_sphere)            // sphere case
        {
            sintheta = s_sin_theta_array[col];
            costheta = s_cos_theta_array[col];

            if (s_sin_phi < 0 && !(g_raytrace_format != raytrace_formats::none|| g_fill_type < fill_type::POINTS))
            {
                cur = s_bad;
                f_cur = s_f_bad;
                goto loopbottom; // another goto !
            }
            //**********************************************************
            // KEEP THIS FOR DOCS - original formula --
            // if (rscale < 0.0)
            // r = 1.0+((double)cur.color/(double)zcoord)*rscale;
            // else
            // r = 1.0-rscale+((double)cur.color/(double)zcoord)*rscale;
            // R = (double)ydots/2;
            // r = r*R;
            // cur.x = xdots/2 + sclx*r*sintheta*aspect + xup ;
            // cur.y = ydots/2 + scly*r*costheta*cosphi - yup ;
            //**********************************************************

            if (s_r_scale < 0.0)
            {
                r = s_radius + s_radius_factor * (double) f_cur.color * costheta;
            }
            else if (s_r_scale > 0.0)
            {
                r = s_radius - s_rXr_scale + s_radius_factor * (double) f_cur.color * costheta;
            }
            else
            {
                r = s_radius;
            }
            // Allow Ray trace to go through so display ok
            if (s_persp || g_raytrace_format != raytrace_formats::none)
            {
                // how do lv[] and cur and f_cur all relate
                // NOTE: fudge was pre-calculated above in r and R
                // (almost) guarantee negative
                lv[2] = (long)(-s_radius - r * costheta * s_sin_phi);      // z
                if ((lv[2] > s_z_cutoff) && !(g_fill_type < fill_type::POINTS))
                {
                    cur = s_bad;
                    f_cur = s_f_bad;
                    goto loopbottom;      // another goto !
                }
                lv[0] = (long)(s_x_center + sintheta * s_scale_x * r);   // x
                lv[1] = (long)(s_y_center + costheta * s_cos_phi * s_scale_y * r);  // y

                if ((g_fill_type >= fill_type::LIGHT_SOURCE_BEFORE) || g_raytrace_format != raytrace_formats::none)
                {
                    // calculate illumination normal before persp

                    r0 = r / 65536L;
                    f_cur.x = (float)(xcenter0 + sintheta * s_scale_x * r0);
                    f_cur.y = (float)(ycenter0 + costheta * s_cos_phi * s_scale_y * r0);
                    f_cur.color = (float)(-r0 * costheta * s_sin_phi);
                }
                if (!(g_user_float_flag || g_raytrace_format != raytrace_formats::none))
                {
                    if (long_persp(lv, s_l_view, 16) == -1)
                    {
                        cur = s_bad;
                        f_cur = s_f_bad;
                        goto loopbottom;   // another goto !
                    }
                    cur.x = (int)(((lv[0] + 32768L) >> 16) + g_xx_adjust);
                    cur.y = (int)(((lv[1] + 32768L) >> 16) + g_yy_adjust);
                }
                if (g_user_float_flag || g_overflow || g_raytrace_format != raytrace_formats::none)
                {
                    v[0] = lv[0];
                    v[1] = lv[1];
                    v[2] = lv[2];
                    v[0] /= fudge;
                    v[1] /= fudge;
                    v[2] /= fudge;
                    perspective(v);
                    cur.x = (int)(v[0] + .5 + g_xx_adjust);
                    cur.y = (int)(v[1] + .5 + g_yy_adjust);
                }
            }
            // Not sure how this an 3rd if above relate
            else
            {
                // Why the xx- and yyadjust here and not above?
                f_cur.x = (float) (s_x_center + sintheta*s_scale_x*r + g_xx_adjust);
                cur.x = (int) f_cur.x;
                f_cur.y = (float)(s_y_center + costheta*s_cos_phi*s_scale_y*r + g_yy_adjust);
                cur.y = (int) f_cur.y;
                if (g_fill_type >= fill_type::LIGHT_SOURCE_BEFORE || g_raytrace_format != raytrace_formats::none)          // why do we do this for filltype>5?
                {
                    f_cur.color = (float)(-r * costheta * s_sin_phi * s_scale_z);
                }
                v[2] = 0;               // Why do we do this?
                v[1] = v[2];
                v[0] = v[1];
            }
        }
        else                            // non-sphere 3D
        {
            if (!g_user_float_flag && g_raytrace_format == raytrace_formats::none)
            {
                if (g_fill_type >= fill_type::LIGHT_SOURCE_BEFORE)         // flag to save vector before perspective
                {
                    lv0[0] = 1;   // in longvmultpersp calculation
                }
                else
                {
                    lv0[0] = 0;
                }

                // use 32-bit multiply math to snap this out
                lv[0] = col;
                lv[0] = lv[0] << 16;
                lv[1] = g_current_row;
                lv[1] = lv[1] << 16;
                if (g_potential_16bit)             // don't truncate fractional part
                {
                    lv[2] = (long)(f_cur.color * 65536.0);
                }
                else                              // there IS no fractional part here!
                {
                    lv[2] = (long) f_cur.color;
                    lv[2] = lv[2] << 16;
                }

                if (long_vec_mat_mul_persp(lv, s_llm, lv0, lv, s_l_view, 16) == -1)
                {
                    cur = s_bad;
                    f_cur = s_f_bad;
                    goto loopbottom;
                }

                cur.x = (int)(((lv[0] + 32768L) >> 16) + g_xx_adjust);
                cur.y = (int)(((lv[1] + 32768L) >> 16) + g_yy_adjust);
                if (g_fill_type >= fill_type::LIGHT_SOURCE_BEFORE && !g_overflow)
                {
                    f_cur.x = (float) lv0[0];
                    f_cur.x /= 65536.0F;
                    f_cur.y = (float) lv0[1];
                    f_cur.y /= 65536.0F;
                    f_cur.color = (float) lv0[2];
                    f_cur.color /= 65536.0F;
                }
            }

            if (g_user_float_flag || g_overflow || g_raytrace_format != raytrace_formats::none)
                // do in float if integer math overflowed or doing Ray trace
            {
                // slow float version for comparison
                v[0] = col;
                v[1] = g_current_row;
                v[2] = f_cur.color;      // Actually the z value

                vec_g_mat_mul(v);     // matrix*vector routine

                if (g_fill_type > fill_type::SOLID_FILL || g_raytrace_format != raytrace_formats::none)
                {
                    f_cur.x = (float) v[0];
                    f_cur.y = (float) v[1];
                    f_cur.color = (float) v[2];

                    if (g_raytrace_format == raytrace_formats::acrospin)
                    {
                        f_cur.x = f_cur.x * (2.0F / g_logical_screen_x_dots) - 1.0F;
                        f_cur.y = f_cur.y * (2.0F / g_logical_screen_y_dots) - 1.0F;
                        f_cur.color = -f_cur.color * (2.0F / g_num_colors) - 1.0F;
                    }
                }

                if (s_persp && g_raytrace_format == raytrace_formats::none)
                {
                    perspective(v);
                }
                cur.x = (int)(v[0] + g_xx_adjust + .5);
                cur.y = (int)(v[1] + g_yy_adjust + .5);

                v[0] = 0;
                v[1] = 0;
                v[2] = g_water_line;
                vec_g_mat_mul(v);
                f_water = (float) v[2];
            }
        }

        if (g_randomize_3d != 0)
        {
            if (cur.color > g_water_line)
            {
                RND = rand15() >> 8;     // 7-bit number
                RND = RND * RND >> s_rand_factor;  // n-bit number

                if (std::rand() & 1)
                {
                    RND = -RND;   // Make +/- n-bit number
                }

                if ((int)(cur.color) + RND >= g_colors)
                {
                    cur.color = g_colors - 2;
                }
                else if ((int)(cur.color) + RND <= g_water_line)
                {
                    cur.color = g_water_line + 1;
                }
                else
                {
                    cur.color = cur.color + RND;
                }
                s_real_color = (BYTE)cur.color;
            }
        }

        if (g_raytrace_format != raytrace_formats::none)
        {
            if (col && g_current_row
                && old.x > s_bad_check
                && old.x < (g_logical_screen_x_dots - s_bad_check)
                && s_last_row[col].x > s_bad_check
                && s_last_row[col].y > s_bad_check
                && s_last_row[col].x < (g_logical_screen_x_dots - s_bad_check)
                && s_last_row[col].y < (g_logical_screen_y_dots - s_bad_check))
            {
                // Get rid of all the triangles in the plane at the base of the object

                if (f_cur.color == f_water
                    && s_f_last_row[col].color == f_water
                    && s_f_last_row[next].color == f_water)
                {
                    goto loopbottom;
                }

                if (g_raytrace_format != raytrace_formats::acrospin)      // Output the vertex info
                {
                    out_triangle(f_cur, f_old, s_f_last_row[col],
                                 cur.color, old.color, s_last_row[col].color);
                }

                tout = true;

                driver_draw_line(old.x, old.y, cur.x, cur.y, old.color);
                driver_draw_line(old.x, old.y, s_last_row[col].x,
                                 s_last_row[col].y, old.color);
                driver_draw_line(s_last_row[col].x, s_last_row[col].y,
                                 cur.x, cur.y, cur.color);
                s_num_tris++;
            }

            if (col < lastdot && g_current_row
                && s_last_row[col].x > s_bad_check
                && s_last_row[col].y > s_bad_check
                && s_last_row[col].x < (g_logical_screen_x_dots - s_bad_check)
                && s_last_row[col].y < (g_logical_screen_y_dots - s_bad_check)
                && s_last_row[next].x > s_bad_check
                && s_last_row[next].y > s_bad_check
                && s_last_row[next].x < (g_logical_screen_x_dots - s_bad_check)
                && s_last_row[next].y < (g_logical_screen_y_dots - s_bad_check))
            {
                // Get rid of all the triangles in the plane at the base of the object

                if (f_cur.color == f_water
                    && s_f_last_row[col].color == f_water
                    && s_f_last_row[next].color == f_water)
                {
                    goto loopbottom;
                }

                if (g_raytrace_format != raytrace_formats::acrospin)      // Output the vertex info
                {
                    out_triangle(f_cur, s_f_last_row[col], s_f_last_row[next],
                                 cur.color, s_last_row[col].color, s_last_row[next].color);
                }

                tout = true;

                driver_draw_line(s_last_row[col].x, s_last_row[col].y, cur.x, cur.y,
                                 cur.color);
                driver_draw_line(s_last_row[next].x, s_last_row[next].y, cur.x, cur.y,
                                 cur.color);
                driver_draw_line(s_last_row[next].x, s_last_row[next].y, s_last_row[col].x,
                                 s_last_row[col].y, s_last_row[col].color);
                s_num_tris++;
            }

            if (g_raytrace_format == raytrace_formats::acrospin)       // Output vertex info for Acrospin
            {
                std::fprintf(s_file_ptr1, "% #4.4f % #4.4f % #4.4f R%dC%d\n",
                        f_cur.x, f_cur.y, f_cur.color, s_ro, s_co);
                if (s_co > s_co_max)
                {
                    s_co_max = s_co;
                }
                s_co++;
            }
            goto loopbottom;
        }

        switch (g_fill_type)
        {
        case fill_type::SURFACE_GRID:
            if (col
                && old.x > s_bad_check
                && old.x < (g_logical_screen_x_dots - s_bad_check))
            {
                driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            }
            if (g_current_row
                && s_last_row[col].x > s_bad_check
                && s_last_row[col].y > s_bad_check
                && s_last_row[col].x < (g_logical_screen_x_dots - s_bad_check)
                && s_last_row[col].y < (g_logical_screen_y_dots - s_bad_check))
            {
                driver_draw_line(s_last_row[col].x, s_last_row[col].y, cur.x,
                                 cur.y, cur.color);
            }
            break;

        case fill_type::POINTS:
            (*g_plot)(cur.x, cur.y, cur.color);
            break;

        case fill_type::WIRE_FRAME:                // connect-a-dot
            if (old.x < g_logical_screen_x_dots && col
                && old.x > s_bad_check
                && old.y > s_bad_check)        // Don't draw from old to cur on col 0
            {
                driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            }
            break;

        case fill_type::SURFACE_INTERPOLATED: // with interpolation
        case fill_type::SURFACE_CONSTANT:     // no interpolation
            //***********************************************************
            // "triangle fill" - consider four points: current point,
            // previous point same row, point opposite current point in
            // previous row, point after current point in previous row.
            // The object is to fill all points inside the two triangles.
            //
            // lastrow[col].x/y___ lastrow[next]
            // /        1                 /
            // /                1         /
            // /                       1  /
            // oldrow/col ________ trow/col
            //***********************************************************

            if (g_current_row && !col)
            {
                putatriangle(s_last_row[next], s_last_row[col], cur, cur.color);
            }
            if (g_current_row && col)  // skip first row and first column
            {
                if (col == 1)
                {
                    putatriangle(s_last_row[col], s_old_last, old, old.color);
                }

                if (col < lastdot)
                {
                    putatriangle(s_last_row[next], s_last_row[col], cur, cur.color);
                }
                putatriangle(old, s_last_row[col], cur, cur.color);
            }
            break;

        case fill_type::SOLID_FILL:
            if (g_sphere)
            {
                if (s_persp)
                {
                    old.x = (int)(s_x_center >> 16);
                    old.y = (int)(s_y_center >> 16);
                }
                else
                {
                    old.x = (int) s_x_center;
                    old.y = (int) s_y_center;
                }
            }
            else
            {
                lv[0] = col;
                lv[1] = g_current_row;
                lv[2] = 0;

                // apply fudge bit shift for integer math
                lv[0] = lv[0] << 16;
                lv[1] = lv[1] << 16;
                // Since 0, unnecessary lv[2] = lv[2] << 16;

                if (long_vec_mat_mul_persp(lv, s_llm, lv0, lv, s_l_view, 16))
                {
                    cur = s_bad;
                    f_cur = s_f_bad;
                    goto loopbottom;
                }

                // Round and fudge back to original
                old.x = (int)((lv[0] + 32768L) >> 16);
                old.y = (int)((lv[1] + 32768L) >> 16);
            }
            if (old.x < 0)
            {
                old.x = 0;
            }
            if (old.x >= g_logical_screen_x_dots)
            {
                old.x = g_logical_screen_x_dots - 1;
            }
            if (old.y < 0)
            {
                old.y = 0;
            }
            if (old.y >= g_logical_screen_y_dots)
            {
                old.y = g_logical_screen_y_dots - 1;
            }
            driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            break;

        case fill_type::LIGHT_SOURCE_BEFORE:
        case fill_type::LIGHT_SOURCE_AFTER:
            // light-source modulated fill
            if (g_current_row && col)  // skip first row and first column
            {
                if (f_cur.color < s_bad_check || f_old.color < s_bad_check ||
                        s_f_last_row[col].color < s_bad_check)
                {
                    break;
                }

                v1[0] = f_cur.x - f_old.x;
                v1[1] = f_cur.y - f_old.y;
                v1[2] = f_cur.color - f_old.color;

                v2[0] = s_f_last_row[col].x - f_cur.x;
                v2[1] = s_f_last_row[col].y - f_cur.y;
                v2[2] = s_f_last_row[col].color - f_cur.color;

                cross_product(v1, v2, s_cross);

                // normalize cross - and check if non-zero
                if (normalize_vector(s_cross))
                {
                    if (g_debug_flag != debug_flags::none)
                    {
                        stopmsg("debug, cur.color=bad");
                    }
                    f_cur.color = (float) s_bad.color;
                    cur.color = (int) f_cur.color;
                }
                else
                {
                    // line-wise averaging scheme
                    if (g_light_avg > 0)
                    {
                        if (crossnotinit)
                        {
                            // initialize array of old normal vectors
                            crossavg[0] = s_cross[0];
                            crossavg[1] = s_cross[1];
                            crossavg[2] = s_cross[2];
                            crossnotinit = false;
                        }
                        s_tmp_cross[0] = (crossavg[0] * g_light_avg + s_cross[0]) /
                                      (g_light_avg + 1);
                        s_tmp_cross[1] = (crossavg[1] * g_light_avg + s_cross[1]) /
                                      (g_light_avg + 1);
                        s_tmp_cross[2] = (crossavg[2] * g_light_avg + s_cross[2]) /
                                      (g_light_avg + 1);
                        s_cross[0] = s_tmp_cross[0];
                        s_cross[1] = s_tmp_cross[1];
                        s_cross[2] = s_tmp_cross[2];
                        if (normalize_vector(s_cross))
                        {
                            // this shouldn't happen
                            if (g_debug_flag != debug_flags::none)
                            {
                                stopmsg("debug, normal vector err2");
                            }
                            f_cur.color = (float) g_colors;
                            cur.color = (int) f_cur.color;
                        }
                    }
                    crossavg[0] = s_tmp_cross[0];
                    crossavg[1] = s_tmp_cross[1];
                    crossavg[2] = s_tmp_cross[2];

                    // dot product of unit vectors is cos of angle between
                    // we will use this value to shade surface

                    cur.color = (int)(1 + (g_colors - 2) *
                                      (1.0 - dot_product(s_cross, s_light_direction)));
                }
                /* if colors out of range, set them to min or max color index
                 * but avoid background index. This makes colors "opaque" so
                 * SOMETHING plots. These conditions shouldn't happen but just
                 * in case                                        */
                if (cur.color < 1)         // prevent transparent colors
                {
                    cur.color = 1;// avoid background
                }
                if (cur.color > g_colors - 1)
                {
                    cur.color = g_colors - 1;
                }

                // why "col < 2"? So we have sufficient geometry for the fill
                // algorithm, which needs previous point in same row to have
                // already been calculated (variable old)
                // fix ragged left margin in preview
                if (col == 1 && g_current_row > 1)
                {
                    putatriangle(s_last_row[next], s_last_row[col], cur, cur.color);
                }

                if (col < 2 || g_current_row < 2)         // don't have valid colors yet
                {
                    break;
                }

                if (col < lastdot)
                {
                    putatriangle(s_last_row[next], s_last_row[col], cur, cur.color);
                }
                putatriangle(old, s_last_row[col], cur, cur.color);

                g_plot = g_standard_plot;
            }
            break;
        }                      // End of CASE statement for fill type
loopbottom:
        if (g_raytrace_format != raytrace_formats::none || (g_fill_type != fill_type::POINTS && g_fill_type != fill_type::SOLID_FILL))
        {
            // for triangle and grid fill purposes
            s_old_last = s_last_row[col];
            s_last_row[col] = cur;
            old = s_last_row[col];

            // for illumination model purposes
            s_f_last_row[col] = f_cur;
            f_old = s_f_last_row[col];
            if (g_current_row && g_raytrace_format != raytrace_formats::none && col >= lastdot)
                // if we're at the end of a row, close the object
            {
                end_object(tout);
                tout = false;
                if (ferror(s_file_ptr1))
                {
                    std::fclose(s_file_ptr1);
                    std::remove(g_light_name.c_str());
                    File_Error(g_raytrace_filename.c_str(), 2);
                    return -1;
                }
            }
        }
        col++;
    }                         // End of while statement for plotting line
    s_ro++;
reallythebottom:

    // stuff that HAS to be done, even in preview mode, goes here
    if (g_sphere)
    {
        // incremental sin/cos phi calc
        if (g_current_row == 0)
        {
            s_sin_phi = s_old_sin_phi2;
            s_cos_phi = s_old_cos_phi2;
        }
        else
        {
            s_sin_phi = s_two_cos_delta_phi * s_old_sin_phi2 - s_old_sin_phi1;
            s_cos_phi = s_two_cos_delta_phi * s_old_cos_phi2 - s_old_cos_phi1;
            s_old_sin_phi1 = s_old_sin_phi2;
            s_old_sin_phi2 = s_sin_phi;
            s_old_cos_phi1 = s_old_cos_phi2;
            s_old_cos_phi2 = s_cos_phi;
        }
    }
    return 0;                  // decoder needs to know all is well !!!
}

// vector version of line draw
static void vdraw_line(double *v1, double *v2, int color)
{
    int x1;
    int y1;
    int x2;
    int y2;
    x1 = (int) v1[0];
    y1 = (int) v1[1];
    x2 = (int) v2[0];
    y2 = (int) v2[1];
    driver_draw_line(x1, y1, x2, y2, color);
}

static void corners(MATRIX m, bool show, double *pxmin, double *pymin, double *pzmin, double *pxmax, double *pymax, double *pzmax)
{
    VECTOR S[2][4];              // Holds the top an bottom points, S[0][]=bottom

    /* define corners of box fractal is in in x,y,z plane "b" stands for
     * "bottom" - these points are the corners of the screen in the x-y plane.
     * The "t"'s stand for Top - they are the top of the cube where 255 color
     * points hit. */
    *pzmin = (int) INT_MAX;
    *pymin = *pzmin;
    *pxmin = *pymin;
    *pzmax = (int) INT_MIN;
    *pymax = *pzmax;
    *pxmax = *pymax;

    for (int j = 0; j < 4; ++j)
    {
        for (int i = 0; i < 3; i++)
        {
            S[1][j][i] = 0;
            S[0][j][i] = S[1][j][i];
        }
    }

    S[1][2][0] = g_logical_screen_x_dots - 1;
    S[1][1][0] = S[1][2][0];
    S[0][2][0] = S[1][1][0];
    S[0][1][0] = S[0][2][0];
    S[1][3][1] = g_logical_screen_y_dots - 1;
    S[1][2][1] = S[1][3][1];
    S[0][3][1] = S[1][2][1];
    S[0][2][1] = S[0][3][1];
    S[1][3][2] = s_z_coord - 1;
    S[1][2][2] = S[1][3][2];
    S[1][1][2] = S[1][2][2];
    S[1][0][2] = S[1][1][2];

    for (int i = 0; i < 4; ++i)
    {
        // transform points
        vec_mat_mul(S[0][i], m, S[0][i]);
        vec_mat_mul(S[1][i], m, S[1][i]);

        // update minimums and maximums
        if (S[0][i][0] <= *pxmin)
        {
            *pxmin = S[0][i][0];
        }
        if (S[0][i][0] >= *pxmax)
        {
            *pxmax = S[0][i][0];
        }
        if (S[1][i][0] <= *pxmin)
        {
            *pxmin = S[1][i][0];
        }
        if (S[1][i][0] >= *pxmax)
        {
            *pxmax = S[1][i][0];
        }
        if (S[0][i][1] <= *pymin)
        {
            *pymin = S[0][i][1];
        }
        if (S[0][i][1] >= *pymax)
        {
            *pymax = S[0][i][1];
        }
        if (S[1][i][1] <= *pymin)
        {
            *pymin = S[1][i][1];
        }
        if (S[1][i][1] >= *pymax)
        {
            *pymax = S[1][i][1];
        }
        if (S[0][i][2] <= *pzmin)
        {
            *pzmin = S[0][i][2];
        }
        if (S[0][i][2] >= *pzmax)
        {
            *pzmax = S[0][i][2];
        }
        if (S[1][i][2] <= *pzmin)
        {
            *pzmin = S[1][i][2];
        }
        if (S[1][i][2] >= *pzmax)
        {
            *pzmax = S[1][i][2];
        }
    }

    if (show)
    {
        if (s_persp)
        {
            for (int i = 0; i < 4; i++)
            {
                perspective(S[0][i]);
                perspective(S[1][i]);
            }
        }

        // Keep the box surrounding the fractal
        for (auto &elem : S)
        {
            for (auto &elem_i : elem)
            {
                elem_i[0] += g_xx_adjust;
                elem_i[1] += g_yy_adjust;
            }
        }

        draw_rect(S[0][0], S[0][1], S[0][2], S[0][3], 2, true);      // Bottom

        draw_rect(S[0][0], S[1][0], S[0][1], S[1][1], 5, false);      // Sides
        draw_rect(S[0][2], S[1][2], S[0][3], S[1][3], 6, false);

        draw_rect(S[1][0], S[1][1], S[1][2], S[1][3], 8, true);      // Top
    }
}

/* This function draws a vector from origin[] to direct[] and a box
        around it. The vector and box are transformed or not depending on
        FILLTYPE.
*/

static void draw_light_box(double *origin, double *direct, MATRIX light_m)
{
    VECTOR S[2][4] = { 0 };
    double temp;

    S[0][0][0] = origin[0];
    S[1][0][0] = S[0][0][0];
    S[0][0][1] = origin[1];
    S[1][0][1] = S[0][0][1];

    S[1][0][2] = direct[2];

    for (auto &elem : S)
    {
        elem[1][0] = elem[0][0];
        elem[1][1] = direct[1];
        elem[1][2] = elem[0][2];
        elem[2][0] = direct[0];
        elem[2][1] = elem[1][1];
        elem[2][2] = elem[0][2];
        elem[3][0] = elem[2][0];
        elem[3][1] = elem[0][1];
        elem[3][2] = elem[0][2];
    }

    // transform the corners if necessary
    if (g_fill_type == fill_type::LIGHT_SOURCE_AFTER)
    {
        for (int i = 0; i < 4; i++)
        {
            vec_mat_mul(S[0][i], light_m, S[0][i]);
            vec_mat_mul(S[1][i], light_m, S[1][i]);
        }
    }

    // always use perspective to aid viewing
    temp = g_view[2];              // save perspective distance for a later restore
    g_view[2] = -s_p * 300.0 / 100.0;

    for (int i = 0; i < 4; i++)
    {
        perspective(S[0][i]);
        perspective(S[1][i]);
    }
    g_view[2] = temp;              // Restore perspective distance

    // Adjust for aspect
    for (int i = 0; i < 4; i++)
    {
        S[0][i][0] = S[0][i][0] * s_aspect;
        S[1][i][0] = S[1][i][0] * s_aspect;
    }

    // draw box connecting transformed points. NOTE order and COLORS
    draw_rect(S[0][0], S[0][1], S[0][2], S[0][3], 2, true);

    vdraw_line(S[0][0], S[1][2], 8);

    // sides
    draw_rect(S[0][0], S[1][0], S[0][1], S[1][1], 4, false);
    draw_rect(S[0][2], S[1][2], S[0][3], S[1][3], 5, false);

    draw_rect(S[1][0], S[1][1], S[1][2], S[1][3], 3, true);

    // Draw the "arrow head"
    for (int i = -3; i < 4; i++)
    {
        for (int j = -3; j < 4; j++)
        {
            if (std::abs(i) + std::abs(j) < 6)
            {
                g_plot((int)(S[1][2][0] + i), (int)(S[1][2][1] + j), 10);
            }
        }
    }
}

static void draw_rect(VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color, bool rect)
{
    VECTOR V[4];

    // Since V[2] is not used by vdraw_line don't bother setting it
    for (int i = 0; i < 2; i++)
    {
        V[0][i] = V0[i];
        V[1][i] = V1[i];
        V[2][i] = V2[i];
        V[3][i] = V3[i];
    }
    if (rect)                    // Draw a rectangle
    {
        for (int i = 0; i < 4; i++)
        {
            if (std::fabs(V[i][0] - V[(i + 1) % 4][0]) < -2 * s_bad_check
                && std::fabs(V[i][1] - V[(i + 1) % 4][1]) < -2 * s_bad_check)
            {
                vdraw_line(V[i], V[(i + 1) % 4], color);
            }
        }
    }
    else
        // Draw 2 lines instead
    {
        for (int i = 0; i < 3; i += 2)
        {
            if (std::fabs(V[i][0] - V[i + 1][0]) < -2 * s_bad_check
                && std::fabs(V[i][1] - V[i + 1][1]) < -2 * s_bad_check)
            {
                vdraw_line(V[i], V[i + 1], color);
            }
        }
    }
}

// replacement for plot - builds a table of min and max x's instead of plot
// called by draw_line as part of triangle fill routine
static void putminmax(int x, int y, int /*color*/)
{
    if (y >= 0 && y < g_logical_screen_y_dots)
    {
        if (x < s_min_max_x[y].minx)
        {
            s_min_max_x[y].minx = x;
        }
        if (x > s_min_max_x[y].maxx)
        {
            s_min_max_x[y].maxx = x;
        }
    }
}

/*
        This routine fills in a triangle. Extreme left and right values for
        each row are calculated by calling the line function for the sides.
        Then rows are filled in with horizontal lines
*/
#define MAXOFFSCREEN  2    // allow two of three points to be off screen

static void putatriangle(point pt1, point pt2, point pt3, int color)
{
    int miny;
    int maxy;
    int xlim;

    // Too many points off the screen?
    if ((offscreen(pt1) + offscreen(pt2) + offscreen(pt3)) > MAXOFFSCREEN)
    {
        return;
    }

    s_p1 = pt1;                    // needed by interpcolor
    s_p2 = pt2;
    s_p3 = pt3;

    // fast way if single point or single line
    if (s_p1.y == s_p2.y && s_p1.x == s_p2.x)
    {
        g_plot = s_fill_plot;
        if (s_p1.y == s_p3.y && s_p1.x == s_p3.x)
        {
            (*g_plot)(s_p1.x, s_p1.y, color);
        }
        else
        {
            driver_draw_line(s_p1.x, s_p1.y, s_p3.x, s_p3.y, color);
        }
        g_plot = s_normal_plot;
        return;
    }
    else if ((s_p3.y == s_p1.y && s_p3.x == s_p1.x) || (s_p3.y == s_p2.y && s_p3.x == s_p2.x))
    {
        g_plot = s_fill_plot;
        driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, color);
        g_plot = s_normal_plot;
        return;
    }

    // find min max y
    maxy = s_p1.y;
    miny = maxy;
    if (s_p2.y < miny)
    {
        miny = s_p2.y;
    }
    else
    {
        maxy = s_p2.y;
    }
    if (s_p3.y < miny)
    {
        miny = s_p3.y;
    }
    else if (s_p3.y > maxy)
    {
        maxy = s_p3.y;
    }

    // only worried about values on screen
    if (miny < 0)
    {
        miny = 0;
    }
    if (maxy >= g_logical_screen_y_dots)
    {
        maxy = g_logical_screen_y_dots - 1;
    }

    for (int y = miny; y <= maxy; y++)
    {
        s_min_max_x[y].minx = (int) INT_MAX;
        s_min_max_x[y].maxx = (int) INT_MIN;
    }

    // set plot to "fake" plot function
    g_plot = putminmax;

    // build table of extreme x's of triangle
    driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, 0);
    driver_draw_line(s_p2.x, s_p2.y, s_p3.x, s_p3.y, 0);
    driver_draw_line(s_p3.x, s_p3.y, s_p1.x, s_p1.y, 0);

    for (int y = miny; y <= maxy; y++)
    {
        xlim = s_min_max_x[y].maxx;
        for (int x = s_min_max_x[y].minx; x <= xlim; x++)
        {
            (*s_fill_plot)(x, y, color);
        }
    }
    g_plot = s_normal_plot;
}

static int offscreen(point pt)
{
    if (pt.x >= 0)
    {
        if (pt.x < g_logical_screen_x_dots)
        {
            if (pt.y >= 0)
            {
                if (pt.y < g_logical_screen_y_dots)
                {
                    return 0;      // point is ok
                }
            }
        }
    }
    if (std::abs(pt.x) > 0 - s_bad_check || std::abs(pt.y) > 0 - s_bad_check)
    {
        return 99;              // point is bad
    }
    return 1;                  // point is off the screen
}

static void clipcolor(int x, int y, int color)
{
    if (0 <= x && x < g_logical_screen_x_dots
        && 0 <= y && y < g_logical_screen_y_dots
        && 0 <= color && color < g_file_colors)
    {
        g_standard_plot(x, y, color);

        if (g_targa_out)
        {
            // standardplot modifies color in these types
            if (!(g_glasses_type == 1 || g_glasses_type == 2))
            {
                targa_color(x, y, color);
            }
        }
    }
}

//*******************************************************************
// This function is the same as clipcolor but checks for color being
// in transparent range. Intended to be called only if transparency
// has been enabled.
//*******************************************************************

static void T_clipcolor(int x, int y, int color)
{
    if (0 <= x && x < g_logical_screen_x_dots       // is the point on screen?
        && 0 <= y && y < g_logical_screen_y_dots    // Yes?
        && 0 <= color && color < g_colors           // Colors in valid range?
        // Lets make sure its not a transparent color
        && (g_transparent_color_3d[0] > color || color > g_transparent_color_3d[1]))
    {
        g_standard_plot(x, y, color);// I guess we can plot then
        if (g_targa_out)
        {
            // standardplot modifies color in these types
            if (!(g_glasses_type == 1 || g_glasses_type == 2))
            {
                targa_color(x, y, color);
            }
        }
    }
}

//**********************************************************************
// A substitute for plotcolor that interpolates the colors according
// to the x and y values of three points (p1,p2,p3) which are static in
// this routine
//
//      In Light source modes, color is light value, not actual color
//      Real_Color always contains the actual color
//**********************************************************************

static void interpcolor(int x, int y, int color)
{
    int D;
    int d1;
    int d2;
    int d3;

    /* this distance formula is not the usual one - but it has the virtue that
     * it uses ONLY additions (almost) and it DOES go to zero as the points
     * get close. */

    d1 = std::abs(s_p1.x - x) + std::abs(s_p1.y - y);
    d2 = std::abs(s_p2.x - x) + std::abs(s_p2.y - y);
    d3 = std::abs(s_p3.x - x) + std::abs(s_p3.y - y);

    D = (d1 + d2 + d3) << 1;
    if (D)
    {
        /* calculate a weighted average of colors long casts prevent integer
           overflow. This can evaluate to zero */
        color = (int)(((long)(d2 + d3) * (long) s_p1.color +
                       (long)(d1 + d3) * (long) s_p2.color +
                       (long)(d1 + d2) * (long) s_p3.color) / D);
    }

    if (0 <= x && x < g_logical_screen_x_dots
        && 0 <= y && y < g_logical_screen_y_dots
        && 0 <= color && color < g_colors
        && (g_transparent_color_3d[1] == 0
            || (int) s_real_color > g_transparent_color_3d[1]
            || g_transparent_color_3d[0] > (int) s_real_color))
    {
        if (g_targa_out)
        {
            // standardplot modifies color in these types
            if (!(g_glasses_type == 1 || g_glasses_type == 2))
            {
                D = targa_color(x, y, color);
            }
        }

        if (g_fill_type >= fill_type::LIGHT_SOURCE_BEFORE)
        {
            if (s_real_v && g_targa_out)
            {
                color = D;
            }
            else
            {
                color = (1 + (unsigned) color * s_i_ambient) / 256;
                if (color == 0)
                {
                    color = 1;
                }
            }
        }
        g_standard_plot(x, y, color);
    }
}

/*
        In non light source modes, both color and Real_Color contain the
        actual pixel color. In light source modes, color contains the
        light value, and Real_Color contains the origninal color

        This routine takes a pixel modifies it for lightshading if appropriate
        and plots it in a Targa file. Used in plot3d.c
*/

int targa_color(int x, int y, int color)
{
    unsigned long H;
    unsigned long S;
    unsigned long V;
    BYTE RGB[3];

    if (g_fill_type == fill_type::SURFACE_INTERPOLATED || g_glasses_type == 1 || g_glasses_type == 2 || g_truecolor)
    {
        s_real_color = (BYTE)color;       // So Targa gets interpolated color
    }

    switch (g_true_mode)
    {
    case true_color_mode::default_color:
    default:
        RGB[0] = (BYTE)(g_dac_box[s_real_color][0] << 2); // Move color space to
        RGB[1] = (BYTE)(g_dac_box[s_real_color][1] << 2); // 256 color primaries
        RGB[2] = (BYTE)(g_dac_box[s_real_color][2] << 2); // from 64 colors
        break;

    case true_color_mode::iterate:
        RGB[0] = (BYTE)((g_real_color_iter >> 16) & 0xff);  // red
        RGB[1] = (BYTE)((g_real_color_iter >> 8) & 0xff);   // green
        RGB[2] = (BYTE)((g_real_color_iter) & 0xff);        // blue
        break;
    }

    // Now lets convert it to HSV
    rgb_to_hsv(RGB[0], RGB[1], RGB[2], &H, &S, &V);

    // Modify original S and V components
    if (g_fill_type > fill_type::SOLID_FILL && !(g_glasses_type == 1 || g_glasses_type == 2))
    {
        // Adjust for Ambient
        V = (V * (65535L - (unsigned)(color * s_i_ambient))) / 65535L;
    }

    if (g_haze)
    {
        // Haze lowers sat of colors
        S = (unsigned long)(S * s_haze_mult) / 100;
        if (V >= 32640)           // Haze reduces contrast
        {
            V = V - 32640;
            V = (unsigned long)((V * s_haze_mult) / 100);
            V = V + 32640;
        }
        else
        {
            V = 32640 - V;
            V = (unsigned long)((V * s_haze_mult) / 100);
            V = 32640 - V;
        }
    }
    // Now lets convert it back to RGB. Original Hue, modified Sat and Val
    hsv_to_rgb(&RGB[0], &RGB[1], &RGB[2], H, S, V);

    if (s_real_v)
    {
        V = (35 * (int) RGB[0] + 45 * (int) RGB[1] + 20 * (int) RGB[2]) / 100;
    }

    // Now write the color triple to its transformed location
    // on the disk.
    targa_writedisk(x + g_logical_screen_x_offset, y + g_logical_screen_y_offset, RGB[0], RGB[1], RGB[2]);

    return (int)(255 - V);
}

static bool set_pixel_buff(BYTE *pixels, BYTE *fraction, unsigned linelen)
{
    if ((s_even_odd_row++ & 1) == 0) // even rows are color value
    {
        for (int i = 0; i < (int) linelen; i++)         // add the fractional part in odd row
        {
            fraction[i] = pixels[i];
        }
        return true;
    }
    else // swap
    {
        BYTE tmp;
        for (int i = 0; i < (int) linelen; i++)       // swap so pixel has color
        {
            tmp = pixels[i];
            pixels[i] = fraction[i];
            fraction[i] = tmp;
        }
    }
    return false;
}

/**************************************************************************

  Common routine for printing error messages to the screen for Targa
    and other files

**************************************************************************/

static void File_Error(char const *File_Name1, int ERROR)
{
    char msgbuf[200];

    s_error = ERROR;
    switch (ERROR)
    {
    case 1:                      // Can't Open
        std::snprintf(msgbuf, std::size(msgbuf), "OOPS, couldn't open  < %s >", File_Name1);
        break;
    case 2:                      // Not enough room
        std::snprintf(msgbuf, std::size(msgbuf), "OOPS, ran out of disk space. < %s >", File_Name1);
        break;
    case 3:                      // Image wrong size
        std::snprintf(msgbuf, std::size(msgbuf), "OOPS, image wrong size\n");
        break;
    case 4:                      // Wrong file type
        std::snprintf(msgbuf, std::size(msgbuf), "OOPS, can't handle this type of file.\n");
        break;
    }
    stopmsg(msgbuf);
}

//**********************************************************************
//
//   This function opens a TARGA_24 file for reading and writing. If
//   it's a new file, (overlay == false) it writes a header. If it is to
//   overlay an existing file (overlay == true) it copies the original
//   header whose length and validity was determined in
//   Targa_validate.
//
//   It Verifies there is enough disk space, and leaves the file
//   at the start of the display data area.
//
//   If this is an overlay, closes source and copies to "targa_temp"
//   If there is an error close the file.
//
// *********************************************************************

bool startdisk1(const std::string &filename, std::FILE *source, bool overlay)
{
    // Open File for both reading and writing
    std::FILE *fps = dir_fopen(g_working_dir.c_str(), filename.c_str(), "w+b");
    if (fps == nullptr)
    {
        File_Error(filename.c_str(), 1);
        return true;            // Oops, somethings wrong!
    }

    int inc = 1;                // Assume we are overlaying a file

    // Write the header
    if (overlay)                   // We are overlaying a file
    {
        for (int i = 0; i < s_targa_header_24; i++)   // Copy the header from the Source
        {
            std::fputc(fgetc(source), fps);
        }
    }
    else
    {
        // Write header for a new file
        // ID field size = 0, No color map, Targa type 2 file
        for (int i = 0; i < 12; i++)
        {
            if (i == 0 && g_truecolor)
            {
                set_upr_lwr();
                std::fputc(4, fps); // make room to write an extra number
                s_targa_header_24 = 18 + 4;
            }
            else if (i == 2)
            {
                std::fputc(i, fps);
            }
            else
            {
                std::fputc(0, fps);
            }
        }
        // Write image size
        for (BYTE &elem : s_upr_lwr)
        {
            std::fputc(elem, fps);
        }
        std::fputc(s_t24, fps);          // Targa 24 file
        std::fputc(s_t32, fps);          // Image at upper left
        inc = 3;
    }

    if (g_truecolor) // write maxit
    {
        std::fputc((BYTE)(g_max_iterations       & 0xff), fps);
        std::fputc((BYTE)((g_max_iterations >> 8) & 0xff), fps);
        std::fputc((BYTE)((g_max_iterations >> 16) & 0xff), fps);
        std::fputc((BYTE)((g_max_iterations >> 24) & 0xff), fps);
    }

    // Finished with the header, now lets work on the display area
    for (int i = 0; i < g_logical_screen_y_dots; i++)  // "clear the screen" (write to the disk)
    {
        for (int j = 0; j < s_line_length1; j = j + inc)
        {
            if (overlay)
            {
                std::fputc(fgetc(source), fps);
            }
            else
            {
                for (int k = 2; k > -1; k--)
                {
                    std::fputc(g_background_color[k], fps);       // Targa order (B, G, R)
                }
            }
        }
        if (ferror(fps))
        {
            // Almost certainly not enough disk space
            std::fclose(fps);
            if (overlay)
            {
                std::fclose(source);
            }
            dir_remove(g_working_dir, filename);
            File_Error(filename.c_str(), 2);
            return true;
        }
        if (driver_key_pressed())
        {
            return true;
        }
    }

    if (targa_startdisk(fps, s_targa_header_24) != 0)
    {
        enddisk();
        dir_remove(g_working_dir, filename);
        return true;
    }
    return false;
}

bool targa_validate(char const *File_Name)
{
    // Attempt to open source file for reading
    std::FILE *fp = dir_fopen(g_working_dir.c_str(), File_Name, "rb");
    if (fp == nullptr)
    {
        File_Error(File_Name, 1);
        return true;              // Oops, file does not exist
    }

    s_targa_header_24 += fgetc(fp);    // Check ID field and adjust header size

    if (fgetc(fp))               // Make sure this is an unmapped file
    {
        File_Error(File_Name, 4);
        return true;
    }

    if (fgetc(fp) != 2)          // Make sure it is a type 2 file
    {
        File_Error(File_Name, 4);
        return true;
    }

    // Skip color map specification
    for (int i = 0; i < 5; i++)
    {
        fgetc(fp);
    }

    for (int i = 0; i < 4; i++)
    {
        // Check image origin
        fgetc(fp);
    }
    // Check Image specs
    for (auto &elem : s_upr_lwr)
    {
        if (fgetc(fp) != (int) elem)
        {
            File_Error(File_Name, 3);
            return true;
        }
    }

    if (fgetc(fp) != (int) s_t24)
    {
        s_error = 4;                // Is it a targa 24 file?
    }
    if (fgetc(fp) != (int) s_t32)
    {
        s_error = 4;                // Is the origin at the upper left?
    }
    if (s_error == 4)
    {
        File_Error(File_Name, 4);
        return true;
    }
    std::rewind(fp);

    // Now that we know its a good file, create a working copy
    if (startdisk1(s_targa_temp, fp, true))
    {
        return true;
    }

    std::fclose(fp);                  // Close the source

    s_t_safe = true;              // Original file successfully copied to targa_temp
    return false;
}

static int rgb_to_hsv(
    BYTE red, BYTE green, BYTE blue, unsigned long *hue, unsigned long *sat, unsigned long *val)
{
    unsigned long R1;
    unsigned long G1;
    unsigned long B1;
    unsigned long DENOM;
    BYTE MIN;

    *val = red;
    MIN = green;
    if (red < green)
    {
        *val = green;
        MIN = red;
        if (green < blue)
        {
            *val = blue;
        }
        if (blue < red)
        {
            MIN = blue;
        }
    }
    else
    {
        if (blue < green)
        {
            MIN = blue;
        }
        if (red < blue)
        {
            *val = blue;
        }
    }
    DENOM = *val - MIN;
    if (*val != 0 && DENOM != 0)
    {
        *sat = ((DENOM << 16) / *val) - 1;
    }
    else
    {
        *sat = 0;      // Color is black! and Sat has no meaning
    }
    if (*sat == 0)    // R=G=B => shade of grey and Hue has no meaning
    {
        *hue = 0;
        *val = *val << 8;
        return 1;               // v or s or both are 0
    }
    if (*val == MIN)
    {
        *hue = 0;
        *val = *val << 8;
        return 0;
    }
    R1 = (((*val - red) * 60) << 6) / DENOM; // distance of color from red
    G1 = (((*val - green) * 60) << 6) / DENOM; // distance of color from green
    B1 = (((*val - blue) * 60) << 6) / DENOM; // distance of color from blue
    if (*val == red)
    {
        if (MIN == green)
        {
            *hue = (300 << 6) + B1;
        }
        else
        {
            *hue = (60 << 6) - G1;
        }
    }
    if (*val == green)
    {
        if (MIN == blue)
        {
            *hue = (60 << 6) + R1;
        }
        else
        {
            *hue = (180 << 6) - B1;
        }
    }
    if (*val == blue)
    {
        if (MIN == red)
        {
            *hue = (180 << 6) + G1;
        }
        else
        {
            *hue = (300 << 6) - R1;
        }
    }
    *val = *val << 8;
    return 0;
}

static void hsv_to_rgb(
    BYTE *red, BYTE *green, BYTE *blue, unsigned long hue, unsigned long sat, unsigned long val)
{
    unsigned long P1;
    unsigned long P2;
    unsigned long P3;
    int RMD;
    int I;

    if (hue >= 23040)
    {
        hue = hue % 23040;            // Makes h circular
    }
    I = (int)(hue / 3840);
    RMD = (int)(hue % 3840);       // RMD = fractional part of H

    P1 = ((val * (65535L - sat)) / 65280L) >> 8;
    P2 = (((val * (65535L - (sat * RMD) / 3840)) / 65280L) - 1) >> 8;
    P3 = (((val * (65535L - (sat * (3840 - RMD)) / 3840)) / 65280L)) >> 8;
    val = val >> 8;
    switch (I)
    {
    case 0:
        *red = (BYTE) val;
        *green = (BYTE) P3;
        *blue = (BYTE) P1;
        break;
    case 1:
        *red = (BYTE) P2;
        *green = (BYTE) val;
        *blue = (BYTE) P1;
        break;
    case 2:
        *red = (BYTE) P1;
        *green = (BYTE) val;
        *blue = (BYTE) P3;
        break;
    case 3:
        *red = (BYTE) P1;
        *green = (BYTE) P2;
        *blue = (BYTE) val;
        break;
    case 4:
        *red = (BYTE) P3;
        *green = (BYTE) P1;
        *blue = (BYTE) val;
        break;
    case 5:
        *red = (BYTE) val;
        *green = (BYTE) P1;
        *blue = (BYTE) P2;
        break;
    }
}

//*************************************************************************
//
// general raytracing code info/notes:
//
//  ray == 0 means no raytracer output  ray == 7 is for dxf
//  ray == 1 is for dkb/pov             ray == 4 is for mtv
//  ray == 2 is for vivid               ray == 5 is for rayshade
//  ray == 3 is for raw                 ray == 6 is for acrospin
//
//  rayshade needs counterclockwise triangles.  raytracers that support
//  the 'heightfield' primitive include rayshade and pov.  anyone want to
//  write code to make heightfields?  they are *MUCH* faster to trace than
//  triangles when doing landscapes...
//
//*************************************************************************

//******************************************************************
//
//  This routine writes a header to a ray tracer data file. It
//  Identifies the version of Iterated Dynamics which created it and the
//  key 3D parameters in effect at the time.
//
//******************************************************************

static int RAY_Header()
{
    // Open the ray tracing output file
    check_writefile(g_raytrace_filename, ".ray");
    s_file_ptr1 = open_save_file(g_raytrace_filename, "w");
    if (s_file_ptr1 == nullptr)
    {
        return -1;              // Oops, somethings wrong!
    }

    if (g_raytrace_format == raytrace_formats::vivid)
    {
        std::fprintf(s_file_ptr1, "//");
    }
    if (g_raytrace_format == raytrace_formats::mtv)
    {
        std::fprintf(s_file_ptr1, "#");
    }
    if (g_raytrace_format == raytrace_formats::rayshade)
    {
        std::fprintf(s_file_ptr1, "/*\n");
    }
    if (g_raytrace_format == raytrace_formats::acrospin)
    {
        std::fprintf(s_file_ptr1, "--");
    }
    if (g_raytrace_format == raytrace_formats::dxf)
        std::fprintf(s_file_ptr1, "  0\nSECTION\n  2\nTABLES\n  0\nTABLE\n  2\nLAYER\n\
 70\n     2\n  0\nLAYER\n  2\n0\n 70\n     0\n 62\n     7\n  6\nCONTINUOUS\n\
  0\nLAYER\n  2\nFRACTAL\n 70\n    64\n 62\n     1\n  6\nCONTINUOUS\n  0\n\
ENDTAB\n  0\nENDSEC\n  0\nSECTION\n  2\nENTITIES\n");

    if (g_raytrace_format != raytrace_formats::dxf)
    {
        std::fprintf(s_file_ptr1, "{ Created by " ID_PROGRAM_NAME " Ver. %#4.2f }\n\n", g_release / 100.);
    }

    if (g_raytrace_format == raytrace_formats::rayshade)
    {
        std::fprintf(s_file_ptr1, "*/\n");
    }

    // Set the default color
    if (g_raytrace_format == raytrace_formats::povray)
    {
        std::fprintf(s_file_ptr1, "DECLARE       F_Dflt = COLOR  RED 0.8 GREEN 0.4 BLUE 0.1\n");
    }
    if (g_brief)
    {
        if (g_raytrace_format == raytrace_formats::vivid)
        {
            std::fprintf(s_file_ptr1, "surf={diff=0.8 0.4 0.1;}\n");
        }
        if (g_raytrace_format == raytrace_formats::mtv)
        {
            std::fprintf(s_file_ptr1, "f 0.8 0.4 0.1 0.95 0.05 5 0 0\n");
        }
        if (g_raytrace_format == raytrace_formats::rayshade)
        {
            std::fprintf(s_file_ptr1, "applysurf diffuse 0.8 0.4 0.1");
        }
    }
    if (g_raytrace_format != raytrace_formats::dxf)
    {
        std::fprintf(s_file_ptr1, "\n");
    }

    // open "grid" opject, a speedy way to do aggregates in rayshade
    if (g_raytrace_format == raytrace_formats::rayshade)
    {
        std::fprintf(s_file_ptr1,
                "/* make a gridded aggregate. this size grid is fast for landscapes. */\n"
                "/* make z grid = 1 always for landscapes. */\n\n"
                "grid 33 25 1\n");
    }

    if (g_raytrace_format == raytrace_formats::acrospin)
    {
        std::fprintf(s_file_ptr1, "Set Layer 1\nSet Color 2\nEndpointList X Y Z Name\n");
    }

    return 0;
}

//******************************************************************
//
//  This routine describes the triangle to the ray tracer, it
//  sets the color of the triangle to the average of the color
//  of its verticies and sets the light parameters to arbitrary
//  values.
//
//  Note: numcolors (number of colors in the source
//  file) is used instead of colors (number of colors avail. with
//  display) so you can generate ray trace files with your LCD
//  or monochrome display
//
//******************************************************************

static int out_triangle(f_point pt1, f_point pt2, f_point pt3, int c1, int c2, int c3)
{
    float c[3];
    float pt_t[3][3];

    // Normalize each vertex to screen size and adjust coordinate system
    pt_t[0][0] = 2 * pt1.x / g_logical_screen_x_dots - 1;
    pt_t[0][1] = (2 * pt1.y / g_logical_screen_y_dots - 1);
    pt_t[0][2] = -2 * pt1.color / g_num_colors - 1;
    pt_t[1][0] = 2 * pt2.x / g_logical_screen_x_dots - 1;
    pt_t[1][1] = (2 * pt2.y / g_logical_screen_y_dots - 1);
    pt_t[1][2] = -2 * pt2.color / g_num_colors - 1;
    pt_t[2][0] = 2 * pt3.x / g_logical_screen_x_dots - 1;
    pt_t[2][1] = (2 * pt3.y / g_logical_screen_y_dots - 1);
    pt_t[2][2] = -2 * pt3.color / g_num_colors - 1;

    // Color of triangle is average of colors of its verticies
    if (!g_brief)
    {
        for (int i = 0; i <= 2; i++)
        {
            c[i] = (float)(g_dac_box[c1][i] + g_dac_box[c2][i] + g_dac_box[c3][i])
                   / (3 * 63);
        }
    }

    // get rid of degenerate triangles: any two points equal
    if ((pt_t[0][0] == pt_t[1][0] && pt_t[0][1] == pt_t[1][1] && pt_t[0][2] == pt_t[1][2])
        || (pt_t[0][0] == pt_t[2][0] && pt_t[0][1] == pt_t[2][1] && pt_t[0][2] == pt_t[2][2])
        || (pt_t[2][0] == pt_t[1][0] && pt_t[2][1] == pt_t[1][1] && pt_t[2][2] == pt_t[1][2]))
    {
        return 0;
    }

    // Describe the triangle
    if (g_raytrace_format == raytrace_formats::povray)
    {
        std::fprintf(s_file_ptr1, " OBJECT\n  TRIANGLE ");
    }
    if (g_raytrace_format == raytrace_formats::vivid && !g_brief)
    {
        std::fprintf(s_file_ptr1, "surf={diff=");
    }
    if (g_raytrace_format == raytrace_formats::mtv && !g_brief)
    {
        std::fprintf(s_file_ptr1, "f");
    }
    if (g_raytrace_format == raytrace_formats::rayshade && !g_brief)
    {
        std::fprintf(s_file_ptr1, "applysurf diffuse ");
    }

    if (!g_brief && g_raytrace_format != raytrace_formats::povray && g_raytrace_format != raytrace_formats::dxf)
    {
        for (int i = 0; i <= 2; i++)
        {
            std::fprintf(s_file_ptr1, "% #4.4f ", c[i]);
        }
    }

    if (g_raytrace_format == raytrace_formats::vivid)
    {
        if (!g_brief)
        {
            std::fprintf(s_file_ptr1, ";}\n");
        }
        std::fprintf(s_file_ptr1, "polygon={points=3;");
    }
    if (g_raytrace_format == raytrace_formats::mtv)
    {
        if (!g_brief)
        {
            std::fprintf(s_file_ptr1, "0.95 0.05 5 0 0\n");
        }
        std::fprintf(s_file_ptr1, "p 3");
    }
    if (g_raytrace_format == raytrace_formats::rayshade)
    {
        if (!g_brief)
        {
            std::fprintf(s_file_ptr1, "\n");
        }
        std::fprintf(s_file_ptr1, "triangle");
    }

    if (g_raytrace_format == raytrace_formats::dxf)
    {
        std::fprintf(s_file_ptr1, "  0\n3DFACE\n  8\nFRACTAL\n 62\n%3d\n", std::min(255, std::max(1, c1)));
    }

    for (int i = 0; i <= 2; i++)     // Describe each  Vertex
    {
        if (g_raytrace_format != raytrace_formats::dxf)
        {
            std::fprintf(s_file_ptr1, "\n");
        }

        if (g_raytrace_format == raytrace_formats::povray)
        {
            std::fprintf(s_file_ptr1, "      <");
        }
        if (g_raytrace_format == raytrace_formats::vivid)
        {
            std::fprintf(s_file_ptr1, " vertex =  ");
        }
        if (g_raytrace_format > raytrace_formats::raw && g_raytrace_format != raytrace_formats::dxf)
        {
            std::fprintf(s_file_ptr1, " ");
        }

        for (int j = 0; j <= 2; j++)
        {
            if (g_raytrace_format == raytrace_formats::dxf)
            {
                // write 3dface entity to dxf file
                std::fprintf(s_file_ptr1, "%3d\n%g\n", 10 * (j + 1) + i, pt_t[i][j]);
                if (i == 2)           // 3dface needs 4 vertecies
                {
                    std::fprintf(s_file_ptr1, "%3d\n%g\n", 10 * (j + 1) + i + 1,
                            pt_t[i][j]);
                }
            }
            else if (!(g_raytrace_format == raytrace_formats::mtv || g_raytrace_format == raytrace_formats::rayshade))
            {
                std::fprintf(s_file_ptr1, "% #4.4f ", pt_t[i][j]); // Right handed
            }
            else
            {
                std::fprintf(s_file_ptr1, "% #4.4f ", pt_t[2 - i][j]);     // Left handed
            }
        }

        if (g_raytrace_format == raytrace_formats::povray)
        {
            std::fprintf(s_file_ptr1, ">");
        }
        if (g_raytrace_format == raytrace_formats::vivid)
        {
            std::fprintf(s_file_ptr1, ";");
        }
    }

    if (g_raytrace_format == raytrace_formats::povray)
    {
        std::fprintf(s_file_ptr1, " END_TRIANGLE \n");
        if (!g_brief)
        {
            std::fprintf(s_file_ptr1,
                    "  TEXTURE\n"
                    "   COLOR  RED% #4.4f GREEN% #4.4f BLUE% #4.4f\n"
                    "      AMBIENT 0.25 DIFFUSE 0.75 END_TEXTURE\n",
                    c[0], c[1], c[2]);
        }
        std::fprintf(s_file_ptr1, "  COLOR  F_Dflt  END_OBJECT");
        triangle_bounds(pt_t);    // update bounding info
    }
    if (g_raytrace_format == raytrace_formats::vivid)
    {
        std::fprintf(s_file_ptr1, "}");
    }
    if (g_raytrace_format == raytrace_formats::raw && !g_brief)
    {
        std::fprintf(s_file_ptr1, "\n");
    }

    if (g_raytrace_format != raytrace_formats::dxf)
    {
        std::fprintf(s_file_ptr1, "\n");
    }

    return 0;
}

//******************************************************************
//
//  This routine calculates the min and max values of a triangle
//  for use in creating ray tracer data files. The values of min
//  and max x, y, and z are assumed to be global.
//
//******************************************************************

static void triangle_bounds(float pt_t[3][3])
{
    for (int i = 0; i <= 2; i++)
    {
        for (int j = 0; j <= 2; j++)
        {
            if (pt_t[i][j] < s_min_xyz[j])
            {
                s_min_xyz[j] = pt_t[i][j];
            }
            if (pt_t[i][j] > s_max_xyz[j])
            {
                s_max_xyz[j] = pt_t[i][j];
            }
        }
    }
}

//******************************************************************
//
//  This routine starts a composite object for ray trace data files
//
//******************************************************************

static int start_object()
{
    if (g_raytrace_format != raytrace_formats::povray)
    {
        return 0;
    }

    // Reset the min/max values, for bounding box
    s_min_xyz[2] = 999999.0F;
    s_min_xyz[1] = s_min_xyz[2];
    s_min_xyz[0] = s_min_xyz[1];
    s_max_xyz[2] = -999999.0F;
    s_max_xyz[1] = s_max_xyz[2];
    s_max_xyz[0] = s_max_xyz[1];

    std::fprintf(s_file_ptr1, "COMPOSITE\n");
    return 0;
}

//******************************************************************
//
//  This routine adds a bounding box for the triangles drawn
//  in the last block and completes the composite object created.
//  It uses the globals min and max x,y and z calculated in
//  z calculated in Triangle_Bounds().
//
//******************************************************************

static int end_object(bool triout)
{
    if (g_raytrace_format == raytrace_formats::dxf)
    {
        return 0;
    }
    if (g_raytrace_format == raytrace_formats::povray)
    {
        if (triout)
        {
            // Make sure the bounding box is slightly larger than the object
            for (int i = 0; i <= 2; i++)
            {
                if (s_min_xyz[i] == s_max_xyz[i])
                {
                    s_min_xyz[i] -= 0.01F;
                    s_max_xyz[i] += 0.01F;
                }
                else
                {
                    s_min_xyz[i] -= (s_max_xyz[i] - s_min_xyz[i]) * 0.01F;
                    s_max_xyz[i] += (s_max_xyz[i] - s_min_xyz[i]) * 0.01F;
                }
            }

            // Add the bounding box info
            std::fprintf(s_file_ptr1, " BOUNDED_BY\n  INTERSECTION\n");
            std::fprintf(s_file_ptr1, "   PLANE <-1.0  0.0  0.0 > % #4.3f END_PLANE\n", -s_min_xyz[0]);
            std::fprintf(s_file_ptr1, "   PLANE < 1.0  0.0  0.0 > % #4.3f END_PLANE\n",  s_max_xyz[0]);
            std::fprintf(s_file_ptr1, "   PLANE < 0.0 -1.0  0.0 > % #4.3f END_PLANE\n", -s_min_xyz[1]);
            std::fprintf(s_file_ptr1, "   PLANE < 0.0  1.0  0.0 > % #4.3f END_PLANE\n",  s_max_xyz[1]);
            std::fprintf(s_file_ptr1, "   PLANE < 0.0  0.0 -1.0 > % #4.3f END_PLANE\n", -s_min_xyz[2]);
            std::fprintf(s_file_ptr1, "   PLANE < 0.0  0.0  1.0 > % #4.3f END_PLANE\n",  s_max_xyz[2]);
            std::fprintf(s_file_ptr1, "  END_INTERSECTION\n END_BOUND\n");
        }

        // Complete the composite object statement
        std::fprintf(s_file_ptr1, "END_%s\n", "COMPOSITE");
    }

    if (g_raytrace_format != raytrace_formats::acrospin && g_raytrace_format != raytrace_formats::rayshade)
    {
        std::fprintf(s_file_ptr1, "\n");
    }

    return 0;
}

static void line3d_cleanup()
{
    if (g_raytrace_format != raytrace_formats::none && s_file_ptr1)
    {
        // Finish up the ray tracing files
        if (g_raytrace_format != raytrace_formats::rayshade && g_raytrace_format != raytrace_formats::dxf)
        {
            std::fprintf(s_file_ptr1, "\n");
        }
        if (g_raytrace_format == raytrace_formats::vivid)
        {
            std::fprintf(s_file_ptr1, "\n\n//");
        }
        if (g_raytrace_format == raytrace_formats::mtv)
        {
            std::fprintf(s_file_ptr1, "\n\n#");
        }

        if (g_raytrace_format == raytrace_formats::rayshade)
        {
            // end grid aggregate
            std::fprintf(s_file_ptr1, "end\n\n/*good landscape:*/\n%s%s\n/*",
                    "screen 640 480\neyep 0 2.1 0.8\nlookp 0 0 -0.95\nlight 1 point -2 1 1.5\n", "background .3 0 0\nreport verbose\n");
        }
        if (g_raytrace_format == raytrace_formats::acrospin)
        {
            std::fprintf(s_file_ptr1, "LineList From To\n");
            for (int i = 0; i < s_ro; i++)
            {
                for (int j = 0; j <= s_co_max; j++)
                {
                    if (j < s_co_max)
                    {
                        std::fprintf(s_file_ptr1, "R%dC%d R%dC%d\n", i, j, i, j + 1);
                    }
                    if (i < s_ro - 1)
                    {
                        std::fprintf(s_file_ptr1, "R%dC%d R%dC%d\n", i, j, i + 1, j);
                    }
                    if (i && i < s_ro && j < s_co_max)
                    {
                        std::fprintf(s_file_ptr1, "R%dC%d R%dC%d\n", i, j, i - 1, j + 1);
                    }
                }
            }
            std::fprintf(s_file_ptr1, "\n\n--");
        }
        if (g_raytrace_format != raytrace_formats::dxf)
        {
            std::fprintf(s_file_ptr1, "{ No. Of Triangles = %ld }*/\n\n", s_num_tris);
        }
        if (g_raytrace_format == raytrace_formats::dxf)
        {
            std::fprintf(s_file_ptr1, "  0\nENDSEC\n  0\nEOF\n");
        }
        std::fclose(s_file_ptr1);
        s_file_ptr1 = nullptr;
    }
    if (g_targa_out)
    {
        // Finish up targa files
        s_targa_header_24 = 18;         // Reset Targa header size
        enddisk();
        if (g_debug_flag == debug_flags::none && (!s_t_safe || s_error) && g_targa_overlay)
        {
            dir_remove(g_working_dir, g_light_name);
            std::rename(s_targa_temp.c_str(), g_light_name.c_str());
        }
        if (g_debug_flag == debug_flags::none && g_targa_overlay)
        {
            dir_remove(g_working_dir, s_targa_temp);
        }
    }
    s_error = 0;
    s_t_safe = false;
}

static void set_upr_lwr()
{
    s_upr_lwr[0] = (BYTE)(g_logical_screen_x_dots & 0xff);
    s_upr_lwr[1] = (BYTE)(g_logical_screen_x_dots >> 8);
    s_upr_lwr[2] = (BYTE)(g_logical_screen_y_dots & 0xff);
    s_upr_lwr[3] = (BYTE)(g_logical_screen_y_dots >> 8);
    s_line_length1 = 3 * g_logical_screen_x_dots;    // line length @ 3 bytes per pixel
}

static int first_time(int linelen, VECTOR v)
{
    int err;
    MATRIX lightm;               // m w/no trans, keeps obj. on screen
    float twocosdeltatheta;
    double xval;
    double yval;
    double zval; // rotation values
    // corners of transformed xdotx by ydots x colors box
    double xmin;
    double ymin;
    double zmin;
    double xmax;
    double ymax;
    double zmax;
    double v_length;
    VECTOR origin;
    VECTOR direct;
    VECTOR tmp;
    float theta;
    float theta1;
    float theta2;     // current,start,stop latitude
    float phi1;
    float phi2;       // current start,stop longitude
    float deltatheta; // increment of latitude
    g_out_line_cleanup = line3d_cleanup;

    s_even_odd_row = 0;
    g_calc_time = s_even_odd_row;
    // mark as in-progress, and enable <tab> timer display
    g_calc_status = calc_status_value::IN_PROGRESS;

    s_i_ambient = (unsigned int)(255 * (float)(100 - g_ambient) / 100.0);
    if (s_i_ambient < 1)
    {
        s_i_ambient = 1;
    }

    s_num_tris = 0;

    // Open file for RAY trace output and write header
    if (g_raytrace_format != raytrace_formats::none)
    {
        RAY_Header();
        g_yy_adjust = 0;
        g_xx_adjust = 0;  // Disable shifting in ray tracing
        g_y_shift = 0;
        g_x_shift = 0;
    }

    s_ro = 0;
    s_co = s_ro;
    s_co_max = s_co;

    set_upr_lwr();
    s_error = 0;

    if (g_which_image < stereo_images::BLUE)
    {
        s_t_safe = false; // Not safe yet to mess with the source image
    }

    if (g_targa_out
        && !((g_glasses_type == 1 || g_glasses_type == 2)
            && g_which_image == stereo_images::BLUE))
    {
        if (g_targa_overlay)
        {
            // Make sure target file is a supportable Targa File
            if (targa_validate(g_light_name.c_str()))
            {
                return -1;
            }
        }
        else
        {
            check_writefile(g_light_name, ".tga");
            if (startdisk1(g_light_name, nullptr, false))     // Open new file
            {
                return -1;
            }
        }
    }

    s_rand_factor = 14 - g_randomize_3d;

    s_z_coord = g_file_colors;

    err = line3dmem();
    if (err != 0)
    {
        return err;
    }

    // get scale factors
    s_scale_x = g_x_scale / 100.0;
    s_scale_y = g_y_scale / 100.0;
    if (g_rough)
    {
        s_scale_z = -g_rough / 100.0;
    }
    else
    {
        s_scale_z = -0.0001;
        s_r_scale = s_scale_z;  // if rough=0 make it very flat but plot something
    }

    // aspect ratio calculation - assume screen is 4 x 3
    s_aspect = (double) g_logical_screen_x_dots *.75 / (double) g_logical_screen_y_dots;

    if (!g_sphere)         // skip this slow stuff in sphere case
    {
        //*******************************************************************
        // What is done here is to create a single matrix, m, which has
        // scale, rotation, and shift all combined. This allows us to use
        // a single matrix to transform any point. Additionally, we create
        // two perspective vectors.
        //
        // Start with a unit matrix. Add scale and rotation. Then calculate
        // the perspective vectors. Finally add enough translation to center
        // the final image plus whatever shift the user has set.
        //*******************************************************************

        // start with identity
        identity(g_m);
        identity(lightm);

        // translate so origin is in center of box, so that when we rotate
        // it, we do so through the center
        trans((double) g_logical_screen_x_dots / (-2.0), (double) g_logical_screen_y_dots / (-2.0),
              (double) s_z_coord / (-2.0), g_m);
        trans((double) g_logical_screen_x_dots / (-2.0), (double) g_logical_screen_y_dots / (-2.0),
              (double) s_z_coord / (-2.0), lightm);

        // apply scale factors
        scale(s_scale_x, s_scale_y, s_scale_z, g_m);
        scale(s_scale_x, s_scale_y, s_scale_z, lightm);

        // rotation values - converting from degrees to radians
        xval = g_x_rot / 57.29577;
        yval = g_y_rot / 57.29577;
        zval = g_z_rot / 57.29577;

        if (g_raytrace_format != raytrace_formats::none)
        {
            zval = 0;
            yval = zval;
            xval = yval;
        }

        x_rot(xval, g_m);
        x_rot(xval, lightm);
        y_rot(yval, g_m);
        y_rot(yval, lightm);
        z_rot(zval, g_m);
        z_rot(zval, lightm);

        // Find values of translation that make all x,y,z negative
        // m current matrix
        // 0 means don't show box
        // returns minimum and maximum values of x,y,z in fractal
        corners(g_m, false, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
    }

    // perspective 3D vector - lview[2] == 0 means no perspective

    // set perspective flag
    s_persp = false;
    if (g_viewer_z != 0)
    {
        s_persp = true;
        if (g_viewer_z < 80)           // force float
        {
            g_user_float_flag = true;
        }
    }

    // set up view vector, and put viewer in center of screen
    s_l_view[0] = g_logical_screen_x_dots >> 1;
    s_l_view[1] = g_logical_screen_y_dots >> 1;

    // z value of user's eye - should be more negative than extreme negative part of image
    if (g_sphere)                    // sphere case
    {
        s_l_view[2] = -(long)((double) g_logical_screen_y_dots * (double) g_viewer_z / 100.0);
    }
    else                             // non-sphere case
    {
        s_l_view[2] = (long)((zmin - zmax) * (double) g_viewer_z / 100.0);
    }

    g_view[0] = s_l_view[0];
    g_view[1] = s_l_view[1];
    g_view[2] = s_l_view[2];
    s_l_view[0] = s_l_view[0] << 16;
    s_l_view[1] = s_l_view[1] << 16;
    s_l_view[2] = s_l_view[2] << 16;

    if (!g_sphere)         // sphere skips this
    {
        /* translate back exactly amount we translated earlier plus enough to
         * center image so maximum values are non-positive */
        trans(((double) g_logical_screen_x_dots - xmax - xmin) / 2,
              ((double) g_logical_screen_y_dots - ymax - ymin) / 2, -zmax, g_m);

        // Keep the box centered and on screen regardless of shifts
        trans(((double) g_logical_screen_x_dots - xmax - xmin) / 2,
              ((double) g_logical_screen_y_dots - ymax - ymin) / 2, -zmax, lightm);

        trans((double)(g_x_shift), (double)(-g_y_shift), 0.0, g_m);

        /* matrix m now contains ALL those transforms composed together !!
         * convert m to long integers shifted 16 bits */
        for (int i = 0; i < 4; i++)
        {
            for (int j = 0; j < 4; j++)
            {
                s_llm[i][j] = (long)(g_m[i][j] * 65536.0);
            }
        }
    }
    else
        // sphere stuff goes here
    {
        /* Sphere is on side - north pole on right. Top is -90 degrees
         * latitude; bottom 90 degrees */

        // Map X to this LATITUDE range
        theta1 = (float)(g_sphere_theta_min * PI / 180.0);
        theta2 = (float)(g_sphere_theta_max * PI / 180.0);

        // Map Y to this LONGITUDE range
        phi1 = (float)(g_sphere_phi_min * PI / 180.0);
        phi2 = (float)(g_sphere_phi_max * PI / 180.0);

        theta = theta1;

        //*******************************************************************
        // Thanks to Hugh Bray for the following idea: when calculating
        // a table of evenly spaced sines or cosines, only a few initial
        // values need be calculated, and the remaining values can be
        // gotten from a derivative of the sine/cosine angle sum formula
        // at the cost of one multiplication and one addition per value!
        //
        // This idea is applied once here to get a complete table for
        // latitude, and near the bottom of this routine to incrementally
        // calculate longitude.
        //
        // Precalculate 2*cos(deltaangle), sin(start) and sin(start+delta).
        // Then apply recursively:
        // sin(angle+2*delta) = sin(angle+delta) * 2cosdelta - sin(angle)
        //
        // Similarly for cosine. Neat!
        //*******************************************************************

        deltatheta = (float)(theta2 - theta1) / (float) linelen;

        // initial sin,cos theta
        s_sin_theta_array[0] = (float) std::sin((double) theta);
        s_cos_theta_array[0] = (float) std::cos((double) theta);
        s_sin_theta_array[1] = (float) std::sin((double)(theta + deltatheta));
        s_cos_theta_array[1] = (float) std::cos((double)(theta + deltatheta));

        // sin,cos delta theta
        twocosdeltatheta = (float)(2.0 * std::cos((double) deltatheta));

        // build table of other sin,cos with trig identity
        for (int i = 2; i < (int) linelen; i++)
        {
            s_sin_theta_array[i] = s_sin_theta_array[i - 1] * twocosdeltatheta -
                               s_sin_theta_array[i - 2];
            s_cos_theta_array[i] = s_cos_theta_array[i - 1] * twocosdeltatheta -
                               s_cos_theta_array[i - 2];
        }

        // now phi - these calculated as we go - get started here
        s_delta_phi = (float)(phi2 - phi1) / (float) g_height;

        // initial sin,cos phi

        s_old_sin_phi1 = (float) std::sin((double) phi1);
        s_sin_phi = s_old_sin_phi1;
        s_old_cos_phi1 = (float) std::cos((double) phi1);
        s_cos_phi = s_old_cos_phi1;
        s_old_sin_phi2 = (float) std::sin((double)(phi1 + s_delta_phi));
        s_old_cos_phi2 = (float) std::cos((double)(phi1 + s_delta_phi));

        // sin,cos delta phi
        s_two_cos_delta_phi = (float)(2.0 * std::cos((double) s_delta_phi));

        // affects how rough planet terrain is
        if (g_rough)
        {
            s_r_scale = .3 * g_rough / 100.0;
        }

        // radius of planet
        s_radius = (double)(g_logical_screen_y_dots) / 2;

        // precalculate factor
        s_rXr_scale = s_radius * s_r_scale;

        s_scale_y = g_sphere_radius/100.0;
        s_scale_x = s_scale_y;
        s_scale_z = s_scale_x;      // Need x,y,z for RAY

        // adjust x scale factor for aspect
        s_scale_x *= s_aspect;

        // precalculation factor used in sphere calc
        s_radius_factor = s_r_scale * s_radius / (double) s_z_coord;

        if (s_persp)                // precalculate fudge factor
        {
            double radius;
            double zview;
            double angle;

            s_x_center = s_x_center << 16;
            s_y_center = s_y_center << 16;

            s_radius_factor *= 65536.0;
            s_radius *= 65536.0;

            /* calculate z cutoff factor attempt to prevent out-of-view surfaces
             * from being written */
            zview = -(long)((double) g_logical_screen_y_dots * (double) g_viewer_z / 100.0);
            radius = (double)(g_logical_screen_y_dots) / 2;
            angle = std::atan(-radius / (zview + radius));
            s_z_cutoff = -radius - std::sin(angle) * radius;
            s_z_cutoff *= 1.1;        // for safety
            s_z_cutoff *= 65536L;
        }
    }

    // set fill plot function
    if (g_fill_type != fill_type::SURFACE_CONSTANT)
    {
        s_fill_plot = interpcolor;
    }
    else
    {
        s_fill_plot = clipcolor;

        if (g_transparent_color_3d[0] || g_transparent_color_3d[1])
        {
            // If transparent colors are set
            s_fill_plot = T_clipcolor;// Use the transparent plot function
        }
    }

    // Both Sphere and Normal 3D
    s_light_direction[0] = g_light_x;
    direct[0] = s_light_direction[0];
    s_light_direction[1] = -g_light_y;
    direct[1] = s_light_direction[1];
    s_light_direction[2] = g_light_z;
    direct[2] = s_light_direction[2];

    /* Needed because sclz = -ROUGH/100 and light_direction is transformed in
     * FILLTYPE 6 but not in 5. */
    if (g_fill_type == fill_type::LIGHT_SOURCE_BEFORE)
    {
        s_light_direction[2] = -g_light_z;
        direct[2] = s_light_direction[2];
    }

    if (g_fill_type == fill_type::LIGHT_SOURCE_AFTER)           // transform light direction
    {
        /* Think of light direction  as a vector with tail at (0,0,0) and head
         * at (light_direction). We apply the transformation to BOTH head and
         * tail and take the difference */

        v[0] = 0.0;
        v[1] = 0.0;
        v[2] = 0.0;
        vec_mat_mul(v, g_m, v);
        vec_mat_mul(s_light_direction, g_m, s_light_direction);

        for (int i = 0; i < 3; i++)
        {
            s_light_direction[i] -= v[i];
        }
    }
    normalize_vector(s_light_direction);

    if (g_preview && g_show_box)
    {
        normalize_vector(direct);

        // move light vector to be more clear with grey scale maps
        origin[0] = (3 * g_logical_screen_x_dots) / 16;
        origin[1] = (3 * g_logical_screen_y_dots) / 4;
        if (g_fill_type == fill_type::LIGHT_SOURCE_AFTER)
        {
            origin[1] = (11 * g_logical_screen_y_dots) / 16;
        }

        origin[2] = 0.0;

        v_length = std::min(g_logical_screen_x_dots, g_logical_screen_y_dots) / 2;
        if (s_persp && g_viewer_z <= s_p)
        {
            v_length *= (long)(s_p + 600) / ((long)(g_viewer_z + 600) * 2);
        }

        /* Set direct[] to point from origin[] in direction of untransformed
         * light_direction (direct[]). */
        for (int i = 0; i < 3; i++)
        {
            direct[i] = origin[i] + direct[i] * v_length;
        }

        // center light box
        for (int i = 0; i < 2; i++)
        {
            tmp[i] = (direct[i] - origin[i]) / 2;
            origin[i] -= tmp[i];
            direct[i] -= tmp[i];
        }

        /* Draw light source vector and box containing it, draw_light_box will
         * transform them if necessary. */
        draw_light_box(origin, direct, lightm);
        /* draw box around original field of view to help visualize effect of
         * rotations 1 means show box - xmin etc. do nothing here */
        if (!g_sphere)
        {
            corners(g_m, true, &xmin, &ymin, &zmin, &xmax, &ymax, &zmax);
        }
    }

    // bad has values caught by clipping
    s_bad.x = g_bad_value;
    s_f_bad.x = (float) s_bad.x;
    s_bad.y = g_bad_value;
    s_f_bad.y = (float) s_bad.y;
    s_bad.color = g_bad_value;
    s_f_bad.color = (float) s_bad.color;
    for (int i = 0; i < (int) linelen; i++)
    {
        s_last_row[i] = s_bad;
        s_f_last_row[i] = s_f_bad;
    }
    g_got_status = status_values::THREE_D;
    return 0;
} // end of once-per-image intializations

static int line3dmem()
{
    /* lastrow stores the previous row of the original GIF image for
       the purpose of filling in gaps with triangle procedure */
    s_last_row.resize(g_logical_screen_x_dots);

    if (g_sphere)
    {
        s_sin_theta_array.resize(g_logical_screen_x_dots);
        s_cos_theta_array.resize(g_logical_screen_x_dots);
    }
    s_f_last_row.resize(g_logical_screen_x_dots);
    if (g_potential_16bit)
    {
        s_fraction.resize(g_logical_screen_x_dots);
    }
    s_min_max_x.clear();

    // these fill types call putatriangle which uses minmax_x
    if (g_fill_type == fill_type::SURFACE_INTERPOLATED || g_fill_type == fill_type::SURFACE_CONSTANT ||
        g_fill_type == fill_type::LIGHT_SOURCE_BEFORE || g_fill_type == fill_type::LIGHT_SOURCE_AFTER)
    {
        s_min_max_x.resize(OLD_MAX_PIXELS);
    }

    return 0;
}
