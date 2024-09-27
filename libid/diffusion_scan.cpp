// SPDX-License-Identifier: GPL-3.0-only
//
#include "diffusion_scan.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "work_list.h"

#include <algorithm>
#include <cmath>
#include <cstring>

// routines in this module
static int diffusion_engine();

// lookup tables to avoid too much bit fiddling :
static char s_dif_la[] =
{
    0, 8, 0, 8,4,12,4,12,0, 8, 0, 8,4,12,4,12, 2,10, 2,10,6,14,6,14,2,10,
    2,10, 6,14,6,14,0, 8,0, 8, 4,12,4,12,0, 8, 0, 8, 4,12,4,12,2,10,2,10,
    6,14, 6,14,2,10,2,10,6,14, 6,14,1, 9,1, 9, 5,13, 5,13,1, 9,1, 9,5,13,
    5,13, 3,11,3,11,7,15,7,15, 3,11,3,11,7,15, 7,15, 1, 9,1, 9,5,13,5,13,
    1, 9, 1, 9,5,13,5,13,3,11, 3,11,7,15,7,15, 3,11, 3,11,7,15,7,15,0, 8,
    0, 8, 4,12,4,12,0, 8,0, 8, 4,12,4,12,2,10, 2,10, 6,14,6,14,2,10,2,10,
    6,14, 6,14,0, 8,0, 8,4,12, 4,12,0, 8,0, 8, 4,12, 4,12,2,10,2,10,6,14,
    6,14, 2,10,2,10,6,14,6,14, 1, 9,1, 9,5,13, 5,13, 1, 9,1, 9,5,13,5,13,
    3,11, 3,11,7,15,7,15,3,11, 3,11,7,15,7,15, 1, 9, 1, 9,5,13,5,13,1, 9,
    1, 9, 5,13,5,13,3,11,3,11, 7,15,7,15,3,11, 3,11, 7,15,7,15
};

static char s_dif_lb[] =
{
    0, 8, 8, 0, 4,12,12, 4, 4,12,12, 4, 8, 0, 0, 8, 2,10,10, 2, 6,14,14,
    6, 6,14,14, 6,10, 2, 2,10, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2,
    2,10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 1, 9, 9, 1, 5,
    13,13, 5, 5,13,13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,
    11, 3, 3,11, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13,
    5, 9, 1, 1, 9, 9, 1, 1, 9,13, 5, 5,13, 1, 9, 9, 1, 5,13,13, 5, 5,13,
    13, 5, 9, 1, 1, 9, 3,11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 3,
    11,11, 3, 7,15,15, 7, 7,15,15, 7,11, 3, 3,11, 5,13,13, 5, 9, 1, 1, 9,
    9, 1, 1, 9,13, 5, 5,13, 2,10,10, 2, 6,14,14, 6, 6,14,14, 6,10, 2, 2,
    10, 4,12,12, 4, 8, 0, 0, 8, 8, 0, 0, 8,12, 4, 4,12, 4,12,12, 4, 8, 0,
    0, 8, 8, 0, 0, 8,12, 4, 4,12, 6,14,14, 6,10, 2, 2,10,10, 2, 2,10,14,
    6, 6,14
};

// next has a skip bit for each maxblock unit;
//   1st pass sets bit  [1]... off only if block's contents guessed;
//   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
//   bits are numbered [..][y/16+1][x+1]&(1<<(y&15))
// size of next puts a limit of MAX_PIXELS pixels across on solid guessing logic
static BYTE s_stack[4096]{}; // common temp, two put_line calls

// vars for diffusion scan
unsigned int g_diffusion_bits{};     // number of bits in the counter
unsigned long g_diffusion_counter{}; // the diffusion counter
unsigned long g_diffusion_limit{};   // the diffusion counter

