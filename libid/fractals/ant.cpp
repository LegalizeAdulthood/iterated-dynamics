// SPDX-License-Identifier: GPL-3.0-only
//
// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original Fractint implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include "fractals/ant.h"

#include "engine/calcfrac.h"
#include "engine/id_data.h"
#include "engine/wait_until.h"
#include "helpdefs.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/cmdfiles.h"
#include "ui/id_keys.h"
#include "ui/temp_msg.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Generate Random Number 0 <= r < n
static int random(int n)
{
    return (int) ((long) std::rand() * (long) n >> 15);
}

#define XO              (g_logical_screen_x_dots/2)
#define YO              (g_logical_screen_y_dots/2)

enum
{
    MAX_ANTS = 256,
    DIRS = 4,
    INNER_LOOP = 100
};

// possible value of idir e relative movement in the 4 directions
// for x 0, 1, 0, -1
// for y 1, 0, -1, 0
//
static std::vector<int> s_inc_x[DIRS];   // table for 4 directions
static std::vector<int> s_inc_y[DIRS];
static int s_last_x_dots{};
static int s_last_y_dots{};

static long change_wait(long wait)
{
    while (true)
    {
        std::ostringstream msg;
        msg << "Delay " << std::setw(4) << wait << "     ";
        show_temp_msg(msg.str());
        switch (driver_get_key())
        {
        case ID_KEY_CTL_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
            wait += 100;
            break;
        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_UP_ARROW:
            wait += 10;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
            wait -= 100;
            break;
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_DOWN_ARROW:
            wait -= 10;
            break;
        default:
            clear_temp_msg();
            return wait;
        }
        wait = std::max(wait, 0L);
    }
}

static bool ant_key(bool &step, long &wait)
{
    // check for a key only every inner_loop times
    int key = driver_key_pressed();
    if (key || step)
    {
        if (key == 0)
        {
            key = driver_get_key();
        }
        switch (key)
        {
        case ID_KEY_SPACE:
            step = !step;
            break;
        case ID_KEY_ESC:
            return true;

        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_UP_ARROW:
        case ID_KEY_DOWN_ARROW:
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_CTL_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
            wait = change_wait(wait);
            break;

        case ID_KEY_ENTER:
            break;

        default:
            return true;
        }
        if (driver_key_pressed())
        {
            driver_get_key();
        }
    }
    return false;
}

struct TurkMite1
{
    int x[MAX_ANTS + 1]{};
    int y[MAX_ANTS + 1]{};
    int next_col[MAX_ANTS + 1]{};
    int rule[MAX_ANTS + 1]{};
    int dir[MAX_ANTS + 1]{};
    bool wrap{g_params[4] != 0};
};

