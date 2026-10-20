// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/SolidGuess.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel_limits.h"
#include "engine/StandardFractal.h"
#include "engine/work_list.h"
#include "fractals/fractalp.h"
#include "misc/debug_flags.h"
#include "ui/video.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>

using namespace id::misc;
using namespace id::ui;
using namespace id::fractals;

namespace id::engine
{

int g_stop_pass{}; // stop at this guessing pass early

namespace
{

bool consume_standard_traversal_yield()
{
    StandardFractal *standard_fractal{active_standard_fractal()};
    return standard_fractal != nullptr && standard_fractal->consume_traversal_yield();
}

} // namespace

int ssg_block_size()
{
    return SolidGuess::block_size();
}

int SolidGuess::block_size()
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
    while (block_size * (MAX_X_BLK - 2) < g_logical_screen.x_dots ||
        block_size * (MAX_Y_BLK - 2) * 16 < g_logical_screen.y_dots)
    {
        block_size += block_size;
    }
    return block_size;
}

SolidGuess::SolidGuess() :
    m_guess_plot(g_plot != g_put_color && g_plot != sym_plot2 && g_plot != sym_plot2j),
    // check if guessing at bottom & right edges is ok
    m_bottom_guess(g_plot == sym_plot2 || (g_plot == g_put_color && g_i_stop_pt.y + 1 == g_logical_screen.y_dots)),
    m_right_guess(g_plot == sym_plot2j ||
        ((g_plot == g_put_color || g_plot == sym_plot2) && g_i_stop_pt.x + 1 == g_logical_screen.x_dots)),
    m_max_block(ssg_block_size()),
    m_stack_row_offset(std::max(g_logical_screen.x_dots, 1)),
    m_stack(static_cast<std::size_t>(m_stack_row_offset) * 2),
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
    g_i_start_pt.x &= -1 - (m_max_block - 1);
    g_i_start_pt.y = g_begin_pt.y;
    g_i_start_pt.y &= -1 - (m_max_block - 1);

    g_passes = Passes::SOLID_GUESS;
}

bool SolidGuess::iterate()
{
    scan();
    return done();
}

bool SolidGuess::select_pixel()
{
    if (done())
    {
        return false;
    }
    if (!m_pixel_selected)
    {
        process_step();
    }
    if (m_pixel_selected)
    {
        g_col = m_selected_pixel.x;
        g_row = m_selected_pixel.y;
    }
    return m_pixel_selected;
}

math::Point2i SolidGuess::selected_pixel() const
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected solid-guess pixel"};
    }
    return m_selected_pixel;
}

void SolidGuess::complete_pixel(const int color)
{
    if (!m_pixel_selected)
    {
        throw std::logic_error{"No selected solid-guess pixel to complete"};
    }
    g_col = m_selected_pixel.x;
    g_row = m_selected_pixel.y;
    g_color = color;
    g_reset_periodicity = false;
    bool repaint{};
    switch (m_pixel_target)
    {
    case PixelTarget::SEED:
        m_current_x = m_selected_pixel.x + m_max_block;
        break;
    case PixelTarget::C33:
        m_c33 = color;
        m_guessed33 = 0;
        break;
    case PixelTarget::C32:
        m_c32 = color;
        m_guessed32 = 0;
        break;
    case PixelTarget::C23:
        m_c23 = color;
        m_guessed23 = 0;
        break;
    case PixelTarget::C21:
        m_c21 = color;
        repaint = true;
        break;
    case PixelTarget::C31:
        m_c31 = color;
        repaint = true;
        break;
    case PixelTarget::C12:
        m_c12 = color;
        repaint = true;
        break;
    case PixelTarget::C13:
        m_c13 = color;
        repaint = true;
        break;
    }
    if (repaint && m_half_block > 1 && color != m_c22)
    {
        plot_block(-1, m_selected_pixel.x, m_selected_pixel.y, color);
    }
    m_pixel_selected = false;
}

