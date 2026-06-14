// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_3d_params.h"

#include "engine/cmdfiles.h"
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

static bool prompt_3d_mode();
static bool select_3d_fill_type();
static bool prompt_3d_geometry();
static bool needs_light_params();
static bool get_light_params();
static bool check_map_file();
static bool get_funny_glasses_params();

static std::filesystem::path targa_save_name(const std::string &name)
{
    std::filesystem::path path{name};
    if (!path.has_extension())
    {
        path.replace_extension(".tga");
    }
    return path;
}

static std::string s_funny_glasses_map_name;

const std::string GLASSES1_MAP_NAME{"glasses1.map"};

int get_3d_params() // prompt for 3D parameters
{
    while (true)
    {
        if (!prompt_3d_mode())
        {
            return -1;
        }

        if (g_raytrace_format == RayTraceFormat::NONE && !select_3d_fill_type())
        {
            continue;
        }

        while (true)
        {
            if (!prompt_3d_geometry())
            {
                break;
            }

            if (!needs_light_params() || !get_light_params())
            {
                return 0;
            }
        }
    }
}

static bool needs_light_params()
{
    return g_targa_out || illumine() || g_raytrace_format != RayTraceFormat::NONE;
}

static bool prompt_3d_mode()
{
    ChoiceBuilder<12> builder;

    if (g_targa_out && g_overlay_3d)
    {
        g_targa_overlay = true;
    }

    static const char *raytrace_formats[]{"No", "DKB/POV-Ray", "VIVID", "Raw", "MTV", "Rayshade", "AcroSpin", "DXF"};
    builder.yes_no("Preview mode?", g_preview)
        .yes_no("    Show box?", g_show_box)
        .int_number("Coarseness, preview/grid/ray (in y dir)", g_preview_factor)
        .yes_no("Spherical projection?", g_sphere)
        .int_number("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,", static_cast<int>(g_glasses_type))
        .comment("                  3=photo,4=stereo pair)")
        .list("Ray trace output? (No, DKB/POV-Ray, VIVID, RAW, MTV,", static_cast<int>(std::size(raytrace_formats)), 11,
            raytrace_formats, static_cast<int>(g_raytrace_format))
        .comment("                Rayshade, AcroSpin, DXF)")
        .yes_no("    Brief output?", g_brief)
        .string("    Output file name", g_raytrace_filename.c_str())
        .yes_no("Targa output?", g_targa_out)
        .yes_no("Use grayscale value for depth? (if \"no\" uses color number)", g_gray_flag);

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_MODE};
        if (builder.prompt("3D Mode Selection") < 0)
        {
            return false;
        }
    }

    g_preview = builder.read_yes_no();
    g_show_box = builder.read_yes_no();
    g_preview_factor = builder.read_int_number();
    const bool sphere = builder.read_yes_no();
    g_glasses_type = static_cast<GlassesType>(builder.read_int_number());
    builder.read_comment();
    g_raytrace_format = static_cast<RayTraceFormat>(builder.read_list());
    builder.read_comment();
    {
        if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
        {
            stop_msg("DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
                     "the online documentation.");
        }
    }
    g_brief = builder.read_yes_no();

    g_raytrace_filename = builder.read_string();

    g_targa_out = builder.read_yes_no();
    g_gray_flag = builder.read_yes_no();

    // check ranges
    g_preview_factor = std::max(g_preview_factor, 2);
    g_preview_factor = std::min(g_preview_factor, 2000);

    if (sphere && !g_sphere)
    {
        g_sphere = true;
        set_3d_defaults();
    }
    else if (!sphere && g_sphere)
    {
        g_sphere = false;
        set_3d_defaults();
    }

    g_glasses_type = std::max(g_glasses_type, GlassesType::NONE);
    g_glasses_type = std::min(g_glasses_type, GlassesType::STEREO_PAIR);
    if (g_glasses_type != GlassesType::NONE)
    {
        g_which_image = StereoImage::RED;
    }

    if (static_cast<int>(g_raytrace_format) < 0)
    {
        g_raytrace_format = RayTraceFormat::NONE;
    }
    g_raytrace_format = std::min(g_raytrace_format, RayTraceFormat::DXF);

    return true;
}

