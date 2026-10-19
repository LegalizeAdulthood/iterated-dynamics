// SPDX-License-Identifier: GPL-3.0-only
//
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
#include "io/gifview.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/color_state.h"
#include "engine/engine_timer.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/pixel_limits.h"
#include "engine/Potential.h"
#include "engine/sound.h"
#include "engine/spindac.h"
#include "engine/VideoInfo.h"
#include "engine/wait_until.h"
#include "fractals/lorenz.h"
#include "geometry/plot3d.h"
#include "io/decoder.h"
#include "io/GifRestore.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "io/RestoredImageOutput.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "ui/diskvid.h"
#include "ui/slideshw.h"
#include "ui/stereo.h"
#include "ui/video.h"

#include <config/path_limits.h>

#include <cstdlib>
#include <filesystem>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

constexpr const char *ALTERNATE_FRACTAL_TYPE{".fra"};

bool g_dither_flag{};                           // true if we want to dither GIFs
unsigned int g_height{};
unsigned int g_num_colors{};

static int out_line_dither(Byte *pixels, int line_len);
static int out_line_migs(Byte *pixels, int line_len);

static int s_col_count{};                       // keeps track of current column for wide images
static unsigned int s_gif_view_image_top{};     // (for migs)
static unsigned int s_gif_view_image_left{};    // (for migs)
static std::vector<char> s_dither_buf;

// Main entry decoder

int gif_view()
{
    g_row_count = 0;
    s_col_count = 0;

    std::filesystem::path input_file{
        g_out_line == out_line_stereo ? std::filesystem::path{g_stereo_texture_filename} : g_read_filename};
    if (!input_file.has_extension())
    {
        input_file.replace_extension(DEFAULT_FRACTAL_TYPE);
    }
    input_file = find_file(ReadFile::IMAGE, input_file.string());
    if (input_file.empty())
    {
        input_file.replace_extension(ALTERNATE_FRACTAL_TYPE);
        input_file = find_file(ReadFile::IMAGE, input_file.string());
    }
    if (input_file.empty())
    {
        return -1;
    }

    GifRestore restore{input_file, g_skip_x_dots, g_skip_y_dots, g_potential.store_16bit};
    if (restore.done())
    {
        return restore.result();
    }
    g_height = restore.height();
    g_num_colors = static_cast<unsigned int>(restore.palette().size());
    if (g_dither_flag && g_num_colors > 2 && g_colors == 2 && g_out_line == out_line)
    {
        g_out_line = out_line_dither;
    }
    if ((g_display_3d == Display3DMode::NONE || !glasses_alternating_or_superimpose()) && g_read_color)
    {
        for (unsigned int i = 0; i < g_num_colors; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                g_dac_box[i][j] = restore.palette()[i][j];
            }
        }
    }
    backwards_id1_3_palette(g_dac_box, g_num_colors);
    g_color_state = ColorState::UNKNOWN_MAP; // colors aren't default and not a known .map file

    // don't read if glasses
    if (g_display_3d != Display3DMode::NONE && g_map_set && !glasses_alternating_or_superimpose())
    {
        validate_luts(g_map_name);  // read the palette file
        refresh_dac();
    }
    if (g_dac_box[0][0] != 255)
    {
        refresh_dac();
    }
    if (driver_is_disk())
    {
        // disk-video
        dvid_status(1, "restoring " + input_file.filename().string());
    }
    g_read_color = true;

    int status{};
    bool image_active{};
    while (!restore.done())
    {
        restore.iterate();
        if (restore.state() == GifRestore::State::IMAGE)
        {
            const GifRestore::Image &image{restore.image()};
            g_height = image.height;
            // adjustments for handling MIGs
            s_gif_view_image_top = image.top;
            if (g_skip_y_dots > 0)
            {
                s_gif_view_image_top /= g_skip_y_dots + 1;
            }
            s_gif_view_image_left = image.left;
            if (g_skip_x_dots > 0)
            {
                s_gif_view_image_left /= g_skip_x_dots + 1;
            }
            if (g_out_line == out_line && (image.width != restore.width() || image.top != 0))
            {
                g_out_line = out_line_migs;
            }
            g_row_count = 0;
            if (g_calc_status == CalcStatus::IN_PROGRESS)
            {
                g_calc_status = CalcStatus::PARAMS_CHANGED;
            }
            g_busy = true; // for slideshow CALCWAIT
            image_active = true;
        }
        else if (restore.state() == GifRestore::State::LINE)
        {
            std::vector<Byte> &pixels{restore.line()};
            status = g_out_line(pixels.data(), static_cast<int>(pixels.size()));
        }
        if (image_active && (restore.state() == GifRestore::State::IMAGE_END || restore.done() || status < 0))
        {
            g_busy = false;
            image_active = false;
            if (g_calc_status == CalcStatus::IN_PROGRESS) // e.g., set by line3d
            {
                g_calc_time = g_timer_interval;
                g_calc_status = CalcStatus::COMPLETED;
            }
        }
        if (status < 0)
        {
            break;
        }
    }
    if (status >= 0)
    {
        status = restore.result();
    }
    if (driver_is_disk())
    {
        // disk-video
        dvid_status(0, "Restore completed");
        dvid_status(1, "");
    }

    return status;
}

