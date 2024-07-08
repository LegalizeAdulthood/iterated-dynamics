#include "solid_guess.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "debug_flags.h"
#include "drivers.h"
#include "get_color.h"
#include "get_line.h"
#include "id_data.h"
#include "pixel_limits.h"
#include "ssg_block_size.h"
#include "work_list.h"

#include <algorithm>
#include <cstring>

// routines in this module
static bool guessrow(bool firstpass, int y, int blocksize);
static void plotblock(int, int, int, int);

static bool guessplot = false;          // paint 1st pass row at a time?
static bool bottom_guess = false;
static bool right_guess = false;
static int maxblock = 0;
static int halfblock = 0;
#define maxyblk 7    // maxxblk*maxyblk*2 <= 4096, the size of "prefix"
#define maxxblk 202  // each maxnblk is oversize by 2 for a "border"
// maxxblk defn must match fracsubr.c
static unsigned int tprefix[2][maxyblk][maxxblk] = { 0 }; // common temp
static BYTE s_stack[4096] = { 0 };              // common temp, two put_line calls

// super solid guessing

// Timothy Wegner invented this solid guessing idea and implemented it in
// more or less the overall framework you see here.  Tim added this note
// now in a possibly vain attempt to secure my place in history, because
// Pieter Branderhorst has totally rewritten this routine, incorporating
// a *MUCH* more sophisticated algorithm.  His revised code is not only
// faster, but is also more accurate. Harrumph!
int solid_guess()
{
    int i;
    int xlim;
    int ylim;
    int blocksize;
    unsigned int *pfxp0;
    unsigned int *pfxp1;
    unsigned int u;

    guessplot = (g_plot != g_put_color && g_plot != symplot2 && g_plot != symplot2J);
    // check if guessing at bottom & right edges is ok
    bottom_guess = (g_plot == symplot2 || (g_plot == g_put_color && g_i_y_stop+1 == g_logical_screen_y_dots));
    right_guess  = (g_plot == symplot2J
        || ((g_plot == g_put_color || g_plot == symplot2) && g_i_x_stop+1 == g_logical_screen_x_dots));

    // there seems to be a bug in solid guessing at bottom and side
    if (g_debug_flag != debug_flags::force_solid_guess_error)
    {
        bottom_guess = false;
        right_guess = false;
    }

    blocksize = ssg_blocksize();
    maxblock = blocksize;
    i = blocksize;
    g_total_passes = 1;
    while ((i >>= 1) > 1)
    {
        ++g_total_passes;
    }

    // ensure window top and left are on required boundary, treat window
    // as larger than it really is if necessary (this is the reason symplot
    // routines must check for > xdots/ydots before plotting sym points)
    g_i_x_start &= -1 - (maxblock-1);
    g_i_y_start = g_yy_begin;
    g_i_y_start &= -1 - (maxblock-1);

    g_got_status = 1;

    if (g_work_pass == 0) // otherwise first pass already done
    {
        // first pass, calc every blocksize**2 pixel, quarter result & paint it
        g_current_pass = 1;
        if (g_i_y_start <= g_yy_start) // first time for this window, init it
        {
            g_current_row = 0;
            std::memset(&tprefix[1][0][0], 0, maxxblk*maxyblk*2); // noskip flags off
            g_reset_periodicity = true;
            g_row = g_i_y_start;
            for (g_col = g_i_x_start; g_col <= g_i_x_stop; g_col += maxblock)
            {
                // calc top row
                if ((*g_calc_type)() == -1)
                {
                    add_worklist(g_xx_start, g_xx_stop, g_xx_begin, g_yy_start, g_yy_stop, g_yy_begin, 0, g_work_symmetry);
                    goto exit_solidguess;
                }
                g_reset_periodicity = false;
            }
        }
        else
        {
            std::memset(&tprefix[1][0][0], -1, maxxblk*maxyblk*2); // noskip flags on
        }
        for (int y = g_i_y_start; y <= g_i_y_stop; y += blocksize)
        {
            g_current_row = y;
            i = 0;
            if (y+blocksize <= g_i_y_stop)
            {
                // calc the row below
                g_row = y+blocksize;
                g_reset_periodicity = true;
                for (g_col = g_i_x_start; g_col <= g_i_x_stop; g_col += maxblock)
                {
                    i = (*g_calc_type)();
                    if (i == -1)
                    {
                        break;
                    }
                    g_reset_periodicity = false;
                }
            }
            g_reset_periodicity = false;
            if (i == -1 || guessrow(true, y, blocksize)) // interrupted?
            {
                if (y < g_yy_start)
                {
                    y = g_yy_start;
                }
                add_worklist(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, y, 0, g_work_symmetry);
                goto exit_solidguess;
            }
        }

        if (g_num_work_list) // work list not empty, just do 1st pass
        {
            add_worklist(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, 1, g_work_symmetry);
            goto exit_solidguess;
        }
        ++g_work_pass;
        g_i_y_start = g_yy_start & (-1 - (maxblock-1));

        // calculate skip flags for skippable blocks
        xlim = (g_i_x_stop+maxblock)/maxblock+1;
        ylim = ((g_i_y_stop+maxblock)/maxblock+15)/16+1;
        if (!right_guess)         // no right edge guessing, zap border
        {
            for (int y = 0; y <= ylim; ++y)
            {
                tprefix[1][y][xlim] = 0xffff;
            }
        }
        if (!bottom_guess)      // no bottom edge guessing, zap border
        {
            i = (g_i_y_stop+maxblock)/maxblock+1;
            int y = i/16+1;
            i = 1 << (i&15);
            for (int x = 0; x <= xlim; ++x)
            {
                tprefix[1][y][x] |= i;
            }
        }
        // set each bit in tprefix[0] to OR of it & surrounding 8 in tprefix[1]
        for (int y = 0; ++y < ylim;)
        {
            pfxp0 = (unsigned int *)&tprefix[0][y][0];
            pfxp1 = (unsigned int *)&tprefix[1][y][0];
            for (int x = 0; ++x < xlim;)
            {
                ++pfxp1;
                u = *(pfxp1-1)|*pfxp1|*(pfxp1+1);
                *(++pfxp0) = u|(u >> 1)|(u << 1)
                             |((*(pfxp1-(maxxblk+1))|*(pfxp1-maxxblk)|*(pfxp1-(maxxblk-1))) >> 15)
                             |((*(pfxp1+(maxxblk-1))|*(pfxp1+maxxblk)|*(pfxp1+(maxxblk+1))) << 15);
            }
        }
    }
    else   // first pass already done
    {
        std::memset(&tprefix[0][0][0], -1, maxxblk*maxyblk*2); // noskip flags on
    }
    if (g_three_pass)
    {
        goto exit_solidguess;
    }

    // remaining pass(es), halve blocksize & quarter each blocksize**2
    i = g_work_pass;
    while (--i > 0)   // allow for already done passes
    {
        blocksize = blocksize >> 1;
    }
    g_reset_periodicity = false;
    while ((blocksize = blocksize >> 1) >= 2)
    {
        if (g_stop_pass > 0)
        {
            if (g_work_pass >= g_stop_pass)
            {
                goto exit_solidguess;
            }
        }
        g_current_pass = g_work_pass + 1;
        for (int y = g_i_y_start; y <= g_i_y_stop; y += blocksize)
        {
            g_current_row = y;
            if (guessrow(false, y, blocksize))
            {
                if (y < g_yy_start)
                {
                    y = g_yy_start;
                }
                add_worklist(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, y, g_work_pass, g_work_symmetry);
                goto exit_solidguess;
            }
        }
        ++g_work_pass;
        if (g_num_work_list // work list not empty, do one pass at a time
            && blocksize > 2) // if 2, we just did last pass
        {
            add_worklist(g_xx_start, g_xx_stop, g_xx_start, g_yy_start, g_yy_stop, g_yy_start, g_work_pass, g_work_symmetry);
            goto exit_solidguess;
        }
        g_i_y_start = g_yy_start & (-1 - (maxblock-1));
    }

exit_solidguess:
    return 0;
}

