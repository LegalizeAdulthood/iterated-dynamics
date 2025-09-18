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
#include "engine/id_data.h"
#include "engine/random_seed.h"
#include "engine/wait_until.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::ui;

namespace id::fractals
{

// Generate Random Number 0 <= r < n
static int random(int n)
{
    return static_cast<int>(static_cast<long>(std::rand()) * static_cast<long>(n) >> 15);
}

#define XO              (g_logical_screen_x_dots/2)
#define YO              (g_logical_screen_y_dots/2)

enum
{
    INNER_LOOP = 100
};

Ant::Ant()
{
    for (int i = 0; i < DIRS; i++)
    {
        inc_x[i].resize(g_logical_screen_x_dots + 2);
        inc_y[i].resize(g_logical_screen_y_dots + 2);
    }

    // In this vectors put all the possible point that the ants can visit.
    // Wrap them from a side to the other instead of simply end calculation
    for (int i = 0; i < g_logical_screen_x_dots; i++)
    {
        inc_x[0][i] = i;
        inc_x[2][i] = i;
    }

    for (int i = 0; i < g_logical_screen_x_dots; i++)
    {
        inc_x[3][i] = i + 1;
    }
    inc_x[3][g_logical_screen_x_dots - 1] = 0; // wrap from right of the screen to left

    for (int i = 1; i < g_logical_screen_x_dots; i++)
    {
        inc_x[1][i] = i - 1;
    }
    inc_x[1][0] = g_logical_screen_x_dots - 1; // wrap from left of the screen to right

    for (int i = 0; i < g_logical_screen_y_dots; i++)
    {
        inc_y[1][i] = i;
        inc_y[3][i] = i;
    }
    for (int i = 0; i < g_logical_screen_y_dots; i++)
    {
        inc_y[0][i] = i + 1;
    }
    inc_y[0][g_logical_screen_y_dots - 1] = 0; // wrap from the top of the screen to the bottom
    for (int i = 1; i < g_logical_screen_y_dots; i++)
    {
        inc_y[2][i] = i - 1;
    }
    inc_y[2][0] = g_logical_screen_y_dots - 1; // wrap from the bottom of the screen to the top

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

    const auto get_rule = [](double param)
    {
        std::ostringstream buff;
        buff << std::setprecision(17) << std::fixed << param;
        return buff.str();
    };
    rule_text = get_rule(g_params[0]);
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
            x[g_color] = random(g_logical_screen_x_dots);
            y[g_color] = random(g_logical_screen_y_dots);
        }
    }
}

// turkmite from Scientific American July 1994 page 91
// Tweaked by Luciano Genero & Fulvio Cappelli
//
void Ant::turk_mite1(bool step, long wait)
{
    for (int i = INNER_LOOP; i; i--)
    {
        for (int color = max_ants; color; color--)
        {
            // move the various turmites
            int x = this->x[color]; // temp vars
            int y = this->y[color];
            int dir = this->dir[color];
            int pixel = get_color(x, y);
            if (wait > 0 && !step)
            {
                g_put_color(x, y, 15);
                sleep_ms(wait);
            }
            g_put_color(x, y, next_col[pixel]);
            dir += rule[pixel];
            dir &= 3;
            if (!wrap)
            {
                if ((dir == 0 && y == g_logical_screen_y_dots - 1) ||
                    (dir == 1 && x == g_logical_screen_x_dots - 1) || (dir == 2 && y == 0) ||
                    (dir == 3 && x == 0))
                {
                    return;
                }
            }
            this->x[color] = inc_x[dir][x];
            this->y[color] = inc_y[dir][y];
            this->dir[color] = dir;
        }
    }
}

static unsigned rotate_left_one(unsigned value)
{
    const unsigned high_bit{~(~0U >> 1)};
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
            rule[color] = std::rand() << random(2) | random(2);
            x[color] = random(g_logical_screen_x_dots);
            y[color] = random(g_logical_screen_y_dots);
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

// this one ignore the color of the current cell is more like a white ant
void Ant::turk_mite2(bool step, long wait)
{
    for (int i = INNER_LOOP; i; i--)
    {
        for (int color = max_ants; color; color--)
        {
            // move the various turmites
            int x = this->x[color]; // temp vars
            int y = this->y[color];
            int dir = this->dir[color];
            int pixel = get_color(x, y);
            g_put_color(x, y, 15);

            if (wait > 0 && !step)
            {
                sleep_ms(wait);
            }

            if (rule[pixel] & rule_mask)
            {
                // turn right
                dir--;
                g_put_color(x, y, 0);
            }
            else
            {
                // turn left
                dir++;
                g_put_color(x, y, color);
            }
            dir &= 3;
            if (!wrap)
            {
                if ((dir == 0 && y == g_logical_screen_y_dots - 1) ||
                    (dir == 1 && x == g_logical_screen_x_dots - 1) || (dir == 2 && y == 0) ||
                    (dir == 3 && x == 0))
                {
                    return;
                }
            }
            this->x[color] = inc_x[dir][x];
            this->y[color] = inc_y[dir][y];
            this->dir[color] = dir;
        }
        rule_mask = rotate_left_one(rule_mask);
    }
}

bool Ant::iterate(bool step, long wait)
{
    if (count < count_end)
    {
        switch (type)
        {
        case AntType::ONE:
            turk_mite1(step, wait);
            break;
        case AntType::TWO:
            turk_mite2(step, wait);
            break;
        }
        ++count;
    }
    return count < count_end;
}

} // namespace id::fractals