void SolidGuess::suspend()
{
    if (done())
    {
        return;
    }

    switch (m_phase)
    {
    case Phase::FIRST_PASS_TOP_ROW:
        add_work_list(g_start_pt.x, g_start_pt.y, //
            g_stop_pt.x, g_stop_pt.y,             //
            g_begin_pt.x, g_begin_pt.y,           //
            0, g_work_symmetry);
        break;

    case Phase::FIRST_PASS_ROWS:
        save_resume_work(m_current_y, 0);
        break;

    case Phase::REMAINING_ROWS:
        save_resume_work(m_current_y, g_work_pass);
        break;

    default:
        save_deferred_work(g_work_pass);
        break;
    }
    m_phase = Phase::DONE;
}

// Timothy Wegner invented this solid guessing idea and implemented it in
// more or less the overall framework you see here.  Tim added this note
// now in a possibly vain attempt to secure his place in history, because
// Pieter Branderhorst has totally rewritten this routine, incorporating
// a *MUCH* more sophisticated algorithm.  His revised code is not only
// faster, but is also more accurate. Harrumph!
// TODO: removed later
// Slice 26.8 moves calculation to the common StandardFractal boundary.
int SolidGuess::scan()
{
    if (m_row_below_active)
    {
        g_reset_periodicity = true;
    }
    while (select_pixel())
    {
        const int color{calc_type()};
        if (color == -1)
        {
            if (!consume_standard_traversal_yield())
            {
                suspend();
            }
            return 0;
        }
        complete_pixel(m_pixel_target == PixelTarget::SEED ? g_color : color);
    }
    return 0;
}

void SolidGuess::fill_prefix_plane(const int plane, const unsigned int value)
{
    std::fill_n(&m_prefix[plane][0][0], MAX_Y_BLK * MAX_X_BLK, value);
}

bool SolidGuess::done() const
{
    return m_phase == Phase::DONE;
}

void SolidGuess::initialize_scan()
{
    if (g_work_pass == 0)
    {
        m_phase = g_i_start_pt.y <= g_start_pt.y ? Phase::FIRST_PASS_TOP_ROW : Phase::FIRST_PASS_ROWS;
        if (m_phase == Phase::FIRST_PASS_ROWS)
        {
            fill_prefix_plane(1, ~0U); // noskip flags on
        }
        m_current_y = g_i_start_pt.y;
        return;
    }

    fill_prefix_plane(0, ~0U); // noskip flags on
    m_phase = Phase::REMAINING_SETUP;
}

void SolidGuess::process_first_pass_top_row()
{
    // first pass, calc every blocksize**2 pixel, quarter result & paint it
    if (!m_top_row_active)
    {
        g_current_pass = 1;
        g_current_row = 0;
        fill_prefix_plane(1, 0); // noskip flags off
        g_reset_periodicity = true;
        m_current_x = g_i_start_pt.x;
        m_top_row_active = true;
    }
    if (select_seed_pixel(g_i_start_pt.y))
    {
        return;
    }
    m_top_row_active = false;
    m_current_y = g_i_start_pt.y;
    m_phase = Phase::FIRST_PASS_ROWS;
}

void SolidGuess::process_first_pass_rows()
{
    if (m_current_y > g_i_stop_pt.y)
    {
        if (g_num_work_list) // work list not empty, just do 1st pass
        {
            save_deferred_work(1);
            m_phase = Phase::DONE;
            return;
        }
        ++g_work_pass;
        g_i_start_pt.y = g_start_pt.y & -1 - (m_max_block - 1);
        m_phase = Phase::PREFIX_BORDER_SETUP;
        return;
    }

    g_current_pass = 1;
    g_current_row = m_current_y;
    if (!m_guess_row_active && m_current_y + m_block_size <= g_i_stop_pt.y)
    {
        // calc the row below
        if (!m_row_below_active)
        {
            m_current_x = g_i_start_pt.x;
            m_row_below_active = true;
            g_reset_periodicity = true;
        }
        if (select_seed_pixel(m_current_y + m_block_size))
        {
            return;
        }
        m_row_below_active = false;
    }
    g_reset_periodicity = false;
    if (guess_row(true, m_current_y, m_block_size))
    {
        return;
    }
    if (!m_guess_row_done)
    {
        return;
    }
    m_current_y += m_block_size;
}

