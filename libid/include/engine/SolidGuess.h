// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/Point.h"

#include <config/port.h>

#include <vector>

namespace id::engine
{

class SolidGuess
{
public:
    SolidGuess();
    SolidGuess(const SolidGuess &) = delete;
    SolidGuess(SolidGuess &&) = delete;
    ~SolidGuess() = default;
    SolidGuess &operator=(const SolidGuess &) = delete;
    SolidGuess &operator=(SolidGuess &&) = delete;

    // False ends this work chunk; done() distinguishes completion.
    // TODO: removed later
    // Slice 26.4 extends selection from seed rows to block probes.
    bool select_pixel();
    math::Point2i selected_pixel() const;
    void complete_pixel(int color);
    bool done() const;
    bool iterate();
    // TODO: removed later
    // Slice 26.8 moves calculation to the common StandardFractal boundary.
    int scan();
    void suspend();
    static int block_size();

private:
    enum
    {
        MAX_Y_BLK = 7,
        MAX_X_BLK = 202
    };

    enum class Phase
    {
        START,
        FIRST_PASS_TOP_ROW,
        FIRST_PASS_ROWS,
        PREFIX_BORDER_SETUP,
        PREFIX_OR_SETUP,
        REMAINING_SETUP,
        REMAINING_PASS_START,
        REMAINING_ROWS,
        DONE
    };

    enum class GuessBlockPhase
    {
        START,
        SKIP_CHECK,
        SETUP,
        GUESS,
        DISPLAY,
        FIXUP,
        ADVANCE,
        DONE
    };

    void fill_prefix_plane(int plane, unsigned int value);
    bool finish_guess_row();
    bool guess_row(bool first_pass, int y, int block_size);
    void fill_d_stack(int x1, int x2, Byte value);
    void initialize_scan();
    void initialize_guess_row(bool first_pass, int y, int block_size);
    bool process_guess_block();
    void process_first_pass_rows();
    void process_first_pass_top_row();
    void process_prefix_border_setup();
    void process_prefix_or_setup();
    void process_remaining_pass_start();
    void process_remaining_rows();
    void process_remaining_setup();
    void process_step();
    void plot_block(int build_row, int x, int y, int color);
    void save_deferred_work(int pass);
    void save_resume_work(int y, int pass);
    bool select_seed_pixel(int row);
    void skip_guess_block();
    void step_guess_block();

    Phase m_phase{Phase::START};
    GuessBlockPhase m_guess_block_phase{GuessBlockPhase::START};
    bool m_guess_plot{};
    bool m_bottom_guess{};
    bool m_right_guess{};
    bool m_guess_row_active{};
    bool m_guess_row_done{};
    bool m_guess_row_first_pass{};
    bool m_row_below_active{};
    bool m_top_row_active{};
    bool m_pixel_selected{};
    math::Point2i m_selected_pixel{};
    int m_max_block{};
    int m_half_block{};
    unsigned int m_prefix[2][MAX_Y_BLK][MAX_X_BLK]{};
    int m_current_y{};
    int m_prefix_or_y{};
    int m_prefix_x_lim{};
    int m_prefix_y_lim{};
    int m_stack_row_offset{};
    std::vector<Byte> m_stack;
    int m_block_size{};
    int m_current_x{};
    int m_guess_row_y{};
    int m_guess_row_block_size{};
    int m_guess_x{};
    int m_x_plus_half{};
    int m_x_plus_block{};
    int m_y_less_block{};
    int m_y_less_half{};
    int m_y_plus_half{};
    int m_y_plus_block{};
    int m_c21{};
    int m_c31{};
    int m_c41{};
    int m_c12{};
    int m_c22{};
    int m_c32{};
    int m_c42{};
    int m_c13{};
    int m_c23{};
    int m_c33{};
    int m_c24{};
    int m_c44{};
    int m_guessed23{};
    int m_guessed32{};
    int m_guessed33{};
    int m_guessed12{};
    int m_guessed13{};
    int m_prev11{};
    int m_fix21{};
    int m_fix31{};
    unsigned int *m_pfx_ptr{};
    unsigned int m_pfx_mask{};
};

extern int g_stop_pass; // stop at this guessing pass early

// used by solid guessing and by zoom panning
int ssg_block_size();

} // namespace id::engine