static bool select_3d_fill_type()
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
    if (g_sphere)
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
            attributes.data(), 0, 0, 0, +g_fill_type + 1, nullptr, nullptr, nullptr, nullptr);
    }
    if (i < 0)
    {
        return false;
    }
    g_fill_type = static_cast<FillType>(i - 1);

    if (g_glasses_type != GlassesType::NONE)
    {
        if (get_funny_glasses_params())
        {
            return false;
        }
    }
    if (check_map_file())
    {
        return false;
    }

    return true;
}

static bool prompt_3d_geometry()
{
    ChoiceBuilder<15> builder;
    const char *s;

    if (g_sphere)
    {
        builder.int_number("Longitude start (degrees)", g_x_rot)
            .int_number("Longitude stop  (degrees)", g_y_rot)
            .int_number("Latitude start  (degrees)", g_z_rot)
            .int_number("Latitude stop   (degrees)", g_x_scale)
            .int_number("Radius scaling factor in pct", g_y_scale);
    }
    else
    {
        if (g_raytrace_format == RayTraceFormat::NONE)
        {
            builder.int_number("X-axis rotation in degrees", g_x_rot)
                .int_number("Y-axis rotation in degrees", g_y_rot)
                .int_number("Z-axis rotation in degrees", g_z_rot);
        }
        builder.int_number("X-axis scaling factor in pct", g_x_scale)
            .int_number("Y-axis scaling factor in pct", g_y_scale);
    }

    builder.int_number("Surface Roughness scaling factor in pct", g_rough)
        .int_number("'Water Level' (minimum color value)", g_water_line);

    if (g_raytrace_format == RayTraceFormat::NONE)
    {
        builder.int_number("Perspective distance [1 - 999, 0 for no persp])", g_viewer_z)
            .int_number("X shift with perspective (positive = right)", g_shift_x)
            .int_number("Y shift with perspective (positive = up   )", g_shift_y)
            .int_number("Image non-perspective X adjust (positive = right)", g_adjust_3d.x)
            .int_number("Image non-perspective Y adjust (positive = up)", g_adjust_3d.y)
            .int_number("First transparent color", g_transparent_color_3d[0])
            .int_number("Last transparent color", g_transparent_color_3d[1]);
    }

    builder.int_number("Randomize Colors      (0 - 7, '0' disables)", g_randomize_3d);

    if (g_sphere)
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

    if (g_raytrace_format == RayTraceFormat::NONE || g_sphere)
    {
        g_x_rot = builder.read_int_number();
        g_y_rot = builder.read_int_number();
        g_z_rot = builder.read_int_number();
    }
    g_x_scale = builder.read_int_number();
    g_y_scale = builder.read_int_number();
    g_rough = builder.read_int_number();
    g_water_line = builder.read_int_number();
    if (g_raytrace_format == RayTraceFormat::NONE)
    {
        g_viewer_z = builder.read_int_number();
        g_shift_x = builder.read_int_number();
        g_shift_y = builder.read_int_number();
        g_adjust_3d.x = builder.read_int_number();
        g_adjust_3d.y = builder.read_int_number();
        g_transparent_color_3d[0] = builder.read_int_number();
        g_transparent_color_3d[1] = builder.read_int_number();
    }
    g_randomize_3d = builder.read_int_number();
    g_randomize_3d = std::min(g_randomize_3d, 7);
    g_randomize_3d = std::max(g_randomize_3d, 0);

    return true;
}

