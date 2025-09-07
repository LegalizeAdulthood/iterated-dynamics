// SPDX-License-Identifier: GPL-3.0-only
//
//**********************************************************************
// This file contains a 3D replacement for the out_line function called
// by the decoder. The purpose is to apply various 3D transformations
// before displaying points. Called once per line of the input file.
//**********************************************************************
#include "3d/line3d.h"

#include "3d/plot3d.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/pixel_limits.h"
#include "io/check_write_file.h"
#include "io/dir_file.h"
#include "io/gifview.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "math/rand15.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/version.h"
#include "ui/diskvid.h"
#include "ui/framain2.h"
#include "ui/rotate.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"
#include "ui/video.h"

#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

template <typename T>
struct PointColorT
{
    T x;
    T y;
    T color;
};

using PointColor = PointColorT<int>;
using FPointColor = PointColorT<float>;

struct MinMax
{
    int min_x;
    int max_x;
};

enum class FileError
{
    NONE = 0,
    OPEN_FAILED = 1,
    DISK_FULL = 2,
    BAD_IMAGE_SIZE = 3,
    BAD_FILE_TYPE = 4,
};

// routines in this module
static int first_time(int line_len, id::Vector v);
static void hsv_to_rgb(
    Byte *red, Byte *green, Byte *blue, unsigned long hue, unsigned long sat, unsigned long val);
static int line3d_mem();
static int rgb_to_hsv(
    Byte red, Byte green, Byte blue, unsigned long *hue, unsigned long *sat, unsigned long *val);
static bool set_pixel_buff(Byte *pixels, Byte *fraction, unsigned line_len);
static void set_upr_lwr();
static int end_object(bool tri_out);
static int off_screen(PointColor pt);
static int out_triangle(FPointColor pt1, FPointColor pt2, FPointColor pt3, int c1, int c2, int c3);
static int ray_header();
static int start_object();
static void draw_light_box(double *origin, double *direct, id::Matrix light_m);
static void draw_rect(id::Vector v0, id::Vector v1, id::Vector v2, id::Vector v3, int color, bool rect);
static void line3d_cleanup();
static void clip_color(int x, int y, int color);
static void interp_color(int x, int y, int color);
static void put_triangle(PointColor pt1, PointColor pt2, PointColor pt3, int color);
static void put_min_max(int x, int y, int color);
static void triangle_bounds(float pt_t[3][3]);
static void transparent_clip_color(int x, int y, int color);
static void vec_draw_line(double *v1, double *v2, int color);
static void file_error(const std::string &filename, FileError error);
static bool targa_validate(const std::string &filename);

// static variables
static void (*s_fill_plot)(int x, int y, int color){};   //
static void (*s_normal_plot)(int x, int y, int color){}; //
static float s_delta_phi{};                              // increment of latitude, longitude
static double s_r_scale{};                               // surface roughness factor
static long s_x_center{}, s_y_center{};                  // circle center
static double s_scale_x{}, s_scale_y{}, s_scale_z{};     // scale factors
static double s_radius{};                                // radius values
static double s_radius_factor{};                         // for intermediate calculation
static id::MatrixL s_llm{};                              // ""
static id::VectorL s_l_view{};                           // for perspective views
static double s_z_cutoff{};                              // perspective backside cutoff value
static float s_two_cos_delta_phi{};                      //
static float s_cos_phi{}, s_sin_phi{};                   // precalculated sin/cos of longitude
static float s_old_cos_phi1{}, s_old_sin_phi1{};         //
static float s_old_cos_phi2{}, s_old_sin_phi2{};         //
static std::vector<Byte> s_fraction;                     // float version of pixels array
static float s_min_xyz[3]{}, s_max_xyz[3]{};             // For Raytrace output
static int s_line_length1{};                             //
static int s_targa_header_24 = 18;                       // Size of current Targa-24 header
static std::FILE *s_raytrace_file{};                     //
static unsigned int s_i_ambient{};                       //
static int s_rand_factor{};                              //
static int s_haze_mult{};                                //
static Byte s_t24 = 24;                                  //
static Byte s_t32 = 32;                                  //
static Byte s_upr_lwr[4]{};                              //
static bool s_temp_safe{};                       // Original Targa Image successfully copied to targa_temp
static id::Vector s_light_direction{};           //
static Byte s_real_color{};                      // Actual color of cur pixel
static int s_ro{}, s_co{}, s_co_max{};           // For use in Acrospin support
static int s_local_preview_factor{};             //
static int s_z_coord = 256;                      //
static double s_aspect{};                        // aspect ratio
static int s_even_odd_row{};                     //
static std::vector<float> s_sin_theta_array;     // all sine thetas go here
static std::vector<float> s_cos_theta_array;     // all cosine thetas go here
static double s_r_x_r_scale{};                   // precalculation factor
static bool s_persp{};                           // flag for indicating perspective transformations
static PointColor s_p1{}, s_p2{}, s_p3{};        //
static FPointColor s_f_bad{};                    // out of range value
static PointColor s_bad{};                       // out of range value
static long s_num_tris{};                        // number of triangles output to ray trace file
static std::vector<FPointColor> s_f_last_row;    //
static int s_real_v{};                           // Actual value of V for fillytpe>4 monochrome images
static FileError s_error{};                      //
static std::string s_targa_temp("fractemp.tga"); //
static int s_p = 250;                            // Perspective dist used when viewing light vector
static const int s_bad_check = -3000;            // check values against this to determine if good
static std::vector<PointColor> s_last_row;       // this array remembers the previous line
static std::vector<MinMax> s_min_max_x;          // array of min and max x values used in triangle fill
static id::Vector s_cross{};                     //
static id::Vector s_tmp_cross{};                 //
static PointColor s_old_last{};                  // old pixels

// global variables defined here
void (*g_standard_plot)(int, int, int){};
id::Matrix g_m{}; // transformation matrix
int g_ambient{};
int g_randomize_3d{};
int g_haze{};
std::string g_light_name{"fract001"};
bool g_targa_overlay{};
Byte g_background_color[3]{};
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
RayTraceFormat g_raytrace_format{}; // Flag to generate Ray trace compatible files in 3d
bool g_brief{};                     // 1 = short ray trace files
id::Vector g_view{};                // position of observer for perspective

