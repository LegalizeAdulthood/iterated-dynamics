// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
//      GIF Encoder and associated routines
//
#include "io/encoder.h"

#include "engine/bailout_formula.h"
#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "engine/engine_timer.h"
#include "engine/fractals.h"
#include "engine/ImageRegion.h"
#include "engine/Inversion.h"
#include "engine/log_map.h"
#include "engine/LogicalScreen.h"
#include "engine/OrbitPlot.h"
#include "engine/pixel.h"
#include "engine/pixel_limits.h"
#include "engine/Potential.h"
#include "engine/random_seed.h"
#include "engine/resume.h"
#include "engine/SolidGuess.h"
#include "engine/spindac.h"
#include "engine/StickyOrbits.h"
#include "engine/trig_fns.h"
#include "engine/UserData.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/formula.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "geometry/3d.h"
#include "geometry/line3d.h"
#include "geometry/plot3d.h"
#include "io/check_write_file.h"
#include "io/gif_extensions.h"
#include "io/gif_file.h"
#include "io/GifWriter.h"
#include "io/is_writeable.h"
#include "io/library.h"
#include "io/loadfile.h"
#include "io/save_timer.h"
#include "io/update_save_name.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"
#include "ui/big_while_loop.h"
#include "io/disk_storage.h"
#include "ui/diskvid.h"
#include "ui/evolve.h"
#include "ui/goodbye.h"
#include "ui/slideshw.h"
#include "ui/SaveImageProgress.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"

#include <gif_lib.h>

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

std::filesystem::path g_save_filename{"fract001"};

static int extend_blk_len(int data_len);
static void store_item_name(GifFileType &extensions, std::string_view name);

// B&W palette
static constexpr Byte s_palette_bw[]{
    0, 0, 0,    //
    63, 63, 63, //
};

// 4-color (CGA) palette
static constexpr Byte s_palette_cga[]{
    0, 0, 0,    //
    21, 63, 63, //
    63, 21, 63, //
    63, 63, 63, //
};

// 16-color (EGA/CGA) pal
static constexpr Byte s_palette_ega[]{
    0, 0, 0,    //
    0, 0, 42,   //
    0, 42, 0,   //
    0, 42, 42,  //
    42, 0, 0,   //
    42, 0, 42,  //
    42, 21, 0,  //
    42, 42, 42, //
    21, 21, 21, //
    21, 21, 63, //
    21, 63, 21, //
    21, 63, 63, //
    63, 21, 21, //
    63, 21, 63, //
    63, 63, 21, //
    63, 63, 63, //
};

static GifWriter::Screen save_screen()
{
    int aspect{};
    if (g_viewport.enabled && (g_viewport.x_dots == 0 || g_viewport.y_dots == 0))
    {
        aspect =
            static_cast<int>(static_cast<double>(g_screen_y_dots) / g_screen_x_dots * 64.0 / g_screen_aspect - 14.5);
    }
    else
    {
        aspect = static_cast<int>(static_cast<double>(g_logical_screen.y_dots) / g_logical_screen.x_dots /
                g_viewport.final_aspect_ratio * 64.0 -
            14.5);
    }
    return {g_logical_screen.x_dots * (g_disk_16_bit ? 2 : 1), g_logical_screen.y_dots, 8, 0,
        static_cast<Byte>(std::clamp(aspect, 1, 255))};
}

static GifWriter::Palette save_palette()
{
    const Byte *colors{g_colors == 2 ? s_palette_bw
            : g_colors == 4          ? s_palette_cga
            : g_got_real_dac         ? &g_dac_box[0][0]
                                     : s_palette_ega};
    GifWriter::Palette palette;
    palette.reserve(g_colors);
    for (int i = 0; i < g_colors; ++i)
    {
        const int color{g_colors > 4 && !g_got_real_dac ? i % 16 : i};
        palette.push_back({colors[3 * color], colors[3 * color + 1], colors[3 * color + 2]});
    }
    return palette;
}

