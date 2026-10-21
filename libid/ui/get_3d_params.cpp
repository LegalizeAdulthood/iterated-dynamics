// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/get_3d_params.h"

#include "engine/cmdfiles.h"
#include "engine/color_state.h"
#include "engine/spindac.h"
#include "fractals/fractype.h"
#include "geometry/line3d.h"
#include "geometry/plot3d.h"
#include "helpdefs.h"
#include "io/check_write_file.h"
#include "io/loadfile.h"
#include "io/loadmap.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/field_prompt.h"
#include "ui/full_screen_choice.h"
#include "ui/help.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"

#include <config/path_limits.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::geometry;
using namespace id::help;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

static bool prompt_3d_mode(ThreeDSettings &settings);
static bool select_3d_fill_type(ThreeDSettings &settings);
static bool prompt_3d_geometry(ThreeDSettings &settings);
static bool needs_light_params(ThreeDSettings &settings);
static bool get_light_params(ThreeDSettings &settings);
static bool check_map_file(ThreeDSettings &settings);
static bool get_funny_glasses_params(ThreeDSettings &settings, FractalType fractal_type);

static std::filesystem::path targa_save_name(const std::string &name)
{
    std::filesystem::path path{name};
    if (!path.has_extension())
    {
        path.replace_extension(".tga");
    }
    return path;
}

const std::string GLASSES1_MAP_NAME{"glasses1.map"};

int get_3d_params(ThreeDSettings &settings) // prompt for 3D parameters
{
    while (true)
    {
        if (!prompt_3d_mode(settings))
        {
            return -1;
        }

        if (settings.raytrace_format == RayTraceFormat::NONE && !select_3d_fill_type(settings))
        {
            continue;
        }

        while (true)
        {
            if (!prompt_3d_geometry(settings))
            {
                break;
            }

            if (!needs_light_params(settings) || !get_light_params(settings))
            {
                return 0;
            }
        }
    }
}

static bool needs_light_params(ThreeDSettings &settings)
{
    return settings.targa_out || settings.fill_type > FillType::SOLID_FILL || settings.raytrace_format != RayTraceFormat::NONE;
}

static bool prompt_3d_mode(ThreeDSettings &settings)
{
    ChoiceBuilder<12> builder;

    if (settings.targa_out && g_overlay_3d)
    {
        settings.targa_overlay = true;
    }

    static const char *raytrace_formats[]{"No", "DKB/POV-Ray", "VIVID", "Raw", "MTV", "Rayshade", "AcroSpin", "DXF"};
    builder.yes_no("Preview mode?", settings.preview)
        .yes_no("    Show box?", settings.show_box)
        .int_number("Coarseness, preview/grid/ray (in y dir)", settings.preview_factor)
        .yes_no("Spherical projection?", settings.sphere)
        .int_number("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,", static_cast<int>(settings.glasses_type))
        .comment("                  3=photo,4=stereo pair)")
        .list("Ray trace output? (No, DKB/POV-Ray, VIVID, RAW, MTV,", static_cast<int>(std::size(raytrace_formats)), 11,
            raytrace_formats, static_cast<int>(settings.raytrace_format))
        .comment("                Rayshade, AcroSpin, DXF)")
        .yes_no("    Brief output?", settings.brief)
        .string("    Output file name", settings.raytrace_filename.c_str())
        .yes_no("Targa output?", settings.targa_out)
        .yes_no("Use grayscale value for depth? (if \"no\" uses color number)", settings.gray_flag);

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_MODE};
        if (builder.prompt("3D Mode Selection") < 0)
        {
            return false;
        }
    }

    settings.preview = builder.read_yes_no();
    settings.show_box = builder.read_yes_no();
    settings.preview_factor = builder.read_int_number();
    const bool sphere = builder.read_yes_no();
    settings.glasses_type = static_cast<GlassesType>(builder.read_int_number());
    builder.read_comment();
    settings.raytrace_format = static_cast<RayTraceFormat>(builder.read_list());
    builder.read_comment();
    {
        if (settings.raytrace_format == RayTraceFormat::DKB_POVRAY)
        {
            stop_msg("DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
                     "the online documentation.");
        }
    }
    settings.brief = builder.read_yes_no();

    settings.raytrace_filename = builder.read_string();

    settings.targa_out = builder.read_yes_no();
    settings.gray_flag = builder.read_yes_no();

    // check ranges
    settings.preview_factor = std::max(settings.preview_factor, 2);
    settings.preview_factor = std::min(settings.preview_factor, 2000);

    if (sphere && !settings.sphere)
    {
        settings.sphere = true;
        settings.reset_geometry();
    }
    else if (!sphere && settings.sphere)
    {
        settings.sphere = false;
        settings.reset_geometry();
    }

    settings.glasses_type = std::max(settings.glasses_type, GlassesType::NONE);
    settings.glasses_type = std::min(settings.glasses_type, GlassesType::STEREO_PAIR);
    if (settings.glasses_type != GlassesType::NONE)
    {
        settings.which_image = StereoImage::RED;
    }

    if (static_cast<int>(settings.raytrace_format) < 0)
    {
        settings.raytrace_format = RayTraceFormat::NONE;
    }
    settings.raytrace_format = std::min(settings.raytrace_format, RayTraceFormat::DXF);

    return true;
}

