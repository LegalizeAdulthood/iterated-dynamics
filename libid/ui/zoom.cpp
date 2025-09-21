// SPDX-License-Identifier: GPL-3.0-only
//
/*
    routines for zoombox manipulation and for panning

*/
#include "ui/zoom.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/resume.h"
#include "engine/solid_guess.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "fractals/FrothyBasin.h"
#include "fractals/lyapunov.h"
#include "math/big.h"
#include "math/biginit.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/evolve.h"
#include "ui/find_special_colors.h"
#include "ui/framain2.h"
#include "ui/frothy_basin.h"
#include "ui/id_keys.h"
#include "ui/mouse.h"
#include "ui/spindac.h"
#include "ui/stop_msg.h"
#include "ui/video.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::ui
{

constexpr double PIXEL_ROUND{0.00001};

enum
{
    NUM_BOX_POINTS = 4096
};

int g_box_x[NUM_BOX_POINTS]{};
int g_box_y[NUM_BOX_POINTS]{};
int g_box_values[NUM_BOX_POINTS]{};
int g_box_color{}; // Zoom-Box color
int g_box_count{};                                              // 0 if no zoom-box yet
int g_zoom_box_rotation{};                                      // zoombox rotation
double g_zoom_box_x{}, g_zoom_box_y{};                          // topleft of zoombox
double g_zoom_box_width{}, g_zoom_box_height{};                 // zoombox size
double g_zoom_box_skew{};                                       // zoombox shape
bool g_zoom_enabled{};                                          // true when zoom is enabled

static int  check_pan();
static void fix_work_list();
static void move_row(int from_row, int to_row, int col);

// big number declarations
static void calc_corner(BigFloat target, BigFloat p1, const double p2, BigFloat p3, const double p4, BigFloat p5)
{
    BigStackSaver saved;
    const BigFloat b_tmp1 = alloc_stack(g_r_bf_length + 2);
    const BigFloat b_tmp2 = alloc_stack(g_r_bf_length + 2);
    const BigFloat b_tmp3 = alloc_stack(g_r_bf_length + 2);

    // use target as temporary variable
    float_to_bf(b_tmp3, p2);
    mult_bf(b_tmp1, b_tmp3, p3);
    mult_bf(b_tmp2, float_to_bf(target, p4), p5);
    add_bf(target, b_tmp1, b_tmp2);
    add_a_bf(target, p1);
}

void display_box()
{
    const int box_color = g_colors - 1 & g_box_color;
    int rgb[3];
    for (int i = 0; i < g_box_count; i++)
    {
        if (g_is_true_color && g_true_mode != TrueColorMode::DEFAULT_COLOR)
        {
            driver_get_true_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, &rgb[0], &rgb[1], &rgb[2], nullptr);
            driver_put_true_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset,
                                 rgb[0]^255, rgb[1]^255, rgb[2]^255, 255);
        }
        else
        {
            g_box_values[i] = get_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset);
        }
    }

    // There is an interaction between getcolor and putcolor, so separate them
    // don't need this for truecolor with truemode set
    if (!g_is_true_color || g_true_mode == TrueColorMode::DEFAULT_COLOR)
    {
        for (int i = 0; i < g_box_count; i++)
        {
            if (g_colors == 2)
            {
                g_put_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, 1 - g_box_values[i]);
            }
            else
            {
                g_put_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, box_color);
            }
        }
    }
}

void clear_box()
{
    if (g_is_true_color && g_true_mode != TrueColorMode::DEFAULT_COLOR)
    {
        display_box();
    }
    else
    {
        for (int i = 0; i < g_box_count; i++)
        {
            g_put_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, g_box_values[i]);
        }
    }
}

