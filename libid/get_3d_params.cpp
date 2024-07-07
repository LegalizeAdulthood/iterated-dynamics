#include "get_3d_params.h"

#include "check_write_file.h"
#include "choice_builder.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "field_prompt.h"
#include "fractype.h"
#include "full_screen_choice.h"
#include "full_screen_prompt.h"
#include "helpdefs.h"
#include "id_data.h"
#include "line3d.h"
#include "loadmap.h"
#include "merge_path_names.h"
#include "plot3d.h"
#include "prototyp.h"
#include "rotate.h"
#include "stereo.h"
#include "stop_msg.h"
#include "value_saver.h"

#include <cstring>
#include <string>

static  bool get_light_params();
static  bool check_mapfile();
static  bool get_funny_glasses_params();

static std::string g_funny_glasses_map_name;

std::string const g_glasses1_map = "glasses1.map";

int get_3d_params()     // prompt for 3D parameters
{
    char const *choices[11];
    int attributes[21];
    int sphere;
    char const *s;
    char const *prompts3d[21];
    fullscreenvalues uvalues[21];
    int k;

restart_1:
    if (g_targa_out && g_overlay_3d)
    {
        g_targa_overlay = true;
    }

    k = -1;

    prompts3d[++k] = "Preview mode?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_preview ? 1 : 0;

    prompts3d[++k] = "    Show box?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_show_box ? 1 : 0;

    prompts3d[++k] = "Coarseness, preview/grid/ray (in y dir)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_preview_factor;

    prompts3d[++k] = "Spherical projection?";
    uvalues[k].type = 'y';
    sphere = g_sphere;
    uvalues[k].uval.ch.val = sphere;

    prompts3d[++k] = "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_glasses_type;

    prompts3d[++k] = "                  3=photo,4=stereo pair)";
    uvalues[k].type = '*';

    prompts3d[++k] = "Ray trace output? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = static_cast<int>(g_raytrace_format);

    prompts3d[++k] = "                4=MTV, 5=RAYSHADE, 6=ACROSPIN, 7=DXF)";
    uvalues[k].type = '*';

    prompts3d[++k] = "    Brief output?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_brief ? 1 : 0;

    check_writefile(g_raytrace_filename, ".ray");
    prompts3d[++k] = "    Output file name";
    uvalues[k].type = 's';
    std::strcpy(uvalues[k].uval.sval, g_raytrace_filename.c_str());

    prompts3d[++k] = "Targa output?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_targa_out ? 1 : 0;

    prompts3d[++k] = "Use grayscale value for depth? (if \"no\" uses color number)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_gray_flag ? 1 : 0;

    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_3D_MODE};
        k = fullscreen_prompt("3D Mode Selection", k+1, prompts3d, uvalues, 0, nullptr);
    }
    if (k < 0)
    {
        return -1;
    }

    k = 0;
    g_preview = uvalues[k++].uval.ch.val != 0;
    g_show_box = uvalues[k++].uval.ch.val != 0;
    g_preview_factor  = uvalues[k++].uval.ival;
    sphere = uvalues[k++].uval.ch.val;
    g_glasses_type = uvalues[k++].uval.ival;
    k++;
    g_raytrace_format = static_cast<raytrace_formats>(uvalues[k++].uval.ival);
    k++;
    {
        if (g_raytrace_format == raytrace_formats::povray)
        {
            stopmsg("DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
                    "the online documentation.");
        }
    }
    g_brief = uvalues[k++].uval.ch.val != 0;

    g_raytrace_filename = uvalues[k++].uval.sval;

    g_targa_out = uvalues[k++].uval.ch.val != 0;
    g_gray_flag  = uvalues[k++].uval.ch.val != 0;

    // check ranges
    if (g_preview_factor < 2)
    {
        g_preview_factor = 2;
    }
    if (g_preview_factor > 2000)
    {
        g_preview_factor = 2000;
    }

    if (sphere && !g_sphere)
    {
        g_sphere = TRUE;
        set_3d_defaults();
    }
    else if (!sphere && g_sphere)
    {
        g_sphere = FALSE;
        set_3d_defaults();
    }

    if (g_glasses_type < 0)
    {
        g_glasses_type = 0;
    }
    if (g_glasses_type > 4)
    {
        g_glasses_type = 4;
    }
    if (g_glasses_type)
    {
        g_which_image = stereo_images::RED;
    }

    if (static_cast<int>(g_raytrace_format) < 0)
    {
        g_raytrace_format = raytrace_formats::none;
    }
    if (g_raytrace_format > raytrace_formats::dxf)
    {
        g_raytrace_format = raytrace_formats::dxf;
    }

    if (g_raytrace_format == raytrace_formats::none)
    {
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
            ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_3D_FILL};
            i = fullscreen_choice(CHOICE_HELP, "Select 3D Fill Type", nullptr, nullptr, k, choices,
                attributes, 0, 0, 0, FILLTYPE + 1, nullptr, nullptr, nullptr, nullptr);
        }
        if (i < 0)
        {
            goto restart_1;
        }
        FILLTYPE = i-1;

        if (g_glasses_type)
        {
            if (get_funny_glasses_params())
            {
                goto restart_1;
            }
        }
        if (check_mapfile())
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
        if (g_raytrace_format == raytrace_formats::none)
        {
            prompts3d[++k] = "X-axis rotation in degrees";
            prompts3d[++k] = "Y-axis rotation in degrees";
            prompts3d[++k] = "Z-axis rotation in degrees";
        }
        prompts3d[++k] = "X-axis scaling factor in pct";
        prompts3d[++k] = "Y-axis scaling factor in pct";
    }
    k = -1;
    if (!(g_raytrace_format != raytrace_formats::none && !g_sphere))
    {
        uvalues[++k].uval.ival   = XROT    ;
        uvalues[k].type = 'i';
        uvalues[++k].uval.ival   = YROT    ;
        uvalues[k].type = 'i';
        uvalues[++k].uval.ival   = ZROT    ;
        uvalues[k].type = 'i';
    }
    uvalues[++k].uval.ival   = XSCALE    ;
    uvalues[k].type = 'i';

    uvalues[++k].uval.ival   = YSCALE    ;
    uvalues[k].type = 'i';

    prompts3d[++k] = "Surface Roughness scaling factor in pct";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = ROUGH     ;

    prompts3d[++k] = "'Water Level' (minimum color value)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = WATERLINE ;

    if (g_raytrace_format == raytrace_formats::none)
    {
        prompts3d[++k] = "Perspective distance [1 - 999, 0 for no persp])";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = ZVIEWER     ;

        prompts3d[++k] = "X shift with perspective (positive = right)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = XSHIFT    ;

        prompts3d[++k] = "Y shift with perspective (positive = up   )";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = YSHIFT    ;

        prompts3d[++k] = "Image non-perspective X adjust (positive = right)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_adjust_3d_x    ;

        prompts3d[++k] = "Image non-perspective Y adjust (positive = up)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_adjust_3d_y    ;

        prompts3d[++k] = "First transparent color";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_transparent_color_3d[0];

        prompts3d[++k] = "Last transparent color";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_transparent_color_3d[1];
    }

    prompts3d[++k] = "Randomize Colors      (0 - 7, '0' disables)";
    uvalues[k].type = 'i';
    uvalues[k++].uval.ival = g_randomize_3d;

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
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_3D_PARAMETERS};
        k = fullscreen_prompt(s, k, prompts3d, uvalues, 0, nullptr);
    }
    if (k < 0)
    {
        goto restart_1;
    }

    k = 0;
    if (!(g_raytrace_format != raytrace_formats::none && !g_sphere))
    {
        XROT    = uvalues[k++].uval.ival;
        YROT    = uvalues[k++].uval.ival;
        ZROT    = uvalues[k++].uval.ival;
    }
    XSCALE     = uvalues[k++].uval.ival;
    YSCALE     = uvalues[k++].uval.ival;
    ROUGH      = uvalues[k++].uval.ival;
    WATERLINE  = uvalues[k++].uval.ival;
    if (g_raytrace_format == raytrace_formats::none)
    {
        ZVIEWER = uvalues[k++].uval.ival;
        XSHIFT     = uvalues[k++].uval.ival;
        YSHIFT     = uvalues[k++].uval.ival;
        g_adjust_3d_x     = uvalues[k++].uval.ival;
        g_adjust_3d_y     = uvalues[k++].uval.ival;
        g_transparent_color_3d[0] = uvalues[k++].uval.ival;
        g_transparent_color_3d[1] = uvalues[k++].uval.ival;
    }
    g_randomize_3d  = uvalues[k++].uval.ival;
    if (g_randomize_3d >= 7)
    {
        g_randomize_3d = 7;
    }
    if (g_randomize_3d <= 0)
    {
        g_randomize_3d = 0;
    }

    if (g_targa_out || ILLUMINE || g_raytrace_format != raytrace_formats::none)
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

    int k;

    // defaults go here

    k = -1;

    if (ILLUMINE || g_raytrace_format != raytrace_formats::none)
    {
        builder.int_number("X value light vector", XLIGHT)
            .int_number("Y value light vector", YLIGHT)
            .int_number("Z value light vector", ZLIGHT);

        if (g_raytrace_format == raytrace_formats::none)
        {
            builder.int_number("Light Source Smoothing Factor", LIGHTAVG);
            builder.int_number("Ambient", g_ambient);
        }
    }

    if (g_targa_out && g_raytrace_format == raytrace_formats::none)
    {
        builder.int_number("Haze Factor        (0 - 100, '0' disables)", g_haze);

        if (!g_targa_overlay)
        {
            check_writefile(g_light_name, ".tga");
        }
        builder.string("Targa File Name  (Assume .tga)", g_light_name.c_str())
            .comment("Back Ground Color (0 - 255)")
            .int_number("   Red", g_background_color[0])
            .int_number("   Green", g_background_color[1])
            .int_number("   Blue", g_background_color[2])
            .yes_no("Overlay Targa File? (Y/N)", g_targa_overlay);
    }
    builder.comment("");

    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_3D_LIGHT};
        k = builder.prompt("Light Source Parameters");
    }
    if (k < 0)
    {
        return true;
    }

    k = 0;
    if (ILLUMINE)
    {
        XLIGHT   = builder.read_int_number();
        YLIGHT   = builder.read_int_number();
        ZLIGHT   = builder.read_int_number();
        if (g_raytrace_format == raytrace_formats::none)
        {
            LIGHTAVG = builder.read_int_number();
            g_ambient  = builder.read_int_number();
            if (g_ambient >= 100)
            {
                g_ambient = 100;
            }
            if (g_ambient <= 0)
            {
                g_ambient = 0;
            }
        }
    }

    if (g_targa_out && g_raytrace_format == raytrace_formats::none)
    {
        g_haze = builder.read_int_number();
        if (g_haze >= 100)
        {
            g_haze = 100;
        }
        if (g_haze <= 0)
        {
            g_haze = 0;
        }
        g_light_name = builder.read_string();
        /* In case light_name conflicts with an existing name it is checked again in line3d */
        k++;
        g_background_color[0] = (char)(builder.read_int_number() % 255);
        g_background_color[1] = (char)(builder.read_int_number() % 255);
        g_background_color[2] = (char)(builder.read_int_number() % 255);
        g_targa_overlay = builder.read_yes_no();
    }
    return false;
}

