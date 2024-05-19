// The Ant Automaton is based on an article in Scientific American, July 1994.
// The original Fractint implementation was by Tim Wegner in Fractint 19.0.
// This routine is a major rewrite by Luciano Genero & Fulvio Cappelli using
// tables for speed, and adds a second ant type, multiple ants, and random
// rules.
//
#include "ant.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "get_color.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "temp_msg.h"
#include "wait_until.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// Generate Random Number 0 <= r < n
inline int random(int n)
{
    return (int) ((long) std::rand() * (long) n >> 15);
}

#define MAX_ANTS        256
#define XO              (g_logical_screen_x_dots/2)
#define YO              (g_logical_screen_y_dots/2)
#define DIRS            4
#define INNER_LOOP      100

// possible value of idir e relative movement in the 4 directions
// for x 0, 1, 0, -1
// for y 1, 0, -1, 0
//
static std::vector<int> s_incx[DIRS];   // table for 4 directions
static std::vector<int> s_incy[DIRS];
static int s_last_xdots = 0;
static int s_last_ydots = 0;

void setwait(long *wait)
{
    int kbdchar;

    while (true)
    {
        std::ostringstream msg;
        msg << "Delay " << std::setw(4) << *wait << "     ";
        showtempmsg(msg.str());
        kbdchar = driver_get_key();
        switch (kbdchar)
        {
        case ID_KEY_CTL_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
            (*wait) += 100;
            break;
        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_UP_ARROW:
            (*wait) += 10;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
            (*wait) -= 100;
            break;
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_DOWN_ARROW:
            (*wait) -= 10;
            break;
        default:
            cleartempmsg();
            return;
        }
        if (*wait < 0)
        {
            *wait = 0;
        }
    }
}

// turkmite from Scientific American July 1994 page 91
// Tweaked by Luciano Genero & Fulvio Cappelli
//
void TurkMite1(int maxtur, int rule_len, char const *ru, long maxpts, long wait)
{
    int ix;
    int iy;
    int idir;
    int pixel;
    int kbdchar;
    int step;
    bool antwrap;
    int x[MAX_ANTS + 1];
    int y[MAX_ANTS + 1];
    int next_col[MAX_ANTS + 1];
    int rule[MAX_ANTS + 1];
    int dir[MAX_ANTS + 1];
    antwrap = g_params[4] != 0;
    step = (int) wait;
    if (step == 1)
    {
        wait = 0;
    }
    else
    {
        step = 0;
    }
    if (rule_len == 0)
    {
        // random rule
        for (g_color = 0; g_color < MAX_ANTS; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            rule[g_color] = 1 - (random(2) * 2);
            next_col[g_color] = g_color + 1;
        }
        // close the cycle
        next_col[g_color] = 0;
    }
    else
    {
        // user defined rule
        for (g_color = 0; g_color < rule_len; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            rule[g_color] = (ru[g_color] * 2) - 1;
            next_col[g_color] = g_color + 1;
        }
        // repeats to last color
        for (g_color = rule_len; g_color < MAX_ANTS; g_color++)
        {
            // init the rules and colors for the
            // turkmites: 1 turn left, -1 turn right
            rule[g_color] = rule[g_color % rule_len];
            next_col[g_color] = g_color + 1;
        }
        // close the cycle
        next_col[g_color] = 0;
    }
    for (g_color = maxtur; g_color; g_color--)
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
    maxpts = maxpts / (long) INNER_LOOP;
    for (long count = 0; count < maxpts; count++)
    {
        // check for a key only every inner_loop times
        kbdchar = driver_key_pressed();
        if (kbdchar || step)
        {
            bool done = false;
            if (kbdchar == 0)
            {
                kbdchar = driver_get_key();
            }
            switch (kbdchar)
            {
            case ID_KEY_SPACE:
                step = 1 - step;
                break;
            case ID_KEY_ESC:
                done = true;
                break;
            case ID_KEY_RIGHT_ARROW:
            case ID_KEY_UP_ARROW:
            case ID_KEY_DOWN_ARROW:
            case ID_KEY_LEFT_ARROW:
            case ID_KEY_CTL_RIGHT_ARROW:
            case ID_KEY_CTL_UP_ARROW:
            case ID_KEY_CTL_DOWN_ARROW:
            case ID_KEY_CTL_LEFT_ARROW:
                setwait(&wait);
                break;
            default:
                done = true;
                break;
            }
            if (done)
            {
                goto exit_ant;
            }
            if (driver_key_pressed())
            {
                driver_get_key();
            }
        }
        for (int i = INNER_LOOP; i; i--)
        {
            if (wait > 0 && step == 0)
            {
                for (int color = maxtur; color; color--)
                {
                    // move the various turmites
                    ix = x[color];   // temp vars
                    iy = y[color];
                    idir = dir[color];

                    pixel = getcolor(ix, iy);
                    g_put_color(ix, iy, 15);
                    sleepms(wait);
                    g_put_color(ix, iy, next_col[pixel]);
                    idir += rule[pixel];
                    idir &= 3;
                    if (!antwrap)
                    {
                        if ((idir == 0 && iy == g_logical_screen_y_dots - 1)
                            || (idir == 1 && ix == g_logical_screen_x_dots - 1)
                            || (idir == 2 && iy == 0)
                            || (idir == 3 && ix == 0))
                        {
                            goto exit_ant;
                        }
                    }
                    x[color] = s_incx[idir][ix];
                    y[color] = s_incy[idir][iy];
                    dir[color] = idir;
                }
            }
            else
            {
                for (int color = maxtur; color; color--)
                {
                    // move the various turmites without delay
                    ix = x[color];   // temp vars
                    iy = y[color];
                    idir = dir[color];
                    pixel = getcolor(ix, iy);
                    g_put_color(ix, iy, next_col[pixel]);
                    idir += rule[pixel];
                    idir &= 3;
                    if (!antwrap)
                    {
                        if ((idir == 0 && iy == g_logical_screen_y_dots - 1)
                            || (idir == 1 && ix == g_logical_screen_x_dots - 1)
                            || (idir == 2 && iy == 0)
                            || (idir == 3 && ix == 0))
                        {
                            goto exit_ant;
                        }
                    }
                    x[color] = s_incx[idir][ix];
                    y[color] = s_incy[idir][iy];
                    dir[color] = idir;
                }
            }
        }
    }
exit_ant:
    return;
}

