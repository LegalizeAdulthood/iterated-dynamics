/*
 *
 * This GIF decoder is designed for use with this program.
 * This decoder code lacks full generality in the following respects:
 * supports non-interlaced GIF files only, and calmly ignores any
 * local color maps and non-Fractint-specific extension blocks.
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 */
#include "port.h"
#include "prototyp.h"

#include "gifview.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "decoder.h"
#include "diskvid.h"
#include "drivers.h"
#include "engine_timer.h"
#include "get_line.h"
#include "has_ext.h"
#include "id.h"
#include "id_data.h"
#include "loadfile.h"
#include "loadmap.h"
#include "os.h"
#include "out_line.h"
#include "pixel_limits.h"
#include "plot3d.h"
#include "rotate.h"
#include "slideshw.h"
#include "sound.h"
#include "spindac.h"
#include "stereo.h"
#include "wait_until.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <vector>

enum
{
    MAXCOLORS = 256
};

constexpr const char *ALTERNATE_FRACTAL_TYPE{".fra"};

/*
 * DECODERLINEWIDTH is the width of the pixel buffer used by the decoder. A
 * larger buffer gives better performance. However, this buffer does not
 * have to be a whole line width, although historically it has
 * been: images were decoded line by line and a whole line written to the
 * screen at once. The requirement to have a whole line buffered at once
 * has now been relaxed in order to support larger images. The one exception
 * to this is in the case where the image is being decoded to a smaller size.
 * The skipxdots and skipydots logic assumes that the buffer holds one line.
 */

constexpr int DECODERLINE_WIDTH{MAX_PIXELS};

static void close_file();
static int out_line_dither(BYTE *, int);
static int out_line_migs(BYTE *, int);
static int out_line_too_wide(BYTE *, int);

static std::FILE *s_fp_in{};
static int s_col_count{};                    // keeps track of current column for wide images
static unsigned int s_gifview_image_top{};   // (for migs)
static unsigned int s_gifview_image_left{};  // (for migs)
static unsigned int s_gifview_image_width{}; // (for migs)
static std::vector<char> s_dither_buf;

unsigned int g_height{};
unsigned int g_num_colors{};

int get_byte()
{
    return getc(s_fp_in); // EOF is -1, as desired
}

int get_bytes(BYTE *where, int how_many)
{
    return (int) std::fread((char *)where, 1, how_many, s_fp_in); // EOF is -1, as desired
}

// Main entry decoder

