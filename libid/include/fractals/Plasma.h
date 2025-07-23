// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "misc/sized_types.h"

namespace id::fractals
{

class Plasma
{
public:
    Plasma();
    ~Plasma();

    bool done() const;
    void iterate();

private:
    enum class Algorithm
    {
        OLD = 0,
        NEW = 1,
    };

    Plasma(const Plasma &rhs) = delete;
    Plasma(Plasma &&rhs) = delete;
    Plasma &operator=(const Plasma &rhs) = delete;
    Plasma &operator=(Plasma &&rhs) = delete;

    U16 m_rnd[4]{};
    bool m_old_pot_flag{};
    bool m_old_pot_16_bit{};
    bool m_done{};
    Algorithm m_algo{Algorithm::OLD};
    int m_i{};
    int m_k{};
};

} // namespace id::fractals