// turkmite from Scientific American July 1994 page 91
// Tweaked by Luciano Genero & Fulvio Cappelli
//
static void turk_mite1(int max_ants, int rule_len, const char *rule, long max_pts, long wait)
{
    TurkMite1 ant;
    bool step = wait == 1;
    if (step)
    {
        wait = 0;
    }
    if (rule_len == 0)
    {
        // random rule
        for (g_color = 0; g_color < MAX_ANTS; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            ant.rule[g_color] = 1 - (random(2) * 2);
            ant.next_col[g_color] = g_color + 1;
        }
        // close the cycle
        ant.next_col[g_color] = 0;
    }
    else
    {
        // user defined rule
        for (g_color = 0; g_color < rule_len; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            ant.rule[g_color] = (rule[g_color] * 2) - 1;
            ant.next_col[g_color] = g_color + 1;
        }
        // repeats to last color
        for (g_color = rule_len; g_color < MAX_ANTS; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            ant.rule[g_color] = ant.rule[g_color % rule_len];
            ant.next_col[g_color] = g_color + 1;
        }
        // close the cycle
        ant.next_col[g_color] = 0;
    }
    for (g_color = max_ants; g_color; g_color--)
    {
        // init the various turmites N.B. non usa
        // x[0], y[0], dir[0]
        if (rule_len)
        {
            ant.dir[g_color] = 1;
            ant.x[g_color] = XO;
            ant.y[g_color] = YO;
        }
        else
        {
            ant.dir[g_color] = random(DIRS);
            ant.x[g_color] = random(g_logical_screen_x_dots);
            ant.y[g_color] = random(g_logical_screen_y_dots);
        }
    }
    max_pts = max_pts / (long) INNER_LOOP;
    for (long count = 0; count < max_pts; count++)
    {
        if (ant_key(step, wait))
        {
            return;
        }
        for (int i = INNER_LOOP; i; i--)
        {
            if (wait > 0 && !step)
            {
                for (int color = max_ants; color; color--)
                {
                    // move the various turmites
                    int x = ant.x[color];   // temp vars
                    int y = ant.y[color];
                    int dir = ant.dir[color];
                    int pixel = get_color(x, y);
                    g_put_color(x, y, 15);
                    sleep_ms(wait);
                    g_put_color(x, y, ant.next_col[pixel]);
                    dir += ant.rule[pixel];
                    dir &= 3;
                    if (!ant.wrap)
                    {
                        if ((dir == 0 && y == g_logical_screen_y_dots - 1)
                            || (dir == 1 && x == g_logical_screen_x_dots - 1)
                            || (dir == 2 && y == 0)
                            || (dir == 3 && x == 0))
                        {
                            return;
                        }
                    }
                    ant.x[color] = s_inc_x[dir][x];
                    ant.y[color] = s_inc_y[dir][y];
                    ant.dir[color] = dir;
                }
            }
            else
            {
                for (int color = max_ants; color; color--)
                {
                    // move the various turmites without delay
                    int x = ant.x[color];   // temp vars
                    int y = ant.y[color];
                    int dir = ant.dir[color];
                    int pixel = get_color(x, y);
                    g_put_color(x, y, ant.next_col[pixel]);
                    dir += ant.rule[pixel];
                    dir &= 3;
                    if (!ant.wrap)
                    {
                        if ((dir == 0 && y == g_logical_screen_y_dots - 1)
                            || (dir == 1 && x == g_logical_screen_x_dots - 1)
                            || (dir == 2 && y == 0)
                            || (dir == 3 && x == 0))
                        {
                            return;
                        }
                    }
                    ant.x[color] = s_inc_x[dir][x];
                    ant.y[color] = s_inc_y[dir][y];
                    ant.dir[color] = dir;
                }
            }
        }
    }
}

static unsigned rotate_left_one(unsigned value)
{
    const unsigned high_bit{~(~0U >> 1)};
    const unsigned result{value << 1};
    return (value & high_bit) ? (result | 1U) : result;
}

struct TurkMite2
{
    int dir[MAX_ANTS + 1]{};
    int x[MAX_ANTS + 1]{};
    int y[MAX_ANTS + 1]{};
    int rule[MAX_ANTS + 1]{};
    bool wrap{g_params[4] != 0};
};

// this one ignore the color of the current cell is more like a white ant
static void turk_mite2(int max_ants, int rule_len, const char *rule, long max_pts, long wait)
{
    TurkMite2 ant;
    bool step = wait == 1;
    if (step)
    {
        wait = 0;
    }
    if (rule_len == 0)
    {
        // random rule
        for (int color = MAX_ANTS-1; color; color--)
        {
            // init the various turmites N.B. don't use
            // x[0], y[0], dir[0]
            ant.dir[color] = random(DIRS);
            ant.rule[color] = (std::rand() << random(2)) | random(2);
            ant.x[color] = random(g_logical_screen_x_dots);
            ant.y[color] = random(g_logical_screen_y_dots);
        }
    }
    else
    {
        // the same rule the user wants for every
        // turkmite (max rule_len = 16 bit)
        rule_len = static_cast<int>(std::min(static_cast<size_t>(rule_len), 8*sizeof(int)));
        ant.rule[0] = 0;
        for (int i = 0; i < rule_len; i++)
        {
            ant.rule[0] = (ant.rule[0] << 1) | rule[i];
        }
        for (int color = MAX_ANTS-1; color; color--)
        {
            // init the various turmites N.B. non usa
            // x[0], y[0], dir[0]
            ant.dir[color] = 0;
            ant.rule[color] = ant.rule[0];
            ant.x[color] = XO;
            ant.y[color] = YO;
        }
    }
    // use this rule when a black pixel is found
    ant.rule[0] = 0;
    unsigned rule_mask = 1U;
    max_pts = max_pts / (long) INNER_LOOP;
    for (long count = 0; count < max_pts; count++)
    {
        if (ant_key(step, wait))
        {
            return;
        }
        for (int i = INNER_LOOP; i; i--)
        {
            for (int color = max_ants; color; color--)
            {
                // move the various turmites
                int x = ant.x[color];      // temp vars
                int y = ant.y[color];
                int dir = ant.dir[color];
                int pixel = get_color(x, y);
                g_put_color(x, y, 15);

                if (wait > 0 && !step)
                {
                    sleep_ms(wait);
                }

                if (ant.rule[pixel] & rule_mask)
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
                if (!ant.wrap)
                {
                    if ((dir == 0 && y == g_logical_screen_y_dots - 1)
                        || (dir == 1 && x == g_logical_screen_x_dots - 1)
                        || (dir == 2 && y == 0)
                        || (dir == 3 && x == 0))
                    {
                        return;
                    }
                }
                ant.x[color] = s_inc_x[dir][x];
                ant.y[color] = s_inc_y[dir][y];
                ant.dir[color] = dir;
            }
            rule_mask = rotate_left_one(rule_mask);
        }
    }
}