int gifview()
{
    BYTE buffer[16];
    unsigned top;
    unsigned left;
    unsigned width;
    char temp1[FILE_MAX_DIR];
    BYTE byte_buf[257]; // for decoder
    int status;
    int planes;

    // using stack for decoder byte buf rather than static mem
    set_byte_buff(byte_buf);

    status = 0;

    // initialize the col and row count for write-lines
    g_row_count = 0;
    s_col_count = g_row_count;

    // Open the file
    if (g_out_line == outline_stereo)
    {
        std::strcpy(temp1, g_stereo_map_filename.c_str());
    }
    else
    {
        std::strcpy(temp1, g_read_filename.c_str());
    }
    if (has_ext(temp1) == nullptr)
    {
        std::strcat(temp1, DEFAULT_FRACTAL_TYPE);
        s_fp_in = std::fopen(temp1, "rb");
        if (s_fp_in != nullptr)
        {
            std::fclose(s_fp_in);
        }
        else
        {
            if (g_out_line == outline_stereo)
            {
                std::strcpy(temp1, g_stereo_map_filename.c_str());
            }
            else
            {
                std::strcpy(temp1, g_read_filename.c_str());
            }
            std::strcat(temp1, ALTERNATE_FRACTAL_TYPE);
        }
    }
    s_fp_in = std::fopen(temp1, "rb");
    if (s_fp_in == nullptr)
    {
        return -1;
    }

    // Get the screen description
    for (int i = 0; i < 13; i++)
    {
        int tmp = get_byte();
        buffer[i] = (BYTE) tmp;
        if (tmp < 0)
        {
            close_file();
            return -1;
        }
    }

    if (std::strncmp((char *)buffer, "GIF87a", 3)             // use updated GIF specs
        || buffer[3] < '0' || buffer[3] > '9'
        || buffer[4] < '0' || buffer[4] > '9'
        || buffer[5] < 'A' || buffer[5] > 'z')
    {
        close_file();
        return -1;
    }

    width  = buffer[6] | (buffer[7] << 8);
    g_height = buffer[8] | (buffer[9] << 8);
    planes = (buffer[10] & 0x0F) + 1;
    s_gifview_image_width = width;

    if ((buffer[10] & 0x80) == 0)    // color map (better be!)
    {
        close_file();
        return -1;
    }
    g_num_colors = 1 << planes;

    if (g_dither_flag && g_num_colors > 2 && g_colors == 2 && g_out_line == out_line)
    {
        g_out_line = out_line_dither;
    }

    for (int i = 0; i < (int)g_num_colors; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            int k = get_byte();
            if (k < 0)
            {
                close_file();
                return -1;
            }
            if ((g_display_3d == display_3d_modes::NONE || (g_glasses_type != 1 && g_glasses_type != 2))
                && g_read_color)
            {
                g_dac_box[i][j] = (BYTE)(k >> 2); // TODO: don't right shift color table by 2
            }
        }
    }
    g_color_state = color_state::UNKNOWN; // colors aren't default and not a known .map file

    // don't read if glasses
    if (g_display_3d != display_3d_modes::NONE && g_map_set && g_glasses_type != 1 && g_glasses_type != 2)
    {
        ValidateLuts(g_map_name.c_str());  // read the palette file
        spindac(0, 1); // load it, but don't spin
    }
    if (g_dac_box[0][0] != 255)
    {
        spindac(0, 1);       // update the DAC
    }
    if (driver_diskp())
    {
        // disk-video
        dvid_status(1, "restoring " + std::filesystem::path(temp1).filename().string());
    }
    g_read_color = true;

    // Now display one or more GIF objects
    bool finished = false;
    while (!finished)
    {
        switch (get_byte())
        {
        case ';':
            // End of the GIF dataset
            finished = true;
            status = 0;
            break;

        case '!':                               // GIF Extension Block
            get_byte();                     // read (and ignore) the ID
            {
                int i;
                while ((i = get_byte()) > 0)      // get the data length
                {
                    for (int j = 0; j < i; j++)
                    {
                        get_byte();     // flush the data
                    }
                }
            }
            break;
        case ',':
            /*
             * Start of an image object. Read the image description.
             */

            for (int i = 0; i < 9; i++)
            {
                int tmp = get_byte();
                buffer[i] = (BYTE) tmp;
                if (tmp < 0)
                {
                    status = -1;
                    break;
                }
            }
            if (status < 0)
            {
                finished = true;
                break;
            }

            left   = buffer[0] | (buffer[1] << 8);
            top    = buffer[2] | (buffer[3] << 8);
            width  = buffer[4] | (buffer[5] << 8);
            g_height = buffer[6] | (buffer[7] << 8);

            // adjustments for handling MIGs
            s_gifview_image_top  = top;
            if (g_skip_x_dots > 0)
            {
                s_gifview_image_top /= (g_skip_y_dots+1);
            }
            s_gifview_image_left = left;
            if (g_skip_y_dots > 0)
            {
                s_gifview_image_left /= (g_skip_x_dots+1);
            }
            if (g_out_line == out_line)
            {
                // what about continuous potential????
                if (width != s_gifview_image_width || top != 0)
                {
                    // we're using normal decoding and we have a MIG
                    g_out_line = out_line_migs;
                }
                else if (width > DECODERLINE_WIDTH && g_skip_x_dots == 0)
                {
                    g_out_line = out_line_too_wide;
                }
            }

            if (g_potential_16bit)
            {
                width >>= 1;
            }

            // Skip local color palette
            if ((buffer[8] & 0x80) == 0x80)
            {
                // local map?
                int numcolors;    // make this local
                planes = (buffer[8] & 0x0F) + 1;
                numcolors = 1 << planes;
                // skip local map
                for (int i = 0; i < numcolors; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        if (get_byte() < 0)
                        {
                            close_file();
                            return -1;
                        }
                    }
                }
            }

            // initialize the row count for write-lines
            g_row_count = 0;

            if (g_calc_status == calc_status_value::IN_PROGRESS)   // should never be so, but make sure
            {
                g_calc_status = calc_status_value::PARAMS_CHANGED;
            }
            g_busy = true;      // for slideshow CALCWAIT
            /*
             * Call decoder(width) via timer.
             * Width is limited to DECODERLINE_WIDTH.
             */
            if (g_skip_x_dots == 0)
            {
                width = std::min(width, static_cast<unsigned>(DECODERLINE_WIDTH));
            }
            status = timer(timer_type::DECODER, nullptr, width);
            g_busy = false;      // for slideshow CALCWAIT
            if (g_calc_status == calc_status_value::IN_PROGRESS) // e.g., set by line3d
            {
                g_calc_time = g_timer_interval; // note how long it took
                if (driver_key_pressed() != 0)
                {
                    g_calc_status = calc_status_value::NON_RESUMABLE; // interrupted, not resumable
                    finished = true;
                }
                else
                {
                    g_calc_status = calc_status_value::COMPLETED; // complete
                }
            }
            // Hey! the decoder doesn't read the last (0-length) block!!
            if (get_byte() != 0)
            {
                status = -1;
                finished = true;
            }
            break;
        default:
            status = -1;
            finished = true;
            break;
        }
    }
    close_file();
    if (driver_diskp())
    {
        // disk-video
        dvid_status(0, "Restore completed");
        dvid_status(1, "");
    }

    return status;
}

