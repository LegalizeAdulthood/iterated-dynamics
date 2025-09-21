// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "config/port.h"
#include "misc/sized_types.h"

#include <stdexcept>
#include <vector>

namespace id::fractals
{

extern bool g_cellular_next_screen;

bool cellular_per_image();

class Cellular
{
public:
    Cellular();
    Cellular(const Cellular &) = delete;
    Cellular(Cellular &&) = delete;
    ~Cellular() = default;
    Cellular &operator=(const Cellular &) = delete;
    Cellular &operator=(Cellular &&) = delete;

    bool iterate();
    void suspend();

    std::string error(int err, int t = 0) const;

private:
    std::vector<Byte> m_cell_array[2];
    misc::S16 m_s_r{};
    misc::S16 m_k_1{};
    misc::S16 m_rule_digits{};
    bool m_last_screen_flag{};

    misc::S32 rand_param{};
    misc::U16 kr{};
    misc::U32 line_num{};
    misc::U16 k{};
    misc::U16 init_string[16]{};
    misc::U16 cell_table[32]{};
    misc::S16 start_row{};
    misc::S16 filled{};
    misc::S16 not_filled{1};
};

class CellularError : public std::runtime_error
{
public:
    CellularError(const Cellular &cellular, const int err, const int t = 0) :
        std::runtime_error(cellular.error(err, t))
    {
    }
};

} // namespace id::fractals