int save_image(std::filesystem::path &filename)
{
    const bool potential{g_disk_16_bit};
    std::filesystem::path open_file;
    std::filesystem::path tmp_file;
    bool new_file{};
    while (true)
    {
        open_file = filename;
        if (!open_file.has_extension())
        {
            open_file.replace_extension(potential ? ".pot" : DEFAULT_FRACTAL_TYPE);
        }
        if (g_resave_flag != TimedSave::STARTED)
        {
            update_save_name(filename);
        }
        open_file = get_save_path(WriteFile::IMAGE, open_file.string());
        if (open_file.empty())
        {
            throw std::logic_error("Empty image save path");
        }
        tmp_file = open_file;
        new_file = !std::filesystem::exists(open_file);
        if (new_file)
        {
            break;
        }
        if (!g_overwrite_file)
        {
            if (g_resave_flag == TimedSave::NONE)
            {
                continue;
            }
            if (!g_started_resaves)
            {
                update_save_name(filename);
                continue;
            }
        }
        if (!is_writeable(open_file))
        {
            // TODO: removed later
            stop_msg("Can't write " + open_file.string());
            return -1;
        }
        tmp_file.replace_filename("id.tmp");
        break;
    }
    g_started_resaves = g_resave_flag == TimedSave::STARTED;
    if (g_resave_flag == TimedSave::FINAL)
    {
        g_resave_flag = TimedSave::NONE;
    }

    bool saved{};
    int error{};
    if (g_logical_screen.x_dots > 0 && g_logical_screen.y_dots > 0 &&
        g_logical_screen.x_dots <= GIF_MAX_PIXELS / (potential ? 2 : 1) && g_logical_screen.y_dots <= GIF_MAX_PIXELS)
    {
        ValueSaver busy{g_busy, true};
        GifWriter writer{tmp_file, save_screen(), save_palette()};
        error = writer.error();
        saved = encoder(writer, open_file);
    }
    if (!saved)
    {
        std::filesystem::remove(tmp_file);
        // TODO: removed later
        stop_msg(error == E_GIF_ERR_OPEN_FAILED ? "Can't create " + tmp_file.string()
                                                : "Error writing image to " + tmp_file.string());
        if (g_init_batch >= BatchMode::NORMAL)
        {
            g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;
        }
        return -1;
    }
    if (!new_file)
    {
        std::filesystem::remove(open_file);
        std::filesystem::rename(tmp_file, open_file);
    }
    if (g_timed_save == TimedSave::NONE)
    {
        // TODO: removed later
        driver_buzzer(Buzzer::COMPLETE);
        if (g_init_batch == BatchMode::NONE)
        {
            // TODO: removed later
            text_temp_msg((" File saved as " + open_file.filename().string() + ' ').c_str());
        }
    }
    if (g_save_time_interval < 0)
    {
        // TODO: removed later
        goodbye();
    }
    return 0;
}