void free_ant_storage()
{
    for (int i = 0; i < DIRS; ++i)
    {
        s_inc_x[i].clear();
        s_inc_y[i].clear();
    }
}

static std::string get_rule()
{
    std::ostringstream buff;
    buff << std::setprecision(17) << std::fixed << g_params[0];
    return buff.str();
}

int ant()
{
    if (g_logical_screen_x_dots != s_last_x_dots || g_logical_screen_y_dots != s_last_y_dots)
    {
        s_last_x_dots = g_logical_screen_x_dots;
        s_last_y_dots = g_logical_screen_y_dots;

        free_ant_storage();
        for (int i = 0; i < DIRS; i++)
        {
            s_inc_x[i].resize(g_logical_screen_x_dots + 2);
            s_inc_y[i].resize(g_logical_screen_y_dots + 2);
        }
    }

    // In this vectors put all the possible point that the ants can visit.
    // Wrap them from a side to the other instead of simply end calculation
    for (int i = 0; i < g_logical_screen_x_dots; i++)
    {
        s_inc_x[0][i] = i;
        s_inc_x[2][i] = i;
    }

    for (int i = 0; i < g_logical_screen_x_dots; i++)
    {
        s_inc_x[3][i] = i + 1;
    }
    s_inc_x[3][g_logical_screen_x_dots-1] = 0; // wrap from right of the screen to left

    for (int i = 1; i < g_logical_screen_x_dots; i++)
    {
        s_inc_x[1][i] = i - 1;
    }
    s_inc_x[1][0] = g_logical_screen_x_dots-1; // wrap from left of the screen to right

    for (int i = 0; i < g_logical_screen_y_dots; i++)
    {
        s_inc_y[1][i] = i;
        s_inc_y[3][i] = i;
    }
    for (int i = 0; i < g_logical_screen_y_dots; i++)
    {
        s_inc_y[0][i] = i + 1;
    }
    s_inc_y[0][g_logical_screen_y_dots - 1] = 0; // wrap from the top of the screen to the bottom
    for (int i = 1; i < g_logical_screen_y_dots; i++)
    {
        s_inc_y[2][i] = i - 1;
    }
    s_inc_y[2][0] = g_logical_screen_y_dots - 1; // wrap from the bottom of the screen to the top
    ValueSaver saved_help_mode(g_help_mode, HelpLabels::HELP_ANT_COMMANDS);
    const long max_pts = std::abs(static_cast<long>(g_params[1]));
    const long wait = std::abs(g_orbit_delay);
    std::string rule{get_rule()};
    int rule_len = (int) rule.length();
    if (rule_len > 1)
    {
        // if rule_len == 0 random rule
        for (int i = 0; i < rule_len; i++)
        {
            if (rule[i] != '1')
            {
                rule[i] = (char) 0;
            }
            else
            {
                rule[i] = (char) 1;
            }
        }
    }
    else
    {
        rule_len = 0;
    }

    // set random seed for reproducibility
    if (!g_random_seed_flag && g_params[5] == 1)
    {
        --g_random_seed;
    }
    if (g_params[5] != 0 && g_params[5] != 1)
    {
        g_random_seed = (int)g_params[5];
    }

    std::srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    int max_ants = static_cast<int>(g_params[2]);
    if (max_ants < 1)               // if maxants == 0 maxants random
    {
        max_ants = 2 + random(256 - 2);
    }
    else if (max_ants > MAX_ANTS)
    {
        max_ants = MAX_ANTS;
        g_params[2] = MAX_ANTS;
    }
    int type = static_cast<int>(g_params[3]);
    if (type < 1 || type > 2)
    {
        type = random(2);         // if type == 0 choose a random type
    }
    switch (type)
    {
    case 1:
        turk_mite1(max_ants, rule_len, rule.c_str(), max_pts, wait);
        break;
    case 2:
        turk_mite2(max_ants, rule_len, rule.c_str(), max_pts, wait);
        break;
    default:
        break;
    }
    return 0;
}
