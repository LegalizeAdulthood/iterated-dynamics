// SPDX-License-Identifier: GPL-3.0-only
//
/*
    A module to view 3D images.
    Written in Borland 'C++' by Paul de Leeuw.
    From an idea in "New Scientist" 9 October 1993 pages 26 - 29.
*/
#include "ui/stereo.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "engine/pixel_limits.h"
#include "helpdefs.h"
#include "io/decoder.h"
#include "io/encoder.h"
#include "io/gifview.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/find_special_colors.h"
#include "ui/id_keys.h"
#include "ui/rotate.h"
#include "ui/spindac.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/video.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

using namespace id;

std::string g_stereo_map_filename;
int g_auto_stereo_depth{100};
double g_auto_stereo_width{10};
bool g_gray_flag{};  // flag to use gray value rather than color number
char g_calibrate{1}; // add calibration bars to image
bool g_image_map{};

// TODO: sort out the crazy usage of this structure

// this structure permits variables to be temporarily static and visible
// to routines in this file without permanently hogging memory
struct StereoData
{
    long avg;
    long avg_ct;
    long depth;
    int bar_height;
    int ground;
    int max_cc;
    int max_c;
    int min_c;
    int reverse;
    int sep;
    double width;
    int x1;
    int x2;
    int x_center;
    int y;
    int y1;
    int y2;
    int y_center;
    Byte *save_dac;
};

using DACBox = Byte (*)[256][3];

static StereoData *s_data{};

// TODO: eliminate all these macros for structure access

#define AVG         (s_data->avg)
#define AVG_CT      (s_data->avg_ct)
#define DEPTH       (s_data->depth)
#define BAR_HEIGHT  (s_data->bar_height)
#define GROUND      (s_data->ground)
#define MAX_CC      (s_data->max_cc)
#define MAX_C       (s_data->max_c)
#define MIN_C       (s_data->min_c)
#define REVERSE     (s_data->reverse)
#define SEP         (s_data->sep)
#define WIDTH       (s_data->width)
#define X1          (s_data->x1)
#define X2          (s_data->x2)
#define Y           (s_data->y)
#define Y1          (s_data->y1)
#define Y2          (s_data->y2)
#define X_CENTER    (s_data->x_center)
#define Y_CENTER    (s_data->y_center)

/*
   The getdepth() function allows using the grayscale value of the color
   as DEPTH, rather than the color number. Maybe I got a little too
   sophisticated trying to avoid a divide, so the comment tells what all
   the multiplies and shifts are trying to do. The result should be from
   0 to 255.
*/

#define DAC   (*((DACBox)(s_data->save_dac)))

static int get_depth(int xd, int yd)
{
    int pal = get_color(xd, yd);
    if (g_gray_flag)
    {
        // effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255
        pal = ((int) DAC[pal][0] * 77 +
               (int) DAC[pal][1] * 151 +
               (int) DAC[pal][2] * 28);
        pal >>= 6;
    }
    return pal;
}

/*
   Get min and max DEPTH value in picture
*/

static bool get_min_max()
{
    MIN_C = g_colors;
    MAX_C = 0;
    for (int yd = 0; yd < g_logical_screen_y_dots; yd++)
    {
        if (driver_key_pressed())
        {
            return true;
        }
        if (yd == 20)
        {
            show_temp_msg("Getting min and max");
        }
        for (int xd = 0; xd < g_logical_screen_x_dots; xd++)
        {
            int depth = get_depth(xd, yd);
            MIN_C = std::min(depth, MIN_C);
            MAX_C = std::max(depth, MAX_C);
        }
    }
    clear_temp_msg();
    return false;
}

static void toggle_bars(bool *bars, int bar_width, const int *colour)
{
    find_special_colors();
    int ct = 0;
    for (int i = X_CENTER; i < (X_CENTER) + bar_width; i++)
    {
        for (int j = Y_CENTER; j < (Y_CENTER) + BAR_HEIGHT; j++)
        {
            if (*bars)
            {
                g_put_color(i + (int)(AVG), j , g_color_bright);
                g_put_color(i - (int)(AVG), j , g_color_bright);
            }
            else
            {
                g_put_color(i + (int)(AVG), j, colour[ct++]);
                g_put_color(i - (int)(AVG), j, colour[ct++]);
            }
        }
    }
    *bars = !*bars;
}

int out_line_stereo(Byte *pixels, int line_len)
{
    if ((Y) >= g_logical_screen_y_dots)
    {
        return 1;
    }

    std::vector<int> same;
    same.resize(g_logical_screen_x_dots);
    for (int x = 0; x < g_logical_screen_x_dots; ++x)
    {
        same[x] = x;
    }
    std::vector<int> colour;
    colour.resize(g_logical_screen_x_dots);
    for (int x = 0; x < g_logical_screen_x_dots; ++x)
    {
        if (REVERSE)
        {
            SEP = GROUND - (int)(DEPTH * (get_depth(x, Y) - MIN_C) / MAX_CC);
        }
        else
        {
            SEP = GROUND - (int)(DEPTH * (MAX_CC - (get_depth(x, Y) - MIN_C)) / MAX_CC);
        }
        SEP = (int)((SEP * 10.0) / WIDTH);         // adjust for media WIDTH

        // get average value under calibration bars
        if (X1 <= x && x <= X2 && Y1 <= Y && Y <= Y2)
        {
            AVG += SEP;
            (AVG_CT)++;
        }
        int i = x - (SEP + (SEP & Y & 1)) / 2;
        int j = i + SEP;
        if (0 <= i && j < g_logical_screen_x_dots)
        {
            // there are cases where next never terminates so we time out
            int ct = 0;
            for (int s = same[i]; s != i && s != j && ct++ < g_logical_screen_x_dots; s = same[i])
            {
                if (s > j)
                {
                    same[i] = j;
                    i = j;
                    j = s;
                }
                else
                {
                    i = s;
                }
            }
            same[i] = j;
        }
    }
    for (int x = g_logical_screen_x_dots - 1; x >= 0; x--)
    {
        if (same[x] == x)
        {
            colour[x] = (int)pixels[x%line_len];
        }
        else
        {
            colour[x] = colour[same[x]];
        }
        g_put_color(x, Y, colour[x]);
    }
    (Y)++;
    return 0;
}