// ---------------------------------------------------------------------


static bool check_mapfile()
{
    bool askflag = false;
    int i;
    if (!g_read_color)
    {
        return false;
    }
    char buff[256] = "*";
    if (g_map_set)
    {
        std::strcpy(buff, g_map_name.c_str());
    }
    if (!(g_glasses_type == 1 || g_glasses_type == 2))
    {
        askflag = true;
    }
    else
    {
        merge_pathnames(buff, g_funny_glasses_map_name.c_str(), cmd_file::AT_CMD_LINE);
    }

    while (true)
    {
        if (askflag)
        {
            ValueSaver saved_help_mode{g_help_mode, help_labels::NONE};
            i = field_prompt("Enter name of .map file to use,\n"
                             "or '*' to use palette from the image to be loaded.",
                nullptr, buff, 60, nullptr);
            if (i < 0)
            {
                return true;
            }
            if (buff[0] == '*')
            {
                g_map_set = false;
                break;
            }
        }
        std::memcpy(g_old_dac_box, g_dac_box, 256*3); // save the DAC
        bool valid = ValidateLuts(buff);
        std::memcpy(g_dac_box, g_old_dac_box, 256*3); // restore the DAC
        if (valid) // Oops, somethings wrong
        {
            askflag = true;
            continue;
        }
        g_map_set = true;
        merge_pathnames(g_map_name, buff, cmd_file::AT_CMD_LINE);
        break;
    }
    return false;
}

