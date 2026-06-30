// SPDX-License-Identifier: GPL-3.0-only
//
// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original Fractint implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include "fractals/Ant.h"

#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/random_seed.h"
#include "misc/version.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

using namespace id::engine;
using namespace id::misc;
using namespace id::ui;

namespace id::fractals
{

// Generate Random Number 0 <= r < n
static int random(const int n)
{
    return static_cast<int>(static_cast<long>(random15()) * static_cast<long>(n) >> 15);
}

namespace
{

bool is_integer_rule(const std::string_view text)
{
    return !text.empty() && std::all_of(text.begin(), text.end(), [](const char ch) { return ch >= '0' && ch <= '9'; });
}

bool use_drifted_id_rule_format()
{
    constexpr Version first_drifted{1, 0, 0, 0, false};
    constexpr Version last_drifted{1, 3, 2, 0, false};
    return !(g_version < first_drifted) && g_version <= last_drifted;
}

std::string ant_numeric_rule_text(const double param)
{
    char buff[64];
    std::snprintf(buff, sizeof(buff), use_drifted_id_rule_format() ? "%.17f" : "%.17g", param);
    return std::string{buff};
}

} // namespace

#define XO (g_logical_screen.x_dots / 2)
#define YO (g_logical_screen.y_dots / 2)

Ant::Ant()
{
    for (int i = 0; i < DIRS; i++)
    {
        inc_x[i].resize(g_logical_screen.x_dots + 2);
        inc_y[i].resize(g_logical_screen.y_dots + 2);
    }

    // In this vectors put all the possible point that the ants can visit.
    // Wrap them from a side to the other instead of simply end calculation
    for (int i = 0; i < g_logical_screen.x_dots; i++)
    {
        inc_x[0][i] = i;
        inc_x[2][i] = i;
    }

    for (int i = 0; i < g_logical_screen.x_dots; i++)
    {
        inc_x[3][i] = i + 1;
    }
    inc_x[3][g_logical_screen.x_dots - 1] = 0; // wrap from right of the screen to left

    for (int i = 1; i < g_logical_screen.x_dots; i++)
    {
        inc_x[1][i] = i - 1;
    }
    inc_x[1][0] = g_logical_screen.x_dots - 1; // wrap from left of the screen to right

    for (int i = 0; i < g_logical_screen.y_dots; i++)
    {
        inc_y[1][i] = i;
        inc_y[3][i] = i;
    }
    for (int i = 0; i < g_logical_screen.y_dots; i++)
    {
        inc_y[0][i] = i + 1;
    }
    inc_y[0][g_logical_screen.y_dots - 1] = 0; // wrap from the top of the screen to the bottom
    for (int i = 1; i < g_logical_screen.y_dots; i++)
    {
        inc_y[2][i] = i - 1;
    }
    inc_y[2][0] = g_logical_screen.y_dots - 1; // wrap from the bottom of the screen to the top

    max_pts = std::abs(static_cast<long>(g_params[1]));
    max_ants = static_cast<int>(g_params[2]);
    if (max_ants < 1) // if maxants == 0 maxants random
    {
        max_ants = 2 + random(256 - 2);
    }
    else if (max_ants > MAX_ANTS)
    {
        max_ants = MAX_ANTS;
        g_params[2] = MAX_ANTS;
    }
    count_end = max_pts / static_cast<long>(INNER_LOOP);

    type = static_cast<AntType>(g_params[3]);
    if (type < AntType::ONE || type > AntType::TWO)
    {
        type = static_cast<AntType>(random(2)); // if type == 0 choose a random type
    }

    wrap = g_params[4] != 0.0;

    rule_text = ant_rule_text();
    rule_len = static_cast<int>(rule_text.length());
    if (rule_len > 1)
    {
        // if rule_len == 0 random rule
        for (size_t i = 0; i < rule_len; i++)
        {
            if (rule_text[i] != '1')
            {
                rule_text[i] = '\000';
            }
            else
            {
                rule_text[i] = '\001';
            }
        }
    }
    else
    {
        rule_len = 0;
    }

    // set random seed for reproducibility
    if (!g_random_seed_flag && g_params[5] == 1.0)
    {
        --g_random_seed;
    }
    if (g_params[5] != 0.0 && g_params[5] != 1.0)
    {
        g_random_seed = static_cast<int>(g_params[5]);
    }

    set_random_seed();

    switch (type)
    {
    case AntType::ONE:
        init_mite1();
        break;
    case AntType::TWO:
        init_mite2();
        break;
    }
}

void Ant::init_mite1()
{
    if (rule_len == 0)
    {
        // random rule
        for (g_color = 0; g_color < MAX_ANTS; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            rule[g_color] = 1 - random(2) * 2;
            next_col[g_color] = g_color + 1;
        }
        // close the cycle
        next_col[g_color] = 0;
    }
    else
    {
        // user defined rule
        for (g_color = 0; g_color < static_cast<int>(rule_len); g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            rule[g_color] = rule_text[g_color] * 2 - 1;
            next_col[g_color] = g_color + 1;
        }
        // repeats to last color
        for (g_color = static_cast<int>(rule_len); g_color < MAX_ANTS; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            rule[g_color] = rule[g_color % rule_len];
            next_col[g_color] = g_color + 1;
        }
        // close the cycle
        next_col[g_color] = 0;
    }
    for (g_color = max_ants; g_color; g_color--)
    {
        // init the various turmites N.B. non usa
        // x[0], y[0], dir[0]
        if (rule_len)
        {
            dir[g_color] = 1;
            x[g_color] = XO;
            y[g_color] = YO;
        }
        else
        {
            dir[g_color] = random(DIRS);
            x[g_color] = random(g_logical_screen.x_dots);
            y[g_color] = random(g_logical_screen.y_dots);
        }
    }
}

static unsigned rotate_left_one(unsigned value);

void Ant::finish_batch()
{
    batch_complete_pending = true;
    ++count;
}

static unsigned rotate_left_one(const unsigned value)
{
    constexpr unsigned high_bit{~(~0U >> 1)};
    const unsigned result{value << 1};
    return value & high_bit ? result | 1U : result;
}

void Ant::init_mite2()
{
    if (rule_len == 0)
    {
        // random rule
        for (int color = MAX_ANTS - 1; color; color--)
        {
            // init the various turmites N.B. don't use
            // x[0], y[0], dir[0]
            dir[color] = random(DIRS);
            rule[color] = random15() << random(2) | random(2);
            x[color] = random(g_logical_screen.x_dots);
            y[color] = random(g_logical_screen.y_dots);
        }
    }
    else
    {
        // the same rule the user wants for every
        // turkmite (max rule_len = 16 bit)
        rule_len = static_cast<int>(std::min(rule_len, 8 * sizeof(int) - 1));
        rule[0] = 0;
        for (size_t i = 0; i < rule_len; i++)
        {
            rule[0] = rule[0] << 1 | rule_text[i];
        }
        for (int color = MAX_ANTS - 1; color; color--)
        {
            // init the various turmites N.B. non usa
            // x[0], y[0], dir[0]
            dir[color] = 0;
            rule[color] = rule[0];
            x[color] = XO;
            y[color] = YO;
        }
    }
    // use this rule when a black pixel is found
    rule[0] = 0;
}

bool Ant::move_mite1(const int color)
{
    const int xx{x[color]};
    const int yy{y[color]};
    int direction{dir[color]};
    const int pixel{get_color(xx, yy)};
    g_put_color(xx, yy, next_col[pixel]);
    direction += rule[pixel];
    direction &= 3;
    if (!wrap)
    {
        const bool at_edge = (direction == 0 && yy == g_logical_screen.y_dots - 1) ||
            (direction == 1 && xx == g_logical_screen.x_dots - 1) || (direction == 2 && yy == 0) ||
            (direction == 3 && xx == 0);
        if (at_edge)
        {
            return false;
        }
    }
    x[color] = inc_x[direction][xx];
    y[color] = inc_y[direction][yy];
    dir[color] = direction;
    return true;
}

bool Ant::move_mite2(const int color)
{
    const int xx{x[color]};
    const int yy{y[color]};
    int direction{dir[color]};
    const int pixel{get_color(xx, yy)};
    g_put_color(xx, yy, 15);
    if (rule[pixel] & rule_mask)
    {
        --direction;
        g_put_color(xx, yy, 0);
    }
    else
    {
        ++direction;
        g_put_color(xx, yy, color);
    }
    direction &= 3;
    if (!wrap)
    {
        const bool at_edge = (direction == 0 && yy == g_logical_screen.y_dots - 1) ||
            (direction == 1 && xx == g_logical_screen.x_dots - 1) || (direction == 2 && yy == 0) ||
            (direction == 3 && xx == 0);
        if (at_edge)
        {
            return false;
        }
    }
    x[color] = inc_x[direction][xx];
    y[color] = inc_y[direction][yy];
    dir[color] = direction;
    return true;
}

bool Ant::done() const
{
    return count >= count_end;
}

bool Ant::consume_batch_complete()
{
    const bool batch_complete{batch_complete_pending};
    batch_complete_pending = false;
    return batch_complete;
}

bool Ant::iterate()
{
    if (done())
    {
        return false;
    }
    if (type != AntType::ONE && type != AntType::TWO)
    {
        finish_batch();
        return !done();
    }

    for (int i = INNER_LOOP; i; --i)
    {
        for (int color = max_ants; color; --color)
        {
            const bool keep_going{type == AntType::ONE ? move_mite1(color) : move_mite2(color)};
            if (!keep_going)
            {
                finish_batch();
                return !done();
            }
        }
        if (type == AntType::TWO)
        {
            rule_mask = rotate_left_one(rule_mask);
        }
    }
    finish_batch();
    return !done();
}

std::string ant_rule_text()
{
    const std::string_view raw_rule{g_param_text[0]};
    return is_integer_rule(raw_rule) ? std::string{raw_rule} : ant_numeric_rule_text(g_params[0]);
}

} // namespace id::fractals
