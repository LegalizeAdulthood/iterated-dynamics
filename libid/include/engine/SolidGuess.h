// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

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

    bool iterate();
    int scan();
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

    void fill_prefix_plane(int plane, unsigned int value);
    bool guess_row(bool first_pass, int y, int block_size);
    bool done() const;
    void fill_d_stack(int x1, int x2, Byte value);
    void initialize_scan();
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

    Phase m_phase{Phase::START};
    bool m_guess_plot{};
    bool m_bottom_guess{};
    bool m_right_guess{};
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
};

extern int g_stop_pass; // stop at this guessing pass early

// used by solid guessing and by zoom panning
int ssg_block_size();

} // namespace id::engine