static void close_file()
{
    std::fclose(s_fp_in);
    s_fp_in = nullptr;
}

// routine for MIGS that generates partial output lines

static int out_line_migs(BYTE *pixels, int linelen)
{
    int row;
    int startcol;
    int stopcol;

    row = s_gifview_image_top + g_row_count;
    startcol = s_gifview_image_left;
    stopcol = startcol+linelen-1;
    put_line(row, startcol, stopcol, pixels);
    g_row_count++;

    return 0;
}

static int out_line_dither(BYTE *pixels, int linelen)
{
    int nexterr;
    int brt;
    int err;
    s_dither_buf.resize(linelen + 1);
    std::fill(s_dither_buf.begin(), s_dither_buf.end(), 0);

    nexterr = (std::rand()&0x1f)-16;
    for (int i = 0; i < linelen; i++)
    {
        brt = (g_dac_box[pixels[i]][0]*5+g_dac_box[pixels[i]][1]*9 +
               g_dac_box[pixels[i]][2]*2) >> 4; // brightness from 0 to 63
        brt += nexterr;
        if (brt > 32)
        {
            pixels[i] = 1;
            err = brt-63;
        }
        else
        {
            pixels[i] = 0;
            err = brt;
        }
        nexterr = s_dither_buf[i+1]+err/3;
        s_dither_buf[i] = (char)(err/3);
        s_dither_buf[i+1] = (char)(err/3);
    }
    return out_line(pixels, linelen);
}

// routine for images wider than the row buffer

static int out_line_too_wide(BYTE *pixels, int linelen)
{
    int twidth = g_logical_screen_x_dots;
    int extra;
    while (linelen > 0)
    {
        extra = s_col_count+linelen-twidth;
        if (extra > 0) // line wraps
        {
            put_line(g_row_count, s_col_count, twidth-1, pixels);
            pixels += twidth-s_col_count;
            linelen -= twidth-s_col_count;
            s_col_count = twidth;
        }
        else
        {
            put_line(g_row_count, s_col_count, s_col_count+linelen-1, pixels);
            s_col_count += linelen;
            linelen = 0;
        }
        if (s_col_count >= twidth)
        {
            s_col_count = 0;
            g_row_count++;
        }
    }
    return 0;
}

static bool put_sound_line(int row, int colstart, int colstop, BYTE *pixels)
{
    for (int col = colstart; col <= colstop; col++)
    {
        g_put_color(col, row, *pixels);
        if (g_orbit_delay > 0)
        {
            sleepms(g_orbit_delay);
        }
        w_snd((int)((int)(*pixels++)*3000/g_colors+g_base_hertz));
        if (driver_key_pressed())
        {
            driver_mute();
            return true;
        }
    }
    return false;
}

int sound_line(BYTE *pixels, int linelen)
{
    int twidth = g_logical_screen_x_dots;
    int extra;
    int ret = 0;
    while (linelen > 0)
    {
        extra = s_col_count+linelen-twidth;
        if (extra > 0) // line wraps
        {
            if (put_sound_line(g_row_count, s_col_count, twidth-1, pixels))
            {
                break;
            }
            pixels += twidth-s_col_count;
            linelen -= twidth-s_col_count;
            s_col_count = twidth;
        }
        else
        {
            if (put_sound_line(g_row_count, s_col_count, s_col_count+linelen-1, pixels))
            {
                break;
            }
            s_col_count += linelen;
            linelen = 0;
        }
        if (s_col_count >= twidth)
        {
            s_col_count = 0;
            g_row_count++;
        }
    }
    driver_mute();
    if (driver_key_pressed())
    {
        ret = -1;
    }
    return ret;
}

int pot_line(BYTE *pixels, int linelen)
{
    int row;
    int saverowcount;
    if (g_row_count == 0)
    {
        if (pot_startdisk() < 0)
        {
            return -1;
        }
    }
    saverowcount = g_row_count;
    g_row_count >>= 1;
    row = g_row_count;
    if ((saverowcount & 1) != 0)   // odd line
    {
        row += g_logical_screen_y_dots;
    }
    else if (!driver_diskp())     // even line - display the line too
    {
        out_line(pixels, linelen);
    }
    for (int col = 0; col < g_logical_screen_x_dots; ++col)
    {
        writedisk(col+g_logical_screen_x_offset, row+g_logical_screen_y_offset, *(pixels+col));
    }
    g_row_count = saverowcount + 1;
    return 0;
}