void draw_box(const bool draw_it)
{
    if (g_zoom_box_width == 0.0)
    {
        // no box to draw
        if (g_box_count != 0)
        {
            // remove the old box from display
            clear_box();
            g_box_count = 0;
        }
        reset_zoom_corners();
        return;
    }

    int saved = 0;
    BigFloat bf_f_x_width;
    BigFloat bf_f_x_skew;
    BigFloat bf_f_y_height;
    BigFloat bf_f_y_skew;
    BigFloat bf_f_x_adj;
    if (g_bf_math != BFMathType::NONE)
    {
        saved = save_stack();
        bf_f_x_width = alloc_stack(g_r_bf_length+2);
        bf_f_x_skew  = alloc_stack(g_r_bf_length+2);
        bf_f_y_height = alloc_stack(g_r_bf_length+2);
        bf_f_y_skew  = alloc_stack(g_r_bf_length+2);
        bf_f_x_adj   = alloc_stack(g_r_bf_length+2);
    }
    double f_temp1 = PI * g_zoom_box_rotation / 72; // convert to radians
    const double rot_cos = std::cos(f_temp1);   // sin & cos of rotation
    const double rot_sin = std::sin(f_temp1);

    // do some calcs just once here to reduce fp work a bit
    const double f_x_width = g_save_x_max - g_save_x_3rd;
    const double f_x_skew = g_save_x_3rd - g_save_x_min;
    const double f_y_height = g_save_y_3rd - g_save_y_max;
    const double f_y_skew = g_save_y_min - g_save_y_3rd;
    const double f_x_adj = g_zoom_box_width * g_zoom_box_skew;

    if (g_bf_math != BFMathType::NONE)
    {
        // do some calcs just once here to reduce fp work a bit
        sub_bf(bf_f_x_width, g_bf_save_x_max, g_bf_save_x_3rd);
        sub_bf(bf_f_x_skew, g_bf_save_x_3rd, g_bf_save_x_min);
        sub_bf(bf_f_y_height, g_bf_save_y_3rd, g_bf_save_y_max);
        sub_bf(bf_f_y_skew, g_bf_save_y_min, g_bf_save_y_3rd);
        float_to_bf(bf_f_x_adj, f_x_adj);
    }

    // calc co-ords of topleft & botright corners of box
    double tmp_x = g_zoom_box_width / -2 + f_x_adj; // from zoombox center as origin, on xdots scale
    double tmp_y = g_zoom_box_height * g_final_aspect_ratio / 2;
    double dx = rot_cos * tmp_x - rot_sin * tmp_y - tmp_x; // delta x to rotate topleft
    double dy = tmp_y - (rot_sin * tmp_x + rot_cos * tmp_y); // delta y to rotate topleft

    // calc co-ords of topleft
    f_temp1 = g_zoom_box_x + dx + f_x_adj;
    double f_temp2 = g_zoom_box_y + dy / g_final_aspect_ratio;

    Coord top_left;
    top_left.x   = static_cast<int>(f_temp1 * (g_logical_screen_x_size_dots + PIXEL_ROUND)); // screen co-ords
    top_left.y   = static_cast<int>(f_temp2 * (g_logical_screen_y_size_dots + PIXEL_ROUND));
    g_x_min  = g_save_x_min + f_temp1*f_x_width + f_temp2*f_x_skew; // real co-ords
    g_y_max  = g_save_y_max + f_temp2*f_y_height + f_temp1*f_y_skew;
    if (g_bf_math != BFMathType::NONE)
    {
        calc_corner(g_bf_x_min, g_bf_save_x_min, f_temp1, bf_f_x_width, f_temp2, bf_f_x_skew);
        calc_corner(g_bf_y_max, g_bf_save_y_max, f_temp2, bf_f_y_height, f_temp1, bf_f_y_skew);
    }

    // calc co-ords of bottom right
    f_temp1 = g_zoom_box_x + g_zoom_box_width - dx - f_x_adj;
    f_temp2 = g_zoom_box_y - dy/g_final_aspect_ratio + g_zoom_box_height;
    Coord bot_right;
    bot_right.x   = static_cast<int>(f_temp1 * (g_logical_screen_x_size_dots + PIXEL_ROUND));
    bot_right.y   = static_cast<int>(f_temp2 * (g_logical_screen_y_size_dots + PIXEL_ROUND));
    g_x_max  = g_save_x_min + f_temp1*f_x_width + f_temp2*f_x_skew;
    g_y_min  = g_save_y_max + f_temp2*f_y_height + f_temp1*f_y_skew;
    if (g_bf_math != BFMathType::NONE)
    {
        calc_corner(g_bf_x_max, g_bf_save_x_min, f_temp1, bf_f_x_width, f_temp2, bf_f_x_skew);
        calc_corner(g_bf_y_min, g_bf_save_y_max, f_temp2, bf_f_y_height, f_temp1, bf_f_y_skew);
    }

    // do the same for botleft & topright
    tmp_x = g_zoom_box_width/-2 - f_x_adj;
    tmp_y = 0.0-tmp_y;
    dx = rot_cos * tmp_x - rot_sin * tmp_y - tmp_x;
    dy = tmp_y - (rot_sin*tmp_x + rot_cos*tmp_y);
    f_temp1 = g_zoom_box_x + dx - f_x_adj;
    f_temp2 = g_zoom_box_y + dy/g_final_aspect_ratio + g_zoom_box_height;
    Coord bot_left;
    bot_left.x   = static_cast<int>(f_temp1 * (g_logical_screen_x_size_dots + PIXEL_ROUND));
    bot_left.y   = static_cast<int>(f_temp2 * (g_logical_screen_y_size_dots + PIXEL_ROUND));
    g_x_3rd  = g_save_x_min + f_temp1*f_x_width + f_temp2*f_x_skew;
    g_y_3rd  = g_save_y_max + f_temp2*f_y_height + f_temp1*f_y_skew;
    if (g_bf_math != BFMathType::NONE)
    {
        calc_corner(g_bf_x_3rd, g_bf_save_x_min, f_temp1, bf_f_x_width, f_temp2, bf_f_x_skew);
        calc_corner(g_bf_y_3rd, g_bf_save_y_max, f_temp2, bf_f_y_height, f_temp1, bf_f_y_skew);
        restore_stack(saved);
    }
    f_temp1 = g_zoom_box_x + g_zoom_box_width - dx + f_x_adj;
    f_temp2 = g_zoom_box_y - dy/g_final_aspect_ratio;
    Coord top_right;
    top_right.x   = static_cast<int>(f_temp1 * (g_logical_screen_x_size_dots + PIXEL_ROUND));
    top_right.y   = static_cast<int>(f_temp2 * (g_logical_screen_y_size_dots + PIXEL_ROUND));

    if (g_box_count != 0)
    {
        // remove the old box from display
        clear_box();
        g_box_count = 0;
    }

    if (draw_it)
    {
        // caller wants box drawn as well as co-ords calc'd
        // build the list of zoom box pixels
        add_box(top_left);
        add_box(top_right);               // corner pixels
        add_box(bot_left);
        add_box(bot_right);
        draw_lines(top_left, top_right, bot_left.x-top_left.x, bot_left.y-top_left.y); // top & bottom lines
        draw_lines(top_left, bot_left, top_right.x-top_left.x, top_right.y-top_left.y); // left & right lines
        display_box();
    }
}

