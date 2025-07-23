// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "config/port.h"
#include "misc/sized_types.h"
#include "misc/ValueSaver.h"

#include <vector>

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

    struct SubdivisionStack
    {
        int top;       // top of stack
        int value[16]; // subdivided value
        int level[16]; // recursion level
    };

    struct Subdivision
    {
        int x1;
        int y1;
        int x2;
        int y2;
        int level;
    };

    Plasma(const Plasma &rhs) = delete;
    Plasma(Plasma &&rhs) = delete;
    Plasma &operator=(const Plasma &rhs) = delete;
    Plasma &operator=(Plasma &&rhs) = delete;
    U16 adjust(int xa, int ya, int x, int y, int xb, int yb, int scale);
    void subdivide();
    void subdivide_new(int x1, int y1, int x2, int y2, int level);

    U16 m_rnd[4]{};
    ValueSaver<bool> m_saved_potential_flag;
    ValueSaver<bool> m_saved_potential_16_bit;
    bool m_done{};
    Algorithm m_algo{Algorithm::OLD};
    int m_level{};
    int m_k{};
    SubdivisionStack m_sub_x{};
    SubdivisionStack m_sub_y{};
    U16 (*m_get_pix)(int x, int y){};
    int m_i_param_x{};   // s_i_param_x = param.x * 8
    int m_shift_value{}; // shift based on #colors
    int m_p_colors{};
    int m_recur_level{};
    int m_scale{};
    U16 m_max_plasma{};
    std::vector<Subdivision> m_subdivs;
};

} // namespace id::fractals