static unsigned rotate_left_one(unsigned value)
{
    unsigned const high_bit{~(~0U >> 1)};
    unsigned const result{value << 1};
    return (value & high_bit) ? (result | 1U) : result;
}

// this one ignore the color of the current cell is more like a white ant
void TurkMite2(int maxtur, int rule_len, char const *ru, long maxpts, long wait)
{
    int ix;
    int iy;
    int idir;
    int pixel;
    int dir[MAX_ANTS + 1];
    int kbdchar;
    int step;
    int x[MAX_ANTS + 1];
    int y[MAX_ANTS + 1];
    int rule[MAX_ANTS + 1];
    bool antwrap = g_params[4] != 0;

    step = (int) wait;
    if (step == 1)
    {
        wait = 0;
    }
    else
    {
        step = 0;
    }
    if (rule_len == 0)
    {
        // random rule
        for (int color = MAX_ANTS-1; color; color--)
        {
            // init the various turmites N.B. don't use
            // x[0], y[0], dir[0]
            dir[color] = random(DIRS);
            rule[color] = (std::rand() << random(2)) | random(2);
            x[color] = random(g_logical_screen_x_dots);
            y[color] = random(g_logical_screen_y_dots);
        }
    }
    else
    {
        // the same rule the user wants for every
        // turkmite (max rule_len = 16 bit)
        rule_len = static_cast<int>(std::min(static_cast<size_t>(rule_len), 8*sizeof(int)));
        rule[0] = 0;
        for (int i = 0; i < rule_len; i++)
        {
            rule[0] = (rule[0] << 1) | ru[i];
        }
        for (int color = MAX_ANTS-1; color; color--)
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
    unsigned rule_mask = 1U;
    maxpts = maxpts / (long) INNER_LOOP;
    for (long count = 0; count < maxpts; count++)
    {
        // check for a key only every inner_loop times
        kbdchar = driver_key_pressed();
        if (kbdchar || step)
        {
            bool done = false;
            if (kbdchar == 0)
            {
                kbdchar = driver_get_key();
            }
            switch (kbdchar)
            {
            case ID_KEY_SPACE:
                step = 1 - step;
                break;
            case ID_KEY_ESC:
                done = true;
                break;
            case ID_KEY_RIGHT_ARROW:
            case ID_KEY_UP_ARROW:
            case ID_KEY_DOWN_ARROW:
            case ID_KEY_LEFT_ARROW:
            case ID_KEY_CTL_RIGHT_ARROW:
            case ID_KEY_CTL_UP_ARROW:
            case ID_KEY_CTL_DOWN_ARROW:
            case ID_KEY_CTL_LEFT_ARROW:
                setwait(&wait);
                break;
            default:
                done = true;
                break;
            }
            if (done)
            {
                goto exit_ant;
            }
            if (driver_key_pressed())
            {
                driver_get_key();
            }
        }
        for (int i = INNER_LOOP; i; i--)
        {
            for (int color = maxtur; color; color--)
            {
                // move the various turmites
                ix = x[color];      // temp vars
                iy = y[color];
                idir = dir[color];
                pixel = getcolor(ix, iy);
                g_put_color(ix, iy, 15);

                if (wait > 0 && step == 0)
                {
                    sleepms(wait);
                }

                if (rule[pixel] & rule_mask)
                {
                    // turn right
                    idir--;
                    g_put_color(ix, iy, 0);
                }
                else
                {
                    // turn left
                    idir++;
                    g_put_color(ix, iy, color);
                }
                idir &= 3;
                if (!antwrap)
                {
                    if ((idir == 0 && iy == g_logical_screen_y_dots - 1)
                        || (idir == 1 && ix == g_logical_screen_x_dots - 1)
                        || (idir == 2 && iy == 0)
                        || (idir == 3 && ix == 0))
                    {
                        goto exit_ant;
                    }
                }
                x[color] = s_incx[idir][ix];
                y[color] = s_incy[idir][iy];
                dir[color] = idir;
            }
            rule_mask = rotate_left_one(rule_mask);
        }
    }
exit_ant:
    return;
}

void free_ant_storage()
{
    for (int i = 0; i < DIRS; ++i)
    {
        s_incx[i].clear();
        s_incy[i].clear();
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
    if (g_logical_screen_x_dots != s_last_xdots || g_logical_screen_y_dots != s_last_ydots)
    {
        s_last_xdots = g_logical_screen_x_dots;
        s_last_ydots = g_logical_screen_y_dots;

        free_ant_storage();
        for (int i = 0; i < DIRS; i++)
        {
            s_incx[i].resize(g_logical_screen_x_dots + 2);
            s_incy[i].resize(g_logical_screen_y_dots + 2);
        }
    }

    // In this vectors put all the possible point that the ants can visit.
    // Wrap them from a side to the other instead of simply end calculation
    for (int i = 0; i < g_logical_screen_x_dots; i++)
    {
        s_incx[0][i] = i;
        s_incx[2][i] = i;
    }

    for (int i = 0; i < g_logical_screen_x_dots; i++)
    {
        s_incx[3][i] = i + 1;
    }
    s_incx[3][g_logical_screen_x_dots-1] = 0; // wrap from right of the screen to left

    for (int i = 1; i < g_logical_screen_x_dots; i++)
    {
        s_incx[1][i] = i - 1;
    }
    s_incx[1][0] = g_logical_screen_x_dots-1; // wrap from left of the screen to right

    for (int i = 0; i < g_logical_screen_y_dots; i++)
    {
        s_incy[1][i] = i;
        s_incy[3][i] = i;
    }
    for (int i = 0; i < g_logical_screen_y_dots; i++)
    {
        s_incy[0][i] = i + 1;
    }
    s_incy[0][g_logical_screen_y_dots - 1] = 0; // wrap from the top of the screen to the bottom
    for (int i = 1; i < g_logical_screen_y_dots; i++)
    {
        s_incy[2][i] = i - 1;
    }
    s_incy[2][0] = g_logical_screen_y_dots - 1; // wrap from the bottom of the screen to the top
    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP_ANT_COMMANDS;
    long const maxpts = labs(static_cast<long>(g_params[1]));
    long const wait = std::abs(g_orbit_delay);
    std::string rule{get_rule()};
    int rule_len = (int) rule.length();;
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

    srand(g_random_seed);
    if (!g_random_seed_flag)
    {
        ++g_random_seed;
    }

    int maxants = static_cast<int>(g_params[2]);
    if (maxants < 1)               // if maxants == 0 maxants random
    {
        maxants = 2 + random(256 - 2);
    }
    else if (maxants > MAX_ANTS)
    {
        maxants = MAX_ANTS;
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
        TurkMite1(maxants, rule_len, rule.c_str(), maxpts, wait);
        break;
    case 2:
        TurkMite2(maxants, rule_len, rule.c_str(), maxpts, wait);
        break;
    default:
        break;
    }
    g_help_mode = old_help_mode;
    return 0;
}