int line3d(Byte * pixels, unsigned line_len)
{
    int rnd;
    float f_water = 0.0F;       // transformed WATERLINE for ray trace files
    double r0;
    int x_center0 = 0;
    int y_center0 = 0;      // Unfudged versions
    double r;                    // sphere radius
    float cos_theta;
    float sin_theta; // precalculated sin/cos of latitude
    int next;       // used by preview and grid
    int col;        // current column (original GIF)
    PointColor cur;      // current pixels
    PointColor old;      // old pixels
    FPointColor f_cur{};
    FPointColor f_old;
    id::Vector v;                    // double vector
    id::Vector v1;
    id::Vector v2;
    id::Vector cross_avg;
    bool cross_not_init;           // flag for crossavg init indication
    id::VectorL lv;                  // long equivalent of v
    id::VectorL lv0;                 // long equivalent of v
    int last_dot;
    long fudge;

    fudge = 1L << 16;

    if (g_transparent_color_3d[0] || g_transparent_color_3d[1])
    {
        s_normal_plot = transparent_clip_color;
        g_plot = s_normal_plot;          // Use transparent plot function
    }
    else                            // Use the usual plot function with clipping
    {
        s_normal_plot = clip_color;
        g_plot = s_normal_plot;
    }

    g_current_row = g_row_count;           // use separate variable to allow for pot16bit files
    if (g_potential_16bit)
    {
        g_current_row >>= 1;
    }

    //**********************************************************************
    // This IF clause is executed ONCE per image. All precalculations are
    // done here, without any special concern about speed. DANGER -
    // communication with the rest of the program is generally via static
    // or global variables.
    //**********************************************************************
    if (g_row_count++ == 0)
    {
        int err = first_time(line_len, v);
        if (err != 0)
        {
            return err;
        }
        if (g_logical_screen_x_dots > OLD_MAX_PIXELS)
        {
            return -1;
        }
        cross_avg[0] = 0;
        cross_avg[1] = 0;
        cross_avg[2] = 0;
        s_x_center = g_logical_screen_x_dots / 2 + g_x_shift;
        x_center0 = (int) s_x_center;
        s_y_center = g_logical_screen_y_dots / 2 - g_y_shift;
        y_center0 = (int) s_y_center;
    }
    // make sure these pixel coordinates are out of range
    old = s_bad;
    f_old = s_f_bad;

    // copies pixels buffer to float type fraction buffer for fill purposes
    if (g_potential_16bit)
    {
        if (set_pixel_buff(pixels, s_fraction.data(), line_len))
        {
            return 0;
        }
    }
    else if (g_gray_flag)             // convert color numbers to grayscale values
    {
        for (col = 0; col < (int) line_len; col++)
        {
            int pal;
            int color_num;
            color_num = pixels[col];
            // effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255
            pal = ((int) g_dac_box[color_num][0] * 77 +
                   (int) g_dac_box[color_num][1] * 151 +
                   (int) g_dac_box[color_num][2] * 28);
            pal >>= 6;
            pixels[col] = (Byte) pal;
        }
    }
    cross_not_init = true;
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
    last_dot = std::min(g_logical_screen_x_dots - 1, (int) line_len - 1);
    if (id::g_fill_type >= id::FillType::LIGHT_SOURCE_BEFORE)
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

    if (g_preview_factor >= g_logical_screen_y_dots || g_preview_factor > last_dot)
    {
        g_preview_factor = std::min(g_logical_screen_y_dots - 1, last_dot);
    }

    s_local_preview_factor = g_logical_screen_y_dots / g_preview_factor;

    bool tout = false;          // triangle has been sent to ray trace file
    // Insure last line is drawn in preview and filltypes <0
    // Draw mod preview lines
    if ((g_raytrace_format != RayTraceFormat::NONE || g_preview || id::g_fill_type < id::FillType::POINTS) //
        && g_current_row != g_logical_screen_y_dots - 1                                            //
        && g_current_row % s_local_preview_factor                                                  //
        && !(g_raytrace_format == RayTraceFormat::NONE                                             //
               && id::g_fill_type > id::FillType::SOLID_FILL                                               //
               && g_current_row == 1))
    {
        // Get init geometry in lightsource modes
        goto really_the_bottom; // skip over most of the line3d calcs
    }
    if (driver_is_disk())
    {
        dvid_status(1, "mapping to 3d, reading line " + std::to_string(g_current_row));
    }

    if (!col && g_raytrace_format != RayTraceFormat::NONE && g_current_row != 0)
    {
        start_object();
    }
    // PROCESS ROW LOOP BEGINS HERE
    while (col < (int) line_len)
    {
        if ((g_raytrace_format != RayTraceFormat::NONE || g_preview || id::g_fill_type < id::FillType::POINTS)
            && (col != last_dot)             // if this is not the last col
                                            // if not the 1st or mod factor col
            && (col % (int)(s_aspect * s_local_preview_factor))
            && !(g_raytrace_format == RayTraceFormat::NONE && id::g_fill_type > id::FillType::SOLID_FILL && col == 1))
        {
            goto loop_bottom;
        }

        s_real_color = pixels[col];
        cur.color = s_real_color;
        f_cur.color = (float) cur.color;

        if (g_raytrace_format != RayTraceFormat::NONE|| g_preview || id::g_fill_type < id::FillType::POINTS)
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
        next = std::min(next, last_dot);

        if (cur.color > 0 && cur.color < id::g_water_line)
        {
            s_real_color = (Byte) id::g_water_line;
            cur.color = s_real_color;
            f_cur.color = (float) cur.color;  // "lake"
        }
        else if (g_potential_16bit)
        {
            f_cur.color += ((float) s_fraction[col]) / (float)(1 << 8);
        }

        if (id::g_sphere)            // sphere case
        {
            sin_theta = s_sin_theta_array[col];
            cos_theta = s_cos_theta_array[col];

            if (s_sin_phi < 0 && g_raytrace_format == RayTraceFormat::NONE && id::g_fill_type >= id::FillType::POINTS)
            {
                cur = s_bad;
                f_cur = s_f_bad;
                goto loop_bottom; // another goto !
            }
            //**********************************************************
            // KEEP THIS FOR DOCS - original formula --
            // if (rscale < 0.0)
            // r = 1.0+((double)cur.color/(double)zcoord)*rscale;
            // else
            // r = 1.0-rscale+((double)cur.color/(double)zcoord)*rscale;
            // R = (double)ydots/2;
            // r = r*R;
            // cur.x = xdots/2 + sclx*r*sin_theta*aspect + xup ;
            // cur.y = ydots/2 + scly*r*cos_theta*cosphi - yup ;
            //**********************************************************

            if (s_r_scale < 0.0)
            {
                r = s_radius + s_radius_factor * (double) f_cur.color * cos_theta;
            }
            else if (s_r_scale > 0.0)
            {
                r = s_radius - s_r_x_r_scale + s_radius_factor * (double) f_cur.color * cos_theta;
            }
            else
            {
                r = s_radius;
            }
            // Allow Ray trace to go through so display ok
            if (s_persp || g_raytrace_format != RayTraceFormat::NONE)
            {
                // how do lv[] and cur and f_cur all relate
                // NOTE: fudge was pre-calculated above in r and R
                // (almost) guarantee negative
                lv[2] = (long)(-s_radius - r * cos_theta * s_sin_phi);      // z
                if (lv[2] > s_z_cutoff && id::g_fill_type >= id::FillType::POINTS)
                {
                    cur = s_bad;
                    f_cur = s_f_bad;
                    goto loop_bottom;      // another goto !
                }
                lv[0] = (long)(s_x_center + sin_theta * s_scale_x * r);   // x
                lv[1] = (long)(s_y_center + cos_theta * s_cos_phi * s_scale_y * r);  // y

                if ((id::g_fill_type >= id::FillType::LIGHT_SOURCE_BEFORE) ||
                    g_raytrace_format != RayTraceFormat::NONE)
                {
                    // calculate illumination normal before persp

                    r0 = r / 65536L;
                    f_cur.x = (float) (x_center0 + sin_theta * s_scale_x * r0);
                    f_cur.y = (float) (y_center0 + cos_theta * s_cos_phi * s_scale_y * r0);
                    f_cur.color = (float) (-r0 * cos_theta * s_sin_phi);
                }
                v[0] = lv[0];
                v[1] = lv[1];
                v[2] = lv[2];
                v[0] /= fudge;
                v[1] /= fudge;
                v[2] /= fudge;
                id::perspective(v);
                cur.x = (int) (v[0] + .5 + g_xx_adjust);
                cur.y = (int) (v[1] + .5 + g_yy_adjust);
            }
            // Not sure how this a 3rd if above relate
            else
            {
                // Why the xx- and yyadjust here and not above?
                f_cur.x = (float) (s_x_center + sin_theta*s_scale_x*r + g_xx_adjust);
                cur.x = (int) f_cur.x;
                f_cur.y = (float)(s_y_center + cos_theta*s_cos_phi*s_scale_y*r + g_yy_adjust);
                cur.y = (int) f_cur.y;
                if (id::g_fill_type >= id::FillType::LIGHT_SOURCE_BEFORE || g_raytrace_format != RayTraceFormat::NONE)          // why do we do this for filltype>5?
                {
                    f_cur.color = (float)(-r * cos_theta * s_sin_phi * s_scale_z);
                }
                v[0] = 0;
                v[1] = 0;
                v[2] = 0;               // TODO: Why do we do this?
            }
        }
        else                            // non-sphere 3D
        {
            // do in float if integer math overflowed or doing Ray trace
            // slow float version for comparison
            v[0] = col;
            v[1] = g_current_row;
            v[2] = f_cur.color; // Actually the z value

            id::vec_g_mat_mul(v);   // matrix*vector routine

            if (id::g_fill_type > id::FillType::SOLID_FILL || g_raytrace_format != RayTraceFormat::NONE)
            {
                f_cur.x = (float) v[0];
                f_cur.y = (float) v[1];
                f_cur.color = (float) v[2];

                if (g_raytrace_format == RayTraceFormat::ACROSPIN)
                {
                    f_cur.x = f_cur.x * (2.0F / g_logical_screen_x_dots) - 1.0F;
                    f_cur.y = f_cur.y * (2.0F / g_logical_screen_y_dots) - 1.0F;
                    f_cur.color = -f_cur.color * (2.0F / g_num_colors) - 1.0F;
                }
            }

            if (s_persp && g_raytrace_format == RayTraceFormat::NONE)
            {
                id::perspective(v);
            }
            cur.x = (int) std::lround(v[0] + g_xx_adjust);
            cur.y = (int) std::lround(v[1] + g_yy_adjust);

            v[0] = 0;
            v[1] = 0;
            v[2] = id::g_water_line;
            id::vec_g_mat_mul(v);
            f_water = (float) v[2];
        }

        if (g_randomize_3d != 0)
        {
            if (cur.color > id::g_water_line)
            {
                rnd = RAND15() >> 8;     // 7-bit number
                rnd = rnd * rnd >> s_rand_factor;  // n-bit number

                if (std::rand() & 1)
                {
                    rnd = -rnd;   // Make +/- n-bit number
                }

                if (cur.color + rnd >= g_colors)
                {
                    cur.color = g_colors - 2;
                }
                else if (cur.color + rnd <= id::g_water_line)
                {
                    cur.color = id::g_water_line + 1;
                }
                else
                {
                    cur.color = cur.color + rnd;
                }
                s_real_color = (Byte)cur.color;
            }
        }

        if (g_raytrace_format != RayTraceFormat::NONE)
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
                    goto loop_bottom;
                }

                if (g_raytrace_format != RayTraceFormat::ACROSPIN)      // Output the vertex info
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

            if (col < last_dot && g_current_row
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
                    goto loop_bottom;
                }

                if (g_raytrace_format != RayTraceFormat::ACROSPIN)      // Output the vertex info
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

            if (g_raytrace_format == RayTraceFormat::ACROSPIN)       // Output vertex info for Acrospin
            {
                fmt::print(s_raytrace_file, "{: 4.4f} {: 4.4f} {: 4.4f} R{:d}C{:d}\n", //
                    f_cur.x, f_cur.y, static_cast<double>(f_cur.color), s_ro, s_co);
                s_co_max = std::max(s_co, s_co_max);
                s_co++;
            }
            goto loop_bottom;
        }

        switch (id::g_fill_type)
        {
        case id::FillType::SURFACE_GRID:
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

        case id::FillType::POINTS:
            g_plot(cur.x, cur.y, cur.color);
            break;

        case id::FillType::WIRE_FRAME:                // connect-a-dot
            if (old.x < g_logical_screen_x_dots && col
                && old.x > s_bad_check
                && old.y > s_bad_check)        // Don't draw from old to cur on col 0
            {
                driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            }
            break;

        case id::FillType::SURFACE_INTERPOLATED: // with interpolation
        case id::FillType::SURFACE_CONSTANT:     // no interpolation
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
                put_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
            }
            if (g_current_row && col)  // skip first row and first column
            {
                if (col == 1)
                {
                    put_triangle(s_last_row[col], s_old_last, old, old.color);
                }

                if (col < last_dot)
                {
                    put_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
                }
                put_triangle(old, s_last_row[col], cur, cur.color);
            }
            break;

        case id::FillType::SOLID_FILL:
            if (id::g_sphere)
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

                if (id::long_vec_mat_mul_persp(lv, s_llm, lv0, lv, s_l_view, 16))
                {
                    cur = s_bad;
                    f_cur = s_f_bad;
                    goto loop_bottom;
                }

                // Round and fudge back to original
                old.x = (int)((lv[0] + 32768L) >> 16);
                old.y = (int)((lv[1] + 32768L) >> 16);
            }
            old.x = std::max(old.x, 0);
            if (old.x >= g_logical_screen_x_dots)
            {
                old.x = g_logical_screen_x_dots - 1;
            }
            old.y = std::max(old.y, 0);
            if (old.y >= g_logical_screen_y_dots)
            {
                old.y = g_logical_screen_y_dots - 1;
            }
            driver_draw_line(old.x, old.y, cur.x, cur.y, cur.color);
            break;

        case id::FillType::LIGHT_SOURCE_BEFORE:
        case id::FillType::LIGHT_SOURCE_AFTER:
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

                id::cross_product(v1, v2, s_cross);

                // normalize cross - and check if non-zero
                if (id::normalize_vector(s_cross))
                {
                    if (g_debug_flag != DebugFlags::NONE)
                    {
                        stop_msg("debug, cur.color=bad");
                    }
                    f_cur.color = (float) s_bad.color;
                    cur.color = f_cur.color;
                }
                else
                {
                    // line-wise averaging scheme
                    if (id::g_light_avg > 0)
                    {
                        if (cross_not_init)
                        {
                            // initialize array of old normal vectors
                            cross_avg[0] = s_cross[0];
                            cross_avg[1] = s_cross[1];
                            cross_avg[2] = s_cross[2];
                            cross_not_init = false;
                        }
                        s_tmp_cross[0] = (cross_avg[0] * id::g_light_avg + s_cross[0]) /
                                      (id::g_light_avg + 1);
                        s_tmp_cross[1] = (cross_avg[1] * id::g_light_avg + s_cross[1]) /
                                      (id::g_light_avg + 1);
                        s_tmp_cross[2] = (cross_avg[2] * id::g_light_avg + s_cross[2]) /
                                      (id::g_light_avg + 1);
                        s_cross[0] = s_tmp_cross[0];
                        s_cross[1] = s_tmp_cross[1];
                        s_cross[2] = s_tmp_cross[2];
                        if (id::normalize_vector(s_cross))
                        {
                            // this shouldn't happen
                            if (g_debug_flag != DebugFlags::NONE)
                            {
                                stop_msg("debug, normal vector err2");
                            }
                            f_cur.color = (float) g_colors;
                            cur.color = f_cur.color;
                        }
                    }
                    cross_avg[0] = s_tmp_cross[0];
                    cross_avg[1] = s_tmp_cross[1];
                    cross_avg[2] = s_tmp_cross[2];

                    // dot product of unit vectors is cos of angle between
                    // we will use this value to shade surface

                    cur.color = (int)(1 + (g_colors - 2) *
                                      (1.0 - id::dot_product(s_cross, s_light_direction)));
                }
                /* if colors out of range, set them to min or max color index
                 * but avoid background index. This makes colors "opaque" so
                 * SOMETHING plots. These conditions shouldn't happen but just
                 * in case                                        */
                // prevent transparent colors
                // avoid background
                cur.color = std::max(cur.color, 1);
                cur.color = std::min(cur.color, g_colors - 1);

                // why "col < 2"? So we have sufficient geometry for the fill
                // algorithm, which needs previous point in same row to have
                // already been calculated (variable old)
                // fix ragged left margin in preview
                if (col == 1 && g_current_row > 1)
                {
                    put_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
                }

                if (col < 2 || g_current_row < 2)         // don't have valid colors yet
                {
                    break;
                }

                if (col < last_dot)
                {
                    put_triangle(s_last_row[next], s_last_row[col], cur, cur.color);
                }
                put_triangle(old, s_last_row[col], cur, cur.color);

                g_plot = g_standard_plot;
            }
            break;
        }                      // End of CASE statement for fill type
