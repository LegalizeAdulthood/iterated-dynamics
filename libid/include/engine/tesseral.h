// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <array>

namespace id::engine
{

class Tesseral
{
public:
    Tesseral();

    bool done() const;
    bool iterate();
    void suspend();

private:
    struct Tess
    {
        int x1{};
        int x2{};
        int y1{};
        int y2{};
        int top{};
        int bot{};
        int lft{};
        int rgt{};
    };

    void fill_box();
    void next_box();
    bool split_box();
    bool split_needed();

    bool m_guess_plot{};
    bool m_interrupted{};
    std::array<Tess, 4096 / sizeof(Tess)> m_stack{};
    Tess *m_tp{};
};

int tesseral();

} // namespace id::engine
