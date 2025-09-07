// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/get_3d_params.h"

#include "3d/line3d.h"
#include "3d/plot3d.h"
#include "engine/cmdfiles.h"
#include "engine/id_data.h"
#include "fractals/fractype.h"
#include "helpdefs.h"
#include "io/check_write_file.h"
#include "io/loadmap.h"
#include "io/merge_path_names.h"
#include "misc/Driver.h"
#include "misc/ValueSaver.h"
#include "ui/ChoiceBuilder.h"
#include "ui/field_prompt.h"
#include "ui/full_screen_choice.h"
#include "ui/full_screen_prompt.h"
#include "ui/rotate.h"
#include "ui/stereo.h"
#include "ui/stop_msg.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <string>

using namespace id::fractals;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

static  bool get_light_params();
static  bool check_map_file();
static  bool get_funny_glasses_params();

static std::string s_funny_glasses_map_name;

const std::string GLASSES1_MAP_NAME{"glasses1.map"};

int get_3d_params()     // prompt for 3D parameters
{
    int sphere;
    const char *s;
    const char *prompts3d[21];
    FullScreenValues values[21];
    int k;

restart_1:
    if (g_targa_out && g_overlay_3d)
    {
        id::g_targa_overlay = true;
    }

    k = -1;

    prompts3d[++k] = "Preview mode?";
    values[k].type = 'y';
    values[k].uval.ch.val = id::g_preview ? 1 : 0;

    prompts3d[++k] = "    Show box?";
    values[k].type = 'y';
    values[k].uval.ch.val = id::g_show_box ? 1 : 0;

    prompts3d[++k] = "Coarseness, preview/grid/ray (in y dir)";
    values[k].type = 'i';
    values[k].uval.ival = id::g_preview_factor;

    prompts3d[++k] = "Spherical projection?";
    values[k].type = 'y';
    sphere = id::g_sphere ? 1 : 0;
    values[k].uval.ch.val = sphere;

    prompts3d[++k] = "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,";
    values[k].type = 'i';
    values[k].uval.ival = static_cast<int>(id::g_glasses_type);

    prompts3d[++k] = "                  3=photo,4=stereo pair)";
    values[k].type = '*';

    prompts3d[++k] = "Ray trace output? (No, DKB/POV-Ray, VIVID, RAW, MTV,";
    values[k].type = 'l';
    static const char *raytrace_formats[]{
        "No", "DKB/POV-Ray", "VIVID", "Raw", "MTV", "Rayshade", "AcroSpin", "DXF"};
    values[k].uval.ch.list = raytrace_formats;
    values[k].uval.ch.list_len = static_cast<int>(std::size(raytrace_formats));
    values[k].uval.ch.vlen = 11;
    values[k].uval.ch.val = static_cast<int>(id::g_raytrace_format);
    prompts3d[++k] = "                Rayshade, AcroSpin, DXF)";
    values[k].type = '*';

    prompts3d[++k] = "    Brief output?";
    values[k].type = 'y';
    values[k].uval.ch.val = id::g_brief ? 1 : 0;

    prompts3d[++k] = "    Output file name";
    values[k].type = 's';
    std::strcpy(values[k].uval.sval, id::g_raytrace_filename.c_str());

    prompts3d[++k] = "Targa output?";
    values[k].type = 'y';
    values[k].uval.ch.val = g_targa_out ? 1 : 0;

    prompts3d[++k] = "Use grayscale value for depth? (if \"no\" uses color number)";
    values[k].type = 'y';
    values[k].uval.ch.val = g_gray_flag ? 1 : 0;

    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_MODE};
        k = full_screen_prompt("3D Mode Selection", k+1, prompts3d, values, 0, nullptr);
    }
    if (k < 0)
    {
        return -1;
    }

    k = 0;
    id::g_preview = values[k++].uval.ch.val != 0;
    id::g_show_box = values[k++].uval.ch.val != 0;
    id::g_preview_factor  = values[k++].uval.ival;
    sphere = values[k++].uval.ch.val;
    id::g_glasses_type = static_cast<id::GlassesType>(values[k++].uval.ival);
    k++;
    id::g_raytrace_format = static_cast<id::RayTraceFormat>(values[k++].uval.ch.val);
    k++;
    {
        if (id::g_raytrace_format == id::RayTraceFormat::DKB_POVRAY)
        {
            stop_msg("DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
                    "the online documentation.");
        }
    }
    id::g_brief = values[k++].uval.ch.val != 0;

    id::g_raytrace_filename = values[k++].uval.sval;

    g_targa_out = values[k++].uval.ch.val != 0;
    g_gray_flag  = values[k++].uval.ch.val != 0;

    // check ranges
    id::g_preview_factor = std::max(id::g_preview_factor, 2);
    id::g_preview_factor = std::min(id::g_preview_factor, 2000);

    if (sphere && !id::g_sphere)
    {
        id::g_sphere = true;
        set_3d_defaults();
    }
    else if (!sphere && id::g_sphere)
    {
        id::g_sphere = false;
        set_3d_defaults();
    }

    id::g_glasses_type = std::max(id::g_glasses_type, id::GlassesType::NONE);
    id::g_glasses_type = std::min(id::g_glasses_type, id::GlassesType::STEREO_PAIR);
    if (id::g_glasses_type != id::GlassesType::NONE)
    {
        id::g_which_image = id::StereoImage::RED;
    }

    if (static_cast<int>(id::g_raytrace_format) < 0)
    {
        id::g_raytrace_format = id::RayTraceFormat::NONE;
    }
    id::g_raytrace_format = std::min(id::g_raytrace_format, id::RayTraceFormat::DXF);

    if (id::g_raytrace_format == id::RayTraceFormat::NONE)
    {
        const char *choices[11];
        int attributes[21];
        k = 0;
        choices[k++] = "make a surface grid";
        choices[k++] = "just draw the points";
        choices[k++] = "connect the dots (wire frame)";
        choices[k++] = "surface fill (colors interpolated)";
        choices[k++] = "surface fill (colors not interpolated)";
        choices[k++] = "solid fill (bars up from \"ground\")";
        if (id::g_sphere)
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
            ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_FILL};
            i = full_screen_choice(ChoiceFlags::HELP, "Select 3D Fill Type", nullptr, nullptr, k, choices,
                attributes, 0, 0, 0, +id::g_fill_type + 1, nullptr, nullptr, nullptr, nullptr);
        }
        if (i < 0)
        {
            goto restart_1;
        }
        id::g_fill_type = static_cast<id::FillType>(i - 1);

        if (id::g_glasses_type != id::GlassesType::NONE)
        {
            if (get_funny_glasses_params())
            {
                goto restart_1;
            }
        }
        if (check_map_file())
        {
            goto restart_1;
        }
    }
