// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/solid_guess.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel_limits.h"
#include "engine/work_list.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "ui/video.h"

#include <algorithm>
#include <cstring>

using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

// MAX_X_BLK defn must match fracsubr.c
enum
{
    MAX_Y_BLK = 7,  // MAX_X_BLK*MAX_Y_BLK*2 <= 4096, the size of "prefix"
    MAX_X_BLK = 202 // each maxnblk is oversize by 2 for a "border"
};

int ssg_block_size()
{
    // blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >=1200
    int block_size = 4;
    int i = 300;
    while (i <= g_logical_screen.y_dots)
    {
        block_size += block_size;
        i += i;
    }
    // increase blocksize if prefix array not big enough
    while (block_size*(MAX_X_BLK-2) < g_logical_screen.x_dots || block_size*(MAX_Y_BLK-2)*16 < g_logical_screen.y_dots)
    {
        block_size += block_size;
    }
    return block_size;
}

namespace id::engine
{

class SolidGuess
{
public:
    SolidGuess();
    SolidGuess(const SolidGuess &rhs) = delete;
    SolidGuess(SolidGuess &&rhs) = delete;
    ~SolidGuess() = default;
    SolidGuess &operator=(const SolidGuess &rhs) = delete;
    SolidGuess &operator=(SolidGuess &&rhs) = delete;

    int scan();

private:
    bool guess_row(bool first_pass, int y, int block_size);
    void fill_d_stack(int x1, int x2, Byte value);
    void plot_block(int build_row, int x, int y, int color);