bool SolidGuess::select_seed_pixel(const int row)
{
    g_row = row;
    g_col = m_current_x;
    if (m_current_x > g_i_stop_pt.x)
    {
        return false;
    }
    select_pixel_at(m_current_x, row, PixelTarget::SEED);
    return true;
}

void SolidGuess::select_pixel_at(const int col, const int row, const PixelTarget target)
{
    m_selected_pixel = {col, row};
    m_pixel_target = target;
    m_pixel_selected = true;
}

void SolidGuess::process_prefix_border_setup()
{
    // calculate skip flags for skippable blocks
    m_prefix_x_lim = (g_i_stop_pt.x + m_max_block) / m_max_block + 1;
    m_prefix_y_lim = ((g_i_stop_pt.y + m_max_block) / m_max_block + 15) / 16 + 1;
    if (!m_right_guess) // no right edge guessing, zap border
    {
        for (int y = 0; y <= m_prefix_y_lim; ++y)
        {
            m_prefix[1][y][m_prefix_x_lim] = 0xffff;
        }
    }
    if (!m_bottom_guess) // no bottom edge guessing, zap border
    {
        int i = (g_i_stop_pt.y + m_max_block) / m_max_block + 1;
        const int y = i / 16 + 1;
        i = 1 << (i & 15);
        for (int x = 0; x <= m_prefix_x_lim; ++x)
        {
            m_prefix[1][y][x] |= i;
        }
    }
    m_prefix_or_y = 0;
    m_phase = Phase::PREFIX_OR_SETUP;
}

void SolidGuess::process_prefix_or_setup()
{
    // set each bit in tprefix[0] to OR of it & surrounding 8 in tprefix[1]
    if (++m_prefix_or_y >= m_prefix_y_lim)
    {
        m_phase = Phase::REMAINING_SETUP;
        return;
    }

    unsigned int *pfx_p0 = &m_prefix[0][m_prefix_or_y][0];
    const unsigned int *pfx_p1 = &m_prefix[1][m_prefix_or_y][0];
    for (int x = 0; ++x < m_prefix_x_lim;)
    {
        ++pfx_p1;
        const unsigned int u = *(pfx_p1 - 1) | *pfx_p1 | *(pfx_p1 + 1);
        *++pfx_p0 = u | u >> 1 | u << 1 |
            (*(pfx_p1 - (MAX_X_BLK + 1)) | *(pfx_p1 - MAX_X_BLK) | *(pfx_p1 - (MAX_X_BLK - 1))) >> 15 |
            (*(pfx_p1 + (MAX_X_BLK - 1)) | *(pfx_p1 + MAX_X_BLK) | *(pfx_p1 + (MAX_X_BLK + 1))) << 15;
    }
}

void SolidGuess::process_remaining_setup()
{
    if (g_three_pass)
    {
        m_phase = Phase::DONE;
        return;
    }

    // remaining pass(es), halve blocksize & quarter each blocksize**2
    int i = g_work_pass;
    while (--i > 0) // allow for already done passes
    {
        m_block_size /= 2;
    }
    g_reset_periodicity = false;
    m_phase = Phase::REMAINING_PASS_START;
}

void SolidGuess::process_remaining_pass_start()
{
    m_block_size >>= 1;
    if (m_block_size < 2)
    {
        m_phase = Phase::DONE;
        return;
    }
    if (g_stop_pass > 0 && g_work_pass >= g_stop_pass)
    {
        m_phase = Phase::DONE;
        return;
    }
    g_current_pass = g_work_pass + 1;
    m_current_y = g_i_start_pt.y;
    m_phase = Phase::REMAINING_ROWS;
}

