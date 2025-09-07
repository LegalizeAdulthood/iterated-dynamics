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
        g_targa_overlay = true;
    }

    k = -1;

    prompts3d[++k] = "Preview mode?";
    values[k].type = 'y';
    values[k].uval.ch.val = g_preview ? 1 : 0;

    prompts3d[++k] = "    Show box?";
    values[k].type = 'y';
    values[k].uval.ch.val = g_show_box ? 1 : 0;

    prompts3d[++k] = "Coarseness, preview/grid/ray (in y dir)";
    values[k].type = 'i';
    values[k].uval.ival = g_preview_factor;

    prompts3d[++k] = "Spherical projection?";
    values[k].type = 'y';
    sphere = g_sphere ? 1 : 0;
    values[k].uval.ch.val = sphere;

    prompts3d[++k] = "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,";
    values[k].type = 'i';
    values[k].uval.ival = static_cast<int>(g_glasses_type);

    prompts3d[++k] = "                  3=photo,4=stereo pair)";
    values[k].type = '*';

    prompts3d[++k] = "Ray trace output? (No, DKB/POV-Ray, VIVID, RAW, MTV,";
    values[k].type = 'l';
    static const char *raytrace_formats[]{
        "No", "DKB/POV-Ray", "VIVID", "Raw", "MTV", "Rayshade", "AcroSpin", "DXF"};
    values[k].uval.ch.list = raytrace_formats;
    values[k].uval.ch.list_len = static_cast<int>(std::size(raytrace_formats));
    values[k].uval.ch.vlen = 11;
    values[k].uval.ch.val = static_cast<int>(g_raytrace_format);
    prompts3d[++k] = "                Rayshade, AcroSpin, DXF)";
    values[k].type = '*';

    prompts3d[++k] = "    Brief output?";
    values[k].type = 'y';
    values[k].uval.ch.val = g_brief ? 1 : 0;

    prompts3d[++k] = "    Output file name";
    values[k].type = 's';
    std::strcpy(values[k].uval.sval, g_raytrace_filename.c_str());

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
    g_preview = values[k++].uval.ch.val != 0;
    g_show_box = values[k++].uval.ch.val != 0;
    g_preview_factor  = values[k++].uval.ival;
    sphere = values[k++].uval.ch.val;
    g_glasses_type = static_cast<GlassesType>(values[k++].uval.ival);
    k++;
    g_raytrace_format = static_cast<RayTraceFormat>(values[k++].uval.ch.val);
    k++;
    {
        if (g_raytrace_format == RayTraceFormat::DKB_POVRAY)
        {
            stop_msg("DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
                    "the online documentation.");
        }
    }
    g_brief = values[k++].uval.ch.val != 0;

    g_raytrace_filename = values[k++].uval.sval;

    g_targa_out = values[k++].uval.ch.val != 0;
    g_gray_flag  = values[k++].uval.ch.val != 0;

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

    if (g_raytrace_format == RayTraceFormat::NONE)
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
            ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_FILL};
            i = full_screen_choice(ChoiceFlags::HELP, "Select 3D Fill Type", nullptr, nullptr, k, choices,
                attributes, 0, 0, 0, +g_fill_type + 1, nullptr, nullptr, nullptr, nullptr);
        }
        if (i < 0)
        {
            goto restart_1;
        }
        g_fill_type = static_cast<FillType>(i - 1);

        if (g_glasses_type != GlassesType::NONE)
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

    if (g_sphere)
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
        if (g_raytrace_format == RayTraceFormat::NONE)
        {
            prompts3d[++k] = "X-axis rotation in degrees";
            prompts3d[++k] = "Y-axis rotation in degrees";
            prompts3d[++k] = "Z-axis rotation in degrees";
        }
        prompts3d[++k] = "X-axis scaling factor in pct";
        prompts3d[++k] = "Y-axis scaling factor in pct";
    }
    k = -1;
    if (g_raytrace_format == RayTraceFormat::NONE || g_sphere)
    {
        values[++k].uval.ival   = g_x_rot    ;
        values[k].type = 'i';
        values[++k].uval.ival   = g_y_rot    ;
        values[k].type = 'i';
        values[++k].uval.ival   = g_z_rot    ;
        values[k].type = 'i';
    }
    values[++k].uval.ival   = g_x_scale    ;
    values[k].type = 'i';

    values[++k].uval.ival   = g_y_scale    ;
    values[k].type = 'i';

    prompts3d[++k] = "Surface Roughness scaling factor in pct";
    values[k].type = 'i';
    values[k].uval.ival = g_rough     ;

    prompts3d[++k] = "'Water Level' (minimum color value)";
    values[k].type = 'i';
    values[k].uval.ival = g_water_line ;

    if (g_raytrace_format == RayTraceFormat::NONE)
    {
        prompts3d[++k] = "Perspective distance [1 - 999, 0 for no persp])";
        values[k].type = 'i';
        values[k].uval.ival = g_viewer_z     ;

        prompts3d[++k] = "X shift with perspective (positive = right)";
        values[k].type = 'i';
        values[k].uval.ival = g_shift_x    ;

        prompts3d[++k] = "Y shift with perspective (positive = up   )";
        values[k].type = 'i';
        values[k].uval.ival = g_shift_y    ;

        prompts3d[++k] = "Image non-perspective X adjust (positive = right)";
        values[k].type = 'i';
        values[k].uval.ival = g_adjust_3d_x    ;

        prompts3d[++k] = "Image non-perspective Y adjust (positive = up)";
        values[k].type = 'i';
        values[k].uval.ival = g_adjust_3d_y    ;

        prompts3d[++k] = "First transparent color";
        values[k].type = 'i';
        values[k].uval.ival = g_transparent_color_3d[0];

        prompts3d[++k] = "Last transparent color";
        values[k].type = 'i';
        values[k].uval.ival = g_transparent_color_3d[1];
    }

    prompts3d[++k] = "Randomize Colors      (0 - 7, '0' disables)";
    values[k].type = 'i';
    values[k++].uval.ival = g_randomize_3d;

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
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_PARAMETERS};
        k = full_screen_prompt(s, k, prompts3d, values, 0, nullptr);
    }
    if (k < 0)
    {
        goto restart_1;
    }

    k = 0;
    if (g_raytrace_format == RayTraceFormat::NONE || g_sphere)
    {
        g_x_rot    = values[k++].uval.ival;
        g_y_rot    = values[k++].uval.ival;
        g_z_rot    = values[k++].uval.ival;
    }
    g_x_scale     = values[k++].uval.ival;
    g_y_scale     = values[k++].uval.ival;
    g_rough      = values[k++].uval.ival;
    g_water_line  = values[k++].uval.ival;
    if (g_raytrace_format == RayTraceFormat::NONE)
    {
        g_viewer_z = values[k++].uval.ival;
        g_shift_x     = values[k++].uval.ival;
        g_shift_y     = values[k++].uval.ival;
        g_adjust_3d_x     = values[k++].uval.ival;
        g_adjust_3d_y     = values[k++].uval.ival;
        g_transparent_color_3d[0] = values[k++].uval.ival;
        g_transparent_color_3d[1] = values[k++].uval.ival;
    }
    g_randomize_3d  = values[k++].uval.ival;
    g_randomize_3d = std::min(g_randomize_3d, 7);
    g_randomize_3d = std::max(g_randomize_3d, 0);

    if (g_targa_out || illumine() || g_raytrace_format != RayTraceFormat::NONE)
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

        if (!g_targa_overlay)
        {
            check_write_file(g_light_name, ".tga");
        }
        builder.string("Targa File Name  (Assume .tga)", g_light_name.c_str())
            .comment("Background Color (0 - 255)")
            .int_number("   Red", g_background_color[0])
            .int_number("   Green", g_background_color[1])
            .int_number("   Blue", g_background_color[2])
            .yes_no("Overlay Targa File? (Y/N)", g_targa_overlay);
    }
    builder.comment("");

    {
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_LIGHT};
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
        /* In case light_name conflicts with an existing name it is checked again in line3d */
        g_background_color[0] = (char)(builder.read_int_number() % 255);
        g_background_color[1] = (char)(builder.read_int_number() % 255);
        g_background_color[2] = (char)(builder.read_int_number() % 255);
        g_targa_overlay = builder.read_yes_no();
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

    ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_GLASSES};
    int k = builder.prompt("Funny Glasses Parameters");
    if (k < 0)
    {
        return true;
    }

    k = 0;
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
        ValueSaver saved_help_mode{g_help_mode, id::help::HelpLabels::HELP_3D_FRACT};
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
