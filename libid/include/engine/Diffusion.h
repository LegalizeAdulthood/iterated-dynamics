// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <array>

namespace id::engine
{

class Diffusion
{
public:
    Diffusion() = default;
    Diffusion(const Diffusion &) = delete;
    Diffusion(Diffusion &&) = delete;
    ~Diffusion() = default;
    Diffusion &operator=(const Diffusion &) = delete;
    Diffusion &operator=(Diffusion &&) = delete;

    unsigned int bits() const;
    unsigned long counter() const;
    bool iterate();
    unsigned long limit() const;
    int scan();
    void suspend();

private:
    enum class Phase
    {
        FIRST_HALF,
        SECOND_HALF,
        DONE
    };

    static void count_to_int(unsigned long counter, int &x, int &y, int dif_offset);

    bool calculate_current_pixel();
    bool done() const;
    void initialize();
    bool process_counter();
    bool process_dither_counter();
    bool process_fill_counter();
    bool process_point_counter();
    void plot_block(int x, int y, int size, int color);
    void plot_block_lim(int x, int y, int size, int color);
    void update_phase();

    std::array<Byte, 4096> m_stack{};
    Phase m_phase{Phase::FIRST_HALF};
    unsigned int m_bits{};
    unsigned long m_counter{};
    unsigned long m_limit{};
    int m_dif_offset{};
    int m_nx{};
    int m_ny{};
    int m_orig_col{};
    int m_orig_row{};
    int m_rem_x{};
    int m_rem_y{};
    int m_size{};
    int m_tile_x{};
    int m_tile_y{};
    bool m_initialized{};
};

} // namespace id::engine
