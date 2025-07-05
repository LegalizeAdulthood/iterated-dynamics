// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "config/port.h"
#include "misc/sized_types.h"

#include <stdexcept>
#include <vector>

extern bool g_cellular_next_screen;

bool cellular_per_image();

namespace id::fractals
{
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
    S16 m_s_r{};
    S16 m_k_1{};
    S16 m_rule_digits{};
    bool m_last_screen_flag{};

    S32 rand_param{};
    U16 kr{};
    U32 line_num{};
    U16 k{};
    U16 init_string[16]{};
    U16 cell_table[32]{};
    S16 start_row{};
    S16 filled{};
    S16 not_filled{1};
};

class CellularError : public std::runtime_error
{
public:
    CellularError(const Cellular &cellular, int err, int t = 0) :
        std::runtime_error(cellular.error(err, t))
    {
    }
};

} // namespace id::fractals
