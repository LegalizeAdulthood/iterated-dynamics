// SPDX-License-Identifier: GPL-3.0-only
//
/*
    routines for zoombox manipulation and for panning

*/
#include "zoom.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "evolve.h"
#include "find_special_colors.h"
#include "fractalp.h"
#include "framain2.h"
#include "frothy_basin.h"
#include "id.h"
#include "id_data.h"
#include "id_keys.h"
#include "miscfrac.h"
#include "os.h"
#include "resume.h"
#include "spindac.h"
#include "ssg_block_size.h"
#include "stop_msg.h"
#include "video.h"
#include "work_list.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

constexpr double PIXELROUND{0.00001};

enum
{
    NUM_BOX_POINTS = 4096
};

int g_box_x[NUM_BOX_POINTS]{};
int g_box_y[NUM_BOX_POINTS]{};
int g_box_values[NUM_BOX_POINTS]{};
int g_box_color{}; // Zoom-Box color

static int  check_pan();
static void fix_work_list();
static void move_row(int fromrow, int torow, int col);

// big number declarations
void calc_corner(bf_t target, bf_t p1, double p2, bf_t p3, double p4, bf_t p5)
{
    const int saved = save_stack();
    const bf_t btmp1 = alloc_stack(g_r_bf_length + 2);
    const bf_t btmp2 = alloc_stack(g_r_bf_length + 2);
    const bf_t btmp3 = alloc_stack(g_r_bf_length + 2);

    // use target as temporary variable
    float_to_bf(btmp3, p2);
    mult_bf(btmp1, btmp3, p3);
    mult_bf(btmp2, float_to_bf(target, p4), p5);
    add_bf(target, btmp1, btmp2);
    add_a_bf(target, p1);
    restore_stack(saved);
}