void draw_lines(Coord fr, Coord to, const int dx, const int dy)
{
    if (std::abs(to.x-fr.x) > std::abs(to.y-fr.y))
    {
        // delta.x > delta.y
        if (fr.x > to.x)
        {
            // swap so from.x is < to.x
            const Coord tmp_p = fr;
            fr = to;
            to = tmp_p;
        }
        const int x_incr = (to.x-fr.x)*4/g_screen_x_dots+1; // do every 1st, 2nd, 3rd, or 4th dot
        int ctr = (to.x - fr.x - 1) / x_incr;
        const int alt_dec = std::abs(to.y - fr.y) * x_incr;
        const int alt_inc = to.x-fr.x;
        int alt_ctr = alt_inc / 2;
        const int y_incr = to.y > fr.y ?1:-1;
        Coord line1;
        line1.y = fr.y;
        line1.x = fr.x;
        Coord line2;
        line2.x = line1.x + dx;
        line2.y = line1.y + dy;
        while (--ctr >= 0)
        {
            line1.x += x_incr;
            line2.x += x_incr;
            alt_ctr -= alt_dec;
            while (alt_ctr < 0)
            {
                alt_ctr  += alt_inc;
                line1.y += y_incr;
                line2.y += y_incr;
            }
            add_box(line1);
            add_box(line2);
        }
    }
    else
    {
        // delta.y > delta.x
        if (fr.y > to.y)
        {
            // swap so from.y is < to.y
            const Coord tmp_p = fr;
            fr = to;
            to = tmp_p;
        }
        const int y_incr = (to.y-fr.y)*4/g_screen_y_dots+1; // do every 1st, 2nd, 3rd, or 4th dot
        int ctr = (to.y - fr.y - 1) / y_incr;
        const int alt_dec = std::abs(to.x - fr.x) * y_incr;
        const int alt_inc = to.y-fr.y;
        int alt_ctr = alt_inc / 2;
        const int x_incr = to.x > fr.x ? 1 : -1;
        Coord line1;
        line1.x = fr.x;
        line1.y = fr.y;
        Coord line2;
        line2.x = line1.x + dx;
        line2.y = line1.y + dy;
        while (--ctr >= 0)
        {
            line1.y += y_incr;
            line2.y += y_incr;
            alt_ctr  -= alt_dec;
            while (alt_ctr < 0)
            {
                alt_ctr  += alt_inc;
                line1.x += x_incr;
                line2.x += x_incr;
            }
            add_box(line1);
            add_box(line2);
        }
    }
}

void add_box(Coord point)
{
    assert(g_box_count < NUM_BOX_POINTS);
    point.x += g_logical_screen_x_offset;
    point.y += g_logical_screen_y_offset;
    if (point.x >= 0 && point.x < g_screen_x_dots
        && point.y >= 0 && point.y < g_screen_y_dots)
    {
        g_box_x[g_box_count] = point.x;
        g_box_y[g_box_count] = point.y;
        ++g_box_count;
    }
}

void move_box(const double dx, const double dy)
{
    const int align = check_pan();
    if (dx != 0.0)
    {
        if ((g_zoom_box_x += dx) + g_zoom_box_width/2 < 0)    // center must stay onscreen
        {
            g_zoom_box_x = g_zoom_box_width/-2;
        }
        if (g_zoom_box_x + g_zoom_box_width/2 > 1)
        {
            g_zoom_box_x = 1.0 - g_zoom_box_width/2;
        }
        if (align != 0)
        {
            if (int col = static_cast<int>(g_zoom_box_x * (g_logical_screen_x_size_dots + PIXEL_ROUND));
                (col & align - 1) != 0)
            {
                if (dx > 0)
                {
                    col += align;
                }
                col -= col & align - 1; // adjust col to pass alignment
                g_zoom_box_x = static_cast<double>(col) / g_logical_screen_x_size_dots;
            }
        }
    }
    if (dy != 0.0)
    {
        if ((g_zoom_box_y += dy) + g_zoom_box_height/2 < 0)
        {
            g_zoom_box_y = g_zoom_box_height/-2;
        }
        if (g_zoom_box_y + g_zoom_box_height/2 > 1)
        {
            g_zoom_box_y = 1.0 - g_zoom_box_height/2;
        }
        if (align != 0)
        {
            if (int row = static_cast<int>(g_zoom_box_y * (g_logical_screen_y_size_dots + PIXEL_ROUND));
                (row & align - 1) != 0)
            {
                if (dy > 0)
                {
                    row += align;
                }
                row -= row & align - 1;
                g_zoom_box_y = static_cast<double>(row) / g_logical_screen_y_size_dots;
            }
        }
    }
}

static void change_box(double delta_width, double delta_height)
{
    if (g_zoom_box_width + delta_width > 1.0)
    {
        delta_width = 1.0 - g_zoom_box_width;
    }
    if (g_zoom_box_width + delta_width < 0.05)
    {
        delta_width = 0.05 - g_zoom_box_width;
    }
    g_zoom_box_width += delta_width;
    if (g_zoom_box_height + delta_height > 1.0)
    {
        delta_height = 1.0 - g_zoom_box_height;
    }
    if (g_zoom_box_height + delta_height < 0.05)
    {
        delta_height = 0.05 - g_zoom_box_height;
    }
    g_zoom_box_height += delta_height;
    move_box(delta_width / -2.0, delta_height / -2.0); // keep it centered & check limits
}

void resize_box(const int steps)
{
    double delta_x;
    double delta_y;
    if (g_zoom_box_height*g_screen_aspect > g_zoom_box_width)
    {
        // box larger on y axis
        delta_y = steps * 0.036 / g_screen_aspect;
        delta_x = g_zoom_box_width * delta_y / g_zoom_box_height;
    }
    else
    {
        // box larger on x axis
        delta_x = steps * 0.036;
        delta_y = g_zoom_box_height * delta_x / g_zoom_box_width;
    }
    change_box(delta_x, delta_y);
}

