// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdlib>
#include <string>
#include <vector>

namespace id::fractals
{

enum class AntType
{
    ONE = 1,
    TWO = 2,
};

class Ant
{
public:
    Ant();
    ~Ant() = default;

    bool iterate(bool step, long wait);

private:
    enum
    {
        DIRS = 4,
        MAX_ANTS = 256,
    };

    void init_mite1();
    void turk_mite1(bool step, long wait);
    void init_mite2();
    void turk_mite2(bool step, long wait);

    // possible value of idir e relative movement in the 4 directions
    // for x 0, 1, 0, -1
    // for y 1, 0, -1, 0
    //
    std::vector<int> inc_x[DIRS]; // table for 4 directions
    std::vector<int> inc_y[DIRS];
    long max_pts{};
    int max_ants{};
    AntType type{};
    bool wrap{};
    std::string rule_text;
    std::size_t rule_len{};

    int dir[MAX_ANTS + 1]{};
    int x[MAX_ANTS + 1]{};
    int y[MAX_ANTS + 1]{};
    int rule[MAX_ANTS + 1]{};
    int next_col[MAX_ANTS + 1]{};
    unsigned rule_mask{1U};
    long count{};
    long count_end{};
};

} // namespace id::fractals