/**************************************************************************
        Convert current image into Auto Stereo Picture
**************************************************************************/

bool auto_stereo_convert()
{
    // TODO: replace this stack variable with static data s_data
    StereoData v;
    Byte save_dac_box[256*3];
    bool ret = false;
    bool bars;
    std::time_t now;
    std::vector<int> colour;
    colour.resize(g_logical_screen_x_dots);

    s_data = &v;   // set static vars to stack structure
    s_data->save_dac = save_dac_box;

    // Use the current time to randomize the random number sequence.
    std::time(&now);
    std::srand((unsigned int)now);

    ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_RDS_KEYS};
    driver_save_graphics();                      // save graphics image
    std::memcpy(save_dac_box, g_dac_box, 256 * 3);  // save colors

    if (g_logical_screen_x_dots > OLD_MAX_PIXELS)
    {
        stop_msg("Stereo not allowed with resolution > 2048 pixels wide");
        driver_buzzer(Buzzer::INTERRUPT);
        ret = true;
        goto exit_stereo;
    }

    // empircally determined adjustment to make WIDTH scale correctly
    WIDTH = g_auto_stereo_width*.67;
    WIDTH = std::max(WIDTH, 1.0);
    GROUND = g_logical_screen_x_dots / 8;
    if (g_auto_stereo_depth < 0)
    {
        REVERSE = 1;
    }
    else
    {
        REVERSE = 0;
    }
    DEPTH = ((long) g_logical_screen_x_dots * (long) g_auto_stereo_depth) / 4000L;
    DEPTH = std::abs(DEPTH) + 1;
    if (get_min_max())
    {
        driver_buzzer(Buzzer::INTERRUPT);
        ret = true;
        goto exit_stereo;
    }
    MAX_CC = MAX_C - MIN_C + 1;
    AVG_CT = 0L;
    AVG = AVG_CT;
    BAR_HEIGHT = 1 + g_logical_screen_y_dots / 20;
    X_CENTER = g_logical_screen_x_dots/2;
    if (g_calibrate > 1)
    {
        Y_CENTER = BAR_HEIGHT/2;
    }
    else
    {
        Y_CENTER = g_logical_screen_y_dots/2;
    }

    // box to average for calibration bars
    X1 = X_CENTER - g_logical_screen_x_dots/16;
    X2 = X_CENTER + g_logical_screen_x_dots/16;
    Y1 = Y_CENTER - BAR_HEIGHT/2;
    Y2 = Y_CENTER + BAR_HEIGHT/2;

    Y = 0;
    if (g_image_map)
    {
        g_out_line = out_line_stereo;
        while ((Y) < g_logical_screen_y_dots)
        {
            if (gif_view())
            {
                ret = true;
                goto exit_stereo;
            }
        }
    }
    else
    {
        std::vector<Byte> buf;
        buf.resize(g_logical_screen_x_dots);
        while (Y < g_logical_screen_y_dots)
        {
            if (driver_key_pressed())
            {
                ret = true;
                goto exit_stereo;
            }
            for (int i = 0; i < g_logical_screen_x_dots; i++)
            {
                buf[i] = (unsigned char)(std::rand()%g_colors);
            }
            out_line_stereo(buf.data(), g_logical_screen_x_dots);
        }
    }

    find_special_colors();
    AVG /= AVG_CT;
    AVG /= 2;
    {
        bool done = false;
        int ct = 0;
        const int bar_width = 1 + g_logical_screen_x_dots / 200;
        for (int i = X_CENTER; i < X_CENTER + bar_width; i++)
        {
            for (int j = Y_CENTER; j < Y_CENTER + BAR_HEIGHT; j++)
            {
                colour[ct++] = get_color(i + (int)(AVG), j);
                colour[ct++] = get_color(i - (int)(AVG), j);
            }
        }
        bars = g_calibrate != 0;
        toggle_bars(&bars, bar_width, colour.data());
        while (!done)
        {
            driver_wait_key_pressed(false);
            switch (int key = driver_get_key(); key)
            {
            case ID_KEY_ENTER:   // toggle bars
            case ID_KEY_SPACE:
                toggle_bars(&bars, bar_width, colour.data());
                break;
            case 'c':
            case '+':
            case '-':
                rotate((key == 'c') ? 0 : ((key == '+') ? 1 : -1));
                break;
            case 's':
            case 'S':
                save_image(g_save_filename);
                break;
            default:
                if (key == ID_KEY_ESC)     // if ESC avoid returning to menu
                {
                    key = 255;
                }
                driver_unget_key(key);
                driver_buzzer(Buzzer::COMPLETE);
                done = true;
                break;
            }
        }
    }

exit_stereo:
    driver_restore_graphics();
    std::memcpy(g_dac_box, save_dac_box, 256 * 3);
    spin_dac(0, 1);
    return ret;
}