loop_bottom:
        if (g_raytrace_format != RayTraceFormat::NONE || (id::g_fill_type != id::FillType::POINTS && id::g_fill_type != id::FillType::SOLID_FILL))
        {
            // for triangle and grid fill purposes
            s_old_last = s_last_row[col];
            s_last_row[col] = cur;
            old = s_last_row[col];

            // for illumination model purposes
            s_f_last_row[col] = f_cur;
            f_old = s_f_last_row[col];
            if (g_current_row && g_raytrace_format != RayTraceFormat::NONE && col >= last_dot)
                // if we're at the end of a row, close the object
            {
                end_object(tout);
                tout = false;
                if (std::ferror(s_raytrace_file))
                {
                    std::fclose(s_raytrace_file);
                    std::remove(g_light_name.c_str());
                    file_error(g_raytrace_filename, FileError::DISK_FULL);
                    return -1;
                }
            }
        }
        col++;
    }                         // End of while statement for plotting line
    s_ro++;
really_the_bottom:

    // stuff that HAS to be done, even in preview mode, goes here
    if (id::g_sphere)
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
static void vec_draw_line(double *v1, double *v2, int color)
{
    int x1 = (int) v1[0];
    int y1 = (int) v1[1];
    int x2 = (int) v2[0];
    int y2 = (int) v2[1];
    driver_draw_line(x1, y1, x2, y2, color);
}

static void corners(id::Matrix m, bool show, double *x_min, double *y_min, double *z_min, double *x_max, double *y_max, double *z_max)
{
    id::Vector s[2][4];              // Holds the top and bottom points, S[0][]=bottom

    /* define corners of box fractal is in x,y,z plane "b" stands for
     * "bottom" - these points are the corners of the screen in the x-y plane.
     * The "t"'s stand for Top - they are the top of the cube where 255 color
     * points hit. */
    *z_min = INT_MAX;
    *y_min = *z_min;
    *x_min = *y_min;
    *z_max = INT_MIN;
    *y_max = *z_max;
    *x_max = *y_max;

    for (int j = 0; j < 4; ++j)
    {
        for (int i = 0; i < 3; i++)
        {
            s[0][j][i] = 0;
            s[1][j][i] = 0;
        }
    }

    s[1][2][0] = g_logical_screen_x_dots - 1;
    s[1][1][0] = s[1][2][0];
    s[0][2][0] = s[1][1][0];
    s[0][1][0] = s[0][2][0];
    s[1][3][1] = g_logical_screen_y_dots - 1;
    s[1][2][1] = s[1][3][1];
    s[0][3][1] = s[1][2][1];
    s[0][2][1] = s[0][3][1];
    s[1][3][2] = s_z_coord - 1;
    s[1][2][2] = s[1][3][2];
    s[1][1][2] = s[1][2][2];
    s[1][0][2] = s[1][1][2];

    for (int i = 0; i < 4; ++i)
    {
        // transform points
        id::vec_mat_mul(s[0][i], m, s[0][i]);
        id::vec_mat_mul(s[1][i], m, s[1][i]);

        // update minimums and maximums
        *x_min = std::min(s[0][i][0], *x_min);
        *x_max = std::max(s[0][i][0], *x_max);
        *x_min = std::min(s[1][i][0], *x_min);
        *x_max = std::max(s[1][i][0], *x_max);
        *y_min = std::min(s[0][i][1], *y_min);
        *y_max = std::max(s[0][i][1], *y_max);
        *y_min = std::min(s[1][i][1], *y_min);
        *y_max = std::max(s[1][i][1], *y_max);
        *z_min = std::min(s[0][i][2], *z_min);
        *z_max = std::max(s[0][i][2], *z_max);
        *z_min = std::min(s[1][i][2], *z_min);
        *z_max = std::max(s[1][i][2], *z_max);
    }

    if (show)
    {
        if (s_persp)
        {
            for (int i = 0; i < 4; i++)
            {
                id::perspective(s[0][i]);
                id::perspective(s[1][i]);
            }
        }

        // Keep the box surrounding the fractal
        for (auto &elem : s)
        {
            for (id::Vector &elem_i : elem)
            {
                elem_i[0] += g_xx_adjust;
                elem_i[1] += g_yy_adjust;
            }
        }

        draw_rect(s[0][0], s[0][1], s[0][2], s[0][3], 2, true);      // Bottom

        draw_rect(s[0][0], s[1][0], s[0][1], s[1][1], 5, false);      // Sides
        draw_rect(s[0][2], s[1][2], s[0][3], s[1][3], 6, false);

        draw_rect(s[1][0], s[1][1], s[1][2], s[1][3], 8, true);      // Top
    }
}

/* This function draws a vector from origin[] to direct[] and a box
        around it. The vector and box are transformed or not depending on
        FILLTYPE.
*/

