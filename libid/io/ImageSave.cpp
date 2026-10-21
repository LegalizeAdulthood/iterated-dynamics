// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "io/ImageSave.h"

#include "engine/calc_frac_init.h"
#include "engine/calcfrac.h"
#include "engine/LogicalScreen.h"
#include "engine/pixel.h"
#include "engine/pixel_limits.h"
#include "engine/resume.h"
#include "engine/spindac.h"
#include "engine/StickyOrbits.h"
#include "engine/VideoInfo.h"
#include "engine/Viewport.h"
#include "fractals/formula.h"
#include "fractals/fractype.h"
#include "fractals/ifs.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/parser.h"
#include "io/disk_storage.h"
#include "io/encoder.h"
#include "io/gif_extensions.h"
#include "io/gif_file.h"
#include "io/GifWriter.h"
#include "math/big.h"
#include "misc/id.h"
#include "ui/big_while_loop.h"
#include "ui/evolve.h"

#include <gif_lib.h>

#include <algorithm>
#include <iterator>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::ui;

namespace id::io
{

static int extend_blk_len(int data_len);
static void store_item_name(GifFileType &extensions, std::string_view name);

// B&W palette
static constexpr Byte s_palette_bw[]{
    0,
    0,
    0,  //
    63,
    63,
    63, //
};

// 4-color (CGA) palette
static constexpr Byte s_palette_cga[]{
    0,
    0,
    0,  //
    21,
    63,
    63, //
    63,
    21,
    63, //
    63,
    63,
    63, //
};

// 16-color (EGA/CGA) pal
static constexpr Byte s_palette_ega[]{
    0,
    0,
    0,  //
    0,
    0,
    42, //
    0,
    42,
    0,  //
    0,
    42,
    42, //
    42,
    0,
    0,  //
    42,
    0,
    42, //
    42,
    21,
    0,  //
    42,
    42,
    42, //
    21,
    21,
    21, //
    21,
    21,
    63, //
    21,
    63,
    21, //
    21,
    63,
    63, //
    63,
    21,
    21, //
    63,
    21,
    63, //
    63,
    63,
    21, //
    63,
    63,
    63, //
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

ImageSave::ImageSave(const std::filesystem::path &filename, const bool file) :
    m_filename{filename},
    m_width{g_logical_screen.x_dots},
    m_height{g_logical_screen.y_dots},
    m_x_offset{g_logical_screen.x_offset},
    m_y_offset{g_logical_screen.y_offset},
    m_potential{g_disk_16_bit},
    m_file{file}
{
    if (filename.empty())
    {
        throw std::logic_error("Empty image save path");
    }
    if (m_width <= 0 || m_height <= 0 || m_width > GIF_MAX_PIXELS / (m_potential ? 2 : 1) || m_height > GIF_MAX_PIXELS)
    {
        m_done = true;
        m_diagnostic = "Error writing image to " + m_filename.string();
        return;
    }
    m_pixels.resize(m_width);
    m_extensions.reset(new GifFileType{});
    setup_save_extensions(*m_extensions);
}

ImageSave::ImageSave(const std::filesystem::path &filename) :
    ImageSave{filename, true}
{
    if (m_done)
    {
        return;
    }
    std::error_code error;
    m_replace = std::filesystem::exists(m_filename, error);
    if (error || (m_replace && !std::filesystem::is_regular_file(m_filename, error)))
    {
        m_done = true;
        m_diagnostic = "Can't write " + m_filename.string();
        return;
    }
    m_work_path = m_filename;
    if (m_replace)
    {
        m_work_path.replace_filename("id.tmp");
        if (std::filesystem::exists(m_work_path, error) || error)
        {
            m_done = true;
            m_diagnostic = "Can't create " + m_work_path.string();
            return;
        }
    }
    try
    {
        m_owns_work_file = true;
        m_writer = std::make_unique<GifWriter>(m_work_path, save_screen(), save_palette());
    }
    catch (...)
    {
        discard_partial_file();
        throw;
    }
    if (m_writer->error() != 0)
    {
        finish(false);
    }
}

ImageSave::ImageSave(std::ostream &output, const std::filesystem::path &filename) :
    ImageSave{filename, false}
{
    if (!m_done)
    {
        m_writer = std::make_unique<GifWriter>(output, save_screen(), save_palette());
        if (m_writer->error() != 0)
        {
            finish(false);
        }
    }
}

ImageSave::~ImageSave()
{
    m_writer.reset();
    discard_partial_file();
}

void ImageSave::free_extensions(GifFileType *extensions)
{
    if (extensions)
    {
        GifFreeExtensions(&extensions->ExtensionBlockCount, &extensions->ExtensionBlocks);
        delete extensions;
    }
}

void ImageSave::discard_partial_file() noexcept
{
    if (m_owns_work_file)
    {
        std::error_code error;
        std::filesystem::remove(m_work_path, error);
        m_owns_work_file = false;
    }
}

bool ImageSave::replace_file()
{
    std::filesystem::path backup{m_work_path};
    backup += ".bak";
    std::error_code error;
    if (std::filesystem::exists(backup, error) || error)
    {
        m_diagnostic = "Can't replace " + m_filename.string() + ": save backup already exists";
        return false;
    }
    std::filesystem::rename(m_filename, backup, error);
    if (error)
    {
        m_diagnostic = "Can't replace " + m_filename.string() + ": " + error.message();
        return false;
    }
    std::filesystem::rename(m_work_path, m_filename, error);
    if (error)
    {
        const std::string message{error.message()};
        std::error_code rollback_error;
        std::filesystem::rename(backup, m_filename, rollback_error);
        m_diagnostic = "Can't replace " + m_filename.string() + ": " + message;
        if (rollback_error)
        {
            m_diagnostic += "; original retained in " + backup.string();
        }
        return false;
    }
    m_owns_work_file = false;
    std::filesystem::remove(backup, error);
    if (error)
    {
        m_diagnostic = "Can't remove save backup " + backup.string() + ": " + error.message();
        return false;
    }
    return true;
}

void ImageSave::finish(const bool saved)
{
    const bool closed{m_writer->finish()};
    m_error = m_writer->error();
    m_writer.reset();
    m_succeeded = saved && closed;
    if (m_succeeded && m_file)
    {
        if (m_replace)
        {
            m_succeeded = replace_file();
        }
        else
        {
            m_owns_work_file = false;
        }
    }
    if (!m_succeeded)
    {
        if (m_diagnostic.empty())
        {
            const std::filesystem::path &path{m_file ? m_work_path : m_filename};
            m_diagnostic = m_error == E_GIF_ERR_OPEN_FAILED ? "Can't create " + path.string()
                                                            : "Error writing image to " + path.string();
        }
        discard_partial_file();
    }
    m_done = true;
}

void ImageSave::iterate()
{
    if (m_done || !m_writer)
    {
        throw std::logic_error("Image save is already complete");
    }
    if (m_row == m_height)
    {
        finish(m_writer->write_extensions(*m_extensions));
        return;
    }
    for (int col = 0; col < m_width; ++col)
    {
        m_pixels[col] = static_cast<Byte>(get_color(col, m_row));
    }
    bool saved{m_writer->write_pixels(m_pixels)};
    if (saved && m_potential)
    {
        for (int col = 0; col < m_width; ++col)
        {
            m_pixels[col] = static_cast<Byte>(read_disk_pixel(col + m_x_offset, m_row + m_height + m_y_offset));
        }
        saved = m_writer->write_pixels(m_pixels);
    }
    if (!saved)
    {
        finish(false);
        return;
    }
    ++m_row;
}

} // namespace id::io