int diffusion_scan()
{
    double log2;

    log2 = (double) std::log(2.0);

    g_got_status = status_values::DIFFUSION;

    // note: the max size of 2048x2048 gives us a 22 bit counter that will
    // fit any 32 bit architecture, the maxinum limit for this case would
    // be 65536x65536

    g_diffusion_bits = (unsigned)(std::min(std::log(static_cast<double>(g_i_y_stop-g_i_y_start+1)), std::log(static_cast<double>(g_i_x_stop-g_i_x_start+1)))/log2);
    g_diffusion_bits <<= 1; // double for two axes
    g_diffusion_limit = 1UL << g_diffusion_bits;

    if (diffusion_engine() == -1)
    {
        add_worklist(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop,
                     (int)(g_diffusion_counter >> 16),            // high,
                     (int)(g_diffusion_counter & 0xffff),         // low order words
                     g_work_symmetry);
        return -1;
    }

    return 0;
}

// little function that plots a filled square of color c, size s with
// top left cornet at (x,y) with optimization from sym_fill_line
inline void plot_block(int x, int y, int s, int c)
{
    std::memset(s_stack, c, s);
    for (int ty = y; ty < y + s; ty++)
    {
        sym_fill_line(ty, x, x + s - 1, s_stack);
    }
}

// function that does the same as above, but checks the limits in x and y
inline void plot_block_lim(int x, int y, int s, int c)
{
    std::memset(s_stack, c, s);
    for (int ty = y; ty < std::min(y + s, g_i_y_stop + 1); ty++)
    {
        sym_fill_line(ty, x, std::min(x + s - 1, g_i_x_stop), s_stack);
    }
}

// count_to_int(dif_counter, colo, rowo, dif_offset)
inline void count_to_int(unsigned long C, int &x, int &y, int dif_offset)
{
    long unsigned tC = C;
    x = s_dif_la[tC & 0xFF];
    y = s_dif_lb[tC & 0xFF];
    tC >>= 8;
    x <<= 4;
    x += s_dif_la[tC & 0xFF];
    y <<= 4;
    y += s_dif_lb[tC & 0xFF];
    tC >>= 8;
    x <<= 4;
    x += s_dif_la[tC & 0xFF];
    y <<= 4;
    y += s_dif_lb[tC & 0xFF];
    tC >>= 8;
    x >>= dif_offset;
    y >>= dif_offset;
}

// Calculate the point
#define calculate               \
    g_reset_periodicity = true;   \
    if ((*g_calc_type)() == -1)    \
        return -1;              \
    g_reset_periodicity = false