// change size by pixels
void change_box(const int dw, const int dh)
{
    change_box(dw / g_logical_screen_x_size_dots, dh / g_logical_screen_y_size_dots);
}

static void zoom_out_calc(BigFloat bf_dx, BigFloat bf_dy, //
    BigFloat bf_new_x, BigFloat bf_new_y,                 //
    BigFloat bf_plot_mx1, BigFloat bf_plot_mx2,           //
    BigFloat bf_plot_my1, BigFloat bf_plot_my2,           //
    BigFloat bf_f_temp)
{
    BigStackSaver saved;
    BigFloat b_tmp1 = alloc_stack(g_r_bf_length + 2);
    BigFloat b_tmp2 = alloc_stack(g_r_bf_length + 2);
    BigFloat b_tmp3 = alloc_stack(g_r_bf_length + 2);
    BigFloat b_tmp4 = alloc_stack(g_r_bf_length + 2);
    BigFloat b_tmp2a = alloc_stack(g_r_bf_length + 2);
    BigFloat b_tmp4a = alloc_stack(g_r_bf_length + 2);
    BigFloat b_temp_x = alloc_stack(g_r_bf_length + 2);
    BigFloat b_temp_y = alloc_stack(g_r_bf_length + 2);

    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft through (1,1) bottom right */

    // tempx = dy * plotmx1 - dx * plotmx2;
    mult_bf(b_tmp1, bf_dy, bf_plot_mx1);
    mult_bf(b_tmp2, bf_dx, bf_plot_mx2);
    sub_bf(b_temp_x, b_tmp1, b_tmp2);

    // tempy = dx * plotmy1 - dy * plotmy2;
    mult_bf(b_tmp1, bf_dx, bf_plot_my1);
    mult_bf(b_tmp2, bf_dy, bf_plot_my2);
    sub_bf(b_temp_y, b_tmp1, b_tmp2);

    // calc new corner by extending from current screen corners
    // *newx = sxmin + tempx*(sxmax-sx3rd)/ftemp + tempy*(sx3rd-sxmin)/ftemp;
    sub_bf(b_tmp1, g_bf_save_x_max, g_bf_save_x_3rd);
    mult_bf(b_tmp2, b_temp_x, b_tmp1);
    div_bf(b_tmp2a, b_tmp2, bf_f_temp);
    sub_bf(b_tmp3, g_bf_save_x_3rd, g_bf_save_x_min);
    mult_bf(b_tmp4, b_temp_y, b_tmp3);
    div_bf(b_tmp4a, b_tmp4, bf_f_temp);
    add_bf(bf_new_x, g_bf_save_x_min, b_tmp2a);
    add_a_bf(bf_new_x, b_tmp4a);

    // *newy = symax + tempy*(sy3rd-symax)/ftemp + tempx*(symin-sy3rd)/ftemp;
    sub_bf(b_tmp1, g_bf_save_y_3rd, g_bf_save_y_max);
    mult_bf(b_tmp2, b_temp_y, b_tmp1);
    div_bf(b_tmp2a, b_tmp2, bf_f_temp);
    sub_bf(b_tmp3, g_bf_save_y_min, g_bf_save_y_3rd);
    mult_bf(b_tmp4, b_temp_x, b_tmp3);
    div_bf(b_tmp4a, b_tmp4, bf_f_temp);
    add_bf(bf_new_y, g_bf_save_y_max, b_tmp2a);
    add_a_bf(bf_new_y, b_tmp4a);
}

static void zoom_out_calc(const double dx, const double dy, double *new_x, double *new_y, const double f_temp)
{
    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft through (1,1) bottom right */
    const double temp_x = dy * g_plot_mx1 - dx * g_plot_mx2;
    const double temp_y = dx * g_plot_my1 - dy * g_plot_my2;

    // calc new corner by extending from current screen corners
    *new_x = g_save_x_min + temp_x*(g_save_x_max-g_save_x_3rd)/f_temp + temp_y*(g_save_x_3rd-g_save_x_min)/f_temp;
    *new_y = g_save_y_max + temp_y*(g_save_y_3rd-g_save_y_max)/f_temp + temp_x*(g_save_y_min-g_save_y_3rd)/f_temp;
}