void SolidGuess::process_remaining_rows()
{
    if (m_current_y > g_i_stop_pt.y)
    {
        ++g_work_pass;
        if (g_num_work_list      // work list not empty, do one pass at a time
            && m_block_size > 2) // if 2, we just did last pass
        {
            save_deferred_work(g_work_pass);
            m_phase = Phase::DONE;
            return;
        }
        g_i_start_pt.y = g_start_pt.y & -1 - (m_max_block - 1);
        m_phase = Phase::REMAINING_PASS_START;
        return;
    }

    g_current_pass = g_work_pass + 1;
    g_current_row = m_current_y;
    if (guess_row(false, m_current_y, m_block_size))
    {
        return;
    }
    if (!m_guess_row_done)
    {
        return;
    }
    m_current_y += m_block_size;
}

void SolidGuess::process_step()
{
    switch (m_phase)
    {
    case Phase::START:
        initialize_scan();
        break;

    case Phase::FIRST_PASS_TOP_ROW:
        process_first_pass_top_row();
        break;

    case Phase::FIRST_PASS_ROWS:
        process_first_pass_rows();
        break;

    case Phase::PREFIX_BORDER_SETUP:
        process_prefix_border_setup();
        break;

    case Phase::PREFIX_OR_SETUP:
        process_prefix_or_setup();
        break;

    case Phase::REMAINING_SETUP:
        process_remaining_setup();
        break;

    case Phase::REMAINING_PASS_START:
        process_remaining_pass_start();
        break;

    case Phase::REMAINING_ROWS:
        process_remaining_rows();
        break;

    case Phase::DONE:
        break;
    }
}

void SolidGuess::save_deferred_work(const int pass)
{
    add_work_list(g_start_pt.x, g_start_pt.y, //
        g_stop_pt.x, g_stop_pt.y,             //
        g_start_pt.x, g_start_pt.y,           //
        pass, g_work_symmetry);
}

void SolidGuess::save_resume_work(const int y, const int pass)
{
    add_work_list(g_start_pt.x, g_start_pt.y,    //
        g_stop_pt.x, g_stop_pt.y,                //
        g_start_pt.x, std::max(y, g_start_pt.y), //
        pass, g_work_symmetry);
}

void SolidGuess::initialize_guess_row(const bool first_pass, const int y, const int block_size)
{
    m_guess_row_active = true;
    m_guess_row_done = false;
    m_guess_row_first_pass = first_pass;
    m_guess_row_y = y;
    m_guess_row_block_size = block_size;
    m_guess_x = g_i_start_pt.x;
    m_guess_block_phase = GuessBlockPhase::START;

    m_half_block = block_size / 2;
    {
        const int i = y / m_max_block;
        m_pfx_ptr = &m_prefix[first_pass ? 1 : 0][(i >> 4) + 1][g_i_start_pt.x / m_max_block];
        m_pfx_mask = 1 << (i & 15);
    }
    m_y_less_half = y - m_half_block;
    m_y_less_block = y - block_size; // constants, for speed
    m_y_plus_half = y + m_half_block;
    m_y_plus_block = y + block_size;
    m_prev11 = -1;
    m_c22 = get_color(g_i_start_pt.x, y);
    m_c13 = m_c22;
    m_c12 = m_c22;
    m_c24 = m_c22;
    m_c21 = get_color(g_i_start_pt.x, y > 0 ? m_y_less_half : 0);
    m_c31 = m_c21;
    m_c42 = 0;
    m_c41 = 0;
    m_c44 = 0;
    if (m_y_plus_block <= g_i_stop_pt.y)
    {
        m_c24 = get_color(g_i_start_pt.x, m_y_plus_block);
    }
    else if (!m_bottom_guess)
    {
        m_c24 = -1;
    }
    m_guessed13 = 0;
    m_guessed12 = 0;
}