static int diffusion_engine()
{
    double log2 = (double) std::log(2.0);

    int i;
    int j;

    int nx;
    int ny; // number of tyles to build in x and y dirs
    // made this to complete the area that is not
    // a square with sides like 2 ** n
    int rem_x;
    int rem_y; // what is left on the last tile to draw

    int dif_offset; // offset for adjusting looked-up values

    int sqsz;  // size of the block being filled

    int colo;
    int rowo; // original col and row

    int s = 1 << (g_diffusion_bits/2); // size of the square

    nx = (int) floor(static_cast<double>((g_i_x_stop-g_i_x_start+1)/s));
    ny = (int) floor(static_cast<double>((g_i_y_stop-g_i_y_start+1)/s));

    rem_x = (g_i_x_stop-g_i_x_start+1) - nx * s;
    rem_y = (g_i_y_stop-g_i_y_start+1) - ny * s;

    if (g_yy_begin == g_i_y_start && g_work_pass == 0)
    {
        // if restarting on pan:
        g_diffusion_counter =0l;
    }
    else
    {
        // yybegin and passes contain data for resuming the type:
        g_diffusion_counter = (((long)((unsigned)g_yy_begin)) << 16) | ((unsigned)g_work_pass);
    }

    dif_offset = 12-(g_diffusion_bits/2); // offset to adjust coordinates
    // (*) for 4 bytes use 16 for 3 use 12 etc.

    // only the points (dithering only) :
    if (g_fill_color == 0)
    {
        while (g_diffusion_counter < (g_diffusion_limit >> 1))
        {
            count_to_int(g_diffusion_counter, colo, rowo, dif_offset);

            i = 0;
            g_col = g_i_x_start + colo; // get the right tiles
            do
            {
                j = 0;
                g_row = g_i_y_start + rowo ;
                do
                {
                    calculate;
                    (*g_plot)(g_col, g_row, g_color);
                    j++;
                    g_row += s;                  // next tile
                } while (j < ny);
                // in the last y tile we may not need to plot the point
                if (rowo < rem_y)
                {
                    calculate;
                    (*g_plot)(g_col, g_row, g_color);
                }
                i++;
                g_col += s;
            } while (i < nx);
            // in the last x tiles we may not need to plot the point
            if (colo < rem_x)
            {
                g_row = g_i_y_start + rowo;
                j = 0;
                do
                {
                    calculate;
                    (*g_plot)(g_col, g_row, g_color);
                    j++;
                    g_row += s; // next tile
                } while (j < ny);
                if (rowo < rem_y)
                {
                    calculate;
                    (*g_plot)(g_col, g_row, g_color);
                }
            }
            g_diffusion_counter++;
        }
    }
    else
    {
        // with progressive filling :
        while (g_diffusion_counter < (g_diffusion_limit >> 1))
        {
            sqsz = 1 << ((int)(g_diffusion_bits-(int)(std::log(g_diffusion_counter+0.5)/log2)-1)/2);

            count_to_int(g_diffusion_counter, colo, rowo, dif_offset);

            i = 0;
            do
            {
                j = 0;
                do
                {
                    g_col = g_i_x_start + colo + i * s; // get the right tiles
                    g_row = g_i_y_start + rowo + j * s;

                    calculate;
                    plot_block(g_col, g_row, sqsz, g_color);
                    j++;
                } while (j < ny);
                // in the last tile we may not need to plot the point
                if (rowo < rem_y)
                {
                    g_row = g_i_y_start + rowo + ny * s;

                    calculate;
                    plot_block_lim(g_col, g_row, sqsz, g_color);
                }
                i++;
            } while (i < nx);
            // in the last tile we may not need to plot the point
            if (colo < rem_x)
            {
                g_col = g_i_x_start + colo + nx * s;
                j = 0;
                do
                {
                    g_row = g_i_y_start + rowo + j * s; // get the right tiles

                    calculate;
                    plot_block_lim(g_col, g_row, sqsz, g_color);
                    j++;
                } while (j < ny);
                if (rowo < rem_y)
                {
                    g_row = g_i_y_start + rowo + ny * s;

                    calculate;
                    plot_block_lim(g_col, g_row, sqsz, g_color);
                }
            }

            g_diffusion_counter++;
        }
    }
    // from half dif_limit on we only plot 1x1 points :-)
    while (g_diffusion_counter < g_diffusion_limit)
    {
        count_to_int(g_diffusion_counter, colo, rowo, dif_offset);

        i = 0;
        do
        {
            j = 0;
            do
            {
                g_col = g_i_x_start + colo + i * s; // get the right tiles
                g_row = g_i_y_start + rowo + j * s;

                calculate;
                (*g_plot)(g_col, g_row, g_color);
                j++;
            } while (j < ny);
            // in the last tile we may not need to plot the point
            if (rowo < rem_y)
            {
                g_row = g_i_y_start + rowo + ny * s;

                calculate;
                (*g_plot)(g_col, g_row, g_color);
            }
            i++;
        } while (i < nx);
        // in the last tile we may nnt need to plot the point
        if (colo < rem_x)
        {
            g_col = g_i_x_start + colo + nx * s;
            j = 0;
            do
            {
                g_row = g_i_y_start + rowo + j * s; // get the right tiles

                calculate;
                (*g_plot)(g_col, g_row, g_color);
                j++;
            } while (j < ny);
            if (rowo < rem_y)
            {
                g_row = g_i_y_start + rowo + ny * s;

                calculate;
                (*g_plot)(g_col, g_row, g_color);
            }
        }
        g_diffusion_counter++;
    }
    return 0;
}