static void zoom_out_bf() // for ctl-enter, calc corners for zooming out
{
    /* (xxmin,yymax), etc., are already set to zoombox corners;
       (sxmin,symax), etc., are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/height 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    BigStackSaver saved;
    const BigFloat save_bf_x_min = alloc_stack(g_r_bf_length + 2);
    const BigFloat save_bf_y_max = alloc_stack(g_r_bf_length + 2);
    const BigFloat bf_f_temp = alloc_stack(g_r_bf_length + 2);
    const BigFloat tmp1 = alloc_stack(g_r_bf_length + 2);
    const BigFloat tmp2 = alloc_stack(g_r_bf_length + 2);
    const BigFloat tmp3 = alloc_stack(g_r_bf_length + 2);
    const BigFloat tmp4 = alloc_stack(g_r_bf_length + 2);
    const BigFloat tmp5 = alloc_stack(g_r_bf_length + 2);
    const BigFloat tmp6 = alloc_stack(g_r_bf_length + 2);
    const BigFloat bf_plot_mx1 = alloc_stack(g_r_bf_length + 2);
    const BigFloat bf_plot_mx2 = alloc_stack(g_r_bf_length + 2);
    const BigFloat bf_plot_my1 = alloc_stack(g_r_bf_length + 2);
    const BigFloat bf_plot_my2 = alloc_stack(g_r_bf_length + 2);
    // ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax);
    sub_bf(tmp1, g_bf_y_min, g_bf_y_3rd);
    sub_bf(tmp2, g_bf_x_3rd, g_bf_x_min);
    sub_bf(tmp3, g_bf_x_max, g_bf_x_3rd);
    sub_bf(tmp4, g_bf_y_3rd, g_bf_y_max);
    mult_bf(tmp5, tmp1, tmp2);
    mult_bf(tmp6, tmp3, tmp4);
    sub_bf(bf_f_temp, tmp5, tmp6);
    // plotmx1 = (xx3rd-xxmin); */ ; /* reuse the plotxxx vars is safe
    copy_bf(bf_plot_mx1, tmp2);
    // plotmx2 = (yy3rd-yymax);
    copy_bf(bf_plot_mx2, tmp4);
    // plotmy1 = (yymin-yy3rd);
    copy_bf(bf_plot_my1, tmp1);
    // plotmy2 = (xxmax-xx3rd);
    copy_bf(bf_plot_my2, tmp3);

    // savxxmin = xxmin; savyymax = yymax;
    copy_bf(save_bf_x_min, g_bf_x_min);
    copy_bf(save_bf_y_max, g_bf_y_max);

    sub_bf(tmp1, g_bf_save_x_min, save_bf_x_min);
    sub_bf(tmp2, g_bf_save_y_max, save_bf_y_max);
    zoom_out_calc(tmp1, tmp2, g_bf_x_min, g_bf_y_max, bf_plot_mx1, bf_plot_mx2, bf_plot_my1, bf_plot_my2, bf_f_temp);
    sub_bf(tmp1, g_bf_save_x_max, save_bf_x_min);
    sub_bf(tmp2, g_bf_save_y_min, save_bf_y_max);
    zoom_out_calc(tmp1, tmp2, g_bf_x_max, g_bf_y_min, bf_plot_mx1, bf_plot_mx2, bf_plot_my1, bf_plot_my2, bf_f_temp);
    sub_bf(tmp1, g_bf_save_x_3rd, save_bf_x_min);
    sub_bf(tmp2, g_bf_save_y_3rd, save_bf_y_max);
    zoom_out_calc(tmp1, tmp2, g_bf_x_3rd, g_bf_y_3rd, bf_plot_mx1, bf_plot_mx2, bf_plot_my1, bf_plot_my2, bf_f_temp);
}

static void zoom_out_dbl() // for ctl-enter, calc corners for zooming out
{
    /* (xxmin,yymax), etc., are already set to zoombox corners;
       (sxmin,symax), etc., are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/height 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    const double f_temp = (g_y_min - g_y_3rd) * (g_x_3rd - g_x_min) - (g_x_max - g_x_3rd) * (g_y_3rd - g_y_max);
    g_plot_mx1 = g_x_3rd - g_x_min; // reuse the plotxxx vars is safe
    g_plot_mx2 = g_y_3rd - g_y_max;
    g_plot_my1 = g_y_min - g_y_3rd;
    g_plot_my2 = g_x_max - g_x_3rd;
    const double save_x_min = g_x_min;
    const double save_y_max = g_y_max;
    zoom_out_calc(g_save_x_min-save_x_min, g_save_y_max-save_y_max, &g_x_min, &g_y_max, f_temp);
    zoom_out_calc(g_save_x_max-save_x_min, g_save_y_min-save_y_max, &g_x_max, &g_y_min, f_temp);
    zoom_out_calc(g_save_x_3rd-save_x_min, g_save_y_3rd-save_y_max, &g_x_3rd, &g_y_3rd, f_temp);
}

void zoom_out() // for ctl-enter, calc corners for zooming out
{
    if (g_bf_math != BFMathType::NONE)
    {
        zoom_out_bf();
    }
    else
    {
        zoom_out_dbl();
    }
}

void aspect_ratio_crop(const float old_aspect, const float new_aspect)
{
    double f_temp;
    double x_margin;
    double y_margin;
    if (new_aspect > old_aspect)
    {
        // new ratio is taller, crop x
        f_temp = (1.0 - old_aspect / new_aspect) / 2;
        x_margin = (g_x_max - g_x_3rd) * f_temp;
        y_margin = (g_y_min - g_y_3rd) * f_temp;
        g_x_3rd += x_margin;
        g_y_3rd += y_margin;
    }
    else
    {
        // new ratio is wider, crop y
        f_temp = (1.0 - new_aspect / old_aspect) / 2;
        x_margin = (g_x_3rd - g_x_min) * f_temp;
        y_margin = (g_y_3rd - g_y_max) * f_temp;
        g_x_3rd -= x_margin;
        g_y_3rd -= y_margin;
    }
    g_x_min += x_margin;
    g_y_max += y_margin;
    g_x_max -= x_margin;
    g_y_min -= y_margin;
}

static int check_pan() // return 0 if can't, alignment requirement if can
{
    if ((g_calc_status != CalcStatus::RESUMABLE && g_calc_status != CalcStatus::COMPLETED) ||
        g_evolving != EvolutionModeFlags::NONE)
    {
        return 0; // not resumable, not complete
    }
    if (g_cur_fractal_specific->calc_type != standard_fractal_type   //
        && g_cur_fractal_specific->calc_type != calc_mandelbrot_type //
        && g_cur_fractal_specific->calc_type != lyapunov_type        //
        && g_cur_fractal_specific->calc_type != froth_type)
    {
        return 0; // not a worklist-driven type
    }
    if (g_zoom_box_width != 1.0 || g_zoom_box_height != 1.0
        || g_zoom_box_skew != 0.0 || g_zoom_box_rotation != 0.0)
    {
        return 0; // not a full size unrotated unskewed zoom box
    }
    if (g_std_calc_mode == CalcMode::TESSERAL)
    {
        return 0; // tesseral, can't do it
    }
    if (g_std_calc_mode == CalcMode::DIFFUSION)
    {
        return 0; // diffusion scan: can't do it either
    }
    if (g_std_calc_mode == CalcMode::ORBIT)
    {
        return 0; // orbits, can't do it
    }

    // can pan if we get this far

    if (g_calc_status == CalcStatus::COMPLETED)
    {
        return 1; // image completed, align on any pixel
    }
    if (g_potential_flag && g_potential_16bit)
    {
        return 1; // 1 pass forced so align on any pixel
    }
    if (g_std_calc_mode == CalcMode::BOUNDARY_TRACE)
    {
        return 1; // boundary trace method, align on any pixel
    }
    if (g_std_calc_mode != CalcMode::SOLID_GUESS ||
        bit_set(g_cur_fractal_specific->flags, FractalFlags::NO_GUESS))
    {
        // align on even pixel for 2pass
        if (g_std_calc_mode == CalcMode::TWO_PASS || g_std_calc_mode == CalcMode::THREE_PASS)
        {
            return 2;
        }
        return 1; // assume 1pass
    }
    // solid guessing
    start_resume();
    get_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    // don't do end_resume! we're just looking
    int i = 9;
    for (int j = 0; j < g_num_work_list; ++j)   // find the lowest pass in any pending window
    {
        i = std::min(g_work_list[j].pass, i);
    }
    int j = ssg_block_size(); // worst-case alignment requirement
    while (--i >= 0)
    {
        j = j >> 1; // reduce requirement
    }
    return j;
}

// move a row on the screen
static void move_row(const int from_row, const int to_row, const int col)
{
    std::vector<Byte> temp(g_logical_screen_x_dots, 0);
    if (from_row >= 0 && from_row < g_logical_screen_y_dots)
    {
        int startcol = 0;
        int tocol = 0;
        int endcol = g_logical_screen_x_dots-1;
        if (col < 0)
        {
            tocol -= col;
            endcol += col;
        }
        if (col > 0)
        {
            startcol += col;
        }
        read_span(from_row, startcol, endcol, &temp[tocol]);
    }
    write_span(to_row, 0, g_logical_screen_x_dots-1, temp.data());
}

// decide to recalc, or to chg worklist & pan
void init_pan_or_recalc(const bool do_zoom_out)
{
    // no zoombox, leave g_calc_status as is
    if (g_zoom_box_width == 0.0)
    {
        return;
    }
    // got a zoombox
    const int align_mask = check_pan() - 1;

    // can't pan, trigger recalc
    if (align_mask < 0 || g_evolving != EvolutionModeFlags::NONE)
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        return;
    }

    // box is full screen, leave g_calc_status as is
    if (g_zoom_box_x == 0.0 && g_zoom_box_y == 0.0)
    {
        clear_box();
        return;
    }

    int col = static_cast<int>(
        g_zoom_box_x * (g_logical_screen_x_size_dots + PIXEL_ROUND)); // calc dest col,row of topleft pixel
    int row = static_cast<int>(g_zoom_box_y * (g_logical_screen_y_size_dots + PIXEL_ROUND));
    if (do_zoom_out)
    {
        // invert row and col
        row = -row;
        col = -col;
    }
    if ((row&align_mask) != 0 || (col&align_mask) != 0)
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED; // not on useable pixel alignment, trigger recalc
        return;
    }

    // pan
    g_num_work_list = 0;
    if (g_calc_status == CalcStatus::RESUMABLE)
    {
        start_resume();
        get_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
        // don't do end_resume! we might still change our mind
    }
    // adjust existing worklist entries
    for (int i = 0; i < g_num_work_list; ++i)
    {
        g_work_list[i].start.y -= row;
        g_work_list[i].stop.y  -= row;
        g_work_list[i].begin.y -= row;
        g_work_list[i].start.x -= col;
        g_work_list[i].stop.x  -= col;
        g_work_list[i].begin.x -= col;
    }
    // add worklist entries for the new edges
    bool list_full{};
    int i = 0;
    int j = g_logical_screen_y_dots-1;
    if (row < 0)
    {
        list_full = add_work_list(0, 0, g_logical_screen_x_dots - 1, 0 - row - 1, 0, 0, 0, 0) || list_full;
        i = -row;
    }
    if (row > 0)
    {
        list_full = add_work_list(0, g_logical_screen_y_dots - row, g_logical_screen_x_dots - 1,
                        g_logical_screen_y_dots - 1, 0, g_logical_screen_y_dots - row, 0, 0) || list_full;
        j = g_logical_screen_y_dots - row - 1;
    }
    if (col < 0)
    {
        list_full = add_work_list(0, i, 0 - col - 1, j, 0, i, 0, 0) || list_full;
    }
    if (col > 0)
    {
        list_full = add_work_list(g_logical_screen_x_dots - col, i, g_logical_screen_x_dots - 1, j,
                        g_logical_screen_x_dots - col, i, 0, 0) || list_full;
    }
    if (list_full)
    {
        if (stop_msg(StopMsgFlags::CANCEL,
            "Tables full, can't pan current image.\n"
            "Cancel resumes old image, continue pans and calculates a new one."))
        {
            g_zoom_box_width = 0; // cancel the zoombox
            draw_box(true);
        }
        else
        {
            g_calc_status = CalcStatus::PARAMS_CHANGED; // trigger recalc
        }
        return;
    }
    // now we're committed
    g_calc_status = CalcStatus::RESUMABLE;
    clear_box();
    if (row > 0)   // move image up
    {
        for (int y = 0; y < g_logical_screen_y_dots; ++y)
        {
            move_row(y+row, y, col);
        }
    }
    else             // move image down
    {
        for (int y = g_logical_screen_y_dots; --y >=0;)
        {
            move_row(y+row, y, col);
        }
    }
    fix_work_list(); // fixup any out of bounds worklist entries
    alloc_resume(sizeof(g_work_list)+20, 2); // post the new worklist
    put_resume_len(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
}

// force a worklist entry to restart
static void restart_window(const int index)
{
    const int y_start = std::max(0, g_work_list[index].start.y);
    const int x_start = std::max(0, g_work_list[index].start.x);
    const int y_stop = std::min(g_logical_screen_y_dots - 1, g_work_list[index].stop.y);
    const int x_stop = std::min(g_logical_screen_x_dots - 1, g_work_list[index].stop.x);
    const std::vector<Byte> temp(g_logical_screen_x_dots, 0);
    for (int y = y_start; y <= y_stop; ++y)
    {
        write_span(y, x_start, x_stop, temp.data());
    }
    g_work_list[index].pass = 0;
    g_work_list[index].symmetry = 0;
    g_work_list[index].begin.y = g_work_list[index].start.y;
    g_work_list[index].begin.x = g_work_list[index].start.x;
}

// fix out of bounds and symmetry related stuff
static void fix_work_list()
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        WorkList *wk = &g_work_list[i];
        if (wk->start.y >= g_logical_screen_y_dots || wk->start.y < 0
            || wk->start.x >= g_logical_screen_x_dots || wk->stop.x < 0)
        {
            // offscreen, delete
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                g_work_list[j-1] = g_work_list[j];
            }
            --g_num_work_list;
            --i;
            continue;
        }
        if (wk->start.y < 0)
        {
            // partly off top edge
            if ((wk->symmetry&1) == 0)
            {
                // no sym, easy
                wk->start.y = 0;
                wk->begin.x = 0;
            }
            else
            {
                // xaxis symmetry
                int j = wk->start.y + wk->start.y;
                if (j > 0 && g_num_work_list < MAX_CALC_WORK)
                {
                    // split the sym part
                    g_work_list[g_num_work_list] = g_work_list[i];
                    g_work_list[g_num_work_list].start.y = 0;
                    g_work_list[g_num_work_list++].stop.y = j;
                    wk->start.y = j+1;
                }
                else
                {
                    wk->start.y = 0;
                }
                restart_window(i); // restart the no-longer sym part
            }
        }
        if (wk->start.y >= g_logical_screen_y_dots)
        {
            // partly off bottom edge
            int j = g_logical_screen_y_dots-1;
            if ((wk->symmetry&1) != 0)
            {
                // uses xaxis symmetry
                int k = wk->start.y + (wk->start.y - j);
                if (k < j)
                {
                    if (g_num_work_list >= MAX_CALC_WORK)   // no room to split
                    {
                        restart_window(i);
                    }
                    else
                    {
                        // split it
                        g_work_list[g_num_work_list] = g_work_list[i];
                        g_work_list[g_num_work_list].start.y = k;
                        g_work_list[g_num_work_list++].stop.y = j;
                        j = k-1;
                    }
                }
                wk->symmetry &= -1 - 1;
            }
            wk->start.y = j;
        }
        if (wk->start.x < 0)
        {
            // partly off left edge
            if ((wk->symmetry&2) == 0)   // no sym, easy
            {
                wk->start.x = 0;
            }
            else
            {
                // yaxis symmetry
                int j = wk->stop.x + wk->start.x;
                if (j > 0 && g_num_work_list < MAX_CALC_WORK)
                {
                    // split the sym part
                    g_work_list[g_num_work_list] = g_work_list[i];
                    g_work_list[g_num_work_list].start.x = 0;
                    g_work_list[g_num_work_list++].stop.x = j;
                    wk->start.x = j+1;
                }
                else
                {
                    wk->start.x = 0;
                }
                restart_window(i); // restart the no-longer sym part
            }
        }
        if (wk->stop.x >= g_logical_screen_x_dots)
        {
            // partly off right edge
            int j = g_logical_screen_x_dots-1;
            if ((wk->symmetry&2) != 0)
            {
                // uses xaxis symmetry
                int k = wk->start.x + (wk->stop.x - j);
                if (k < j)
                {
                    if (g_num_work_list >= MAX_CALC_WORK)   // no room to split
                    {
                        restart_window(i);
                    }
                    else
                    {
                        // split it
                        g_work_list[g_num_work_list] = g_work_list[i];
                        g_work_list[g_num_work_list].start.x = k;
                        g_work_list[g_num_work_list++].stop.x = j;
                        j = k-1;
                    }
                }
                wk->symmetry &= -1 - 2;
            }
            wk->stop.x = j;
        }
        wk->begin.y = std::max(wk->begin.y, wk->start.y);
        wk->begin.y = std::min(wk->begin.y, wk->start.y);
        wk->begin.x = std::max(wk->begin.x, wk->start.x);
        wk->begin.x = std::min(wk->begin.x, wk->stop.x);
    }
    tidy_work_list(); // combine where possible, re-sort
}

void clear_zoom_box()
{
    g_zoom_box_width = 0.0;
    draw_box(false);
    reset_zoom_corners();
}

// do all pending movement at once for smooth mouse diagonal moves
MainState move_zoom_box(MainContext &context)
{
    int horizontal{};
    int vertical{};
    int getmore{1};
    while (getmore)
    {
        switch (context.key)
        {
        case ID_KEY_LEFT_ARROW:               // cursor left
            --horizontal;
            break;
        case ID_KEY_RIGHT_ARROW:              // cursor right
            ++horizontal;
            break;
        case ID_KEY_UP_ARROW:                 // cursor up
            --vertical;
            break;
        case ID_KEY_DOWN_ARROW:               // cursor down
            ++vertical;
            break;
        case ID_KEY_CTL_LEFT_ARROW:             // Ctrl-cursor left
            horizontal -= 8;
            break;
        case ID_KEY_CTL_RIGHT_ARROW:             // Ctrl-cursor right
            horizontal += 8;
            break;
        case ID_KEY_CTL_UP_ARROW:               // Ctrl-cursor up
            vertical -= 8;
            break;
        case ID_KEY_CTL_DOWN_ARROW:             // Ctrl-cursor down
            vertical += 8;
            break;                      // += 8 needed by VESA scrolling
        default:
            getmore = 0;
        }
        if (getmore)
        {
            if (getmore == 2)                // eat last key used
            {
                driver_get_key();
            }
            getmore = 2;
            context.key = driver_key_pressed();         // next pending key
        }
    }
    if (g_box_count)
    {
        move_box(static_cast<double>(horizontal) /g_logical_screen_x_size_dots, static_cast<double>(vertical) /g_logical_screen_y_size_dots);
    }
    return MainState::NOTHING;
}

void reset_zoom_corners()
{
    g_x_min = g_save_x_min;
    g_x_max = g_save_x_max;
    g_x_3rd = g_save_x_3rd;
    g_y_max = g_save_y_max;
    g_y_min = g_save_y_min;
    g_y_3rd = g_save_y_3rd;
    if (g_bf_math != BFMathType::NONE)
    {
        copy_bf(g_bf_x_min, g_bf_save_x_min);
        copy_bf(g_bf_x_max, g_bf_save_x_max);
        copy_bf(g_bf_y_min, g_bf_save_y_min);
        copy_bf(g_bf_y_max, g_bf_save_y_max);
        copy_bf(g_bf_x_3rd, g_bf_save_x_3rd);
        copy_bf(g_bf_y_3rd, g_bf_save_y_3rd);
    }
}

MainState request_zoom_in(MainContext &context)
{
    if (g_zoom_box_width != 0.0)
    {
        // do a zoom
        init_pan_or_recalc(false);
        context.more_keys = false;
    }
    if (g_calc_status != CalcStatus::COMPLETED) // don't restart if image complete
    {
        context.more_keys = false;
    }
    return MainState::NOTHING;
}

MainState request_zoom_out(MainContext &context)
{
    init_pan_or_recalc(true);
    context.more_keys = false;
    zoom_out(); // calc corners for zooming out
    return MainState::NOTHING;
}

MainState skew_zoom_left(MainContext &/*context*/)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_ROTATE))
    {
        const int i = key_count(ID_KEY_CTL_HOME);
        if ((g_zoom_box_skew -= 0.02 * i) < -0.48)
        {
            g_zoom_box_skew = -0.48;
        }
    }
    return MainState::NOTHING;
}