bool SolidGuess::guess_row(const bool first_pass, const int y, const int block_size)
{
    if (!m_guess_row_active)
    {
        initialize_guess_row(first_pass, y, block_size);
    }
    while (!m_guess_row_done)
    {
        if (m_guess_x > g_i_stop_pt.x)
        {
            return finish_guess_row();
        }
        if (process_guess_block())
        {
            return true;
        }
    }
    return false;
}

bool SolidGuess::initialize_guess_block()
{
    if ((m_guess_x & (m_max_block - 1)) == 0)                          // time for skip flag stuff
    {
        ++m_pfx_ptr;
        if (!m_guess_row_first_pass && (*m_pfx_ptr & m_pfx_mask) == 0) // check for fast skip
        {
            skip_guess_block();
            return false;
        }
    }

    if (m_guess_row_first_pass) // 1st pass, paint topleft corner
    {
        plot_block(0, m_guess_x, m_guess_row_y, m_c22);
    }

    m_x_plus_half = m_guess_x + m_half_block;
    m_x_plus_block = m_x_plus_half + m_half_block;
    if (m_x_plus_half > g_i_stop_pt.x)
    {
        if (!m_right_guess)
        {
            m_c31 = -1;
        }
    }
    else if (m_guess_row_y > 0)
    {
        m_c31 = get_color(m_x_plus_half, m_y_less_half);
    }
    if (m_x_plus_block <= g_i_stop_pt.x)
    {
        if (m_y_plus_block <= g_i_stop_pt.y)
        {
            m_c44 = get_color(m_x_plus_block, m_y_plus_block);
        }
        m_c41 = get_color(m_x_plus_block, m_guess_row_y > 0 ? m_y_less_half : 0);
        m_c42 = get_color(m_x_plus_block, m_guess_row_y);
    }
    else if (!m_right_guess)
    {
        m_c44 = -1;
        m_c42 = -1;
        m_c41 = -1;
    }
    if (m_y_plus_block > g_i_stop_pt.y)
    {
        m_c44 = m_bottom_guess ? m_c42 : -1;
    }

    m_guessed33 = 1;
    m_guessed32 = 1;
    m_guessed23 = 1;
    m_c33 = m_c22;
    m_c32 = m_c22;
    m_c23 = m_c22;
    if (m_y_plus_half > g_i_stop_pt.y)
    {
        if (!m_bottom_guess)
        {
            m_c33 = -1;
            m_c23 = -1;
        }
        m_guessed33 = -1;
        m_guessed23 = -1;
        m_guessed13 = 0;
    }
    if (m_x_plus_half > g_i_stop_pt.x)
    {
        if (!m_right_guess)
        {
            m_c33 = -1;
            m_c32 = -1;
        }
        m_guessed33 = -1;
        m_guessed32 = -1;
    }
    m_guess_block_phase = GuessBlockPhase::GUESS;
    return true;
}