void display_box()
{
    const int boxc = (g_colors - 1) & g_box_color;
    int rgb[3];
    for (int i = 0; i < g_box_count; i++)
    {
        if (g_is_true_color && g_true_mode != TrueColorMode::DEFAULT_COLOR)
        {
            driver_get_truecolor(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, &rgb[0], &rgb[1], &rgb[2], nullptr);
            driver_put_truecolor(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset,
                                 rgb[0]^255, rgb[1]^255, rgb[2]^255, 255);
        }
        else
        {
            g_box_values[i] = get_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset);
        }
    }

    // There is an interaction between getcolor and putcolor, so separate them
    // don't need this for truecolor with truemode set
    if (!(g_is_true_color && g_true_mode != TrueColorMode::DEFAULT_COLOR))
    {
        for (int i = 0; i < g_box_count; i++)
        {
            if (g_colors == 2)
            {
                g_put_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, (1 - g_box_values[i]));
            }
            else
            {
                g_put_color(g_box_x[i]-g_logical_screen_x_offset, g_box_y[i]-g_logical_screen_y_offset, boxc);
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

void draw_box(bool draw_it)
{
    if (g_zoom_box_width == 0)
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
    bf_t bffxwidth;
    bf_t bffxskew;
    bf_t bffydepth;
    bf_t bffyskew;
    bf_t bffxadj;
    if (g_bf_math != BFMathType::NONE)
    {
        saved = save_stack();
        bffxwidth = alloc_stack(g_r_bf_length+2);
        bffxskew  = alloc_stack(g_r_bf_length+2);
        bffydepth = alloc_stack(g_r_bf_length+2);
        bffyskew  = alloc_stack(g_r_bf_length+2);
        bffxadj   = alloc_stack(g_r_bf_length+2);
    }
    double ftemp1 = PI * g_zoom_box_rotation / 72; // convert to radians
    const double rotcos = std::cos(ftemp1);   // sin & cos of rotation
    const double rotsin = std::sin(ftemp1);

    // do some calcs just once here to reduce fp work a bit
    const double fxwidth = g_save_x_max - g_save_x_3rd;
    const double fxskew = g_save_x_3rd - g_save_x_min;
    const double fydepth = g_save_y_3rd - g_save_y_max;
    const double fyskew = g_save_y_min - g_save_y_3rd;
    const double fxadj = g_zoom_box_width * g_zoom_box_skew;

    if (g_bf_math != BFMathType::NONE)
    {
        // do some calcs just once here to reduce fp work a bit
        sub_bf(bffxwidth, g_bf_save_x_max, g_bf_save_x_3rd);
        sub_bf(bffxskew, g_bf_save_x_3rd, g_bf_save_x_min);
        sub_bf(bffydepth, g_bf_save_y_3rd, g_bf_save_y_max);
        sub_bf(bffyskew, g_bf_save_y_min, g_bf_save_y_3rd);
        float_to_bf(bffxadj, fxadj);
    }

    // calc co-ords of topleft & botright corners of box
    double tmpx = g_zoom_box_width / -2 + fxadj; // from zoombox center as origin, on xdots scale
    double tmpy = g_zoom_box_height * g_final_aspect_ratio / 2;
    double dx = (rotcos * tmpx - rotsin * tmpy) - tmpx; // delta x to rotate topleft
    double dy = tmpy - (rotsin * tmpx + rotcos * tmpy); // delta y to rotate topleft

    // calc co-ords of topleft
    ftemp1 = g_zoom_box_x + dx + fxadj;
    double ftemp2 = g_zoom_box_y + dy / g_final_aspect_ratio;

    Coord top_left;
    top_left.x   = (int)(ftemp1*(g_logical_screen_x_size_dots+PIXELROUND)); // screen co-ords
    top_left.y   = (int)(ftemp2*(g_logical_screen_y_size_dots+PIXELROUND));
    g_x_min  = g_save_x_min + ftemp1*fxwidth + ftemp2*fxskew; // real co-ords
    g_y_max  = g_save_y_max + ftemp2*fydepth + ftemp1*fyskew;
    if (g_bf_math != BFMathType::NONE)
    {
        calc_corner(g_bf_x_min, g_bf_save_x_min, ftemp1, bffxwidth, ftemp2, bffxskew);
        calc_corner(g_bf_y_max, g_bf_save_y_max, ftemp2, bffydepth, ftemp1, bffyskew);
    }

    // calc co-ords of bottom right
    ftemp1 = g_zoom_box_x + g_zoom_box_width - dx - fxadj;
    ftemp2 = g_zoom_box_y - dy/g_final_aspect_ratio + g_zoom_box_height;
    Coord bot_right;
    bot_right.x   = (int)(ftemp1*(g_logical_screen_x_size_dots+PIXELROUND));
    bot_right.y   = (int)(ftemp2*(g_logical_screen_y_size_dots+PIXELROUND));
    g_x_max  = g_save_x_min + ftemp1*fxwidth + ftemp2*fxskew;
    g_y_min  = g_save_y_max + ftemp2*fydepth + ftemp1*fyskew;
    if (g_bf_math != BFMathType::NONE)
    {
        calc_corner(g_bf_x_max, g_bf_save_x_min, ftemp1, bffxwidth, ftemp2, bffxskew);
        calc_corner(g_bf_y_min, g_bf_save_y_max, ftemp2, bffydepth, ftemp1, bffyskew);
    }

    // do the same for botleft & topright
    tmpx = g_zoom_box_width/-2 - fxadj;
    tmpy = 0.0-tmpy;
    dx = (rotcos*tmpx - rotsin*tmpy) - tmpx;
    dy = tmpy - (rotsin*tmpx + rotcos*tmpy);
    ftemp1 = g_zoom_box_x + dx - fxadj;
    ftemp2 = g_zoom_box_y + dy/g_final_aspect_ratio + g_zoom_box_height;
    Coord bot_left;
    bot_left.x   = (int)(ftemp1*(g_logical_screen_x_size_dots+PIXELROUND));
    bot_left.y   = (int)(ftemp2*(g_logical_screen_y_size_dots+PIXELROUND));
    g_x_3rd  = g_save_x_min + ftemp1*fxwidth + ftemp2*fxskew;
    g_y_3rd  = g_save_y_max + ftemp2*fydepth + ftemp1*fyskew;
    if (g_bf_math != BFMathType::NONE)
    {
        calc_corner(g_bf_x_3rd, g_bf_save_x_min, ftemp1, bffxwidth, ftemp2, bffxskew);
        calc_corner(g_bf_y_3rd, g_bf_save_y_max, ftemp2, bffydepth, ftemp1, bffyskew);
        restore_stack(saved);
    }
    ftemp1 = g_zoom_box_x + g_zoom_box_width - dx + fxadj;
    ftemp2 = g_zoom_box_y - dy/g_final_aspect_ratio;
    Coord top_right;
    top_right.x   = (int)(ftemp1*(g_logical_screen_x_size_dots+PIXELROUND));
    top_right.y   = (int)(ftemp2*(g_logical_screen_y_size_dots+PIXELROUND));

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

void draw_lines(Coord fr, Coord to, int dx, int dy)
{
    if (std::abs(to.x-fr.x) > std::abs(to.y-fr.y))
    {
        // delta.x > delta.y
        if (fr.x > to.x)
        {
            // swap so from.x is < to.x
            Coord const tmpp = fr;
            fr = to;
            to = tmpp;
        }
        const int xincr = (to.x-fr.x)*4/g_screen_x_dots+1; // do every 1st, 2nd, 3rd, or 4th dot
        int ctr = (to.x - fr.x - 1) / xincr;
        const int altdec = std::abs(to.y - fr.y) * xincr;
        const int altinc = to.x-fr.x;
        int altctr = altinc / 2;
        const int yincr = (to.y > fr.y)?1:-1;
        Coord line1;
        line1.y = fr.y;
        line1.x = fr.x;
        Coord line2;
        line2.x = line1.x + dx;
        line2.y = line1.y + dy;
        while (--ctr >= 0)
        {
            line1.x += xincr;
            line2.x += xincr;
            altctr -= altdec;
            while (altctr < 0)
            {
                altctr  += altinc;
                line1.y += yincr;
                line2.y += yincr;
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
            Coord const tmpp = fr;
            fr = to;
            to = tmpp;
        }
        const int yincr = (to.y-fr.y)*4/g_screen_y_dots+1; // do every 1st, 2nd, 3rd, or 4th dot
        int ctr = (to.y - fr.y - 1) / yincr;
        const int altdec = std::abs(to.x - fr.x) * yincr;
        const int altinc = to.y-fr.y;
        int altctr = altinc / 2;
        const int xincr = (to.x > fr.x) ? 1 : -1;
        Coord line1;
        line1.x = fr.x;
        line1.y = fr.y;
        Coord line2;
        line2.x = line1.x + dx;
        line2.y = line1.y + dy;
        while (--ctr >= 0)
        {
            line1.y += yincr;
            line2.y += yincr;
            altctr  -= altdec;
            while (altctr < 0)
            {
                altctr  += altinc;
                line1.x += xincr;
                line2.x += xincr;
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

void move_box(double dx, double dy)
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
            if (int col = (int) (g_zoom_box_x * (g_logical_screen_x_size_dots + PIXELROUND));
                (col & (align - 1)) != 0)
            {
                if (dx > 0)
                {
                    col += align;
                }
                col -= col & (align - 1); // adjust col to pass alignment
                g_zoom_box_x = (double) col / g_logical_screen_x_size_dots;
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
            if (int row = (int) (g_zoom_box_y * (g_logical_screen_y_size_dots + PIXELROUND));
                (row & (align - 1)) != 0)
            {
                if (dy > 0)
                {
                    row += align;
                }
                row -= row & (align - 1);
                g_zoom_box_y = (double) row / g_logical_screen_y_size_dots;
            }
        }
    }
}

static void change_box(double dwidth, double ddepth)
{
    if (g_zoom_box_width+dwidth > 1)
    {
        dwidth = 1.0-g_zoom_box_width;
    }
    if (g_zoom_box_width+dwidth < 0.05)
    {
        dwidth = 0.05-g_zoom_box_width;
    }
    g_zoom_box_width += dwidth;
    if (g_zoom_box_height+ddepth > 1)
    {
        ddepth = 1.0-g_zoom_box_height;
    }
    if (g_zoom_box_height+ddepth < 0.05)
    {
        ddepth = 0.05-g_zoom_box_height;
    }
    g_zoom_box_height += ddepth;
    move_box(dwidth/-2, ddepth/-2); // keep it centered & check limits
}

void resize_box(int steps)
{
    double deltax;
    double deltay;
    if (g_zoom_box_height*g_screen_aspect > g_zoom_box_width)
    {
        // box larger on y axis
        deltay = steps * 0.036 / g_screen_aspect;
        deltax = g_zoom_box_width * deltay / g_zoom_box_height;
    }
    else
    {
        // box larger on x axis
        deltax = steps * 0.036;
        deltay = g_zoom_box_height * deltax / g_zoom_box_width;
    }
    change_box(deltax, deltay);
}

void change_box(int dw, int dd)
{
    // change size by pixels
    change_box(dw / g_logical_screen_x_size_dots, dd / g_logical_screen_y_size_dots);
}

static void zoom_out_calc(bf_t bfdx, bf_t bfdy, //
    bf_t bfnewx, bf_t bfnewy,                   //
    bf_t bfplotmx1, bf_t bfplotmx2,             //
    bf_t bfplotmy1, bf_t bfplotmy2,             //
    bf_t bfftemp)
{
    const int saved = save_stack();
    bf_t btmp1 = alloc_stack(g_r_bf_length + 2);
    bf_t btmp2 = alloc_stack(g_r_bf_length + 2);
    bf_t btmp3 = alloc_stack(g_r_bf_length + 2);
    bf_t btmp4 = alloc_stack(g_r_bf_length + 2);
    bf_t btmp2a = alloc_stack(g_r_bf_length + 2);
    bf_t btmp4a = alloc_stack(g_r_bf_length + 2);
    bf_t btempx = alloc_stack(g_r_bf_length + 2);
    bf_t btempy = alloc_stack(g_r_bf_length + 2);

    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */

    // tempx = dy * plotmx1 - dx * plotmx2;
    mult_bf(btmp1, bfdy, bfplotmx1);
    mult_bf(btmp2, bfdx, bfplotmx2);
    sub_bf(btempx, btmp1, btmp2);

    // tempy = dx * plotmy1 - dy * plotmy2;
    mult_bf(btmp1, bfdx, bfplotmy1);
    mult_bf(btmp2, bfdy, bfplotmy2);
    sub_bf(btempy, btmp1, btmp2);

    // calc new corner by extending from current screen corners
    // *newx = sxmin + tempx*(sxmax-sx3rd)/ftemp + tempy*(sx3rd-sxmin)/ftemp;
    sub_bf(btmp1, g_bf_save_x_max, g_bf_save_x_3rd);
    mult_bf(btmp2, btempx, btmp1);
    div_bf(btmp2a, btmp2, bfftemp);
    sub_bf(btmp3, g_bf_save_x_3rd, g_bf_save_x_min);
    mult_bf(btmp4, btempy, btmp3);
    div_bf(btmp4a, btmp4, bfftemp);
    add_bf(bfnewx, g_bf_save_x_min, btmp2a);
    add_a_bf(bfnewx, btmp4a);

    // *newy = symax + tempy*(sy3rd-symax)/ftemp + tempx*(symin-sy3rd)/ftemp;
    sub_bf(btmp1, g_bf_save_y_3rd, g_bf_save_y_max);
    mult_bf(btmp2, btempy, btmp1);
    div_bf(btmp2a, btmp2, bfftemp);
    sub_bf(btmp3, g_bf_save_y_min, g_bf_save_y_3rd);
    mult_bf(btmp4, btempx, btmp3);
    div_bf(btmp4a, btmp4, bfftemp);
    add_bf(bfnewy, g_bf_save_y_max, btmp2a);
    add_a_bf(bfnewy, btmp4a);
    restore_stack(saved);
}

static void zoom_out_calc(double dx, double dy, double *newx, double *newy, double ftemp)
{
    /* calc cur screen corner relative to zoombox, when zoombox co-ords
       are taken as (0,0) topleft thru (1,1) bottom right */
    const double tempx = dy * g_plot_mx1 - dx * g_plot_mx2;
    const double tempy = dx * g_plot_my1 - dy * g_plot_my2;

    // calc new corner by extending from current screen corners
    *newx = g_save_x_min + tempx*(g_save_x_max-g_save_x_3rd)/ftemp + tempy*(g_save_x_3rd-g_save_x_min)/ftemp;
    *newy = g_save_y_max + tempy*(g_save_y_3rd-g_save_y_max)/ftemp + tempx*(g_save_y_min-g_save_y_3rd)/ftemp;
}

static void zoom_out_bf() // for ctl-enter, calc corners for zooming out
{
    /* (xxmin,yymax), etc, are already set to zoombox corners;
       (sxmin,symax), etc, are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/depth 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    const int saved = save_stack();
    const bf_t savbfxmin = alloc_stack(g_r_bf_length + 2);
    const bf_t savbfymax = alloc_stack(g_r_bf_length + 2);
    const bf_t bfftemp = alloc_stack(g_r_bf_length + 2);
    const bf_t tmp1 = alloc_stack(g_r_bf_length + 2);
    const bf_t tmp2 = alloc_stack(g_r_bf_length + 2);
    const bf_t tmp3 = alloc_stack(g_r_bf_length + 2);
    const bf_t tmp4 = alloc_stack(g_r_bf_length + 2);
    const bf_t tmp5 = alloc_stack(g_r_bf_length + 2);
    const bf_t tmp6 = alloc_stack(g_r_bf_length + 2);
    const bf_t bfplotmx1 = alloc_stack(g_r_bf_length + 2);
    const bf_t bfplotmx2 = alloc_stack(g_r_bf_length + 2);
    const bf_t bfplotmy1 = alloc_stack(g_r_bf_length + 2);
    const bf_t bfplotmy2 = alloc_stack(g_r_bf_length + 2);
    // ftemp = (yymin-yy3rd)*(xx3rd-xxmin) - (xxmax-xx3rd)*(yy3rd-yymax);
    sub_bf(tmp1, g_bf_y_min, g_bf_y_3rd);
    sub_bf(tmp2, g_bf_x_3rd, g_bf_x_min);
    sub_bf(tmp3, g_bf_x_max, g_bf_x_3rd);
    sub_bf(tmp4, g_bf_y_3rd, g_bf_y_max);
    mult_bf(tmp5, tmp1, tmp2);
    mult_bf(tmp6, tmp3, tmp4);
    sub_bf(bfftemp, tmp5, tmp6);
    // plotmx1 = (xx3rd-xxmin); */ ; /* reuse the plotxxx vars is safe
    copy_bf(bfplotmx1, tmp2);
    // plotmx2 = (yy3rd-yymax);
    copy_bf(bfplotmx2, tmp4);
    // plotmy1 = (yymin-yy3rd);
    copy_bf(bfplotmy1, tmp1);
    // plotmy2 = (xxmax-xx3rd);
    copy_bf(bfplotmy2, tmp3);

    // savxxmin = xxmin; savyymax = yymax;
    copy_bf(savbfxmin, g_bf_x_min);
    copy_bf(savbfymax, g_bf_y_max);

    sub_bf(tmp1, g_bf_save_x_min, savbfxmin);
    sub_bf(tmp2, g_bf_save_y_max, savbfymax);
    zoom_out_calc(tmp1, tmp2, g_bf_x_min, g_bf_y_max, bfplotmx1, bfplotmx2, bfplotmy1, bfplotmy2, bfftemp);
    sub_bf(tmp1, g_bf_save_x_max, savbfxmin);
    sub_bf(tmp2, g_bf_save_y_min, savbfymax);
    zoom_out_calc(tmp1, tmp2, g_bf_x_max, g_bf_y_min, bfplotmx1, bfplotmx2, bfplotmy1, bfplotmy2, bfftemp);
    sub_bf(tmp1, g_bf_save_x_3rd, savbfxmin);
    sub_bf(tmp2, g_bf_save_y_3rd, savbfymax);
    zoom_out_calc(tmp1, tmp2, g_bf_x_3rd, g_bf_y_3rd, bfplotmx1, bfplotmx2, bfplotmy1, bfplotmy2, bfftemp);
    restore_stack(saved);
}

static void zoom_out_dbl() // for ctl-enter, calc corners for zooming out
{
    /* (xxmin,yymax), etc, are already set to zoombox corners;
       (sxmin,symax), etc, are still the screen's corners;
       use the same logic as plot_orbit stuff to first calculate current screen
       corners relative to the zoombox, as if the zoombox were a square with
       upper left (0,0) and width/depth 1; ie calc the current screen corners
       as if plotting them from the zoombox;
       then extend these co-ords from current real screen corners to get
       new actual corners
       */
    const double ftemp = (g_y_min - g_y_3rd) * (g_x_3rd - g_x_min) - (g_x_max - g_x_3rd) * (g_y_3rd - g_y_max);
    g_plot_mx1 = (g_x_3rd-g_x_min); // reuse the plotxxx vars is safe
    g_plot_mx2 = (g_y_3rd-g_y_max);
    g_plot_my1 = (g_y_min-g_y_3rd);
    g_plot_my2 = (g_x_max - g_x_3rd);
    const double savxxmin = g_x_min;
    const double savyymax = g_y_max;
    zoom_out_calc(g_save_x_min-savxxmin, g_save_y_max-savyymax, &g_x_min, &g_y_max, ftemp);
    zoom_out_calc(g_save_x_max-savxxmin, g_save_y_min-savyymax, &g_x_max, &g_y_min, ftemp);
    zoom_out_calc(g_save_x_3rd-savxxmin, g_save_y_3rd-savyymax, &g_x_3rd, &g_y_3rd, ftemp);
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

void aspect_ratio_crop(float old_aspect, float new_aspect)
{
    double ftemp;
    double xmargin;
    double ymargin;
    if (new_aspect > old_aspect)
    {
        // new ratio is taller, crop x
        ftemp = (1.0 - old_aspect / new_aspect) / 2;
        xmargin = (g_x_max - g_x_3rd) * ftemp;
        ymargin = (g_y_min - g_y_3rd) * ftemp;
        g_x_3rd += xmargin;
        g_y_3rd += ymargin;
    }
    else
    {
        // new ratio is wider, crop y
        ftemp = (1.0 - new_aspect / old_aspect) / 2;
        xmargin = (g_x_3rd - g_x_min) * ftemp;
        ymargin = (g_y_3rd - g_y_max) * ftemp;
        g_x_3rd -= xmargin;
        g_y_3rd -= ymargin;
    }
    g_x_min += xmargin;
    g_y_max += ymargin;
    g_x_max -= xmargin;
    g_y_min -= ymargin;
}

static int check_pan() // return 0 if can't, alignment requirement if can
{
    if ((g_calc_status != CalcStatus::RESUMABLE && g_calc_status != CalcStatus::COMPLETED) ||
        g_evolving != EvolutionModeFlags::NONE)
    {
        return 0; // not resumable, not complete
    }
    if (g_cur_fractal_specific->calctype != standard_fractal
        && g_cur_fractal_specific->calctype != calc_mand
        && g_cur_fractal_specific->calctype != calc_mand_fp
        && g_cur_fractal_specific->calctype != lyapunov
        && g_cur_fractal_specific->calctype != calc_froth)
    {
        return 0; // not a worklist-driven type
    }
    if (g_zoom_box_width != 1.0 || g_zoom_box_height != 1.0
        || g_zoom_box_skew != 0.0 || g_zoom_box_rotation != 0.0)
    {
        return 0; // not a full size unrotated unskewed zoombox
    }
    if (g_std_calc_mode == 't')
    {
        return 0; // tesselate, can't do it
    }
    if (g_std_calc_mode == 'd')
    {
        return 0; // diffusion scan: can't do it either
    }
    if (g_std_calc_mode == 'o')
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
    if (g_std_calc_mode == 'b')
    {
        return 1; // btm, align on any pixel
    }
    if (g_std_calc_mode != 'g' || bit_set(g_cur_fractal_specific->flags, FractalFlags::NOGUESS))
    {
        if (g_std_calc_mode == '2' || g_std_calc_mode == '3')   // align on even pixel for 2pass
        {
            return 2;
        }
        return 1; // assume 1pass
    }
    // solid guessing
    start_resume();
    get_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    // don't do end_resume! we're just looking
    int i = 9;
    for (int j = 0; j < g_num_work_list; ++j)   // find lowest pass in any pending window
    {
        if (g_work_list[j].pass < i)
        {
            i = g_work_list[j].pass;
        }
    }
    int j = ssg_blocksize(); // worst-case alignment requirement
    while (--i >= 0)
    {
        j = j >> 1; // reduce requirement
    }
    return j;
}

// move a row on the screen
static void move_row(int fromrow, int torow, int col)
{
    std::vector<BYTE> temp(g_logical_screen_x_dots, 0);
    if (fromrow >= 0 && fromrow < g_logical_screen_y_dots)
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
        read_span(fromrow, startcol, endcol, &temp[tocol]);
    }
    write_span(torow, 0, g_logical_screen_x_dots-1, temp.data());
}

// decide to recalc, or to chg worklist & pan
int init_pan_or_recalc(bool do_zoom_out)
{
    // no zoombox, leave g_calc_status as is
    if (g_zoom_box_width == 0.0)
    {
        return 0;
    }
    // got a zoombox
    const int align_mask = check_pan() - 1;

    // can't pan, trigger recalc
    if (align_mask < 0 || g_evolving != EvolutionModeFlags::NONE)
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        return 0;
    }

    // box is full screen, leave g_calc_status as is
    if (g_zoom_box_x == 0.0 && g_zoom_box_y == 0.0)
    {
        clear_box();
        return 0;
    }

    int col = (int) (g_zoom_box_x *
        (g_logical_screen_x_size_dots + PIXELROUND)); // calc dest col,row of topleft pixel
    int row = (int) (g_zoom_box_y * (g_logical_screen_y_size_dots + PIXELROUND));
    if (do_zoom_out)
    {
        // invert row and col
        row = -row;
        col = -col;
    }
    if ((row&align_mask) != 0 || (col&align_mask) != 0)
    {
        g_calc_status = CalcStatus::PARAMS_CHANGED; // not on useable pixel alignment, trigger recalc
        return 0;
    }

    // pan
    g_num_work_list = 0;
    if (g_calc_status == CalcStatus::RESUMABLE)
    {
        start_resume();
        get_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
        // don't do end_resume! we might still change our mind
    }
    // adjust existing worklist entries
    for (int i = 0; i < g_num_work_list; ++i)
    {
        g_work_list[i].yystart -= row;
        g_work_list[i].yystop  -= row;
        g_work_list[i].yybegin -= row;
        g_work_list[i].xxstart -= col;
        g_work_list[i].xxstop  -= col;
        g_work_list[i].xxbegin -= col;
    }
    // add worklist entries for the new edges
    int listfull = 0;
    int i = 0;
    int j = g_logical_screen_y_dots-1;
    if (row < 0)
    {
        listfull |= add_work_list(0, g_logical_screen_x_dots-1, 0, 0, 0-row-1, 0, 0, 0);
        i = -row;
    }
    if (row > 0)
    {
        listfull |= add_work_list(0, g_logical_screen_x_dots-1, 0, g_logical_screen_y_dots-row, g_logical_screen_y_dots-1, g_logical_screen_y_dots-row, 0, 0);
        j = g_logical_screen_y_dots - row - 1;
    }
    if (col < 0)
    {
        listfull |= add_work_list(0, 0-col-1, 0, i, j, i, 0, 0);
    }
    if (col > 0)
    {
        listfull |= add_work_list(g_logical_screen_x_dots-col, g_logical_screen_x_dots-1, g_logical_screen_x_dots-col, i, j, i, 0, 0);
    }
    if (listfull != 0)
    {
        if (stop_msg(stopmsg_flags::CANCEL,
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
        return 0;
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
    put_resume(sizeof(g_num_work_list), &g_num_work_list, sizeof(g_work_list), g_work_list, 0);
    return 0;
}

// force a worklist entry to restart
static void restart_window(int index)
{
    const int y_start = std::max(0, g_work_list[index].yystart);
    const int x_start = std::max(0, g_work_list[index].xxstart);
    const int y_stop = std::min(g_logical_screen_y_dots - 1, g_work_list[index].yystop);
    const int x_stop = std::min(g_logical_screen_x_dots - 1, g_work_list[index].xxstop);
    std::vector<BYTE> temp(g_logical_screen_x_dots, 0);
    for (int y = y_start; y <= y_stop; ++y)
    {
        write_span(y, x_start, x_stop, temp.data());
    }
    g_work_list[index].pass = 0;
    g_work_list[index].sym = g_work_list[index].pass;
    g_work_list[index].yybegin = g_work_list[index].yystart;
    g_work_list[index].xxbegin = g_work_list[index].xxstart;
}

// fix out of bounds and symmetry related stuff
static void fix_work_list()
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        WorkList *wk = &g_work_list[i];
        if (wk->yystart >= g_logical_screen_y_dots || wk->yystop < 0
            || wk->xxstart >= g_logical_screen_x_dots || wk->xxstop < 0)
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
        if (wk->yystart < 0)
        {
            // partly off top edge
            if ((wk->sym&1) == 0)
            {
                // no sym, easy
                wk->yystart = 0;
                wk->xxbegin = 0;
            }
            else
            {
                // xaxis symmetry
                int j = wk->yystop + wk->yystart;
                if (j > 0 && g_num_work_list < MAX_CALC_WORK)
                {
                    // split the sym part
                    g_work_list[g_num_work_list] = g_work_list[i];
                    g_work_list[g_num_work_list].yystart = 0;
                    g_work_list[g_num_work_list++].yystop = j;
                    wk->yystart = j+1;
                }
                else
                {
                    wk->yystart = 0;
                }
                restart_window(i); // restart the no-longer sym part
            }
        }
        if (wk->yystop >= g_logical_screen_y_dots)
        {
            // partly off bottom edge
            int j = g_logical_screen_y_dots-1;
            if ((wk->sym&1) != 0)
            {
                // uses xaxis symmetry
                int k = wk->yystart + (wk->yystop - j);
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
                        g_work_list[g_num_work_list].yystart = k;
                        g_work_list[g_num_work_list++].yystop = j;
                        j = k-1;
                    }
                }
                wk->sym &= -1 - 1;
            }
            wk->yystop = j;
        }
        if (wk->xxstart < 0)
        {
            // partly off left edge
            if ((wk->sym&2) == 0)   // no sym, easy
            {
                wk->xxstart = 0;
            }
            else
            {
                // yaxis symmetry
                int j = wk->xxstop + wk->xxstart;
                if (j > 0 && g_num_work_list < MAX_CALC_WORK)
                {
                    // split the sym part
                    g_work_list[g_num_work_list] = g_work_list[i];
                    g_work_list[g_num_work_list].xxstart = 0;
                    g_work_list[g_num_work_list++].xxstop = j;
                    wk->xxstart = j+1;
                }
                else
                {
                    wk->xxstart = 0;
                }
                restart_window(i); // restart the no-longer sym part
            }
        }
        if (wk->xxstop >= g_logical_screen_x_dots)
        {
            // partly off right edge
            int j = g_logical_screen_x_dots-1;
            if ((wk->sym&2) != 0)
            {
                // uses xaxis symmetry
                int k = wk->xxstart + (wk->xxstop - j);
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
                        g_work_list[g_num_work_list].xxstart = k;
                        g_work_list[g_num_work_list++].xxstop = j;
                        j = k-1;
                    }
                }
                wk->sym &= -1 - 2;
            }
            wk->xxstop = j;
        }
        if (wk->yybegin < wk->yystart)
        {
            wk->yybegin = wk->yystart;
        }
        if (wk->yybegin > wk->yystop)
        {
            wk->yybegin = wk->yystop;
        }
        if (wk->xxbegin < wk->xxstart)
        {
            wk->xxbegin = wk->xxstart;
        }
        if (wk->xxbegin > wk->xxstop)
        {
            wk->xxbegin = wk->xxstop;
        }
    }
    tidy_work_list(); // combine where possible, re-sort
}

void clear_zoom_box()
{
    g_zoom_box_width = 0;
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
        move_box((double)horizontal/g_logical_screen_x_size_dots, (double)vertical/g_logical_screen_y_size_dots);
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

MainState skew_zoom_left(MainContext &)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NOROTATE))
    {
        const int i = key_count(ID_KEY_CTL_HOME);
        if ((g_zoom_box_skew -= 0.02 * i) < -0.48)
        {
            g_zoom_box_skew = -0.48;
        }
    }
    return MainState::NOTHING;
}

