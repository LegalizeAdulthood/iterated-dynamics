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
#include "engine/Viewport.h"
#include "fractals/fractalp.h"
#include "fractals/lorenz.h"
#include "geometry/plot3d.h"
#include "geometry/stereo_params.h"
#include "geometry/StereoView.h"
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
#include "ui/big_while_loop.h"
#include "ui/diskvid.h"
#include "ui/evolve.h"
#include "ui/get_3d_params.h"
#include "ui/get_video_mode.h"
#include "ui/slideshw.h"
#include "ui/SoundSession.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"
#include "ui/tab_display.h"
#include "ui/video_mode.h"

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <optional>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

int prepare_image_restore()
{
    g_show_file = ShowFile::IMAGE_LOADED;
    g_init_mode = -1;
    if (!g_read_filename.has_extension())
    {
        g_read_filename.replace_extension(".gif");
    }

    ImageMetadata metadata;
    try
    {
        const std::filesystem::path path{find_file(ReadFile::IMAGE, g_read_filename)};
        metadata = read_image_metadata(path.empty() ? g_read_filename : path);
    }
    catch (const std::runtime_error &error)
    {
        stop_msg(error.what());
        return -1;
    }
    ValueSaver pending_tab{g_tab_metadata, std::make_shared<const ImageMetadata>(metadata)};
    std::optional<ImageVideoMode> video;
    ThreeDSettings three_d;
    if (g_overlay_3d)
    {
        const int width{
            metadata.fractal.info_version > 4 && metadata.fractal.pot16bit ? metadata.width / 2 : metadata.width};
        if (width > g_logical_screen.x_dots || metadata.height > g_logical_screen.y_dots)
        {
            stop_msg("Can't overlay with a larger image");
            return -1;
        }
    }
    else
    {
        Viewport viewport{g_viewport};
        viewport.enabled = false;
        video = get_video_mode(metadata, viewport);
        driver_check_memory();
        if (!video)
        {
            return -1;
        }
    }
    if (g_display_3d != Display3DMode::NONE && g_init_batch == BatchMode::NONE && get_3d_params(three_d) < 0)
    {
        return -1;
    }

    apply_image_parameters(metadata);
    if (video)
    {
        g_init_mode = video->mode;
        g_video_entry = video->video;
        g_video_entry_key = g_video_mode_keys[video->mode];
        if (g_init_mode == MAX_VIDEO_MODES - 1)
        {
            g_video_table[g_init_mode] = video->video;
            g_video_mode_keys[g_init_mode] = 0;
            g_video_entry_key = 0;
        }
        g_viewport = video->viewport;
        g_file_x_dots = video->width;
        g_file_y_dots = video->height;
        g_file_aspect_ratio = video->aspect_ratio;
        g_skip_x_dots = video->skip_x;
        g_skip_y_dots = video->skip_y;
        g_calc_status = video->calc_status;
    }
    else
    {
        g_init_mode = g_adapter;
    }
    if (g_display_3d != Display3DMode::NONE)
    {
        three_d.apply();
        g_calc_status = CalcStatus::PARAMS_CHANGED;
        set_fractal_type(FractalType::PLASMA);
        g_params[0] = 0;
    }

    apply_image_extensions(metadata);
    if (metadata.evolution.got_data)
    {
        const ExtBlock6 &blk_6_info{metadata.evolution};
        g_have_evolve_info = g_calc_status == CalcStatus::RESUMABLE;
        if (g_have_evolve_info)
        {
            g_evolve_info.x_parameter_range = blk_6_info.x_parameter_range;
            g_evolve_info.y_parameter_range = blk_6_info.y_parameter_range;
            g_evolve_info.x_parameter_offset = blk_6_info.x_parameter_offset;
            g_evolve_info.y_parameter_offset = blk_6_info.y_parameter_offset;
            g_evolve_info.discrete_x_parameter_offset = blk_6_info.discrete_x_parameter_offset;
            g_evolve_info.discrete_y_parameter_offset = blk_6_info.discrete_y_parameter_offset;
            g_evolve_info.px = blk_6_info.px;
            g_evolve_info.py = blk_6_info.py;
            g_evolve_info.screen_x_offset = blk_6_info.sx_offs;
            g_evolve_info.screen_y_offset = blk_6_info.sy_offs;
            g_evolve_info.x_dots = blk_6_info.x_dots;
            g_evolve_info.y_dots = blk_6_info.y_dots;
            g_evolve_info.image_grid_size = blk_6_info.image_grid_size;
            g_evolve_info.evolving = blk_6_info.evolving;
            g_evolve_info.this_generation_random_seed = blk_6_info.this_generation_random_seed;
            g_evolve_info.max_random_mutation = blk_6_info.max_random_mutation;
            g_evolve_info.count = blk_6_info.e_count;
        }
        save_param_history();
    }
    g_show_file = ShowFile::LOAD_IMAGE;
    return 0;
}

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
        display_disk_storage_progress();
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

int restore_image_views()
{
    int status{-1};
    std::exception_ptr failure;
    try
    {
        StereoView view;
        view.start(false);
        status = restore_image();
        if (status == 0 && view.next_view_required())
        {
            if (g_glasses_type == GlassesType::PHOTO)
            {
                lorenz_stereo_photographer_prompt();
            }
            view.next_view();
            status = restore_image();
            if (status == 0 && g_glasses_type == GlassesType::PHOTO)
            {
                stop_msg(StopMsgFlags::INFO_ONLY, "Second image (right eye) is ready");
            }
        }
    }
    catch (...)
    {
        failure = std::current_exception();
    }
    // The two views share output resources; release them after restoring the screen.
    if (g_out_line_cleanup)
    {
        try
        {
            std::exchange(g_out_line_cleanup, nullptr)();
        }
        catch (const std::runtime_error &error)
        {
            stop_msg(error.what());
            status = -1;
            if (g_init_batch != BatchMode::NONE)
            {
                g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;
            }
        }
    }
    if (failure)
    {
        std::rethrow_exception(failure);
    }
    return status;
}

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