static bool get_funny_glasses_params()
{
    // defaults
    if (ZVIEWER == 0)
    {
        ZVIEWER = 150;
    }
    if (g_eye_separation == 0)
    {
        if (g_fractal_type == fractal_type::IFS3D || g_fractal_type == fractal_type::LLORENZ3D || g_fractal_type == fractal_type::FPLORENZ3D)
        {
            g_eye_separation =  2;
            g_converge_x_adjust       = -2;
        }
        else
        {
            g_eye_separation =  3;
            g_converge_x_adjust       =  0;
        }
    }

    if (g_glasses_type == 1)
    {
        g_funny_glasses_map_name = g_glasses1_map;
    }
    else if (g_glasses_type == 2)
    {
        if (FILLTYPE == +fill_type::SURFACE_GRID)
        {
            g_funny_glasses_map_name = "grid.map";
        }
        else
        {
            std::string glasses2_map{g_glasses1_map};
            glasses2_map.replace(glasses2_map.find('1'), 1, "2");
            g_funny_glasses_map_name = glasses2_map;
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
    if (g_glasses_type == 1 || g_glasses_type == 2)
    {
        builder.string("Map file name", g_funny_glasses_map_name.c_str());
    }

    ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_3D_GLASSES};
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

    if (g_glasses_type == 1 || g_glasses_type == 2)
    {
        g_funny_glasses_map_name = builder.read_string();
    }
    return false;
}

int get_fract3d_params() // prompt for 3D fractal parameters
{
    driver_stack_screen();
    ChoiceBuilder<7> builder;
    builder.int_number("X-axis rotation in degrees", XROT)
        .int_number("Y-axis rotation in degrees", YROT)
        .int_number("Z-axis rotation in degrees", ZROT)
        .int_number("Perspective distance [1 - 999, 0 for no persp]", ZVIEWER)
        .int_number("X shift with perspective (positive = right)", XSHIFT)
        .int_number("Y shift with perspective (positive = up   )", YSHIFT)
        .int_number("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)", g_glasses_type);

    int i;
    {
        ValueSaver saved_help_mode{g_help_mode, help_labels::HELP_3D_FRACT};
        i = builder.prompt("3D Parameters");
    }

    int ret{};
    if (i < 0)
    {
        ret = -1;
        goto get_f3d_exit;
    }

    XROT    = builder.read_int_number();
    YROT    = builder.read_int_number();
    ZROT    = builder.read_int_number();
    ZVIEWER = builder.read_int_number();
    XSHIFT  = builder.read_int_number();
    YSHIFT  = builder.read_int_number();
    g_glasses_type = builder.read_int_number();
    if (g_glasses_type < 0 || g_glasses_type > 4)
    {
        g_glasses_type = 0;
    }
    if (g_glasses_type)
    {
        if (get_funny_glasses_params() || check_mapfile())
        {
            ret = -1;
        }
    }

get_f3d_exit:
    driver_unstack_screen();
    return ret;
}