// routine for MIGS that generates partial output lines

static int out_line_migs(Byte *pixels, const int line_len)
{
    const int row = s_gif_view_image_top + g_row_count;
    const int start_col = s_gif_view_image_left;
    const int stop_col = start_col + line_len - 1;
    write_span(row, start_col, stop_col, pixels);
    g_row_count++;

    return 0;
}

static int out_line_dither(Byte *pixels, const int line_len)
{
    int err;
    s_dither_buf.resize(line_len + 1);
    std::fill(s_dither_buf.begin(), s_dither_buf.end(), 0);

    // Viewing-only dither noise; do not consume the image RNG stream.
    int next_err = (std::rand() & 0x1f) - 16;
    for (int i = 0; i < line_len; i++)
    {
        // TODO: 6-bit color
        int brt = (g_dac_box[pixels[i]][0] * 5 + g_dac_box[pixels[i]][1] * 9 + g_dac_box[pixels[i]][2] * 2) >>
            4; // brightness from 0 to 63
        brt += next_err;
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
        next_err = s_dither_buf[i+1]+err/3;
        s_dither_buf[i] = static_cast<char>(err / 3);
        s_dither_buf[i+1] = static_cast<char>(err / 3);
    }
    return out_line(pixels, line_len);
}

int sound_line(Byte *pixels, const int line_len)
{
    RestoredImageOutput output{g_logical_screen.x_dots, g_row_count, s_col_count};
    output.set_line(pixels, line_len);
    // TODO: removed later
    while (!output.done())
    {
        const int color{output.pending_color()};
        output.iterate();
        if (g_orbit_delay > 0)
        {
            sleep_orbit_delay(g_orbit_delay);
        }
        write_sound(color * 3000 / g_colors + g_base_hertz);
        g_row_count = output.row();
        s_col_count = output.column();
    }
    driver_mute();

    return 0;
}

int pot_line(Byte *pixels, const int line_len)
{
    if (g_row_count == 0)
    {
        if (pot_start_disk() < 0)
        {
            return -1;
        }
    }
    int save_row_count = g_row_count;
    g_row_count >>= 1;
    int row = g_row_count;
    if ((save_row_count & 1) != 0)   // odd line
    {
        row += g_logical_screen.y_dots;
    }
    else if (!driver_is_disk())     // even line - display the line too
    {
        out_line(pixels, line_len);
    }
    for (int col = 0; col < g_logical_screen.x_dots; ++col)
    {
        disk_write_pixel(col+g_logical_screen.x_offset, row+g_logical_screen.y_offset, *(pixels+col));
    }
    g_row_count = save_row_count + 1;
    return 0;
}

} // namespace id::io