static void draw_light_box(double *origin, double *direct, id::Matrix light_m)
{
    id::Vector s[2][4]{};

    s[0][0][0] = origin[0];
    s[1][0][0] = s[0][0][0];
    s[0][0][1] = origin[1];
    s[1][0][1] = s[0][0][1];

    s[1][0][2] = direct[2];

    for (auto &elem : s)
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
    if (id::g_fill_type == id::FillType::LIGHT_SOURCE_AFTER)
    {
        for (int i = 0; i < 4; i++)
        {
            id::vec_mat_mul(s[0][i], light_m, s[0][i]);
            id::vec_mat_mul(s[1][i], light_m, s[1][i]);
        }
    }

    // always use perspective to aid viewing
    double temp = g_view[2];              // save perspective distance for a later restore
    g_view[2] = -s_p * 300.0 / 100.0;

    for (int i = 0; i < 4; i++)
    {
        id::perspective(s[0][i]);
        id::perspective(s[1][i]);
    }
    g_view[2] = temp;              // Restore perspective distance

    // Adjust for aspect
    for (int i = 0; i < 4; i++)
    {
        s[0][i][0] = s[0][i][0] * s_aspect;
        s[1][i][0] = s[1][i][0] * s_aspect;
    }

    // draw box connecting transformed points. NOTE order and COLORS
    draw_rect(s[0][0], s[0][1], s[0][2], s[0][3], 2, true);

    vec_draw_line(s[0][0], s[1][2], 8);

    // sides
    draw_rect(s[0][0], s[1][0], s[0][1], s[1][1], 4, false);
    draw_rect(s[0][2], s[1][2], s[0][3], s[1][3], 5, false);

    draw_rect(s[1][0], s[1][1], s[1][2], s[1][3], 3, true);

    // Draw the "arrow head"
    for (int i = -3; i < 4; i++)
    {
        for (int j = -3; j < 4; j++)
        {
            if (std::abs(i) + std::abs(j) < 6)
            {
                g_plot((int)(s[1][2][0] + i), (int)(s[1][2][1] + j), 10);
            }
        }
    }
}

static void draw_rect(id::Vector v0, id::Vector v1, id::Vector v2, id::Vector v3, int color, bool rect)
{
    id::Vector v[4];

    // Since V[2] is not used by vdraw_line don't bother setting it
    for (int i = 0; i < 2; i++)
    {
        v[0][i] = v0[i];
        v[1][i] = v1[i];
        v[2][i] = v2[i];
        v[3][i] = v3[i];
    }
    if (rect)                    // Draw a rectangle
    {
        for (int i = 0; i < 4; i++)
        {
            if (std::abs(v[i][0] - v[(i + 1) % 4][0]) < -2 * s_bad_check
                && std::abs(v[i][1] - v[(i + 1) % 4][1]) < -2 * s_bad_check)
            {
                vec_draw_line(v[i], v[(i + 1) % 4], color);
            }
        }
    }
    else
        // Draw 2 lines instead
    {
        for (int i = 0; i < 3; i += 2)
        {
            if (std::abs(v[i][0] - v[i + 1][0]) < -2 * s_bad_check
                && std::abs(v[i][1] - v[i + 1][1]) < -2 * s_bad_check)
            {
                vec_draw_line(v[i], v[i + 1], color);
            }
        }
    }
}

// replacement for plot - builds a table of min and max x's instead of plot
// called by draw_line as part of triangle fill routine
static void put_min_max(int x, int y, int /*color*/)
{
    if (y >= 0 && y < g_logical_screen_y_dots)
    {
        s_min_max_x[y].min_x = std::min(x, s_min_max_x[y].min_x);
        s_min_max_x[y].max_x = std::max(x, s_min_max_x[y].max_x);
    }
}

/*
        This routine fills in a triangle. Extreme left and right values for
        each row are calculated by calling the line function for the sides.
        Then rows are filled in with horizontal lines
*/
enum
{
    MAXOFFSCREEN = 2    // allow two of three points to be off-screen
};

static void put_triangle(PointColor pt1, PointColor pt2, PointColor pt3, int color)
{
    // Too many points off the screen?
    if ((off_screen(pt1) + off_screen(pt2) + off_screen(pt3)) > MAXOFFSCREEN)
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
            g_plot(s_p1.x, s_p1.y, color);
        }
        else
        {
            driver_draw_line(s_p1.x, s_p1.y, s_p3.x, s_p3.y, color);
        }
        g_plot = s_normal_plot;
        return;
    }
    if ((s_p3.y == s_p1.y && s_p3.x == s_p1.x) || (s_p3.y == s_p2.y && s_p3.x == s_p2.x))
    {
        g_plot = s_fill_plot;
        driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, color);
        g_plot = s_normal_plot;
        return;
    }

    // find min max y
    int maxy = s_p1.y;
    int miny = maxy;
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
    miny = std::max(miny, 0);
    if (maxy >= g_logical_screen_y_dots)
    {
        maxy = g_logical_screen_y_dots - 1;
    }

    for (int y = miny; y <= maxy; y++)
    {
        s_min_max_x[y].min_x = INT_MAX;
        s_min_max_x[y].max_x = INT_MIN;
    }

    // set plot to "fake" plot function
    g_plot = put_min_max;

    // build table of extreme x's of triangle
    driver_draw_line(s_p1.x, s_p1.y, s_p2.x, s_p2.y, 0);
    driver_draw_line(s_p2.x, s_p2.y, s_p3.x, s_p3.y, 0);
    driver_draw_line(s_p3.x, s_p3.y, s_p1.x, s_p1.y, 0);

    for (int y = miny; y <= maxy; y++)
    {
        int x_lim = s_min_max_x[y].max_x;
        for (int x = s_min_max_x[y].min_x; x <= x_lim; x++)
        {
            s_fill_plot(x, y, color);
        }
    }
    g_plot = s_normal_plot;
}

static int off_screen(PointColor pt)
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