MainState skew_zoom_right(MainContext &)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NOROTATE))
    {
        const int i = key_count(ID_KEY_CTL_END);
        if ((g_zoom_box_skew += 0.02 * i) > 0.48)
        {
            g_zoom_box_skew = 0.48;
        }
    }
    return MainState::NOTHING;
}

MainState decrease_zoom_aspect(MainContext &)
{
    if (g_box_count)
    {
        change_box(0, -2 * key_count(ID_KEY_CTL_PAGE_UP));
    }
    return MainState::NOTHING;
}

MainState increase_zoom_aspect(MainContext &)
{
    if (g_box_count)
    {
        change_box(0, 2 * key_count(ID_KEY_CTL_PAGE_DOWN));
    }
    return MainState::NOTHING;
}

MainState zoom_box_in(MainContext &)
{
    if (g_zoom_enabled)
    {
        if (g_zoom_box_width == 0)
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

MainState zoom_box_out(MainContext &)
{
    if (g_box_count)
    {
        if (g_zoom_box_width >= .999 && g_zoom_box_height >= 0.999) // end zoombox
        {
            g_zoom_box_width = 0;
        }
        else
        {
            resize_box(key_count(ID_KEY_PAGE_DOWN));
        }
    }
    return MainState::NOTHING;
}

MainState zoom_box_increase_rotation(MainContext &)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NOROTATE))
    {
        g_zoom_box_rotation += key_count(ID_KEY_CTL_MINUS);
    }
    return MainState::NOTHING;
}

MainState zoom_box_decrease_rotation(MainContext &)
{
    if (g_box_count && bit_clear(g_cur_fractal_specific->flags, FractalFlags::NOROTATE))
    {
        g_zoom_box_rotation -= key_count(ID_KEY_CTL_PLUS);
    }
    return MainState::NOTHING;
}

MainState zoom_box_increase_color(MainContext &)
{
    g_box_color += key_count(ID_KEY_CTL_INSERT);
    return MainState::NOTHING;
}

MainState zoom_box_decrease_color(MainContext &)
{
    g_box_color -= key_count(ID_KEY_CTL_DEL);
    return MainState::NOTHING;
}