inline int calc_a_dot(int x, int y)
{
    g_col = x;
    g_row = y;
    return (*g_calc_type)();
}

static bool guessrow(bool firstpass, int y, int blocksize)
{
    int j;
    int color;
    int xplushalf;
    int xplusblock;
    int ylessblock;
    int ylesshalf;
    int yplushalf;
    int yplusblock;
    int c21; // cxy is the color of pixel at (x,y)
    int c31; // where c22 is the top left corner of
    int c41; // the block being handled in current
    int c12; // iteration
    int c22;
    int c32;
    int c42;
    int c13;
    int c23;
    int c33;
    int c24;
    int c44; 
    int guessed23;
    int guessed32;
    int guessed33;
    int guessed12;
    int guessed13;
    int prev11;
    int fix21;
    int fix31;
    unsigned int *pfxptr;
    unsigned int pfxmask;

    c42 = 0;  // just for warning
    c41 = 0;
    c44 = 0;

    halfblock = blocksize >> 1;
    {
        const int i = y / maxblock;
        pfxptr = (unsigned int *) &tprefix[firstpass ? 1 : 0][(i >> 4) + 1][g_i_x_start / maxblock];
        pfxmask = 1 << (i & 15);
    }
    ylesshalf = y - halfblock;
    ylessblock = y - blocksize; // constants, for speed
    yplushalf = y + halfblock;
    yplusblock = y + blocksize;
    prev11 = -1;
    c22 = getcolor(g_i_x_start, y);
    c13 = c22;
    c12 = c22;
    c24 = c22;
    c21 = getcolor(g_i_x_start, (y > 0)?ylesshalf:0);
    c31 = c21;
    if (yplusblock <= g_i_y_stop)
    {
        c24 = getcolor(g_i_x_start, yplusblock);
    }
    else if (!bottom_guess)
    {
        c24 = -1;
    }
    guessed13 = 0;
    guessed12 = 0;

    for (int x = g_i_x_start; x <= g_i_x_stop;)   // increment at end, or when doing continue
    {
        if ((x&(maxblock-1)) == 0)  // time for skip flag stuff
        {
            ++pfxptr;
            if (!firstpass && (*pfxptr&pfxmask) == 0)  // check for fast skip
            {
                x += maxblock;
                c13 = c22;
                c12 = c22;
                c24 = c22;
                c21 = c22;
                c31 = c22;
                prev11 = c22;
                guessed13 = 0;
                guessed12 = 0;
                continue;
            }
        }

        if (firstpass)    // 1st pass, paint topleft corner
        {
            plotblock(0, x, y, c22);
        }
        // setup variables
        xplushalf = x + halfblock;
        xplusblock = xplushalf + halfblock;
        if (xplushalf > g_i_x_stop)
        {
            if (!right_guess)
            {
                c31 = -1;
            }
        }
        else if (y > 0)
        {
            c31 = getcolor(xplushalf, ylesshalf);
        }
        if (xplusblock <= g_i_x_stop)
        {
            if (yplusblock <= g_i_y_stop)
            {
                c44 = getcolor(xplusblock, yplusblock);
            }
            c41 = getcolor(xplusblock, (y > 0)?ylesshalf:0);
            c42 = getcolor(xplusblock, y);
        }
        else if (!right_guess)
        {
            c44 = -1;
            c42 = -1;
            c41 = -1;
        }
        if (yplusblock > g_i_y_stop)
        {
            c44 = bottom_guess ? c42 : -1;
        }

        // guess or calc the remaining 3 quarters of current block
        guessed33 = 1;
        guessed32 = 1;
        guessed23 = 1;
        c33 = c22;
        c32 = c22;
        c23 = c22;
        if (yplushalf > g_i_y_stop)
        {
            if (!bottom_guess)
            {
                c33 = -1;
                c23 = -1;
            }
            guessed33 = -1;
            guessed23 = -1;
            guessed13 = 0;
        }
        if (xplushalf > g_i_x_stop)
        {
            if (!right_guess)
            {
                c33 = -1;
                c32 = -1;
            }
            guessed33 = -1;
            guessed32 = -1;
        }
        while (true) // go around till none of 23,32,33 change anymore
        {
            if (guessed33 > 0
                && (c33 != c44 || c33 != c42 || c33 != c24 || c33 != c32 || c33 != c23))
            {
                c33 = calc_a_dot(xplushalf, yplushalf);
                if (c33 == -1)
                {
                    return true;
                }
                guessed33 = 0;
            }
            if (guessed32 > 0
                && (c32 != c33 || c32 != c42 || c32 != c31 || c32 != c21 || c32 != c41 || c32 != c23))
            {
                c32 = calc_a_dot(xplushalf, y);
                if (c32 == -1)
                {
                    return true;
                }
                guessed32 = 0;
                continue;
            }
            if (guessed23 > 0
                && (c23 != c33 || c23 != c24 || c23 != c13 || c23 != c12 || c23 != c32))
            {
                c23 = calc_a_dot(x, yplushalf);
                if (c23 == -1)
                {
                    return true;
                }
                guessed23 = 0;
                continue;
            }
            break;
        }

        if (firstpass)   // note whether any of block's contents were calculated
        {
            if (guessed23 == 0 || guessed32 == 0 || guessed33 == 0)
            {
                *pfxptr |= pfxmask;
            }
        }

        if (halfblock > 1)
        {
            // not last pass, check if something to display
            if (firstpass)  // display guessed corners, fill in block
            {
                if (guessplot)
                {
                    if (guessed23 > 0)
                    {
                        (*g_plot)(x, yplushalf, c23);
                    }
                    if (guessed32 > 0)
                    {
                        (*g_plot)(xplushalf, y, c32);
                    }
                    if (guessed33 > 0)
                    {
                        (*g_plot)(xplushalf, yplushalf, c33);
                    }
                }
                plotblock(1, x, yplushalf, c23);
                plotblock(0, xplushalf, y, c32);
                plotblock(1, xplushalf, yplushalf, c33);
            }
            else  // repaint changed blocks
            {
                if (c23 != c22)
                {
                    plotblock(-1, x, yplushalf, c23);
                }
                if (c32 != c22)
                {
                    plotblock(-1, xplushalf, y, c32);
                }
                if (c33 != c22)
                {
                    plotblock(-1, xplushalf, yplushalf, c33);
                }
            }
        }

        // check if some calcs in this block mean earlier guesses need fixing
        fix21 = ((c22 != c12 || c22 != c32)
            && c21 == c22 && c21 == c31 && c21 == prev11
            && y > 0
            && (x == g_i_x_start || c21 == getcolor(x-halfblock, ylessblock))
            && (xplushalf > g_i_x_stop || c21 == getcolor(xplushalf, ylessblock))
            && c21 == getcolor(x, ylessblock));
        fix31 = (c22 != c32
            && c31 == c22 && c31 == c42 && c31 == c21 && c31 == c41
            && y > 0 && xplushalf <= g_i_x_stop
            && c31 == getcolor(xplushalf, ylessblock)
            && (xplusblock > g_i_x_stop || c31 == getcolor(xplusblock, ylessblock))
            && c31 == getcolor(x, ylessblock));
        prev11 = c31; // for next time around
        if (fix21)
        {
            c21 = calc_a_dot(x, ylesshalf);
            if (c21 == -1)
            {
                return true;
            }
            if (halfblock > 1 && c21 != c22)
            {
                plotblock(-1, x, ylesshalf, c21);
            }
        }
        if (fix31)
        {
            c31 = calc_a_dot(xplushalf, ylesshalf);
            if (c31 == -1)
            {
                return true;
            }
            if (halfblock > 1 && c31 != c22)
            {
                plotblock(-1, xplushalf, ylesshalf, c31);
            }
        }
        if (c23 != c22)
        {
            if (guessed12)
            {
                c12 = calc_a_dot(x - halfblock, y);
                if (c12 == -1)
                {
                    return true;
                }
                if (halfblock > 1 && c12 != c22)
                {
                    plotblock(-1, x-halfblock, y, c12);
                }
            }
            if (guessed13)
            {
                c13 = calc_a_dot(x - halfblock, yplushalf);
                if (c13 == -1)
                {
                    return true;
                }
                if (halfblock > 1 && c13 != c22)
                {
                    plotblock(-1, x-halfblock, yplushalf, c13);
                }
            }
        }
        c22 = c42;
        c24 = c44;
        c13 = c33;
        c21 = c41;
        c31 = c41;
        c12 = c32;
        guessed12 = guessed32;
        guessed13 = guessed33;
        x += blocksize;
    } // end x loop

    if (!firstpass || guessplot)
    {
        return false;
    }

    // paint rows the fast way
    for (int i = 0; i < halfblock; ++i)
    {
        j = y+i;
        if (j <= g_i_y_stop)
        {
            put_line(j, g_xx_start, g_i_x_stop, &s_stack[g_xx_start]);
        }
        j = y+i+halfblock;
        if (j <= g_i_y_stop)
        {
            put_line(j, g_xx_start, g_i_x_stop, &s_stack[g_xx_start+OLD_MAX_PIXELS]);
        }
        if (driver_key_pressed())
        {
            return true;
        }
    }
    if (g_plot != g_put_color)  // symmetry, just vertical & origin the fast way
    {
        if (g_plot == symplot2J)   // origin sym, reverse lines
        {
            for (int i = (g_i_x_stop+g_xx_start+1)/2; --i >= g_xx_start;)
            {
                color = s_stack[i];
                j = g_i_x_stop - (i - g_xx_start);
                s_stack[i] = s_stack[j];
                s_stack[j] = (BYTE)color;
                j += OLD_MAX_PIXELS;
                color = s_stack[i + OLD_MAX_PIXELS];
                s_stack[i + OLD_MAX_PIXELS] = s_stack[j];
                s_stack[j] = (BYTE)color;
            }
        }
        for (int i = 0; i < halfblock; ++i)
        {
            j = g_yy_stop-(y+i-g_yy_start);
            if (j > g_i_y_stop && j < g_logical_screen_y_dots)
            {
                put_line(j, g_xx_start, g_i_x_stop, &s_stack[g_xx_start]);
            }
            j = g_yy_stop-(y+i+halfblock-g_yy_start);
            if (j > g_i_y_stop && j < g_logical_screen_y_dots)
            {
                put_line(j, g_xx_start, g_i_x_stop, &s_stack[g_xx_start+OLD_MAX_PIXELS]);
            }
            if (driver_key_pressed())
            {
                return true;
            }
        }
    }
    return false;
}