bool SolidGuess::process_guess_block()
{
    if (m_guess_block_phase == GuessBlockPhase::START && !initialize_guess_block())
    {
        return false;
    }
    if (m_guess_block_phase == GuessBlockPhase::GUESS)
    {
        // Revisit the remaining guesses after each completion.
        if (m_guessed33 > 0 && (m_c33 != m_c44 || m_c33 != m_c42 || m_c33 != m_c24 || m_c33 != m_c32 || m_c33 != m_c23))
        {
            select_pixel_at(m_x_plus_half, m_y_plus_half, PixelTarget::C33);
            return true;
        }
        if (m_guessed32 > 0 &&
            (m_c32 != m_c33 || m_c32 != m_c42 || m_c32 != m_c31 || m_c32 != m_c21 || m_c32 != m_c41 || m_c32 != m_c23))
        {
            select_pixel_at(m_x_plus_half, m_guess_row_y, PixelTarget::C32);
            return true;
        }
        if (m_guessed23 > 0 && (m_c23 != m_c33 || m_c23 != m_c24 || m_c23 != m_c13 || m_c23 != m_c12 || m_c23 != m_c32))
        {
            select_pixel_at(m_guess_x, m_y_plus_half, PixelTarget::C23);
            return true;
        }
        if (m_guess_row_first_pass && (m_guessed23 == 0 || m_guessed32 == 0 || m_guessed33 == 0))
        {
            *m_pfx_ptr |= m_pfx_mask;
        }
        m_guess_block_phase = GuessBlockPhase::DISPLAY;
    }
    if (m_guess_block_phase == GuessBlockPhase::DISPLAY)
    {
        display_guess_block();
        m_guess_block_phase = GuessBlockPhase::FIX21;
    }
    if (m_guess_block_phase == GuessBlockPhase::FIX21)
    {
        m_guess_block_phase = GuessBlockPhase::FIX31;
        if (m_fix21)
        {
            select_pixel_at(m_guess_x, m_y_less_half, PixelTarget::C21);
            return true;
        }
    }
    if (m_guess_block_phase == GuessBlockPhase::FIX31)
    {
        m_guess_block_phase = GuessBlockPhase::FIX12;
        if (m_fix31)
        {
            select_pixel_at(m_x_plus_half, m_y_less_half, PixelTarget::C31);
            return true;
        }
    }
    if (m_guess_block_phase == GuessBlockPhase::FIX12)
    {
        m_guess_block_phase = GuessBlockPhase::FIX13;
        if (m_c23 != m_c22 && m_guessed12)
        {
            select_pixel_at(m_guess_x - m_half_block, m_guess_row_y, PixelTarget::C12);
            return true;
        }
    }
    if (m_guess_block_phase == GuessBlockPhase::FIX13)
    {
        m_guess_block_phase = GuessBlockPhase::ADVANCE;
        if (m_c23 != m_c22 && m_guessed13)
        {
            select_pixel_at(m_guess_x - m_half_block, m_y_plus_half, PixelTarget::C13);
            return true;
        }
    }
    step_guess_block();
    return false;
}

void SolidGuess::display_guess_block()
{
    if (m_half_block > 1)
    {
        // not last pass, check if something to display
        if (m_guess_row_first_pass) // display guessed corners, fill in block
        {
            if (m_guess_plot)
            {
                if (m_guessed23 > 0)
                {
                    g_plot(m_guess_x, m_y_plus_half, m_c23);
                }
                if (m_guessed32 > 0)
                {
                    g_plot(m_x_plus_half, m_guess_row_y, m_c32);
                }
                if (m_guessed33 > 0)
                {
                    g_plot(m_x_plus_half, m_y_plus_half, m_c33);
                }
            }
            plot_block(1, m_guess_x, m_y_plus_half, m_c23);
            plot_block(0, m_x_plus_half, m_guess_row_y, m_c32);
            plot_block(1, m_x_plus_half, m_y_plus_half, m_c33);
        }
        else // repaint changed blocks
        {
            if (m_c23 != m_c22)
            {
                plot_block(-1, m_guess_x, m_y_plus_half, m_c23);
            }
            if (m_c32 != m_c22)
            {
                plot_block(-1, m_x_plus_half, m_guess_row_y, m_c32);
            }
            if (m_c33 != m_c22)
            {
                plot_block(-1, m_x_plus_half, m_y_plus_half, m_c33);
            }
        }
    }

    // check if some calcs in this block mean earlier guesses need fixing
    m_fix21 = (m_c22 != m_c12 || m_c22 != m_c32) && m_c21 == m_c22 && m_c21 == m_c31 && m_c21 == m_prev11 &&
        m_guess_row_y > 0 &&
        (m_guess_x == g_i_start_pt.x || m_c21 == get_color(m_guess_x - m_half_block, m_y_less_block)) &&
        (m_x_plus_half > g_i_stop_pt.x || m_c21 == get_color(m_x_plus_half, m_y_less_block)) &&
        m_c21 == get_color(m_guess_x, m_y_less_block);
    m_fix31 = m_c22 != m_c32 && m_c31 == m_c22 && m_c31 == m_c42 && m_c31 == m_c21 && m_c31 == m_c41 &&
        m_guess_row_y > 0 && m_x_plus_half <= g_i_stop_pt.x && m_c31 == get_color(m_x_plus_half, m_y_less_block) &&
        (m_x_plus_block > g_i_stop_pt.x || m_c31 == get_color(m_x_plus_block, m_y_less_block)) &&
        m_c31 == get_color(m_guess_x, m_y_less_block);
    m_prev11 = m_c31; // for next time around
}

