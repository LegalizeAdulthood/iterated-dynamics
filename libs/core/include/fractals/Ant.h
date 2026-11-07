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
    Ant(const Ant &) = delete;
    Ant(Ant &&) = delete;
    ~Ant() = default;
    Ant &operator=(const Ant &) = delete;
    Ant &operator=(Ant &&) = delete;

    bool done() const;
    bool consume_batch_complete();
    bool iterate();

private:
    enum
    {
        DIRS = 4,
        MAX_ANTS = 256,
        INNER_LOOP = 100,
    };

    void init_mite1();
    void init_mite2();
    void finish_batch();
    bool move_mite1(int color);
    bool move_mite2(int color);

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
    bool batch_complete_pending{};
};

std::string ant_rule_text();

} // namespace id::fractals