static bool select_3d_fill_type(ThreeDSettings &settings)
{
    std::array<const char *, 11> choices;
    std::array<int, 21> attributes;
    int k = 0;
    choices[k++] = "make a surface grid";
    choices[k++] = "just draw the points";
    choices[k++] = "connect the dots (wire frame)";
    choices[k++] = "surface fill (colors interpolated)";
    choices[k++] = "surface fill (colors not interpolated)";
    choices[k++] = "solid fill (bars up from \"ground\")";
    if (settings.sphere)
    {
        choices[k++] = "light source";
    }
    else
    {
        choices[k++] = "light source before transformation";
        choices[k++] = "light source after transformation";
    }
    for (int i = 0; i < k; ++i)
    {
        attributes[i] = 1;
    }
    int i;
    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_FILL};
        i = full_screen_choice(ChoiceFlags::HELP, "Select 3D Fill Type", nullptr, nullptr, k, choices.data(),
            attributes.data(), 0, 0, 0, +settings.fill_type + 1, nullptr, nullptr, nullptr, nullptr);
    }
    if (i < 0)
    {
        return false;
    }
    settings.fill_type = static_cast<FillType>(i - 1);

    if (settings.glasses_type != GlassesType::NONE)
    {
        if (get_funny_glasses_params(settings, FractalType::PLASMA))
        {
            return false;
        }
    }
    if (check_map_file(settings))
    {
        return false;
    }

    return true;
}

