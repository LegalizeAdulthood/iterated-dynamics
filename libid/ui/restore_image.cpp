// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#include "ui/restore_image.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/color_state.h"
#include "engine/engine_timer.h"
#include "engine/LogicalScreen.h"
#include "engine/Potential.h"
#include "engine/sound.h"
#include "engine/spindac.h"
#include "engine/VideoInfo.h"
#include "fractals/lorenz.h"
#include "geometry/plot3d.h"
#include "io/decoder.h"
#include "io/GifRestore.h"
#include "io/gifview.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "io/RestoredImageOutput.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/ValueSaver.h"
#include "ui/diskvid.h"
#include "ui/evolve.h"
#include "ui/slideshw.h"
#include "ui/SoundSession.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"

#include <filesystem>
#include <optional>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

namespace
{

constexpr const char *ALTERNATE_FRACTAL_TYPE{".fra"};

int restore_image_impl(SoundSession *sound)
{
    ValueSaver busy{g_busy};
    if (g_out_line != out_line_stereo)
    {
        setup_stereo_palette();
    }
    g_row_count = 0;
    std::optional<RestoredImageOutput> output;
    unsigned int image_top{};
    unsigned int image_left{};
    bool migs{};

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
    if (sound == nullptr && g_dither_flag && g_num_colors > 2 && g_colors == 2 && g_out_line == out_line)
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
        const PaletteLoadResult result{validate_luts(g_map_name)};
        if (!result.diagnostic.empty())
        {
            stop_msg(result.diagnostic);
        }
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
            image_top = image.top;
            if (g_skip_y_dots > 0)
            {
                image_top /= g_skip_y_dots + 1;
            }
            image_left = image.left;
            if (g_skip_x_dots > 0)
            {
                image_left /= g_skip_x_dots + 1;
            }
            if (sound == nullptr && g_out_line == out_line && (image.width != restore.width() || image.top != 0))
            {
                migs = true;
            }
            g_row_count = 0;
            if (sound != nullptr)
            {
                output.emplace(g_logical_screen.x_dots, 0, output ? output->column() : 0);
            }
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
            if (sound != nullptr)
            {
                output->set_line(pixels.data(), static_cast<int>(pixels.size()));
                while (!output->done())
                {
                    if (!sound->wait_until_ready(SoundSession::InputPolicy::ATOMIC))
                    {
                        status = -1;
                        break;
                    }
                    output->iterate();
                }
                if (status == 0 && !sound->wait_until_ready(SoundSession::InputPolicy::ATOMIC))
                {
                    status = -1;
                }
                g_row_count = output->row();
            }
            else
            {
                status = migs ? out_line_migs(pixels.data(), static_cast<int>(pixels.size()), image_left, image_top)
                              : g_out_line(pixels.data(), static_cast<int>(pixels.size()));
            }
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

} // namespace

int restore_image()
{
    if (g_out_line == out_line && (g_sound_flag & SOUNDFLAG_ORBIT_MASK) > SOUNDFLAG_BEEP &&
        g_evolving == EvolutionModeFlags::NONE)
    {
        SoundSession sound{DriverSoundOutput::Source::RESTORED_IMAGE};
        return restore_image(sound);
    }
    return restore_image_impl(nullptr);
}

int restore_image(SoundSession &sound)
{
    const int status{restore_image_impl(&sound)};
    return sound.finish(SoundSession::InputPolicy::ATOMIC) ? status : -1;
}

} // namespace id::ui