static void setup_save_extensions(GifFileType &extensions)
{
    FractalInfo save_info{};
    setup_save_info(&save_info);
    if (!g_resume_data.empty() && g_calc_status == CalcStatus::RESUMABLE)
    {
        if (g_resume_len < 0 || static_cast<std::size_t>(g_resume_len) > g_resume_data.size())
        {
            throw std::logic_error("Invalid image resume length");
        }
        put_resume_info(&extensions, {g_resume_data.begin(), g_resume_data.begin() + g_resume_len});
    }
    if (g_fractal_type == FractalType::FORMULA)
    {
        store_item_name(extensions, g_formula_name);
    }
    if (g_fractal_type == FractalType::L_SYSTEM)
    {
        store_item_name(extensions, g_l_system_name);
    }
    if (g_fractal_type == FractalType::IFS || g_fractal_type == FractalType::IFS_3D)
    {
        store_item_name(extensions, g_ifs_name);
    }
    if (g_display_3d <= Display3DMode::NONE && !g_iteration_ranges.empty())
    {
        put_ranges_info(&extensions, g_iteration_ranges);
    }
    if (g_bf_math != BFMathType::NONE)
    {
        if (g_bf_x_min == nullptr || g_bf_length < 0 || g_bf_length > std::numeric_limits<int>::max() / 22 - 2)
        {
            throw std::logic_error("Invalid arbitrary precision save parameters");
        }
        // The saved corners and parameters occupy consecutive big-float slots.
        const char *parameters{reinterpret_cast<const char *>(g_bf_x_min)};
        put_extended_param_info(&extensions, {parameters, parameters + 22 * (g_bf_length + 2)});
    }
    if (bit_set(g_evolving, EvolutionModeFlags::FIELD_MAP))
    {
        EvolutionInfo evolution_info{};
        if (!g_have_evolve_info || g_calc_status == CalcStatus::COMPLETED)
        {
            evolution_info.x_parameter_range = g_evolve_x_parameter_range;
            evolution_info.y_parameter_range = g_evolve_y_parameter_range;
            evolution_info.x_parameter_offset = g_evolve_x_parameter_offset;
            evolution_info.y_parameter_offset = g_evolve_y_parameter_offset;
            evolution_info.discrete_x_parameter_offset =
                static_cast<std::int16_t>(g_evolve_discrete_x_parameter_offset);
            evolution_info.discrete_y_parameter_offset =
                static_cast<std::int16_t>(g_evolve_discrete_y_parameter_offset);
            evolution_info.px = static_cast<std::int16_t>(g_evolve_param_grid_x);
            evolution_info.py = static_cast<std::int16_t>(g_evolve_param_grid_y);
            evolution_info.screen_x_offset = static_cast<std::int16_t>(g_logical_screen.x_offset);
            evolution_info.screen_y_offset = static_cast<std::int16_t>(g_logical_screen.y_offset);
            evolution_info.x_dots = static_cast<std::uint16_t>(g_logical_screen.x_dots);
            evolution_info.y_dots = static_cast<std::uint16_t>(g_logical_screen.y_dots);
            evolution_info.image_grid_size = static_cast<std::int16_t>(g_evolve_image_grid_size);
            evolution_info.evolving = static_cast<std::int16_t>(+g_evolving);
            evolution_info.this_generation_random_seed =
                static_cast<std::uint16_t>(g_evolve_this_generation_random_seed);
            evolution_info.max_random_mutation = g_evolve_max_random_mutation;
            evolution_info.count = static_cast<std::int16_t>(g_evolve_image_grid_size * g_evolve_image_grid_size);
        }
        else
        {
            evolution_info = g_evolve_info;
        }
        for (int i = 0; i < NUM_GENES; ++i)
        {
            evolution_info.mutate[i] = static_cast<std::int16_t>(g_gene_bank[i].mutate);
        }
        put_evolution_info(&extensions, evolution_info);
    }
    if (g_std_calc_mode == CalcMode::ORBIT)
    {
        OrbitsInfo orbits_info{};
        orbits_info.orbit_corner_min_x = g_orbit_corner.m_min.x;
        orbits_info.orbit_corner_max_x = g_orbit_corner.m_max.x;
        orbits_info.orbit_corner_min_y = g_orbit_corner.m_min.y;
        orbits_info.orbit_corner_max_y = g_orbit_corner.m_max.y;
        orbits_info.orbit_corner_3rd_x = g_orbit_corner.m_3rd.x;
        orbits_info.orbit_corner_3rd_y = g_orbit_corner.m_3rd.y;
        orbits_info.keep_screen_coords = static_cast<std::int16_t>(g_keep_screen_coords);
        orbits_info.draw_mode = static_cast<char>(g_draw_mode);
        put_orbits_info(&extensions, orbits_info);
    }
    std::int64_t length{extend_blk_len(GIF_EXTENSION1_FRACTAL_INFO_LENGTH)};
    for (int i = 0; i < extensions.ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{extensions.ExtensionBlocks[i]};
        length += block.ByteCount + 1;
        if (block.Function != CONTINUE_EXT_FUNC_CODE)
        {
            length += 3;
        }
    }
    if (length > std::numeric_limits<std::int32_t>::max())
    {
        throw std::logic_error("Image extensions exceed the saved length field");
    }
    save_info.tot_extend_len = static_cast<std::int32_t>(length);
    put_fractal_info(&extensions, save_info);
}