static bool prompt_3d_geometry(ThreeDSettings &settings)
{
    ChoiceBuilder<15> builder;
    const char *s;

    if (settings.sphere)
    {
        builder.int_number("Longitude start (degrees)", settings.x_rot)
            .int_number("Longitude stop  (degrees)", settings.y_rot)
            .int_number("Latitude start  (degrees)", settings.z_rot)
            .int_number("Latitude stop   (degrees)", settings.x_scale)
            .int_number("Radius scaling factor in pct", settings.y_scale);
    }
    else
    {
        if (settings.raytrace_format == RayTraceFormat::NONE)
        {
            builder.int_number("X-axis rotation in degrees", settings.x_rot)
                .int_number("Y-axis rotation in degrees", settings.y_rot)
                .int_number("Z-axis rotation in degrees", settings.z_rot);
        }
        builder.int_number("X-axis scaling factor in pct", settings.x_scale)
            .int_number("Y-axis scaling factor in pct", settings.y_scale);
    }

    builder.int_number("Surface Roughness scaling factor in pct", settings.rough)
        .int_number("'Water Level' (minimum color value)", settings.water_line);

    if (settings.raytrace_format == RayTraceFormat::NONE)
    {
        builder.int_number("Perspective distance [1 - 999, 0 for no persp])", settings.viewer_z)
            .int_number("X shift with perspective (positive = right)", settings.shift_x)
            .int_number("Y shift with perspective (positive = up   )", settings.shift_y)
            .int_number("Image non-perspective X adjust (positive = right)", settings.adjust_3d.x)
            .int_number("Image non-perspective Y adjust (positive = up)", settings.adjust_3d.y)
            .int_number("First transparent color", settings.transparent_color_3d[0])
            .int_number("Last transparent color", settings.transparent_color_3d[1]);
    }

    builder.int_number("Randomize Colors      (0 - 7, '0' disables)", settings.randomize_3d);

    if (settings.sphere)
    {
        s = "Sphere 3D Parameters\n"
            "Sphere is on its side; North pole to right\n"
            "Long. 180 is top, 0 is bottom; Lat. -90 is left, 90 is right";
    }
    else
    {
        s = "Planar 3D Parameters\n"
            "Pre-rotation X axis is screen top; Y axis is left side\n"
            "Pre-rotation Z axis is coming at you out of the screen!";
    }
    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_PARAMETERS};
        if (builder.prompt(s) < 0)
        {
            return false;
        }
    }

    if (settings.raytrace_format == RayTraceFormat::NONE || settings.sphere)
    {
        settings.x_rot = builder.read_int_number();
        settings.y_rot = builder.read_int_number();
        settings.z_rot = builder.read_int_number();
    }
    settings.x_scale = builder.read_int_number();
    settings.y_scale = builder.read_int_number();
    settings.rough = builder.read_int_number();
    settings.water_line = builder.read_int_number();
    if (settings.raytrace_format == RayTraceFormat::NONE)
    {
        settings.viewer_z = builder.read_int_number();
        settings.shift_x = builder.read_int_number();
        settings.shift_y = builder.read_int_number();
        settings.adjust_3d.x = builder.read_int_number();
        settings.adjust_3d.y = builder.read_int_number();
        settings.transparent_color_3d[0] = builder.read_int_number();
        settings.transparent_color_3d[1] = builder.read_int_number();
    }
    settings.randomize_3d = builder.read_int_number();
    settings.randomize_3d = std::min(settings.randomize_3d, 7);
    settings.randomize_3d = std::max(settings.randomize_3d, 0);

    return true;
}

// ---------------------------------------------------------------------
static bool get_light_params(ThreeDSettings &settings)
{
    ChoiceBuilder<13> builder;

    // defaults go here
    if (settings.fill_type > FillType::SOLID_FILL || settings.raytrace_format != RayTraceFormat::NONE)
    {
        builder.int_number("X value light vector", settings.light_x)
            .int_number("Y value light vector", settings.light_y)
            .int_number("Z value light vector", settings.light_z);

        if (settings.raytrace_format == RayTraceFormat::NONE)
        {
            builder.int_number("Light Source Smoothing Factor", settings.light_avg);
            builder.int_number("Ambient", settings.ambient);
        }
    }

    if (settings.targa_out && settings.raytrace_format == RayTraceFormat::NONE)
    {
        builder.int_number("Haze Factor        (0 - 100, '0' disables)", settings.haze);

        builder.string("Targa File Name  (Assume .tga)", settings.light_name.c_str())
            .comment("Background Color (0 - 255)")
            .int_number("   Red", settings.background_color[0])
            .int_number("   Green", settings.background_color[1])
            .int_number("   Blue", settings.background_color[2])
            .yes_no("Overlay Targa File? (Y/N)", settings.targa_overlay);
    }
    builder.comment("");

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_LIGHT};
        if (builder.prompt("Light Source Parameters") < 0)
        {
            return true;
        }
    }

    if (settings.fill_type > FillType::SOLID_FILL)
    {
        settings.light_x = builder.read_int_number();
        settings.light_y = builder.read_int_number();
        settings.light_z = builder.read_int_number();
        if (settings.raytrace_format == RayTraceFormat::NONE)
        {
            settings.light_avg = builder.read_int_number();
            settings.ambient = builder.read_int_number();
            settings.ambient = std::min(settings.ambient, 100);
            settings.ambient = std::max(settings.ambient, 0);
        }
    }

    if (settings.targa_out && settings.raytrace_format == RayTraceFormat::NONE)
    {
        settings.haze = builder.read_int_number();
        settings.haze = std::min(settings.haze, 100);
        settings.haze = std::max(settings.haze, 0);
        settings.light_name = builder.read_string();
        builder.read_comment();
        settings.background_color[0] = static_cast<char>(builder.read_int_number() % 255);
        settings.background_color[1] = static_cast<char>(builder.read_int_number() % 255);
        settings.background_color[2] = static_cast<char>(builder.read_int_number() % 255);
        const bool targa_overlay = builder.read_yes_no();
        if (!targa_overlay)
        {
            settings.light_name = targa_save_name(settings.light_name).string();
        }
        settings.targa_overlay = targa_overlay;
    }
    return false;
}

