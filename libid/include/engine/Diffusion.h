// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/Point.h"

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
    // False ends this work chunk; done() distinguishes completion.
    bool select_pixel();
    math::Point2i selected_pixel() const;
    void complete_pixel(int color);
    bool done() const;
    bool iterate();
    unsigned long limit() const;
    // TODO: removed later
    // Slice 26.8 moves calculation to the common StandardFractal boundary.
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

    void initialize();
    void initialize_counter();
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
    int m_fill_size{};
    int m_counters_processed{};
    math::Point2i m_selected_pixel{};
    bool m_pixel_selected{};
    bool m_initialized{};
};

} // namespace id::engine