bool encoder(GifWriter &writer, const std::filesystem::path &filename)
{
    if (writer.error() != 0)
    {
        return writer.finish();
    }
    std::unique_ptr<GifFileType, void (*)(GifFileType *)> extensions{new GifFileType{}, [](GifFileType *gif)
        {
            GifFreeExtensions(&gif->ExtensionBlockCount, &gif->ExtensionBlocks);
            delete gif;
        }};
    setup_save_extensions(*extensions);
    SaveImageProgress progress{filename};
    std::vector<Byte> pixels(g_logical_screen.x_dots);
    bool saved{true};
    for (int row = 0; row < g_logical_screen.y_dots && saved; ++row)
    {
        for (int col = 0; col < g_logical_screen.x_dots; ++col)
        {
            pixels[col] = static_cast<Byte>(get_color(col, row));
        }
        saved = writer.write_pixels(pixels);
        if (saved && g_disk_16_bit)
        {
            for (int col = 0; col < g_logical_screen.x_dots; ++col)
            {
                pixels[col] = static_cast<Byte>(disk_read_pixel(
                    col + g_logical_screen.x_offset, row + g_logical_screen.y_dots + g_logical_screen.y_offset));
            }
            saved = writer.write_pixels(pixels);
        }
        if (saved)
        {
            progress.advance(row);
        }
    }
    if (saved)
    {
        saved = writer.write_extensions(*extensions);
    }
    const bool finished{writer.finish()};
    return saved && finished;
}

static int extend_blk_len(const int data_len)
{
    return data_len + (data_len + 254) / 255 + 15;
    // data   +     1.per.block   + 14 for id + 1 for null at end
}

static void store_item_name(GifFileType &extensions, const std::string_view name)
{
    FormulaInfo formula_info{};
    const std::size_t name_len{std::min(name.size(), std::size(formula_info.form_name) - 1)};
    std::copy_n(name.data(), name_len, formula_info.form_name);
    if (g_fractal_type == FractalType::FORMULA)
    {
        formula_info.uses_p1 = static_cast<std::int16_t>(g_formula.uses_p1);
        formula_info.uses_p2 = static_cast<std::int16_t>(g_formula.uses_p2);
        formula_info.uses_p3 = static_cast<std::int16_t>(g_formula.uses_p3);
        formula_info.uses_p4 = static_cast<std::int16_t>(g_formula.uses_p4);
        formula_info.uses_p5 = static_cast<std::int16_t>(g_formula.uses_p5);
        formula_info.uses_ismand = static_cast<std::int16_t>(g_formula.uses_ismand);
        formula_info.ismand = static_cast<std::int16_t>(g_frm_is_mandelbrot);
    }
    put_formula_info(&extensions, formula_info);
}