// ---------------------------------------------------------------------

static bool check_map_file(ThreeDSettings &settings)
{
    bool ask_flag = false;
    if (!g_read_color)
    {
        return false;
    }
    std::filesystem::path map_name{"*"};
    if (settings.map_set)
    {
        map_name = settings.map_name;
    }
    if (!alternating_or_superimpose(settings.glasses_type))
    {
        ask_flag = true;
    }
    else
    {
        map_name = settings.funny_glasses_map_name;
    }

    while (true)
    {
        if (ask_flag)
        {
            constexpr int MAP_NAME_DISPLAY_LEN{60};
            std::array<char, id::config::ID_FILE_MAX_PATH> buff{};
            const std::string map_name_text{map_name.string()};
            const std::size_t copy_len{std::min(map_name_text.size(), buff.size() - 1)};
            std::copy_n(map_name_text.data(), copy_len, buff.data());
            ValueSaver saved_help_mode{g_help_mode, HelpLabels::NONE};
            const int i = field_prompt("Enter name of .map file to use,\n"
                                       "or '*' to use palette from the image to be loaded.",
                nullptr, buff.data(), static_cast<int>(buff.size() - 1), MAP_NAME_DISPLAY_LEN, nullptr);
            if (i < 0)
            {
                return true;
            }
            map_name = buff.data();
            if (buff[0] == '*')
            {
                settings.map_set = false;
                break;
            }
        }
        std::array<Byte, 256 * 3> palette;
        std::memcpy(palette.data(), g_dac_box, palette.size());
        PaletteLoadResult result;
        {
            ValueSaver saved_color_state{g_color_state};
            ValueSaver saved_map_name{g_last_map_name};
            result = validate_luts(map_name.string());
            settings.color_state = g_color_state;
            settings.last_map_name = g_last_map_name;
        }
        std::memcpy(g_dac_box, palette.data(), palette.size());
        if (!result.diagnostic.empty())
        {
            stop_msg(result.diagnostic);
        }
        if (result.failed)
        {
            ask_flag = true;
            continue;
        }
        settings.map_set = true;
        settings.map_name = map_name.filename().string();
        break;
    }
    return false;
}