void SolidGuess::skip_guess_block()
{
    m_guess_x += m_max_block;
    m_c13 = m_c22;
    m_c12 = m_c22;
    m_c24 = m_c22;
    m_c21 = m_c22;
    m_c31 = m_c22;
    m_prev11 = m_c22;
    m_guessed13 = 0;
    m_guessed12 = 0;
    m_guess_block_phase = GuessBlockPhase::START;
}

void SolidGuess::step_guess_block()
{
    m_c22 = m_c42;
    m_c24 = m_c44;
    m_c13 = m_c33;
    m_c21 = m_c41;
    m_c31 = m_c41;
    m_c12 = m_c32;
    m_guessed12 = m_guessed32;
    m_guessed13 = m_guessed33;
    m_guess_x += m_guess_row_block_size;
    m_guess_block_phase = GuessBlockPhase::START;
}

bool SolidGuess::finish_guess_row()
{
    if (!m_guess_row_first_pass || m_guess_plot)
    {
        m_guess_row_active = false;
        m_guess_row_done = true;
        return false;
    }

    // paint rows the fast way
    for (int i = 0; i < m_half_block; ++i)
    {
        int j = m_guess_row_y + i;
        if (j <= g_i_stop_pt.y)
        {
            write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x]);
        }
        j = m_guess_row_y + i + m_half_block;
        if (j <= g_i_stop_pt.y)
        {
            write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x + m_stack_row_offset]);
        }
    }
    if (g_plot != g_put_color)    // symmetry, just vertical & origin the fast way
    {
        if (g_plot == sym_plot2j) // origin sym, reverse lines
        {
            for (int i = (g_i_stop_pt.x + g_start_pt.x + 1) / 2; --i >= g_start_pt.x;)
            {
                int color = m_stack[i];
                int j = g_i_stop_pt.x - (i - g_start_pt.x);
                m_stack[i] = m_stack[j];
                m_stack[j] = static_cast<Byte>(color);
                j += m_stack_row_offset;
                color = m_stack[i + m_stack_row_offset];
                m_stack[i + m_stack_row_offset] = m_stack[j];
                m_stack[j] = static_cast<Byte>(color);
            }
        }
        for (int i = 0; i < m_half_block; ++i)
        {
            int j = g_stop_pt.y - (m_guess_row_y + i - g_start_pt.y);
            if (j > g_i_stop_pt.y && j < g_logical_screen.y_dots)
            {
                write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x]);
            }
            j = g_stop_pt.y - (m_guess_row_y + i + m_half_block - g_start_pt.y);
            if (j > g_i_stop_pt.y && j < g_logical_screen.y_dots)
            {
                write_span(j, g_start_pt.x, g_i_stop_pt.x, &m_stack[g_start_pt.x + m_stack_row_offset]);
            }
        }
    }
    m_guess_row_active = false;
    m_guess_row_done = true;
    return false;
}

void SolidGuess::fill_d_stack(const int x1, const int x2, const Byte value)
{
    const int begin = std::min(x1, x2);
    const int end = std::max(x1, x2);
    assert(0 <= begin && end <= static_cast<int>(m_stack.size()));
    std::fill(m_stack.begin() + begin, m_stack.begin() + end, value);
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
            fill_d_stack(x + m_stack_row_offset, x_lim + m_stack_row_offset, static_cast<Byte>(color));
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