    bool m_guess_plot{};                              // paint 1st pass row at a time?
    bool m_bottom_guess{};                            //
    bool m_right_guess{};                             //
    int m_max_block{};                                //
    int m_half_block{};                               //
    unsigned int m_prefix[2][MAX_Y_BLK][MAX_X_BLK]{}; // common temp
    Byte m_stack[4096]{};                             // common temp, two put_line calls
    int m_block_size{};                               //
};

SolidGuess::SolidGuess() :
    m_guess_plot(g_plot != g_put_color && g_plot != sym_plot2 && g_plot != sym_plot2j),
    // check if guessing at bottom & right edges is ok
    m_bottom_guess(
        g_plot == sym_plot2 || (g_plot == g_put_color && g_i_stop_pt.y + 1 == g_logical_screen.y_dots)),
    m_right_guess(g_plot == sym_plot2j ||
        ((g_plot == g_put_color || g_plot == sym_plot2) && g_i_stop_pt.x + 1 == g_logical_screen.x_dots)),
    m_max_block(ssg_block_size()),
    m_block_size(m_max_block)
{
    // there seems to be a bug in solid guessing at bottom and side
    if (g_debug_flag != DebugFlags::FORCE_SOLID_GUESS_ERROR)
    {
        m_bottom_guess = false;
        m_right_guess = false;
    }

    int i = m_block_size;
    g_total_passes = 1;
    while ((i >>= 1) > 1)
    {
        ++g_total_passes;
    }

    // ensure window top and left are on required boundary, treat window
    // as larger than it really is if necessary (this is the reason symplot
    // routines must check for > xdots/ydots before plotting sym points)
    g_i_start_pt.x &= -1 - (m_max_block-1);
    g_i_start_pt.y = g_begin_pt.y;
    g_i_start_pt.y &= -1 - (m_max_block-1);

    g_passes = Passes::SOLID_GUESS;
}

// Timothy Wegner invented this solid guessing idea and implemented it in
// more or less the overall framework you see here.  Tim added this note
// now in a possibly vain attempt to secure his place in history, because
// Pieter Branderhorst has totally rewritten this routine, incorporating
// a *MUCH* more sophisticated algorithm.  His revised code is not only
// faster, but is also more accurate. Harrumph!
int SolidGuess::scan()
{
    if (g_work_pass == 0) // otherwise first pass already done
    {
        // first pass, calc every blocksize**2 pixel, quarter result & paint it
        g_current_pass = 1;
        if (g_i_start_pt.y <= g_start_pt.y) // first time for this window, init it
        {
            g_current_row = 0;
            std::memset(&m_prefix[1][0][0], 0, MAX_X_BLK*MAX_Y_BLK*2); // noskip flags off
            g_reset_periodicity = true;
            g_row = g_i_start_pt.y;
            for (g_col = g_i_start_pt.x; g_col <= g_i_stop_pt.x; g_col += m_max_block)
            {
                // calc top row
                if (g_calc_type() == -1)
                {
                    add_work_list(g_start_pt.x, g_start_pt.y, //
                        g_stop_pt.x, g_stop_pt.y,             //
                        g_begin_pt.x, g_begin_pt.y,           //
                        0, g_work_symmetry);
                    return 0;
                }
                g_reset_periodicity = false;
            }
        }
        else
        {
            std::memset(&m_prefix[1][0][0], -1, MAX_X_BLK*MAX_Y_BLK*2); // noskip flags on
        }
        for (int y = g_i_start_pt.y; y <= g_i_stop_pt.y; y += m_block_size)
        {
            g_current_row = y;
            int i = 0;
            if (y+m_block_size <= g_i_stop_pt.y)
            {
                // calc the row below
                g_row = y+m_block_size;
                g_reset_periodicity = true;
                for (g_col = g_i_start_pt.x; g_col <= g_i_stop_pt.x; g_col += m_max_block)
                {
                    i = g_calc_type();
                    if (i == -1)
                    {
                        break;
                    }
                    g_reset_periodicity = false;
                }
            }
            g_reset_periodicity = false;
            if (i == -1 || guess_row(true, y, m_block_size)) // interrupted?
            {
                y = std::max(y, g_start_pt.y);
                add_work_list(g_start_pt.x, g_start_pt.y,                //
                    g_stop_pt.x, g_stop_pt.y,                            //
                    g_start_pt.x, y,                                     //
                    0, g_work_symmetry);
                return 0;
            }
        }

        if (g_num_work_list) // work list not empty, just do 1st pass
        {
            add_work_list(g_start_pt.x, g_start_pt.y, //
                g_stop_pt.x, g_stop_pt.y,             //
                g_start_pt.x, g_start_pt.y,           //
                1, g_work_symmetry);
            return 0;
        }
        ++g_work_pass;
        g_i_start_pt.y = g_start_pt.y & -1 - (m_max_block - 1);

        // calculate skip flags for skippable blocks
        const int x_lim = (g_i_stop_pt.x + m_max_block) / m_max_block + 1;
        const int y_lim = ((g_i_stop_pt.y + m_max_block) / m_max_block + 15) / 16 + 1;
        if (!m_right_guess)         // no right edge guessing, zap border
        {
            for (int y = 0; y <= y_lim; ++y)
            {
                m_prefix[1][y][x_lim] = 0xffff;
            }
        }
        if (!m_bottom_guess)      // no bottom edge guessing, zap border
        {
            int i = (g_i_stop_pt.y + m_max_block) / m_max_block + 1;
            const int y = i/16+1;
            i = 1 << (i&15);
            for (int x = 0; x <= x_lim; ++x)
            {
                m_prefix[1][y][x] |= i;
            }
        }
        // set each bit in tprefix[0] to OR of it & surrounding 8 in tprefix[1]
        for (int y = 0; ++y < y_lim;)
        {
            unsigned int *pfx_p0 = &m_prefix[0][y][0];
            const unsigned int *pfx_p1 = &m_prefix[1][y][0];
            for (int x = 0; ++x < x_lim;)
            {
                ++pfx_p1;
                const unsigned int u = *(pfx_p1 - 1) | *pfx_p1 | *(pfx_p1 + 1);
                *++pfx_p0 = u| u >> 1 | u << 1
                             |
                    (*(pfx_p1 - (MAX_X_BLK + 1)) | *(pfx_p1 - MAX_X_BLK) | *(pfx_p1 - (MAX_X_BLK - 1))) >>
                        15
                             |
                    (*(pfx_p1 + (MAX_X_BLK - 1)) | *(pfx_p1 + MAX_X_BLK) | *(pfx_p1 + (MAX_X_BLK + 1))) << 15;
            }
        }
    }
    else   // first pass already done
    {
        std::memset(&m_prefix[0][0][0], -1, MAX_X_BLK*MAX_Y_BLK*2); // noskip flags on
    }
    if (g_three_pass)
    {
        return 0;
    }

    // remaining pass(es), halve blocksize & quarter each blocksize**2
    int i = g_work_pass;
    while (--i > 0)   // allow for already done passes
    {
        m_block_size /= 2;
    }
    g_reset_periodicity = false;
    while ((m_block_size = m_block_size >> 1) >= 2)
    {
        if (g_stop_pass > 0 && g_work_pass >= g_stop_pass)
        {
            return 0;
        }
        g_current_pass = g_work_pass + 1;
        for (int y = g_i_start_pt.y; y <= g_i_stop_pt.y; y += m_block_size)
        {
            g_current_row = y;
            if (guess_row(false, y, m_block_size))
            {
                y = std::max(y, g_start_pt.y);
                add_work_list(g_start_pt.x, g_start_pt.y, //
                    g_stop_pt.x, g_stop_pt.y,             //
                    g_start_pt.x, y,                      //
                    g_work_pass, g_work_symmetry);
                return 0;
            }
        }
        ++g_work_pass;
        if (g_num_work_list // work list not empty, do one pass at a time
            && m_block_size > 2) // if 2, we just did last pass
        {
            add_work_list(g_start_pt.x, g_start_pt.y, //
                g_stop_pt.x, g_stop_pt.y,             //
                g_start_pt.x, g_start_pt.y,           //
                g_work_pass, g_work_symmetry);
            return 0;
        }
        g_i_start_pt.y = g_start_pt.y & -1 - (m_max_block - 1);
    }

    return 0;
}

static int calc_a_dot(const int x, const int y)
{
    g_col = x;
    g_row = y;
    return g_calc_type();
}

bool SolidGuess::guess_row(bool first_pass, int y, int block_size)
{
    int j;
    int x_plus_half;
    int x_plus_block;
    int y_less_block;
    int y_less_half;
    int y_plus_half;
    int y_plus_block;
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
    unsigned int *pfx_ptr;
    unsigned int pfx_mask;

    c42 = 0;  // just for warning
    c41 = 0;
    c44 = 0;

    m_half_block = block_size / 2;
    {
        const int i = y / m_max_block;
        pfx_ptr = &m_prefix[first_pass ? 1 : 0][(i >> 4) + 1][g_i_start_pt.x / m_max_block];
        pfx_mask = 1 << (i & 15);
    }
    y_less_half = y - m_half_block;
    y_less_block = y - block_size; // constants, for speed
    y_plus_half = y + m_half_block;
    y_plus_block = y + block_size;
    prev11 = -1;
    c22 = get_color(g_i_start_pt.x, y);
    c13 = c22;
    c12 = c22;
    c24 = c22;
    c21 = get_color(g_i_start_pt.x, y > 0 ?y_less_half:0);
    c31 = c21;
    if (y_plus_block <= g_i_stop_pt.y)
    {
        c24 = get_color(g_i_start_pt.x, y_plus_block);
    }
    else if (!m_bottom_guess)
    {
        c24 = -1;
    }
    guessed13 = 0;
    guessed12 = 0;

    for (int x = g_i_start_pt.x; x <= g_i_stop_pt.x;)   // increment at end, or when doing continue
    {
        if ((x& m_max_block - 1) == 0)  // time for skip flag stuff
        {
            ++pfx_ptr;
            if (!first_pass && (*pfx_ptr&pfx_mask) == 0)  // check for fast skip
            {
                x += m_max_block;
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

        if (first_pass)    // 1st pass, paint topleft corner
        {
            plot_block(0, x, y, c22);
        }
        // setup variables
        x_plus_half = x + m_half_block;
        x_plus_block = x_plus_half + m_half_block;
        if (x_plus_half > g_i_stop_pt.x)
        {
            if (!m_right_guess)
            {
                c31 = -1;
            }
        }
        else if (y > 0)
        {
            c31 = get_color(x_plus_half, y_less_half);
        }
        if (x_plus_block <= g_i_stop_pt.x)
        {
            if (y_plus_block <= g_i_stop_pt.y)
            {
                c44 = get_color(x_plus_block, y_plus_block);
            }
            c41 = get_color(x_plus_block, y > 0 ?y_less_half:0);
            c42 = get_color(x_plus_block, y);
        }
        else if (!m_right_guess)
        {
            c44 = -1;
            c42 = -1;
            c41 = -1;
        }
        if (y_plus_block > g_i_stop_pt.y)
        {
            c44 = m_bottom_guess ? c42 : -1;
        }

        // guess or calc the remaining 3 quarters of current block
        guessed33 = 1;
        guessed32 = 1;
        guessed23 = 1;
        c33 = c22;
        c32 = c22;
        c23 = c22;
        if (y_plus_half > g_i_stop_pt.y)
        {
            if (!m_bottom_guess)
            {
                c33 = -1;
                c23 = -1;
            }
            guessed33 = -1;
            guessed23 = -1;
            guessed13 = 0;
        }
        if (x_plus_half > g_i_stop_pt.x)
        {
            if (!m_right_guess)
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
                c33 = calc_a_dot(x_plus_half, y_plus_half);
                if (c33 == -1)
                {
                    return true;
                }
                guessed33 = 0;
            }
            if (guessed32 > 0
                && (c32 != c33 || c32 != c42 || c32 != c31 || c32 != c21 || c32 != c41 || c32 != c23))
            {
                c32 = calc_a_dot(x_plus_half, y);
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
                c23 = calc_a_dot(x, y_plus_half);
                if (c23 == -1)
                {
                    return true;
                }
                guessed23 = 0;
                continue;
            }
            break;
        }

        if (first_pass)   // note whether any of block's contents were calculated
        {
            if (guessed23 == 0 || guessed32 == 0 || guessed33 == 0)
            {
                *pfx_ptr |= pfx_mask;
            }
        }

        if (m_half_block > 1)
        {
            // not last pass, check if something to display
            if (first_pass)  // display guessed corners, fill in block
            {
                if (m_guess_plot)
                {
                    if (guessed23 > 0)
                    {
                        g_plot(x, y_plus_half, c23);
                    }
                    if (guessed32 > 0)
                    {
                        g_plot(x_plus_half, y, c32);
                    }
                    if (guessed33 > 0)
                    {
                        g_plot(x_plus_half, y_plus_half, c33);
                    }
                }
                plot_block(1, x, y_plus_half, c23);
                plot_block(0, x_plus_half, y, c32);
                plot_block(1, x_plus_half, y_plus_half, c33);
            }
            else  // repaint changed blocks
            {
                if (c23 != c22)
                {
                    plot_block(-1, x, y_plus_half, c23);
                }
                if (c32 != c22)
                {
                    plot_block(-1, x_plus_half, y, c32);
                }
                if (c33 != c22)
                {
                    plot_block(-1, x_plus_half, y_plus_half, c33);
                }
            }
        }

        // check if some calcs in this block mean earlier guesses need fixing
        fix21 = (c22 != c12 || c22 != c32) && c21 == c22 && c21 == c31 && c21 == prev11 && y > 0 &&
            (x == g_i_start_pt.x || c21 == get_color(x - m_half_block, y_less_block)) &&
            (x_plus_half > g_i_stop_pt.x || c21 == get_color(x_plus_half, y_less_block)) &&
            c21 == get_color(x, y_less_block);
        fix31 = c22 != c32 && c31 == c22 && c31 == c42 && c31 == c21 && c31 == c41 && y > 0 &&
            x_plus_half <= g_i_stop_pt.x && c31 == get_color(x_plus_half, y_less_block) &&
            (x_plus_block > g_i_stop_pt.x || c31 == get_color(x_plus_block, y_less_block)) &&
            c31 == get_color(x, y_less_block);
        prev11 = c31; // for next time around
        if (fix21)
        {
            c21 = calc_a_dot(x, y_less_half);
            if (c21 == -1)
            {
                return true;
            }
            if (m_half_block > 1 && c21 != c22)
            {
                plot_block(-1, x, y_less_half, c21);
            }
        }
        if (fix31)
        {
            c31 = calc_a_dot(x_plus_half, y_less_half);
            if (c31 == -1)
            {
                return true;
            }
            if (m_half_block > 1 && c31 != c22)
            {
                plot_block(-1, x_plus_half, y_less_half, c31);
            }
        }
        if (c23 != c22)
        {
            if (guessed12)
            {
                c12 = calc_a_dot(x - m_half_block, y);
                if (c12 == -1)
                {
                    return true;
                }
                if (m_half_block > 1 && c12 != c22)
                {
                    plot_block(-1, x-m_half_block, y, c12);
                }
            }
            if (guessed13)
            {
                c13 = calc_a_dot(x - m_half_block, y_plus_half);
                if (c13 == -1)
                {
                    return true;
                }
                if (m_half_block > 1 && c13 != c22)
                {
                    plot_block(-1, x-m_half_block, y_plus_half, c13);
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
        x += block_size;
    } // end x loop

    if (!first_pass || m_guess_plot)
    {
        return false;
    }

    // paint rows the fast way
    for (int i = 0; i < m_half_block; ++i)
    {
        j = y+i;
        if (j <= g_i_stop_pt.y)
        {
            write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x]);
        }
        j = y+i+m_half_block;
        if (j <= g_i_stop_pt.y)
        {
            write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x+OLD_MAX_PIXELS]);
        }
        if (driver_key_pressed())
        {
            return true;
        }
    }
    if (g_plot != g_put_color)  // symmetry, just vertical & origin the fast way
    {
        if (g_plot == sym_plot2j)   // origin sym, reverse lines
        {
            int color;
            for (int i = (g_i_stop_pt.x+g_start_pt.x+1)/2; --i >= g_start_pt.x;)
            {
                color = m_stack[i];
                j = g_i_stop_pt.x - (i - g_start_pt.x);
                m_stack[i] = m_stack[j];
                m_stack[j] = static_cast<Byte>(color);
                j += OLD_MAX_PIXELS;
                color = m_stack[i + OLD_MAX_PIXELS];
                m_stack[i + OLD_MAX_PIXELS] = m_stack[j];
                m_stack[j] = static_cast<Byte>(color);
            }
        }
        for (int i = 0; i < m_half_block; ++i)
        {
            j = g_stop_pt.y-(y+i-g_start_pt.y);
            if (j > g_i_stop_pt.y && j < g_logical_screen.y_dots)
            {
                write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x]);
            }
            j = g_stop_pt.y-(y+i+m_half_block-g_start_pt.y);
            if (j > g_i_stop_pt.y && j < g_logical_screen.y_dots)
            {
                write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x+OLD_MAX_PIXELS]);
            }
            if (driver_key_pressed())
            {
                return true;
            }
        }
    }
    return false;
}