// ---------------------------------------------------------------------
static bool get_light_params()
{
    ChoiceBuilder<13> builder;

    // defaults go here
    if (illumine() || g_raytrace_format != RayTraceFormat::NONE)
    {
        builder.int_number("X value light vector", g_light_x)
            .int_number("Y value light vector", g_light_y)
            .int_number("Z value light vector", g_light_z);

        if (g_raytrace_format == RayTraceFormat::NONE)
        {
            builder.int_number("Light Source Smoothing Factor", g_light_avg);
            builder.int_number("Ambient", g_ambient);
        }
    }

    if (g_targa_out && g_raytrace_format == RayTraceFormat::NONE)
    {
        builder.int_number("Haze Factor        (0 - 100, '0' disables)", g_haze);

        builder.string("Targa File Name  (Assume .tga)", g_light_name.c_str())
            .comment("Background Color (0 - 255)")
            .int_number("   Red", g_background_color[0])
            .int_number("   Green", g_background_color[1])
            .int_number("   Blue", g_background_color[2])
            .yes_no("Overlay Targa File? (Y/N)", g_targa_overlay);
    }
    builder.comment("");

    {
        ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_LIGHT};
        if (builder.prompt("Light Source Parameters") < 0)
        {
            return true;
        }
    }

    if (illumine())
    {
        g_light_x   = builder.read_int_number();
        g_light_y   = builder.read_int_number();
        g_light_z   = builder.read_int_number();
        if (g_raytrace_format == RayTraceFormat::NONE)
        {
            g_light_avg = builder.read_int_number();
            g_ambient  = builder.read_int_number();
            g_ambient = std::min(g_ambient, 100);
            g_ambient = std::max(g_ambient, 0);
        }
    }

    if (g_targa_out && g_raytrace_format == RayTraceFormat::NONE)
    {
        g_haze = builder.read_int_number();
        g_haze = std::min(g_haze, 100);
        g_haze = std::max(g_haze, 0);
        g_light_name = builder.read_string();
        builder.read_comment();
        g_background_color[0] = static_cast<char>(builder.read_int_number() % 255);
        g_background_color[1] = static_cast<char>(builder.read_int_number() % 255);
        g_background_color[2] = static_cast<char>(builder.read_int_number() % 255);
        const bool targa_overlay = builder.read_yes_no();
        if (!targa_overlay)
        {
            g_light_name = targa_save_name(g_light_name).string();
        }
        g_targa_overlay = targa_overlay;
    }
    return false;
}

// ---------------------------------------------------------------------

static bool check_map_file()
{
    bool ask_flag = false;
    if (!g_read_color)
    {
        return false;
    }
    std::filesystem::path map_name{"*"};
    if (g_map_set)
    {
        map_name = g_map_name;
    }
    if (!glasses_alternating_or_superimpose())
    {
        ask_flag = true;
    }
    else
    {
        map_name = s_funny_glasses_map_name;
    }

    while (true)
    {
        if (ask_flag)
        {
            char buff[80]{};
            std::strcpy(buff, map_name.string().c_str());
            ValueSaver saved_help_mode{g_help_mode, HelpLabels::NONE};
            const int i = field_prompt("Enter name of .map file to use,\n"
                                       "or '*' to use palette from the image to be loaded.",
                nullptr, buff, 60, nullptr);
            if (i < 0)
            {
                return true;
            }
            map_name = buff;
            if (buff[0] == '*')
            {
                g_map_set = false;
                break;
            }
        }
        std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save the DAC
        const bool valid = validate_luts(map_name.string());
        std::memcpy(g_dac_box, g_old_dac_box, 256*3); // restore the DAC
        if (valid) // Oops, something's wrong
        {
            ask_flag = true;
            continue;
        }
        g_map_set = true;
        g_map_name = map_name.filename().string();
        break;
    }
    return false;
}