static void clip_color(int x, int y, int color)
{
    if (0 <= x && x < g_logical_screen_x_dots
        && 0 <= y && y < g_logical_screen_y_dots
        && 0 <= color && color < g_file_colors)
    {
        g_standard_plot(x, y, color);

        if (g_targa_out)
        {
            // standardplot modifies color in these types
            if (!glasses_alternating_or_superimpose())
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

static void transparent_clip_color(int x, int y, int color)
{
    if (0 <= x && x < g_logical_screen_x_dots       // is the point on screen?
        && 0 <= y && y < g_logical_screen_y_dots    // Yes?
        && 0 <= color && color < g_colors           // Colors in valid range?
        // Let's make sure it's not a transparent color
        && (g_transparent_color_3d[0] > color || color > g_transparent_color_3d[1]))
    {
        g_standard_plot(x, y, color);// I guess we can plot then
        if (g_targa_out)
        {
            // standardplot modifies color in these types
            if (!glasses_alternating_or_superimpose())
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

static void interp_color(int x, int y, int color)
{
    /* this distance formula is not the usual one - but it has the virtue that
     * it uses ONLY additions (almost) and it DOES go to zero as the points
     * get close. */

    int d1 = std::abs(s_p1.x - x) + std::abs(s_p1.y - y);
    int d2 = std::abs(s_p2.x - x) + std::abs(s_p2.y - y);
    int d3 = std::abs(s_p3.x - x) + std::abs(s_p3.y - y);

    int d = (d1 + d2 + d3) << 1;
    if (d)
    {
        /* calculate a weighted average of colors long casts prevent integer
           overflow. This can evaluate to zero */
        color = (int)(((long)(d2 + d3) * (long) s_p1.color +
                       (long)(d1 + d3) * (long) s_p2.color +
                       (long)(d1 + d2) * (long) s_p3.color) / d);
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
            if (!glasses_alternating_or_superimpose())
            {
                d = targa_color(x, y, color);
            }
        }

        if (id::g_fill_type >= id::FillType::LIGHT_SOURCE_BEFORE)
        {
            if (s_real_v && g_targa_out)
            {
                color = d;
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
        In non-light source modes, both color and Real_Color contain the
        actual pixel color. In light source modes, color contains the
        light value, and Real_Color contains the original color

        This routine takes a pixel modifies it for light shading if appropriate
        and plots it in a Targa file. Used in plot3d.c
*/

int targa_color(int x, int y, int color)
{
    unsigned long hue;
    unsigned long sat;
    unsigned long val;
    Byte rgb[3];

    if (id::g_fill_type == id::FillType::SURFACE_INTERPOLATED || glasses_alternating_or_superimpose() || g_true_color)
    {
        s_real_color = (Byte)color;       // So Targa gets interpolated color
    }

    switch (g_true_mode)
    {
    case TrueColorMode::DEFAULT_COLOR:
    default:
        rgb[0] = g_dac_box[s_real_color][0]; // Move color space to
        rgb[1] = g_dac_box[s_real_color][1]; // 256 color primaries
        rgb[2] = g_dac_box[s_real_color][2]; // from 64 colors
        break;

    case TrueColorMode::ITERATE:
        rgb[0] = (Byte)((g_real_color_iter >> 16) & 0xff);  // red
        rgb[1] = (Byte)((g_real_color_iter >> 8) & 0xff);   // green
        rgb[2] = (Byte)((g_real_color_iter) & 0xff);        // blue
        break;
    }

    // Now lets convert it to HSV
    rgb_to_hsv(rgb[0], rgb[1], rgb[2], &hue, &sat, &val);

    // Modify original S and V components
    if (id::g_fill_type > id::FillType::SOLID_FILL && !glasses_alternating_or_superimpose())
    {
        // Adjust for Ambient
        val = (val * (65535L - color * s_i_ambient)) / 65535L;
    }

    if (g_haze)
    {
        // Haze lowers sat of colors
        sat = sat * s_haze_mult / 100;
        if (val >= 32640)           // Haze reduces contrast
        {
            val = val - 32640;
            val = val * s_haze_mult / 100;
            val = val + 32640;
        }
        else
        {
            val = 32640 - val;
            val = val * s_haze_mult / 100;
            val = 32640 - val;
        }
    }
    // Now lets convert it back to RGB. Original Hue, modified Sat and Val
    hsv_to_rgb(&rgb[0], &rgb[1], &rgb[2], hue, sat, val);

    if (s_real_v)
    {
        val = (35 * (int) rgb[0] + 45 * (int) rgb[1] + 20 * (int) rgb[2]) / 100;
    }

    // Now write the color triple to its transformed location
    // on the disk.
    targa_write_disk(x + g_logical_screen_x_offset, y + g_logical_screen_y_offset, rgb[0], rgb[1], rgb[2]);

    return (int)(255 - val);
}

static bool set_pixel_buff(Byte *pixels, Byte *fraction, unsigned line_len)
{
    if ((s_even_odd_row++ & 1) == 0) // even rows are color value
    {
        for (int i = 0; i < (int) line_len; i++) // add the fractional part in odd row
        {
            fraction[i] = pixels[i];
        }
        return true;
    }
    // swap
    for (int i = 0; i < (int) line_len; i++) // swap so pixel has color
    {
        Byte tmp = pixels[i];
        pixels[i] = fraction[i];
        fraction[i] = tmp;
    }
    return false;
}

/**************************************************************************

  Common routine for printing error messages to the screen for Targa
    and other files

**************************************************************************/

static void file_error(const std::string &filename, FileError error)
{
    std::string msg;
    s_error = error;
    switch (error)
    {
    case FileError::NONE:
        return;

    case FileError::OPEN_FAILED:        // Can't Open
        msg = "OOPS, couldn't open  < " + filename + " >";
        break;

    case FileError::DISK_FULL:          // Not enough room
        msg = "OOPS, ran out of disk space. < " + filename + " >";
        break;

    case FileError::BAD_IMAGE_SIZE:     // Image wrong size
        msg = "OOPS, image wrong size";
        break;

    case FileError::BAD_FILE_TYPE:      // Wrong file type
        msg = "OOPS, can't handle this type of file.";
        break;
    }
    stop_msg(msg);
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

static bool start_targa_overlay(const std::string &path, std::FILE *source, bool overlay)
{
    // Open File for both reading and writing
    std::FILE *fps = std::fopen(path.c_str(), "w+b");
    if (fps == nullptr)
    {
        file_error(path, FileError::OPEN_FAILED);
        return true;            // Oops, something's wrong!
    }

    int inc = 1;                // Assume we are overlaying a file

    // Write the header
    if (overlay)                   // We are overlaying a file
    {
        for (int i = 0; i < s_targa_header_24; i++)   // Copy the header from the Source
        {
            std::fputc(std::fgetc(source), fps);
        }
    }
    else
    {
        // Write header for a new file
        // ID field size = 0, No color map, Targa type 2 file
        for (int i = 0; i < 12; i++)
        {
            if (i == 0 && g_true_color)
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
        for (Byte &elem : s_upr_lwr)
        {
            std::fputc(elem, fps);
        }
        std::fputc(s_t24, fps);          // Targa 24 file
        std::fputc(s_t32, fps);          // Image at upper left
        inc = 3;
    }

    if (g_true_color) // write maxit
    {
        std::fputc((Byte)(g_max_iterations       & 0xff), fps);
        std::fputc((Byte)((g_max_iterations >> 8) & 0xff), fps);
        std::fputc((Byte)((g_max_iterations >> 16) & 0xff), fps);
        std::fputc((Byte)((g_max_iterations >> 24) & 0xff), fps);
    }

    // Finished with the header, now lets work on the display area
    for (int i = 0; i < g_logical_screen_y_dots; i++)  // "clear the screen" (write to the disk)
    {
        for (int j = 0; j < s_line_length1; j = j + inc)
        {
            if (overlay)
            {
                std::fputc(std::fgetc(source), fps);
            }
            else
            {
                for (int k = 2; k > -1; k--)
                {
                    std::fputc(g_background_color[k], fps);       // Targa order (B, G, R)
                }
            }
        }
        if (std::ferror(fps))
        {
            // Almost certainly not enough disk space
            std::fclose(fps);
            if (overlay)
            {
                std::fclose(source);
            }
            std::error_code ec;
            std::filesystem::remove(path, ec);
            file_error(path, FileError::DISK_FULL);
            return true;
        }
    }

    if (targa_start_disk(fps, s_targa_header_24) != 0)
    {
        end_disk();
        std::error_code ec;
        std::filesystem::remove(path, ec);
        return true;
    }
    return false;
}

bool start_targa_overlay(const std::string &path, std::FILE *source)
{
    return start_targa_overlay(path, source, true);
}

bool start_targa(const std::string &path)
{
    return start_targa_overlay(path, nullptr, false);
}

static bool targa_validate(const std::string &filename)
{
    // Attempt to open source file for reading
    std::filesystem::path path{id::io::find_file(id::io::ReadFile::IMAGE, filename)};
    if (path.empty())
    {
        file_error(filename, FileError::OPEN_FAILED);
        return true;              // Oops, file does not exist
    }
    std::FILE *fp = std::fopen(path.string().c_str(), "rb");
    if (fp == nullptr)
    {
        file_error(filename, FileError::OPEN_FAILED);
        return true;              // Oops, file does not exist
    }

    s_targa_header_24 += std::fgetc(fp);    // Check ID field and adjust header size

    if (std::fgetc(fp))               // Make sure this is an unmapped file
    {
        file_error(filename, FileError::BAD_FILE_TYPE);
        return true;
    }

    if (std::fgetc(fp) != 2)          // Make sure it is a type 2 file
    {
        file_error(filename, FileError::BAD_IMAGE_SIZE);
        return true;
    }

    // Skip color map specification
    for (int i = 0; i < 5; i++)
    {
        std::fgetc(fp);
    }

    for (int i = 0; i < 4; i++)
    {
        // Check image origin
        std::fgetc(fp);
    }
    // Check Image specs
    for (Byte &elem : s_upr_lwr)
    {
        if (std::fgetc(fp) != (int) elem)
        {
            file_error(filename, FileError::BAD_IMAGE_SIZE);
            return true;
        }
    }

    if (std::fgetc(fp) != (int) s_t24)
    {
        s_error = FileError::BAD_FILE_TYPE; // Is it a targa 24 file?
    }
    if (std::fgetc(fp) != (int) s_t32)
    {
        s_error = FileError::BAD_FILE_TYPE;                // Is the origin at the upper left?
    }
    if (s_error == FileError::BAD_FILE_TYPE)
    {
        file_error(filename, FileError::BAD_FILE_TYPE);
        return true;
    }
    std::fseek(fp, 0, SEEK_SET);

    // Now that we know it's a good file, create a working copy
    std::filesystem::path temp_path{id::io::get_save_path(id::io::WriteFile::IMAGE, s_targa_temp)};
    assert(!temp_path.empty());
    if (start_targa_overlay(temp_path.string(), fp))
    {
        return true;
    }

    std::fclose(fp);                  // Close the source

    s_temp_safe = true;               // Original file successfully copied to targa_temp
    return false;
}

static int rgb_to_hsv(
    Byte red, Byte green, Byte blue, unsigned long *hue, unsigned long *sat, unsigned long *val)
{
    *val = red;
    Byte min = green;
    if (red < green)
    {
        *val = green;
        min = red;
        if (green < blue)
        {
            *val = blue;
        }
        if (blue < red)
        {
            min = blue;
        }
    }
    else
    {
        if (blue < green)
        {
            min = blue;
        }
        if (red < blue)
        {
            *val = blue;
        }
    }
    unsigned long denom = *val - min;
    if (*val != 0 && denom != 0)
    {
        *sat = ((denom << 16) / *val) - 1;
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
    if (*val == min)
    {
        *hue = 0;
        *val = *val << 8;
        return 0;
    }
    unsigned long r1 = (((*val - red) * 60) << 6) / denom; // distance of color from red
    unsigned long g1 = (((*val - green) * 60) << 6) / denom; // distance of color from green
    unsigned long b1 = (((*val - blue) * 60) << 6) / denom; // distance of color from blue
    if (*val == red)
    {
        if (min == green)
        {
            *hue = (300 << 6) + b1;
        }
        else
        {
            *hue = (60 << 6) - g1;
        }
    }
    if (*val == green)
    {
        if (min == blue)
        {
            *hue = (60 << 6) + r1;
        }
        else
        {
            *hue = (180 << 6) - b1;
        }
    }
    if (*val == blue)
    {
        if (min == red)
        {
            *hue = (180 << 6) + g1;
        }
        else
        {
            *hue = (300 << 6) - r1;
        }
    }
    *val = *val << 8;
    return 0;
}

static void hsv_to_rgb(
    Byte *red, Byte *green, Byte *blue, unsigned long hue, unsigned long sat, unsigned long val)
{
    if (hue >= 23040)
    {
        hue = hue % 23040;            // Makes h circular
    }
    int i = (int) (hue / 3840);
    int rmd = (int) (hue % 3840);       // RMD = fractional part of H

    unsigned long p1 = ((val * (65535L - sat)) / 65280L) >> 8;
    unsigned long p2 = (((val * (65535L - (sat * rmd) / 3840)) / 65280L) - 1) >> 8;
    unsigned long p3 = (((val * (65535L - (sat * (3840 - rmd)) / 3840)) / 65280L)) >> 8;
    val = val >> 8;
    switch (i)
    {
    case 0:
        *red = (Byte) val;
        *green = (Byte) p3;
        *blue = (Byte) p1;
        break;
    case 1:
        *red = (Byte) p2;
        *green = (Byte) val;
        *blue = (Byte) p1;
        break;
    case 2:
        *red = (Byte) p1;
        *green = (Byte) val;
        *blue = (Byte) p3;
        break;
    case 3:
        *red = (Byte) p1;
        *green = (Byte) p2;
        *blue = (Byte) val;
        break;
    case 4:
        *red = (Byte) p3;
        *green = (Byte) p1;
        *blue = (Byte) val;
        break;
    case 5:
        *red = (Byte) val;
        *green = (Byte) p1;
        *blue = (Byte) p2;
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

constexpr const char *DXF_HEADER{R"(  0
SECTION
  2
TABLES
  0
TABLE
  2
LAYER
 70
     2
  0
LAYER
  2
0
 70
     0
 62
     7
  6
CONTINUOUS
  0
LAYER
  2
FRACTAL
 70
    64
 62
     1
  6
CONTINUOUS
  0
ENDTAB
  0
ENDSEC
  0
SECTION
  2
ENTITIES
)"};

//******************************************************************
//
//  This routine writes a header to a ray tracer data file. It
//  Identifies the version of Iterated Dynamics which created it and the
//  key 3D parameters in effect at the time.
//
//******************************************************************

static int ray_header()
{
    // Open the ray tracing output file
    std::string path{id::io::get_save_path(id::io::WriteFile::RAYTRACE, g_raytrace_filename).string()};
    if (path.empty())
    {
        return -1;              // Oops, something's wrong!
    }
    check_write_file(path, ".ray");
    s_raytrace_file = std::fopen(path.c_str(), "w");
    if (s_raytrace_file == nullptr)
    {
        return -1;              // Oops, something's wrong!
    }

    if (g_raytrace_format == RayTraceFormat::VIVID)
    {
        fmt::print(s_raytrace_file, "//");
    }
    if (g_raytrace_format == RayTraceFormat::MTV)
    {
        fmt::print(s_raytrace_file, "#");
    }
    if (g_raytrace_format == RayTraceFormat::RAYSHADE)
    {
        fmt::print(s_raytrace_file, "/*\n");
    }
    if (g_raytrace_format == RayTraceFormat::ACROSPIN)
    {
        fmt::print(s_raytrace_file, "--");
    }
    if (g_raytrace_format == RayTraceFormat::DXF)
    {
        fmt::print(s_raytrace_file, DXF_HEADER);
    }

    if (g_raytrace_format != RayTraceFormat::DXF)
    {
        fmt::print(s_raytrace_file, "{{ Created by " ID_PROGRAM_NAME " Ver. {:s} }}\n\n", to_string(g_version));
    }

    if (g_raytrace_format == RayTraceFormat::RAYSHADE)
    {
        fmt::print(s_raytrace_file, "*/\n");
    }

    // Set the default color
    if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
    {
        fmt::print(s_raytrace_file, "DECLARE       F_Dflt = COLOR  RED 0.8 GREEN 0.4 BLUE 0.1\n");
    }
    if (g_brief)
    {
        if (g_raytrace_format == RayTraceFormat::VIVID)
        {
            fmt::print(s_raytrace_file, "surf={{diff=0.8 0.4 0.1;}}\n");
        }
        if (g_raytrace_format == RayTraceFormat::MTV)
        {
            fmt::print(s_raytrace_file, "f 0.8 0.4 0.1 0.95 0.05 5 0 0\n");
        }
        if (g_raytrace_format == RayTraceFormat::RAYSHADE)
        {
            fmt::print(s_raytrace_file, "applysurf diffuse 0.8 0.4 0.1");
        }
    }
    if (g_raytrace_format != RayTraceFormat::DXF)
    {
        fmt::print(s_raytrace_file, "\n");
    }

    // open "grid" opject, a speedy way to do aggregates in rayshade
    if (g_raytrace_format == RayTraceFormat::RAYSHADE)
    {
        fmt::print(s_raytrace_file,
                "/* make a gridded aggregate. this size grid is fast for landscapes. */\n"
                "/* make z grid = 1 always for landscapes. */\n\n"
                "grid 33 25 1\n");
    }

    if (g_raytrace_format == RayTraceFormat::ACROSPIN)
    {
        fmt::print(s_raytrace_file,
            "Set Layer 1\n"
            "Set Color 2\n"
            "EndpointList X Y Z Name\n");
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

static int out_triangle(FPointColor pt1, FPointColor pt2, FPointColor pt3, int c1, int c2, int c3)
{
    float c[3];
    float pt_t[3][3];

    // Normalize each vertex to screen size and adjust coordinate system
    pt_t[0][0] = 2 * pt1.x / g_logical_screen_x_dots - 1;
    pt_t[0][1] = 2 * pt1.y / g_logical_screen_y_dots - 1;
    pt_t[0][2] = -2.0f * pt1.color / g_num_colors - 1;
    pt_t[1][0] = 2 * pt2.x / g_logical_screen_x_dots - 1;
    pt_t[1][1] = 2 * pt2.y / g_logical_screen_y_dots - 1;
    pt_t[1][2] = -2.0f * pt2.color / g_num_colors - 1;
    pt_t[2][0] = 2 * pt3.x / g_logical_screen_x_dots - 1;
    pt_t[2][1] = 2 * pt3.y / g_logical_screen_y_dots - 1;
    pt_t[2][2] = -2.0f * pt3.color / g_num_colors - 1;

    // Color of triangle is average of colors of its vertices
    if (!g_brief)
    {
        for (int i = 0; i < 3; i++)
        {
            c[i] = static_cast<float>(g_dac_box[c1][i] + g_dac_box[c2][i] + g_dac_box[c3][i]) / (3.0F * 255.0F);
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
    if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
    {
        fmt::print(s_raytrace_file,
            " OBJECT\n"
            "  TRIANGLE ");
    }
    if (g_raytrace_format == RayTraceFormat::VIVID && !g_brief)
    {
        fmt::print(s_raytrace_file, "surf={{diff=");
    }
    if (g_raytrace_format == RayTraceFormat::MTV && !g_brief)
    {
        fmt::print(s_raytrace_file, "f");
    }
    if (g_raytrace_format == RayTraceFormat::RAYSHADE && !g_brief)
    {
        fmt::print(s_raytrace_file, "applysurf diffuse ");
    }

    if (!g_brief && g_raytrace_format != RayTraceFormat::DKB_POVRAY && g_raytrace_format != RayTraceFormat::DXF)
    {
        for (int i = 0; i <= 2; i++)
        {
            fmt::print(s_raytrace_file, "{: 4.4f} ", c[i]);
        }
    }

    if (g_raytrace_format == RayTraceFormat::VIVID)
    {
        if (!g_brief)
        {
            fmt::print(s_raytrace_file, ";}}\n");
        }
        fmt::print(s_raytrace_file, "polygon={{points=3;");
    }
    if (g_raytrace_format == RayTraceFormat::MTV)
    {
        if (!g_brief)
        {
            fmt::print(s_raytrace_file, "0.95 0.05 5 0 0\n");
        }
        fmt::print(s_raytrace_file, "p 3");
    }
    if (g_raytrace_format == RayTraceFormat::RAYSHADE)
    {
        if (!g_brief)
        {
            fmt::print(s_raytrace_file, "\n");
        }
        fmt::print(s_raytrace_file, "triangle");
    }

    if (g_raytrace_format == RayTraceFormat::DXF)
    {
        fmt::print(s_raytrace_file,
            "  0\n"
            "3DFACE\n"
            "  8\n"
            "FRACTAL\n"
            " 62\n"
            "{:3d}\n",
            std::min(255, std::max(1, c1)));
    }

    for (int i = 0; i <= 2; i++)     // Describe each  Vertex
    {
        if (g_raytrace_format != RayTraceFormat::DXF)
        {
            fmt::print(s_raytrace_file, "\n");
        }

        if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
        {
            fmt::print(s_raytrace_file, "      <");
        }
        if (g_raytrace_format == RayTraceFormat::VIVID)
        {
            fmt::print(s_raytrace_file, " vertex =  ");
        }
        if (g_raytrace_format > RayTraceFormat::RAW && g_raytrace_format != RayTraceFormat::DXF)
        {
            fmt::print(s_raytrace_file, " ");
        }

        for (int j = 0; j <= 2; j++)
        {
            if (g_raytrace_format == RayTraceFormat::DXF)
            {
                // write 3dface entity to dxf file
                fmt::print(s_raytrace_file,
                    "{:3d}\n"
                    "{:g}\n",
                    10 * (j + 1) + i, //
                    pt_t[i][j]);
                if (i == 2)           // 3dface needs 4 vertecies
                {
                    fmt::print(s_raytrace_file,
                        "{:3d}\n"
                        "{:g}\n",
                        10 * (j + 1) + i + 1, //
                        pt_t[i][j]);
                }
            }
            else if (!(g_raytrace_format == RayTraceFormat::MTV || g_raytrace_format == RayTraceFormat::RAYSHADE))
            {
                fmt::print(s_raytrace_file, "{: 4.4f} ", pt_t[i][j]); // Right-handed
            }
            else
            {
                fmt::print(s_raytrace_file, "{: 4.4f} ", pt_t[2 - i][j]);     // Left-handed
            }
        }

        if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
        {
            fmt::print(s_raytrace_file, ">");
        }
        if (g_raytrace_format == RayTraceFormat::VIVID)
        {
            fmt::print(s_raytrace_file, ";");
        }
    }

    if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
    {
        fmt::print(s_raytrace_file, " END_TRIANGLE\n");
        if (!g_brief)
        {
            fmt::print(s_raytrace_file,
                    "  TEXTURE\n"
                    "   COLOR  RED {:4.4f} GREEN {:4.4f} BLUE {:4.4f}\n"
                    "      AMBIENT 0.25 DIFFUSE 0.75 END_TEXTURE\n",
                    c[0], c[1], c[2]);
        }
        fmt::print(s_raytrace_file, "  COLOR  F_Dflt  END_OBJECT");
        triangle_bounds(pt_t);    // update bounding info
    }
    if (g_raytrace_format == RayTraceFormat::VIVID)
    {
        fmt::print(s_raytrace_file, "}}");
    }
    if (g_raytrace_format == RayTraceFormat::RAW && !g_brief)
    {
        fmt::print(s_raytrace_file, "\n");
    }

    if (g_raytrace_format != RayTraceFormat::DXF)
    {
        fmt::print(s_raytrace_file, "\n");
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
            s_min_xyz[j] = std::min(pt_t[i][j], s_min_xyz[j]);
            s_max_xyz[j] = std::max(pt_t[i][j], s_max_xyz[j]);
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
    if (g_raytrace_format != RayTraceFormat::DKB_POVRAY)
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

    fmt::print(s_raytrace_file, "COMPOSITE\n");
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

static int end_object(bool tri_out)
{
    if (g_raytrace_format == RayTraceFormat::DXF)
    {
        return 0;
    }
    if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
    {
        if (tri_out)
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
            fmt::print(s_raytrace_file,
                " BOUNDED_BY\n"
                "  INTERSECTION\n"
                "   PLANE <-1.0  0.0  0.0 > {:4.3f} END_PLANE\n"
                "   PLANE < 1.0  0.0  0.0 > {:4.3f} END_PLANE\n"
                "   PLANE < 0.0 -1.0  0.0 > {:4.3f} END_PLANE\n"
                "   PLANE < 0.0  1.0  0.0 > {:4.3f} END_PLANE\n"
                "   PLANE < 0.0  0.0 -1.0 > {:4.3f} END_PLANE\n"
                "   PLANE < 0.0  0.0  1.0 > {:4.3f} END_PLANE\n"
                "  END_INTERSECTION\n"
                " END_BOUND\n",
                -s_min_xyz[0], //
                s_max_xyz[0],  //
                -s_min_xyz[1], //
                s_max_xyz[1],  //
                -s_min_xyz[2], //
                s_max_xyz[2]);
        }

        // Complete the composite object statement
        fmt::print(s_raytrace_file, "END_COMPOSITE\n");
    }

    if (g_raytrace_format != RayTraceFormat::ACROSPIN && g_raytrace_format != RayTraceFormat::RAYSHADE)
    {
        fmt::print(s_raytrace_file, "\n");
    }

    return 0;
}

static void line3d_cleanup()
{
    if (g_raytrace_format != RayTraceFormat::NONE && s_raytrace_file)
    {
        // Finish up the ray tracing files
        if (g_raytrace_format != RayTraceFormat::RAYSHADE && g_raytrace_format != RayTraceFormat::DXF)
        {
            fmt::print(s_raytrace_file, "\n");
        }
        if (g_raytrace_format == RayTraceFormat::VIVID)
        {
            fmt::print(s_raytrace_file,
                "\n"
                "\n"
                "//");
        }
        if (g_raytrace_format == RayTraceFormat::MTV)
        {
            fmt::print(s_raytrace_file,
                "\n"
                "\n"
                "#");
        }

        if (g_raytrace_format == RayTraceFormat::RAYSHADE)
        {
            // end grid aggregate
            fmt::print(s_raytrace_file,
                "end\n"
                "\n"
                "/*good landscape:*/\n"
                "background .3 0 0\n"
                "report verbose\n"
                "\n"
                "/*"
                "screen 640 480\n"
                "eyep 0 2.1 0.8\n"
                "lookp 0 0 -0.95\n"
                "light 1 point -2 1 1.5\n");
        }
        if (g_raytrace_format == RayTraceFormat::ACROSPIN)
        {
            fmt::print(s_raytrace_file, "LineList From To\n");
            for (int i = 0; i < s_ro; i++)
            {
                for (int j = 0; j <= s_co_max; j++)
                {
                    if (j < s_co_max)
                    {
                        fmt::print(s_raytrace_file, "R{:d}C{:d} R{:d}C{:d}\n", i, j, i, j + 1);
                    }
                    if (i < s_ro - 1)
                    {
                        fmt::print(s_raytrace_file, "R{:d}C{:d} R{:d}C{:d}\n", i, j, i + 1, j);
                    }
                    if (i && i < s_ro && j < s_co_max)
                    {
                        fmt::print(s_raytrace_file, "R{:d}C{:d} R{:d}C{:d}\n", i, j, i - 1, j + 1);
                    }
                }
            }
            fmt::print(s_raytrace_file,
                "\n"
                "\n"
                "--");
        }
        if (g_raytrace_format != RayTraceFormat::DXF)
        {
            fmt::print(s_raytrace_file,
                "{{ No. Of Triangles = {:d} }}*/\n"
                "\n",
                s_num_tris);
        }
        if (g_raytrace_format == RayTraceFormat::DXF)
        {
            fmt::print(s_raytrace_file,
                "  0\n"
                "ENDSEC\n"
                "  0\n"
                "EOF\n");
        }
        std::fclose(s_raytrace_file);
        s_raytrace_file = nullptr;
    }
    if (g_targa_out)
    {
        // Finish up targa files
        s_targa_header_24 = 18;         // Reset Targa header size
        end_disk();
        if (g_debug_flag == DebugFlags::NONE && (!s_temp_safe || s_error != FileError::NONE) && g_targa_overlay)
        {
            std::filesystem::path light_path{id::io::get_save_path(id::io::WriteFile::IMAGE, g_light_name)};
            assert(!light_path.empty());
            std::filesystem::remove(light_path);
            std::filesystem::path temp_path{id::io::get_save_path(id::io::WriteFile::IMAGE, s_targa_temp)};
            assert(!temp_path.empty());
            std::filesystem::rename(temp_path, light_path);
        }
        if (g_debug_flag == DebugFlags::NONE && g_targa_overlay)
        {
            std::filesystem::path temp_path{id::io::get_save_path(id::io::WriteFile::IMAGE, s_targa_temp)};
            assert(!temp_path.empty());
            std::filesystem::remove(temp_path);
        }
    }
    s_error = FileError::NONE;
    s_temp_safe = false;
}

static void set_upr_lwr()
{
    s_upr_lwr[0] = (Byte)(g_logical_screen_x_dots & 0xff);
    s_upr_lwr[1] = (Byte)(g_logical_screen_x_dots >> 8);
    s_upr_lwr[2] = (Byte)(g_logical_screen_y_dots & 0xff);
    s_upr_lwr[3] = (Byte)(g_logical_screen_y_dots >> 8);
    s_line_length1 = 3 * g_logical_screen_x_dots;    // line length @ 3 bytes per pixel
}

static int first_time(int line_len, id::Vector v)
{
    id::Matrix light_mat;               // m w/no trans, keeps obj. on screen
                   // rotation values
    // corners of transformed xdotx by ydots x colors box
    double x_min;
    double y_min;
    double z_min;
    double x_max;
    double y_max;
    double z_max;
    id::Vector direct;
    // current,start,stop latitude
    // current start,stop longitude
    // increment of latitude
    g_out_line_cleanup = line3d_cleanup;

    s_even_odd_row = 0;
    g_calc_time = 0;
    // mark as in-progress, and enable <tab> timer display
    g_calc_status = CalcStatus::IN_PROGRESS;

    s_i_ambient = (unsigned int)(255 * (float)(100 - g_ambient) / 100.0);
    s_i_ambient = std::max<unsigned int>(s_i_ambient, 1);

    s_num_tris = 0;

    // Open file for RAY trace output and write header
    if (g_raytrace_format != RayTraceFormat::NONE)
    {
        ray_header();
        g_yy_adjust = 0;
        g_xx_adjust = 0;  // Disable shifting in ray tracing
        g_y_shift = 0;
        g_x_shift = 0;
    }

    s_ro = 0;
    s_co = 0;
    s_co_max = 0;

    set_upr_lwr();
    s_error = FileError::NONE;

    if (g_which_image < StereoImage::BLUE)
    {
        s_temp_safe = false; // Not safe yet to mess with the source image
    }

    if (g_targa_out && !(glasses_alternating_or_superimpose() && g_which_image == StereoImage::BLUE))
    {
        if (g_targa_overlay)
        {
            // Make sure target file is a supportable Targa File
            if (targa_validate(g_light_name))
            {
                return -1;
            }
        }
        else
        {
            std::string path{id::io::get_save_path(id::io::WriteFile::IMAGE, g_light_name).string()};
            assert(!path.empty());
            check_write_file(path, ".tga");
            if (start_targa(path))     // Open new file
            {
                return -1;
            }
        }
    }

    s_rand_factor = 14 - g_randomize_3d;

    s_z_coord = g_file_colors;

    int err = line3d_mem();
    if (err != 0)
    {
        return err;
    }

    // get scale factors
    s_scale_x = id::g_x_scale / 100.0;
    s_scale_y = id::g_y_scale / 100.0;
    if (id::g_rough)
    {
        s_scale_z = -id::g_rough / 100.0;
    }
    else
    {
        s_scale_z = -0.0001;
        s_r_scale = s_scale_z;  // if rough=0 make it very flat but plot something
    }

    // aspect ratio calculation - assume screen is 4 x 3
    s_aspect = (double) g_logical_screen_x_dots *.75 / (double) g_logical_screen_y_dots;

    if (!id::g_sphere)         // skip this slow stuff in sphere case
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
        id::identity(g_m);
        id::identity(light_mat);

        // translate so origin is in center of box, so that when we rotate
        // it, we do so through the center
        id::trans((double) g_logical_screen_x_dots / (-2.0), (double) g_logical_screen_y_dots / (-2.0),
              (double) s_z_coord / (-2.0), g_m);
        id::trans((double) g_logical_screen_x_dots / (-2.0), (double) g_logical_screen_y_dots / (-2.0),
              (double) s_z_coord / (-2.0), light_mat);

        // apply scale factors
        id::scale(s_scale_x, s_scale_y, s_scale_z, g_m);
        id::scale(s_scale_x, s_scale_y, s_scale_z, light_mat);

        // rotation values - converting from degrees to radians
        double x_val = id::g_x_rot / 57.29577;
        double y_val = id::g_y_rot / 57.29577;
        double z_val = id::g_z_rot / 57.29577;

        if (g_raytrace_format != RayTraceFormat::NONE)
        {
            x_val = 0;
            y_val = 0;
            z_val = 0;
        }

        id::x_rot(x_val, g_m);
        id::x_rot(x_val, light_mat);
        id::y_rot(y_val, g_m);
        id::y_rot(y_val, light_mat);
        id::z_rot(z_val, g_m);
        id::z_rot(z_val, light_mat);

        // Find values of translation that make all x,y,z negative
        // m current matrix
        // 0 means don't show box
        // returns minimum and maximum values of x,y,z in fractal
        corners(g_m, false, &x_min, &y_min, &z_min, &x_max, &y_max, &z_max);
    }

    // perspective 3D vector - lview[2] == 0 means no perspective

    // set perspective flag
    s_persp = false;
    if (id::g_viewer_z != 0)
    {
        s_persp = true;
    }

    // set up view vector, and put viewer in center of screen
    s_l_view[0] = g_logical_screen_x_dots >> 1;
    s_l_view[1] = g_logical_screen_y_dots >> 1;

    // z value of user's eye - should be more negative than extreme negative part of image
    if (id::g_sphere)                    // sphere case
    {
        s_l_view[2] = -(long)((double) g_logical_screen_y_dots * (double) id::g_viewer_z / 100.0);
    }
    else                             // non-sphere case
    {
        s_l_view[2] = (long)((z_min - z_max) * (double) id::g_viewer_z / 100.0);
    }

    g_view[0] = s_l_view[0];
    g_view[1] = s_l_view[1];
    g_view[2] = s_l_view[2];
    s_l_view[0] = s_l_view[0] << 16;
    s_l_view[1] = s_l_view[1] << 16;
    s_l_view[2] = s_l_view[2] << 16;

    if (!id::g_sphere)         // sphere skips this
    {
        /* translate back exactly amount we translated earlier plus enough to
         * center image so maximum values are non-positive */
        id::trans(((double) g_logical_screen_x_dots - x_max - x_min) / 2,
              ((double) g_logical_screen_y_dots - y_max - y_min) / 2, -z_max, g_m);

        // Keep the box centered and on screen regardless of shifts
        id::trans(((double) g_logical_screen_x_dots - x_max - x_min) / 2,
              ((double) g_logical_screen_y_dots - y_max - y_min) / 2, -z_max, light_mat);

        id::trans(g_x_shift, -g_y_shift, 0.0, g_m);

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
        float theta1 = (float) (id::g_sphere_theta_min * PI / 180.0);
        float theta2 = (float) (id::g_sphere_theta_max * PI / 180.0);

        // Map Y to this LONGITUDE range
        float phi1 = (float) (id::g_sphere_phi_min * PI / 180.0);
        float phi2 = (float) (id::g_sphere_phi_max * PI / 180.0);

        float theta = theta1;

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

        float delta_theta = (theta2 - theta1) / (float) line_len;

        // initial sin,cos theta
        s_sin_theta_array[0] = std::sin(theta);
        s_cos_theta_array[0] = std::cos(theta);
        s_sin_theta_array[1] = std::sin(theta + delta_theta);
        s_cos_theta_array[1] = std::cos(theta + delta_theta);

        // sin,cos delta theta
        float two_cos_delta_theta = 2.0F * std::cos(delta_theta);

        // build table of other sin,cos with trig identity
        for (int i = 2; i < line_len; i++)
        {
            s_sin_theta_array[i] = s_sin_theta_array[i - 1] * two_cos_delta_theta -
                               s_sin_theta_array[i - 2];
            s_cos_theta_array[i] = s_cos_theta_array[i - 1] * two_cos_delta_theta -
                               s_cos_theta_array[i - 2];
        }

        // now phi - these calculated as we go - get started here
        s_delta_phi = (phi2 - phi1) / (float) g_height;

        // initial sin,cos phi

        s_old_sin_phi1 = std::sin(phi1);
        s_sin_phi = s_old_sin_phi1;
        s_old_cos_phi1 = std::cos(phi1);
        s_cos_phi = s_old_cos_phi1;
        s_old_sin_phi2 = std::sin(phi1 + s_delta_phi);
        s_old_cos_phi2 = std::cos(phi1 + s_delta_phi);

        // sin,cos delta phi
        s_two_cos_delta_phi = 2.0F * std::cos(s_delta_phi);

        // affects how rough planet terrain is
        if (id::g_rough)
        {
            s_r_scale = .3 * id::g_rough / 100.0;
        }

        // radius of planet
        s_radius = (double)(g_logical_screen_y_dots) / 2;

        // precalculate factor
        s_r_x_r_scale = s_radius * s_r_scale;

        s_scale_y = id::g_sphere_radius/100.0;
        s_scale_x = s_scale_y;
        s_scale_z = s_scale_x;      // Need x,y,z for RAY

        // adjust x scale factor for aspect
        s_scale_x *= s_aspect;

        // precalculation factor used in sphere calc
        s_radius_factor = s_r_scale * s_radius / (double) s_z_coord;

        if (s_persp)                // precalculate fudge factor
        {
            s_x_center = s_x_center << 16;
            s_y_center = s_y_center << 16;

            s_radius_factor *= 65536.0;
            s_radius *= 65536.0;

            /* calculate z cutoff factor attempt to prevent out-of-view surfaces
             * from being written */
            double z_view = -(long) ((double) g_logical_screen_y_dots * (double) id::g_viewer_z / 100.0);
            double radius = (double) (g_logical_screen_y_dots) / 2;
            double angle = std::atan(-radius / (z_view + radius));
            s_z_cutoff = -radius - std::sin(angle) * radius;
            s_z_cutoff *= 1.1;        // for safety
            s_z_cutoff *= 65536L;
        }
    }

    // set fill plot function
    if (id::g_fill_type != id::FillType::SURFACE_CONSTANT)
    {
        s_fill_plot = interp_color;
    }
    else
    {
        s_fill_plot = clip_color;

        if (g_transparent_color_3d[0] || g_transparent_color_3d[1])
        {
            // If transparent colors are set
            s_fill_plot = transparent_clip_color;// Use the transparent plot function
        }
    }

    // Both Sphere and Normal 3D
    s_light_direction[0] = id::g_light_x;
    direct[0] = s_light_direction[0];
    s_light_direction[1] = -id::g_light_y;
    direct[1] = s_light_direction[1];
    s_light_direction[2] = id::g_light_z;
    direct[2] = s_light_direction[2];

    /* Needed because sclz = -ROUGH/100 and light_direction is transformed in
     * FILLTYPE 6 but not in 5. */
    if (id::g_fill_type == id::FillType::LIGHT_SOURCE_BEFORE)
    {
        s_light_direction[2] = -id::g_light_z;
        direct[2] = s_light_direction[2];
    }

    if (id::g_fill_type == id::FillType::LIGHT_SOURCE_AFTER)           // transform light direction
    {
        /* Think of light direction  as a vector with tail at (0,0,0) and head
         * at (light_direction). We apply the transformation to BOTH head and
         * tail and take the difference */

        v[0] = 0.0;
        v[1] = 0.0;
        v[2] = 0.0;
        id::vec_mat_mul(v, g_m, v);
        id::vec_mat_mul(s_light_direction, g_m, s_light_direction);

        for (int i = 0; i < 3; i++)
        {
            s_light_direction[i] -= v[i];
        }
    }
    id::normalize_vector(s_light_direction);

    if (g_preview && g_show_box)
    {
        id::Vector origin;
        id::Vector tmp;
        id::normalize_vector(direct);

        // move light vector to be more clear with grey scale maps
        origin[0] = (3 * g_logical_screen_x_dots) / 16;
        origin[1] = (3 * g_logical_screen_y_dots) / 4;
        if (id::g_fill_type == id::FillType::LIGHT_SOURCE_AFTER)
        {
            origin[1] = (11 * g_logical_screen_y_dots) / 16;
        }

        origin[2] = 0.0;

        double v_length = std::min(g_logical_screen_x_dots, g_logical_screen_y_dots) / 2;
        if (s_persp && id::g_viewer_z <= s_p)
        {
            v_length *= (long)(s_p + 600) / ((long)(id::g_viewer_z + 600) * 2);
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
        draw_light_box(origin, direct, light_mat);
        /* draw box around original field of view to help visualize effect of
         * rotations 1 means show box - xmin etc. do nothing here */
        if (!id::g_sphere)
        {
            corners(g_m, true, &x_min, &y_min, &z_min, &x_max, &y_max, &z_max);
        }
    }

    // bad has values caught by clipping
    s_bad.x = BAD_VALUE;
    s_f_bad.x = (float) s_bad.x;
    s_bad.y = BAD_VALUE;
    s_f_bad.y = (float) s_bad.y;
    s_bad.color = BAD_VALUE;
    s_f_bad.color = (float) s_bad.color;
    for (int i = 0; i < line_len; i++)
    {
        s_last_row[i] = s_bad;
        s_f_last_row[i] = s_f_bad;
    }
    g_passes = Passes::THREE_D;
    return 0;
} // end of once-per-image intializations

static int line3d_mem()
{
    /* lastrow stores the previous row of the original GIF image for
       the purpose of filling in gaps with triangle procedure */
    s_last_row.resize(g_logical_screen_x_dots);

    if (id::g_sphere)
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
    if (id::g_fill_type == id::FillType::SURFACE_INTERPOLATED || id::g_fill_type == id::FillType::SURFACE_CONSTANT ||
        id::g_fill_type == id::FillType::LIGHT_SOURCE_BEFORE || id::g_fill_type == id::FillType::LIGHT_SOURCE_AFTER)
    {
        s_min_max_x.resize(OLD_MAX_PIXELS);
    }

    return 0;
}