void SolidGuess::fill_d_stack(const int x1, const int x2, const Byte value)
{
    const int begin = std::min(x1, x2);
    const int end = std::max(x1, x2);
    std::fill(&m_stack[begin], &m_stack[end], value);
}

void SolidGuess::plot_block(const int build_row, const int x, int y, const int color)
{
    int x_lim = x + m_half_block;
    if (x_lim > g_i_stop_pt.x)
    {
        x_lim = g_i_stop_pt.x+1;
    }
    if (build_row >= 0 && !m_guess_plot) // save it for later put_line
    {
        if (build_row == 0)
        {
            fill_d_stack(x, x_lim, static_cast<Byte>(color));
        }
        else
        {
            fill_d_stack(x + OLD_MAX_PIXELS, x_lim + OLD_MAX_PIXELS, static_cast<Byte>(color));
        }
        if (x >= g_start_pt.x)   // when x reduced for alignment, paint those dots too
        {
            return; // the usual case
        }
    }
    // paint it
    int y_lim = y + m_half_block;
    if (y_lim > g_i_stop_pt.y)
    {
        if (y > g_i_stop_pt.y)
        {
            return;
        }
        y_lim = g_i_stop_pt.y+1;
    }
    for (int i = x; ++i < x_lim;)
    {
        g_plot(i, y, color); // skip 1st dot on 1st row
    }
    while (++y < y_lim)
    {
        for (int i = x; i < x_lim; ++i)
        {
            g_plot(i, y, color);
        }
    }
}

} // namespace id::engine

int solid_guess()
{
    id::engine::SolidGuess sg;

    return sg.scan();
}

} // namespace id::engine