restart_3:

    if (id::g_sphere)
    {
        k = -1;
        prompts3d[++k] = "Longitude start (degrees)";
        prompts3d[++k] = "Longitude stop  (degrees)";
        prompts3d[++k] = "Latitude start  (degrees)";
        prompts3d[++k] = "Latitude stop   (degrees)";
        prompts3d[++k] = "Radius scaling factor in pct";
    }
    else
    {
        k = -1;
        if (id::g_raytrace_format == id::RayTraceFormat::NONE)
        {
            prompts3d[++k] = "X-axis rotation in degrees";
            prompts3d[++k] = "Y-axis rotation in degrees";
            prompts3d[++k] = "Z-axis rotation in degrees";
        }
        prompts3d[++k] = "X-axis scaling factor in pct";
        prompts3d[++k] = "Y-axis scaling factor in pct";
    }
    k = -1;
    if (id::g_raytrace_format == id::RayTraceFormat::NONE || id::g_sphere)
    {
        values[++k].uval.ival   = id::g_x_rot    ;
        values[k].type = 'i';
        values[++k].uval.ival   = id::g_y_rot    ;
        values[k].type = 'i';
        values[++k].uval.ival   = id::g_z_rot    ;
        values[k].type = 'i';
    }
    values[++k].uval.ival   = id::g_x_scale    ;
    values[k].type = 'i';

    values[++k].uval.ival   = id::g_y_scale    ;
    values[k].type = 'i';

    prompts3d[++k] = "Surface Roughness scaling factor in pct";
    values[k].type = 'i';
    values[k].uval.ival = id::g_rough     ;

    prompts3d[++k] = "'Water Level' (minimum color value)";
    values[k].type = 'i';
    values[k].uval.ival = id::g_water_line ;

    if (id::g_raytrace_format == id::RayTraceFormat::NONE)
    {
        prompts3d[++k] = "Perspective distance [1 - 999, 0 for no persp])";
        values[k].type = 'i';
        values[k].uval.ival = id::g_viewer_z     ;

        prompts3d[++k] = "X shift with perspective (positive = right)";
        values[k].type = 'i';
        values[k].uval.ival = id::g_shift_x    ;

        prompts3d[++k] = "Y shift with perspective (positive = up   )";
        values[k].type = 'i';
        values[k].uval.ival = id::g_shift_y    ;

        prompts3d[++k] = "Image non-perspective X adjust (positive = right)";
        values[k].type = 'i';
        values[k].uval.ival = id::g_adjust_3d_x    ;

        prompts3d[++k] = "Image non-perspective Y adjust (positive = up)";
        values[k].type = 'i';
        values[k].uval.ival = id::g_adjust_3d_y    ;

        prompts3d[++k] = "First transparent color";
        values[k].type = 'i';
        values[k].uval.ival = g_transparent_color_3d[0];

        prompts3d[++k] = "Last transparent color";
        values[k].type = 'i';
        values[k].uval.ival = g_transparent_color_3d[1];
    }

    prompts3d[++k] = "Randomize Colors      (0 - 7, '0' disables)";
    values[k].type = 'i';
    values[k++].uval.ival = id::g_randomize_3d;

    if (id::g_sphere)
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
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_PARAMETERS};
        k = full_screen_prompt(s, k, prompts3d, values, 0, nullptr);
    }
    if (k < 0)
    {
        goto restart_1;
    }

    k = 0;
    if (id::g_raytrace_format == id::RayTraceFormat::NONE || id::g_sphere)
    {
        id::g_x_rot    = values[k++].uval.ival;
        id::g_y_rot    = values[k++].uval.ival;
        id::g_z_rot    = values[k++].uval.ival;
    }
    id::g_x_scale     = values[k++].uval.ival;
    id::g_y_scale     = values[k++].uval.ival;
    id::g_rough      = values[k++].uval.ival;
    id::g_water_line  = values[k++].uval.ival;
    if (id::g_raytrace_format == id::RayTraceFormat::NONE)
    {
        id::g_viewer_z = values[k++].uval.ival;
        id::g_shift_x     = values[k++].uval.ival;
        id::g_shift_y     = values[k++].uval.ival;
        id::g_adjust_3d_x     = values[k++].uval.ival;
        id::g_adjust_3d_y     = values[k++].uval.ival;
        g_transparent_color_3d[0] = values[k++].uval.ival;
        g_transparent_color_3d[1] = values[k++].uval.ival;
    }
    id::g_randomize_3d  = values[k++].uval.ival;
    id::g_randomize_3d = std::min(id::g_randomize_3d, 7);
    id::g_randomize_3d = std::max(id::g_randomize_3d, 0);

    if (g_targa_out || id::illumine() || id::g_raytrace_format != id::RayTraceFormat::NONE)
    {
        if (get_light_params())
        {
            goto restart_3;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------
static bool get_light_params()
{
    ChoiceBuilder<13> builder;

    // defaults go here
    if (id::illumine() || id::g_raytrace_format != id::RayTraceFormat::NONE)
    {
        builder.int_number("X value light vector", id::g_light_x)
            .int_number("Y value light vector", id::g_light_y)
            .int_number("Z value light vector", id::g_light_z);

        if (id::g_raytrace_format == id::RayTraceFormat::NONE)
        {
            builder.int_number("Light Source Smoothing Factor", id::g_light_avg);
            builder.int_number("Ambient", id::g_ambient);
        }
    }

    if (g_targa_out && id::g_raytrace_format == id::RayTraceFormat::NONE)
    {
        builder.int_number("Haze Factor        (0 - 100, '0' disables)", id::g_haze);

        if (!id::g_targa_overlay)
        {
            check_write_file(id::g_light_name, ".tga");
        }
        builder.string("Targa File Name  (Assume .tga)", id::g_light_name.c_str())
            .comment("Background Color (0 - 255)")
            .int_number("   Red", id::g_background_color[0])
            .int_number("   Green", id::g_background_color[1])
            .int_number("   Blue", id::g_background_color[2])
            .yes_no("Overlay Targa File? (Y/N)", id::g_targa_overlay);
    }
    builder.comment("");

    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_LIGHT};
        if (builder.prompt("Light Source Parameters") < 0)
        {
            return true;
        }
    }

    if (id::illumine())
    {
        id::g_light_x   = builder.read_int_number();
        id::g_light_y   = builder.read_int_number();
        id::g_light_z   = builder.read_int_number();
        if (id::g_raytrace_format == id::RayTraceFormat::NONE)
        {
            id::g_light_avg = builder.read_int_number();
            id::g_ambient  = builder.read_int_number();
            id::g_ambient = std::min(id::g_ambient, 100);
            id::g_ambient = std::max(id::g_ambient, 0);
        }
    }

    if (g_targa_out && id::g_raytrace_format == id::RayTraceFormat::NONE)
    {
        id::g_haze = builder.read_int_number();
        id::g_haze = std::min(id::g_haze, 100);
        id::g_haze = std::max(id::g_haze, 0);
        id::g_light_name = builder.read_string();
        /* In case light_name conflicts with an existing name it is checked again in line3d */
        id::g_background_color[0] = (char)(builder.read_int_number() % 255);
        id::g_background_color[1] = (char)(builder.read_int_number() % 255);
        id::g_background_color[2] = (char)(builder.read_int_number() % 255);
        id::g_targa_overlay = builder.read_yes_no();
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
    if (!id::glasses_alternating_or_superimpose())
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
            ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::NONE};
            int i = field_prompt("Enter name of .map file to use,\n"
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
        bool valid = validate_luts(map_name.string());
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
    if (id::g_viewer_z == 0)
    {
        id::g_viewer_z = 150;
    }
    if (id::g_eye_separation == 0)
    {
        if (g_fractal_type == FractalType::IFS_3D || g_fractal_type == FractalType::LORENZ_3D)
        {
            id::g_eye_separation = 2;
            id::g_converge_x_adjust = -2;
        }
        else
        {
            id::g_eye_separation = 3;
            id::g_converge_x_adjust = 0;
        }
    }

    if (id::g_glasses_type == id::GlassesType::ALTERNATING)
    {
        s_funny_glasses_map_name = GLASSES1_MAP_NAME;
    }
    else if (id::g_glasses_type == id::GlassesType::SUPERIMPOSE)
    {
        if (id::g_fill_type == id::FillType::SURFACE_GRID)
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
    builder.int_number("Interocular distance (as % of screen)", id::g_eye_separation)
        .int_number("Convergence adjust (positive = spread greater)", id::g_converge_x_adjust)
        .int_number("Left  red image crop (% of screen)", id::g_red_crop_left)
        .int_number("Right red image crop (% of screen)", id::g_red_crop_right)
        .int_number("Left  blue image crop (% of screen)", id::g_blue_crop_left)
        .int_number("Right blue image crop (% of screen)", id::g_blue_crop_right)
        .int_number("Red brightness factor (%)", id::g_red_bright)
        .int_number("Blue brightness factor (%)", id::g_blue_bright);
    if (id::glasses_alternating_or_superimpose())
    {
        builder.string("Map file name", s_funny_glasses_map_name.c_str());
    }

    ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_GLASSES};
    int k = builder.prompt("Funny Glasses Parameters");
    if (k < 0)
    {
        return true;
    }

    k = 0;
    id::g_eye_separation   =  builder.read_int_number();
    id::g_converge_x_adjust = builder.read_int_number();
    id::g_red_crop_left   =  builder.read_int_number();
    id::g_red_crop_right  =  builder.read_int_number();
    id::g_blue_crop_left  =  builder.read_int_number();
    id::g_blue_crop_right =  builder.read_int_number();
    id::g_red_bright      =  builder.read_int_number();
    id::g_blue_bright     =  builder.read_int_number();

    if (id::glasses_alternating_or_superimpose())
    {
        s_funny_glasses_map_name = builder.read_string();
    }
    return false;
}

int get_fract3d_params() // prompt for 3D fractal parameters
{
    driver_stack_screen();
    ChoiceBuilder<7> builder;
    builder.int_number("X-axis rotation in degrees", id::g_x_rot)
        .int_number("Y-axis rotation in degrees", id::g_y_rot)
        .int_number("Z-axis rotation in degrees", id::g_z_rot)
        .int_number("Perspective distance [1 - 999, 0 for no persp]", id::g_viewer_z)
        .int_number("X shift with perspective (positive = right)", id::g_shift_x)
        .int_number("Y shift with perspective (positive = up   )", id::g_shift_y)
        .int_number("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)",
            static_cast<int>(id::g_glasses_type));

    int i;
    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_FRACT};
        i = builder.prompt("3D Parameters");
    }

    int ret{};
    if (i < 0)
    {
        ret = -1;
        goto get_f3d_exit;
    }

    id::g_x_rot    = builder.read_int_number();
    id::g_y_rot    = builder.read_int_number();
    id::g_z_rot    = builder.read_int_number();
    id::g_viewer_z = builder.read_int_number();
    id::g_shift_x  = builder.read_int_number();
    id::g_shift_y  = builder.read_int_number();
    id::g_glasses_type = static_cast<id::GlassesType>(builder.read_int_number());
    if (id::g_glasses_type < id::GlassesType::NONE || id::g_glasses_type > id::GlassesType::STEREO_PAIR)
    {
        id::g_glasses_type = id::GlassesType::NONE;
    }
    if (id::g_glasses_type != id::GlassesType::NONE)
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