void setup_save_info(FractalInfo *save_info)
{
    if (g_fractal_type != FractalType::FORMULA)
    {
        g_formula.max_function = 0;
    }
    // set save parameters in save structure
    static_assert(sizeof(INFO_ID) == sizeof(save_info->info_id));
    std::copy_n(INFO_ID, sizeof(save_info->info_id), save_info->info_id);
    save_info->info_version = FRACTAL_INFO_VERSION;
    save_info->iterations_old =
        static_cast<std::int16_t>(g_max_iterations <= std::numeric_limits<short>::max() ? g_max_iterations : std::numeric_limits<short>::max());
    save_info->fractal_type = static_cast<std::int16_t>(+g_fractal_type);
    save_info->x_min = g_image_region.m_min.x;
    save_info->x_max = g_image_region.m_max.x;
    save_info->y_min = g_image_region.m_min.y;
    save_info->y_max = g_image_region.m_max.y;
    save_info->c_real = g_params[0];
    save_info->c_imag = g_params[1];
    save_info->ax = 0;
    save_info->bx = 0;
    save_info->cx = 0;
    save_info->dx = 0;
    constexpr std::int16_t LEGACY_DOT_MODE{19};
    save_info->dot_mode = LEGACY_DOT_MODE;
    save_info->x_dots = static_cast<std::uint16_t>(g_video_entry.x_dots);
    save_info->y_dots = static_cast<std::uint16_t>(g_video_entry.y_dots);
    save_info->colors = static_cast<std::int16_t>(g_video_entry.colors);
    save_info->param3 = 0;        // pre version==7 fields
    save_info->param4 = 0;
    save_info->d_param3 = g_params[2];
    save_info->d_param4 = g_params[3];
    save_info->d_param5 = g_params[4];
    save_info->d_param6 = g_params[5];
    save_info->d_param7 = g_params[6];
    save_info->d_param8 = g_params[7];
    save_info->d_param9 = g_params[8];
    save_info->d_param10 = g_params[9];
    save_info->fill_color = static_cast<std::int16_t>(g_fill_color);
    save_info->potential[0] = static_cast<float>(g_potential.params[0]);
    save_info->potential[1] = static_cast<float>(g_potential.params[1]);
    save_info->potential[2] = static_cast<float>(g_potential.params[2]);
    save_info->random_seed_flag = static_cast<std::int16_t>(g_random_seed_flag ? 1 : 0);
    save_info->random_seed = static_cast<std::int16_t>(g_random_seed);
    if (g_inside_method != ColorMethod::COLOR)
    {
        save_info->inside = static_cast<std::int16_t>(+g_inside_method);
    }
    else
    {
        save_info->inside = static_cast<std::int16_t>(g_inside_color);
    }
    save_info->log_map_old = static_cast<std::int16_t>(g_log_map_flag <= std::numeric_limits<short>::max() ? g_log_map_flag : std::numeric_limits<short>::max());
    save_info->invert[0] = static_cast<float>(g_inversion.params[0]);
    save_info->invert[1] = static_cast<float>(g_inversion.params[1]);
    save_info->invert[2] = static_cast<float>(g_inversion.params[2]);
    save_info->decomp[0] = static_cast<std::int16_t>(g_decomp[0]);
    save_info->biomorph = static_cast<std::int16_t>(g_user.biomorph_value);
    save_info->symmetry = static_cast<std::int16_t>(g_force_symmetry);
    save_info->init3d[0] = static_cast<std::int16_t>(g_sphere ? 1 : 0);  // sphere? 1 = yes, 0 = no
    save_info->init3d[1] = static_cast<std::int16_t>(g_x_rot);            // rotate x-axis 60 degrees
    save_info->init3d[2] = static_cast<std::int16_t>(g_y_rot);            // rotate y-axis 90 degrees
    save_info->init3d[3] = static_cast<std::int16_t>(g_z_rot);            // rotate x-axis  0 degrees
    save_info->init3d[4] = static_cast<std::int16_t>(g_x_scale);          // scale x-axis, 90 percent
    save_info->init3d[5] = static_cast<std::int16_t>(g_y_scale);          // scale y-axis, 90 percent
    save_info->init3d[1] = static_cast<std::int16_t>(g_sphere_phi_min);   // longitude start, 180
    save_info->init3d[2] = static_cast<std::int16_t>(g_sphere_phi_max);   // longitude end ,   0
    save_info->init3d[3] = static_cast<std::int16_t>(g_sphere_theta_min); // latitude start,-90 degrees
    save_info->init3d[4] = static_cast<std::int16_t>(g_sphere_theta_max); // latitude stop,  90 degrees
    save_info->init3d[5] = static_cast<std::int16_t>(g_sphere_radius);    // should be user input
    save_info->init3d[6] = static_cast<std::int16_t>(g_rough);            // scale z-axis, 30 percent
    save_info->init3d[7] = static_cast<std::int16_t>(g_water_line);       // water level
    save_info->init3d[8] = static_cast<std::int16_t>(g_fill_type);        // fill type
    save_info->init3d[9] = static_cast<std::int16_t>(g_viewer_z);         // perspective view point
    save_info->init3d[10] = static_cast<std::int16_t>(g_shift_x);         // x shift
    save_info->init3d[11] = static_cast<std::int16_t>(g_shift_y);         // y shift
    save_info->init3d[12] = static_cast<std::int16_t>(g_light_x);         // x light vector coordinate
    save_info->init3d[13] = static_cast<std::int16_t>(g_light_y);         // y light vector coordinate
    save_info->init3d[14] = static_cast<std::int16_t>(g_light_z);         // z light vector coordinate
    save_info->init3d[15] = static_cast<std::int16_t>(g_light_avg);       // number of points to average
    save_info->preview_factor = static_cast<std::int16_t>(g_preview_factor);
    save_info->x_trans = static_cast<std::int16_t>(g_adjust_3d.x);
    save_info->y_trans = static_cast<std::int16_t>(g_adjust_3d.y);
    save_info->red_crop_left = static_cast<std::int16_t>(g_red_crop_left);
    save_info->red_crop_right = static_cast<std::int16_t>(g_red_crop_right);
    save_info->blue_crop_left = static_cast<short>(g_blue_crop_left);
    save_info->blue_crop_right = static_cast<std::int16_t>(g_blue_crop_right);
    save_info->red_bright = static_cast<std::int16_t>(g_red_bright);
    save_info->blue_bright = static_cast<std::int16_t>(g_blue_bright);
    save_info->x_adjust = static_cast<std::int16_t>(g_converge_x_adjust);
    save_info->y_adjust = static_cast<std::int16_t>(g_converge_y_adjust);
    save_info->eye_separation = static_cast<std::int16_t>(g_eye_separation);
    save_info->glasses_type = static_cast<std::int16_t>(g_glasses_type);
    if (g_outside_method != ColorMethod::COLOR)
    {
        save_info->outside = static_cast<std::int16_t>(+g_outside_method);
    }
    else
    {
        save_info->outside = static_cast<std::int16_t>(g_outside_color);
    }
    save_info->x3rd = g_image_region.m_3rd.x;
    save_info->y3rd = g_image_region.m_3rd.y;
    save_info->calc_status = static_cast<std::int16_t>(g_calc_status);
    save_info->std_calc_mode =
        g_three_pass && g_std_calc_mode == CalcMode::THREE_PASS ? 127 : static_cast<char>(g_std_calc_mode);
    save_info->dist_est_old =
        static_cast<std::int16_t>(g_distance_estimator <= 32000 ? g_distance_estimator : 32000);
    save_info->float_flag = static_cast<std::int16_t>(!g_version.legacy || g_user.legacy_float);
    save_info->bailout_old = static_cast<std::int16_t>(
        g_user.bailout_value >= 4 && g_user.bailout_value <= 32000 ? g_user.bailout_value : 0);
    save_info->calc_time = static_cast<std::int32_t>(g_calc_time);
    save_info->trig_index[0] = static_cast<Byte>(g_trig_index[0]);
    save_info->trig_index[1] = static_cast<Byte>(g_trig_index[1]);
    save_info->trig_index[2] = static_cast<Byte>(g_trig_index[2]);
    save_info->trig_index[3] = static_cast<Byte>(g_trig_index[3]);
    save_info->finite_attractor = static_cast<std::int16_t>(g_attractor.enabled ? 1 : 0);
    save_info->init_orbit[0] = g_init_orbit.x;
    save_info->init_orbit[1] = g_init_orbit.y;
    save_info->use_init_orbit = static_cast<char>(g_use_init_orbit);
    save_info->periodicity = static_cast<std::int16_t>(g_periodicity_check);
    save_info->pot16bit = static_cast<std::int16_t>(g_disk_16_bit ? 1 : 0);
    save_info->final_aspect_ratio = g_viewport.final_aspect_ratio;
    save_info->system = static_cast<std::int16_t>(SaveSystem::WINDOWS); // always "Windows" for saved files
    save_info->release = static_cast<std::int16_t>(20 * 100 + 4);       // legacy FRACTINT 20.04
    save_info->display_3d = static_cast<std::int16_t>(g_display_3d);
    save_info->ambient = static_cast<std::int16_t>(g_ambient);
    save_info->randomize = static_cast<std::int16_t>(g_randomize_3d);
    save_info->haze = static_cast<std::int16_t>(g_haze);
    save_info->transparent[0] = static_cast<std::int16_t>(g_transparent_color_3d[0]);
    save_info->transparent[1] = static_cast<std::int16_t>(g_transparent_color_3d[1]);
    save_info->rotate_lo = static_cast<std::int16_t>(g_color_cycle_range_lo);
    save_info->rotate_hi = static_cast<std::int16_t>(g_color_cycle_range_hi);
    save_info->dist_est_width = static_cast<std::int16_t>(g_distance_estimator_width_factor);
    save_info->julibrot_x_max = g_julibrot_x_max;
    save_info->julibrot_x_min = g_julibrot_x_min;
    save_info->julibrot_y_max = g_julibrot_y_max;
    save_info->julibrot_y_min = g_julibrot_y_min;
    save_info->julibrot_z_dots = static_cast<std::int16_t>(g_julibrot_z_dots);
    save_info->julibrot_origin_fp = g_julibrot_origin;
    save_info->julibrot_depth_fp = g_julibrot_depth;
    save_info->julibrot_height_fp = g_julibrot_height;
    save_info->julibrot_width_fp = g_julibrot_width;
    save_info->julibrot_dist_fp = g_julibrot_dist;
    save_info->eyes_fp = g_eyes;
    save_info->orbit_type = static_cast<std::int16_t>(g_new_orbit_type);
    save_info->juli3d_mode = static_cast<short>(g_julibrot_3d_mode);
    save_info->max_fn = static_cast<std::int16_t>(g_formula.max_function);
    save_info->inverse_julia = static_cast<std::int16_t>((+g_major_method << 8) + +g_inverse_julia_minor_method);
    save_info->bailout = static_cast<std::int32_t>(g_user.bailout_value);
    save_info->bailout_test = static_cast<std::int16_t>(g_bailout_test);
    save_info->iterations = static_cast<std::int32_t>(g_max_iterations);
    save_info->bf_length = static_cast<std::int16_t>(g_bn_length);
    save_info->bf_math = static_cast<std::int16_t>(g_bf_math);
    save_info->old_demm_colors = static_cast<std::int16_t>(g_old_demm_colors ? 1 : 0);
    save_info->log_map = static_cast<std::int32_t>(g_log_map_flag);
    save_info->dist_est = static_cast<std::int32_t>(g_distance_estimator);
    save_info->d_invert[0] = g_inversion.params[0];
    save_info->d_invert[1] = g_inversion.params[1];
    save_info->d_invert[2] = g_inversion.params[2];
    save_info->log_calc = static_cast<std::int16_t>(g_log_map_fly_calculate);
    save_info->stop_pass = static_cast<std::int16_t>(g_stop_pass);
    save_info->quick_calc = static_cast<std::int16_t>(g_quick_calc ? 1 : 0);
    save_info->close_prox = g_close_proximity;
    save_info->no_bof = static_cast<std::int16_t>(g_bof_match_book_images ? 0 : 1);
    save_info->orbit_interval = static_cast<std::int32_t>(g_orbit_interval);
    save_info->orbit_delay = static_cast<std::int16_t>(g_orbit_delay);
    save_info->math_tol[0] = g_math_tol[0]; // can't use std::copy_n because of gcc complaints about packed structs
    save_info->math_tol[1] = g_math_tol[1];
    if (g_version.legacy)
    {
        save_info->release = static_cast<std::int16_t>(g_version.major * 100 + g_version.minor);
        save_info->version_major = 0;
        save_info->version_minor = 0;
        save_info->version_patch = 0;
        save_info->version_tweak = 0;
    }
    else
    {
        save_info->release = static_cast<std::int16_t>(20 * 100 + 4); // legacy FRACTINT 20.04
        save_info->version_major = static_cast<std::uint8_t>(g_version.major);
        save_info->version_minor = static_cast<std::uint8_t>(g_version.minor);
        save_info->version_patch = static_cast<std::uint8_t>(g_version.patch);
        save_info->version_tweak = static_cast<std::uint8_t>(g_version.tweak);
    }
    // NOTE: can't use std::fill here on gcc due to packed constraints.
    for (int i = 0; i < static_cast<int>(std::size(save_info->future)); ++i) // NOLINT(modernize-loop-convert)
    {
        save_info->future[i] = 0;
    }
}

} // namespace id::io