inline void fill_dstack(int x1, int x2, BYTE value)
{
    const int begin = std::min(x1, x2);
    const int end = std::max(x1, x2);
    std::fill(&s_stack[begin], &s_stack[end], value);
}

static void plotblock(int buildrow, int x, int y, int color)
{
    int xlim;
    int ylim;
    xlim = x+halfblock;
    if (xlim > g_i_x_stop)
    {
        xlim = g_i_x_stop+1;
    }
    if (buildrow >= 0 && !guessplot) // save it for later put_line
    {
        if (buildrow == 0)
        {
            fill_dstack(x, xlim, (BYTE) color);
        }
        else
        {
            fill_dstack(x + OLD_MAX_PIXELS, xlim + OLD_MAX_PIXELS, (BYTE) color);
        }
        if (x >= g_xx_start)   // when x reduced for alignment, paint those dots too
        {
            return; // the usual case
        }
    }
    // paint it
    ylim = y+halfblock;
    if (ylim > g_i_y_stop)
    {
        if (y > g_i_y_stop)
        {
            return;
        }
        ylim = g_i_y_stop+1;
    }
    for (int i = x; ++i < xlim;)
    {
        (*g_plot)(i, y, color); // skip 1st dot on 1st row
    }
    while (++y < ylim)
    {
        for (int i = x; i < xlim; ++i)
        {
            (*g_plot)(i, y, color);
        }
    }
}