MainState skew_zoom_right(MainContext &/*context*/)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_ROTATE))
    {
        const int i = key_count(ID_KEY_CTL_END);
        if ((g_zoom_box_skew += 0.02 * i) > 0.48)
        {
            g_zoom_box_skew = 0.48;
        }
    }
    return MainState::NOTHING;
}

MainState decrease_zoom_aspect(MainContext &/*context*/)
{
    if (g_box_count)
    {
        change_box(0, -2 * key_count(ID_KEY_CTL_PAGE_UP));
    }
    return MainState::NOTHING;
}

MainState increase_zoom_aspect(MainContext &/*context*/)
{
    if (g_box_count)
    {
        change_box(0, 2 * key_count(ID_KEY_CTL_PAGE_DOWN));
    }
    return MainState::NOTHING;
}

MainState zoom_box_in(MainContext &/*context*/)
{
    if (g_zoom_enabled)
    {
        if (g_zoom_box_width == 0.0)
        {
            // start zoombox
            g_zoom_box_height = 1.0;
            g_zoom_box_width = 1.0;
            g_zoom_box_rotation = 0.0;
            g_zoom_box_skew = 0.0;
            g_zoom_box_x = 0.0;
            g_zoom_box_y = 0.0;
            find_special_colors();
            g_box_color = g_color_bright;
            g_evolve_param_grid_y = g_evolve_image_grid_size / 2;
            g_evolve_param_grid_x = g_evolve_image_grid_size / 2;
            move_box(0.0, 0.0); // force scrolling
        }
        else
        {
            resize_box(0 - key_count(ID_KEY_PAGE_UP));
        }
    }
    return MainState::NOTHING;
}

MainState zoom_box_out(MainContext &/*context*/)
{
    if (g_box_count)
    {
        // end zoombox
        if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999)
        {
            g_zoom_box_width = 0.0;
        }
        else
        {
            resize_box(key_count(ID_KEY_PAGE_DOWN));
        }
    }
    return MainState::NOTHING;
}

MainState zoom_box_increase_rotation(MainContext &/*context*/)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_ROTATE))
    {
        g_zoom_box_rotation += key_count(ID_KEY_CTL_MINUS);
    }
    return MainState::NOTHING;
}

MainState zoom_box_decrease_rotation(MainContext &/*context*/)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NO_ROTATE))
    {
        g_zoom_box_rotation -= key_count(ID_KEY_CTL_PLUS);
    }
    return MainState::NOTHING;
}

MainState zoom_box_increase_color(MainContext &/*context*/)
{
    g_box_color += key_count(ID_KEY_CTL_INSERT);
    return MainState::NOTHING;
}

MainState zoom_box_decrease_color(MainContext &/*context*/)
{
    g_box_color -= key_count(ID_KEY_CTL_DEL);
    return MainState::NOTHING;
}

} // namespace id::ui
