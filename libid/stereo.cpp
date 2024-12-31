// SPDX-License-Identifier: GPL-3.0-only
//
/*
    A module to view 3D images.
    Written in Borland 'C++' by Paul de Leeuw.
    From an idea in "New Scientist" 9 October 1993 pages 26 - 29.
*/
#include "stereo.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "decoder.h"
#include "drivers.h"
#include "encoder.h"
#include "find_special_colors.h"
#include "gifview.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "pixel_limits.h"
#include "rotate.h"
#include "spindac.h"
#include "stop_msg.h"
#include "temp_msg.h"
#include "ValueSaver.h"
#include "video.h"

#include <cstdlib>
#include <cstring>
#include <ctime>
#include <vector>

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
    long avgct;
    long depth;
    int barheight;
    int ground;
    int maxcc;
    int maxc;
    int minc;
    int reverse;
    int sep;
    double width;
    int x1;
    int x2;
    int xcen;
    int y;
    int y1;
    int y2;
    int ycen;
    BYTE *savedac;
};

using DACBox = BYTE (*)[256][3];

static StereoData *s_data{};

// TODO: eliminate all these macros for structure access

#define AVG         (s_data->avg)
#define AVGCT       (s_data->avgct)
#define DEPTH       (s_data->depth)
#define BARHEIGHT   (s_data->barheight)
#define GROUND      (s_data->ground)
#define MAXCC       (s_data->maxcc)
#define MAXC        (s_data->maxc)
#define MINC        (s_data->minc)
#define REVERSE     (s_data->reverse)
#define SEP         (s_data->sep)
#define WIDTH       (s_data->width)
#define X1          (s_data->x1)
#define X2          (s_data->x2)
#define Y           (s_data->y)
#define Y1          (s_data->y1)
#define Y2          (s_data->y2)
#define XCEN        (s_data->xcen)
#define YCEN        (s_data->ycen)

/*
   The getdepth() function allows using the grayscale value of the color
   as DEPTH, rather than the color number. Maybe I got a little too
   sophisticated trying to avoid a divide, so the comment tells what all
   the multiplies and shifts are trying to do. The result should be from
   0 to 255.
*/

#define dac   (*((DACBox)(s_data->savedac)))

static int get_depth(int xd, int yd)
{
    int pal;
    pal = get_color(xd, yd);
    if (g_gray_flag)
    {
        // effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255
        pal = ((int) dac[pal][0] * 77 +
               (int) dac[pal][1] * 151 +
               (int) dac[pal][2] * 28);
        pal >>= 6;
    }
    return pal;
}

/*
   Get min and max DEPTH value in picture
*/

static bool get_min_max()
{
    MINC = g_colors;
    MAXC = 0;
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
            int ldepth = get_depth(xd, yd);
            if (ldepth < MINC)
            {
                MINC = ldepth;
            }
            if (ldepth > MAXC)
            {
                MAXC = ldepth;
            }
        }
    }
    clear_temp_msg();
    return false;
}