static bool get_funny_glasses_params()
{
    // defaults
    if (g_viewer_z == 0)
    {
        g_viewer_z = 150;
    }
    if (g_eye_separation == 0)
    {
        if (g_fractal_type == FractalType::IFS_3D || g_fractal_type == FractalType::LORENZ_3D)
        {
            g_eye_separation = 2;
            g_converge_x_adjust = -2;
        }
        else
        {
            g_eye_separation = 3;
            g_converge_x_adjust = 0;
        }
    }

    if (g_glasses_type == GlassesType::ALTERNATING)
    {
        s_funny_glasses_map_name = GLASSES1_MAP_NAME;
    }
    else if (g_glasses_type == GlassesType::SUPERIMPOSE)
    {
        if (g_fill_type == FillType::SURFACE_GRID)
        {
            s_funny_glasses_map_name = "grid.map";
        }
        else
        {
            std::string glasses2_map{GLASSES1_MAP_NAME};
            glasses2_map.replace(glasses2_map.find('1'), 1, "2");
            s_funny_glasses_map_name = glasses2_map;
        }
    }

    ChoiceBuilder<10> builder;
    builder.int_number("Interocular distance (as % of screen)", g_eye_separation)
        .int_number("Convergence adjust (positive = spread greater)", g_converge_x_adjust)
        .int_number("Left  red image crop (% of screen)", g_red_crop_left)
        .int_number("Right red image crop (% of screen)", g_red_crop_right)
        .int_number("Left  blue image crop (% of screen)", g_blue_crop_left)
        .int_number("Right blue image crop (% of screen)", g_blue_crop_right)
        .int_number("Red brightness factor (%)", g_red_bright)
        .int_number("Blue brightness factor (%)", g_blue_bright);
    if (glasses_alternating_or_superimpose())
    {
        builder.string("Map file name", s_funny_glasses_map_name.c_str());
    }

    ValueSaver saved_help_mode{g_help_mode, HelpLabels::HELP_3D_GLASSES};
    int k = builder.prompt("Funny Glasses Parameters");
    if (k < 0)
    {
        return true;
    }

    g_eye_separation   =  builder.read_int_number();
    g_converge_x_adjust = builder.read_int_number();
    g_red_crop_left   =  builder.read_int_number();
    g_red_crop_right  =  builder.read_int_number();
    g_blue_crop_left  =  builder.read_int_number();
    g_blue_crop_right =  builder.read_int_number();
    g_red_bright      =  builder.read_int_number();
    g_blue_bright     =  builder.read_int_number();

    if (glasses_alternating_or_superimpose())
    {
        s_funny_glasses_map_name = builder.read_string();
    }
    return false;
}

int get_fract3d_params() // prompt for 3D fractal parameters
{
    driver_stack_screen();
    ChoiceBuilder<7> builder;
    builder.int_number("X-axis rotation in degrees", g_x_rot)
        .int_number("Y-axis rotation in degrees", g_y_rot)
        .int_number("Z-axis rotation in degrees", g_z_rot)
        .int_number("Perspective distance [1 - 999, 0 for no persp]", g_viewer_z)
        .int_number("X shift with perspective (positive = right)", g_shift_x)
        .int_number("Y shift with perspective (positive = up   )", g_shift_y)
        .int_number("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)",
            static_cast<int>(g_glasses_type));

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

    g_x_rot    = builder.read_int_number();
    g_y_rot    = builder.read_int_number();
    g_z_rot    = builder.read_int_number();
    g_viewer_z = builder.read_int_number();
    g_shift_x  = builder.read_int_number();
    g_shift_y  = builder.read_int_number();
    g_glasses_type = static_cast<GlassesType>(builder.read_int_number());
    if (g_glasses_type < GlassesType::NONE || g_glasses_type > GlassesType::STEREO_PAIR)
    {
        g_glasses_type = GlassesType::NONE;
    }
    if (g_glasses_type != GlassesType::NONE)
    {
        if (get_funny_glasses_params() || check_map_file())
        {
            ret = -1;
        }
    }

get_f3d_exit:
    driver_unstack_screen();
    return ret;
}

} // namespace id::ui
