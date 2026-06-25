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

    void fill_prefix_plane(int plane, unsigned int value);
    bool guess_row(bool first_pass, int y, int block_size);
    void fill_d_stack(int x1, int x2, Byte value);
    void plot_block(int build_row, int x, int y, int color);

    bool m_guess_plot{};
    bool m_bottom_guess{};
    bool m_right_guess{};
    int m_max_block{};
    int m_half_block{};
    unsigned int m_prefix[2][MAX_Y_BLK][MAX_X_BLK]{};
    int m_stack_row_offset{};
    std::vector<Byte> m_stack;
    int m_block_size{};
};

extern int g_stop_pass; // stop at this guessing pass early

// used by solid guessing and by zoom panning
int ssg_block_size();
int solid_guess();

} // namespace id::engine