static bool get_funny_glasses_params(ThreeDSettings &settings, FractalType fractal_type)
{
    // defaults
    if (settings.viewer_z == 0)
    {
        settings.viewer_z = 150;
    }
    if (settings.eye_separation == 0)
    {
        if (fractal_type == FractalType::IFS_3D || fractal_type == FractalType::LORENZ_3D)
        {
            settings.eye_separation = 2;
            settings.converge_x_adjust = -2;
        }
        else
        {
            settings.eye_separation = 3;
            settings.converge_x_adjust = 0;
        }
    }

    if (settings.glasses_type == GlassesType::ALTERNATING)
    {
        settings.funny_glasses_map_name = GLASSES1_MAP_NAME;
    }
    else if (settings.glasses_type == GlassesType::SUPERIMPOSE)
    {
        if (settings.fill_type == FillType::SURFACE_GRID)
        {
            settings.funny_glasses_map_name = "grid.map";
        }
        else
        {
            std::string glasses2_map{GLASSES1_MAP_NAME};
            glasses2_map.replace(glasses2_map.find('1'), 1, "2");
            settings.funny_glasses_map_name = glasses2_map;
        }
    }

    ChoiceBuilder<10> builder;
    builder.int_number("Interocular distance (as % of screen)", settings.eye_separation)
        .int_number("Convergence adjust (positive = spread greater)", settings.converge_x_adjust)
        .int_number("Left  red image crop (% of screen)", settings.red_crop_left)
        .int_number("Right red image crop (% of screen)", settings.red_crop_right)
        .int_number("Left  blue image crop (% of screen)", settings.blue_crop_left)
        .int_number("Right blue image crop (% of screen)", settings.blue_crop_right)
        .int_number("Red brightness factor (%)", settings.red_bright)
        .int_number("Blue brightness factor (%)", settings.blue_bright);
    if (alternating_or_superimpose(settings.glasses_type))
    {
        builder.string("Map file name", settings.funny_glasses_map_name.c_str());
    }

    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_GLASSES};
    int k = builder.prompt("Funny Glasses Parameters");
    if (k < 0)
    {
        return true;
    }

    settings.eye_separation = builder.read_int_number();
    settings.converge_x_adjust = builder.read_int_number();
    settings.red_crop_left = builder.read_int_number();
    settings.red_crop_right = builder.read_int_number();
    settings.blue_crop_left = builder.read_int_number();
    settings.blue_crop_right = builder.read_int_number();
    settings.red_bright = builder.read_int_number();
    settings.blue_bright = builder.read_int_number();

    if (alternating_or_superimpose(settings.glasses_type))
    {
        settings.funny_glasses_map_name = builder.read_string();
    }
    return false;
}

int get_fract3d_params() // prompt for 3D fractal parameters
{
    ThreeDSettings settings;
    driver_stack_screen();
    ChoiceBuilder<7> builder;
    builder.int_number("X-axis rotation in degrees", settings.x_rot)
        .int_number("Y-axis rotation in degrees", settings.y_rot)
        .int_number("Z-axis rotation in degrees", settings.z_rot)
        .int_number("Perspective distance [1 - 999, 0 for no persp]", settings.viewer_z)
        .int_number("X shift with perspective (positive = right)", settings.shift_x)
        .int_number("Y shift with perspective (positive = up   )", settings.shift_y)
        .int_number("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)",
            static_cast<int>(settings.glasses_type));

    int i;
    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_FRACT};
        i = builder.prompt("3D Parameters");
    }

    int ret{};
    if (i < 0)
    {
        ret = -1;
        goto get_f3d_exit;
    }

    settings.x_rot = builder.read_int_number();
    settings.y_rot = builder.read_int_number();
    settings.z_rot = builder.read_int_number();
    settings.viewer_z = builder.read_int_number();
    settings.shift_x = builder.read_int_number();
    settings.shift_y = builder.read_int_number();
    settings.glasses_type = static_cast<GlassesType>(builder.read_int_number());
    if (settings.glasses_type < GlassesType::NONE || settings.glasses_type > GlassesType::STEREO_PAIR)
    {
        settings.glasses_type = GlassesType::NONE;
    }
    if (settings.glasses_type != GlassesType::NONE)
    {
        if (get_funny_glasses_params(settings, g_fractal_type) || check_map_file(settings))
        {
            ret = -1;
        }
    }

get_f3d_exit:
    if (ret >= 0)
    {
        settings.apply();
    }
    driver_unstack_screen();
    return ret;
}

} // namespace id::ui
