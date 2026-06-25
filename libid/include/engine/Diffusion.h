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

private:
    static void count_to_int(unsigned long counter, int &x, int &y, int dif_offset);

    int engine();
    void plot_block(int x, int y, int size, int color);
    void plot_block_lim(int x, int y, int size, int color);

    std::array<Byte, 4096> m_stack{};
    unsigned int m_bits{};
    unsigned long m_counter{};
    unsigned long m_limit{};
};

} // namespace id::engine