static void toggle_bars(bool *bars, int barwidth, int const *colour)
{
    find_special_colors();
    int ct = 0;
    for (int i = XCEN; i < (XCEN) + barwidth; i++)
    {
        for (int j = YCEN; j < (YCEN) + BARHEIGHT; j++)
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

int out_line_stereo(BYTE *pixels, int linelen)
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
            SEP = GROUND - (int)(DEPTH * (get_depth(x, Y) - MINC) / MAXCC);
        }
        else
        {
            SEP = GROUND - (int)(DEPTH * (MAXCC - (get_depth(x, Y) - MINC)) / MAXCC);
        }
        SEP = (int)((SEP * 10.0) / WIDTH);         // adjust for media WIDTH

        // get average value under calibration bars
        if (X1 <= x && x <= X2 && Y1 <= Y && Y <= Y2)
        {
            AVG += SEP;
            (AVGCT)++;
        }
        int i = x - (SEP + (SEP & Y & 1)) / 2;
        int j = i + SEP;
        if (0 <= i && j < g_logical_screen_x_dots)
        {
            // there are cases where next never terminates so we timeout
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
            colour[x] = (int)pixels[x%linelen];
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
    BYTE savedacbox[256*3];
    bool ret = false;
    bool bars;
    int ct;
    int kbdchar;
    int barwidth;
    std::time_t ltime;
    std::vector<int> colour;
    colour.resize(g_logical_screen_x_dots);
    bool done = false;

    s_data = &v;   // set static vars to stack structure
    s_data->savedac = savedacbox;

    // Use the current time to randomize the random number sequence.
    std::time(&ltime);
    std::srand((unsigned int)ltime);

    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_RDS_KEYS};
    driver_save_graphics();                      // save graphics image
    std::memcpy(savedacbox, g_dac_box, 256 * 3);  // save colors

    if (g_logical_screen_x_dots > OLD_MAX_PIXELS)
    {
        stop_msg("Stereo not allowed with resolution > 2048 pixels wide");
        driver_buzzer(Buzzer::INTERRUPT);
        ret = true;
        goto exit_stereo;
    }

    // empircally determined adjustment to make WIDTH scale correctly
    WIDTH = g_auto_stereo_width*.67;
    if (WIDTH < 1)
    {
        WIDTH = 1;
    }
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
    DEPTH = labs(DEPTH) + 1;
    if (get_min_max())
    {
        driver_buzzer(Buzzer::INTERRUPT);
        ret = true;
        goto exit_stereo;
    }
    MAXCC = MAXC - MINC + 1;
    AVGCT = 0L;
    AVG = AVGCT;
    barwidth  = 1 + g_logical_screen_x_dots / 200;
    BARHEIGHT = 1 + g_logical_screen_y_dots / 20;
    XCEN = g_logical_screen_x_dots/2;
    if (g_calibrate > 1)
    {
        YCEN = BARHEIGHT/2;
    }
    else
    {
        YCEN = g_logical_screen_y_dots/2;
    }

    // box to average for calibration bars
    X1 = XCEN - g_logical_screen_x_dots/16;
    X2 = XCEN + g_logical_screen_x_dots/16;
    Y1 = YCEN - BARHEIGHT/2;
    Y2 = YCEN + BARHEIGHT/2;

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
        std::vector<BYTE> buf;
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
            out_line_stereo(&buf[0], g_logical_screen_x_dots);
        }
    }

    find_special_colors();
    AVG /= AVGCT;
    AVG /= 2;
    ct = 0;
    for (int i = XCEN; i < XCEN + barwidth; i++)
    {
        for (int j = YCEN; j < YCEN + BARHEIGHT; j++)
        {
            colour[ct++] = get_color(i + (int)(AVG), j);
            colour[ct++] = get_color(i - (int)(AVG), j);
        }
    }
    bars = g_calibrate != 0;
    toggle_bars(&bars, barwidth, &colour[0]);
    while (!done)
    {
        driver_wait_key_pressed(0);
        kbdchar = driver_get_key();
        switch (kbdchar)
        {
        case ID_KEY_ENTER:   // toggle bars
        case ID_KEY_SPACE:
            toggle_bars(&bars, barwidth, &colour[0]);
            break;
        case 'c':
        case '+':
        case '-':
            rotate((kbdchar == 'c') ? 0 : ((kbdchar == '+') ? 1 : -1));
            break;
        case 's':
        case 'S':
            save_image(g_save_filename);
            break;
        default:
            if (kbdchar == ID_KEY_ESC)     // if ESC avoid returning to menu
            {
                kbdchar = 255;
            }
            driver_unget_key(kbdchar);
            driver_buzzer(Buzzer::COMPLETE);
            done = true;
            break;
        }
    }

exit_stereo:
    driver_restore_graphics();
    std::memcpy(g_dac_box, savedacbox, 256 * 3);
    spin_dac(0, 1);
    return ret;
}
