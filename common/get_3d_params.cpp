#include "get_3d_params.h"

#include "port.h"
#include "prototyp.h"

#include "choice_builder.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fractype.h"
#include "full_screen_prompt.h"
#include "helpdefs.h"
#include "id_data.h"
#include "line3d.h"
#include "loadmap.h"
#include "merge_path_names.h"
#include "miscres.h"
#include "plot3d.h"
#include "prompts1.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"

#include <cstring>
#include <string>

static  bool get_light_params();
static  bool check_mapfile();
static  bool get_funny_glasses_params();

static char funnyglasses_map_name[16];

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

    prompts3d[++k] = "Preview Mode?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_preview ? 1 : 0;

    prompts3d[++k] = "    Show Box?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_show_box ? 1 : 0;

    prompts3d[++k] = "Coarseness, preview/grid/ray (in y dir)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_preview_factor;

    prompts3d[++k] = "Spherical Projection?";
    uvalues[k].type = 'y';
    sphere = SPHERE;
    uvalues[k].uval.ch.val = sphere;

    prompts3d[++k] = "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_glasses_type;

    prompts3d[++k] = "                  3=photo,4=stereo pair)";
    uvalues[k].type = '*';

    prompts3d[++k] = "Ray trace out? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = static_cast<int>(g_raytrace_format);

    prompts3d[++k] = "                4=MTV, 5=RAYSHADE, 6=ACROSPIN, 7=DXF)";
    uvalues[k].type = '*';

    prompts3d[++k] = "    Brief output?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_brief ? 1 : 0;

    check_writefile(g_raytrace_filename, ".ray");
    prompts3d[++k] = "    Output File Name";
    uvalues[k].type = 's';
    std::strcpy(uvalues[k].uval.sval, g_raytrace_filename.c_str());

    prompts3d[++k] = "Targa output?";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_targa_out ? 1 : 0;

    prompts3d[++k] = "Use grayscale value for depth? (if \"no\" uses color number)";
    uvalues[k].type = 'y';
    uvalues[k].uval.ch.val = g_gray_flag ? 1 : 0;

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP3DMODE;

    k = fullscreen_prompt("3D Mode Selection", k+1, prompts3d, uvalues, 0, nullptr);
    g_help_mode = old_help_mode;
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
            stopmsg(STOPMSG_NONE,
                    "DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
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

    if (sphere && !SPHERE)
    {
        SPHERE = TRUE;
        set_3d_defaults();
    }
    else if (!sphere && SPHERE)
    {
        SPHERE = FALSE;
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
        if (SPHERE)
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
        g_help_mode = help_labels::HELP3DFILL;
        int i = fullscreen_choice(CHOICE_HELP, "Select 3D Fill Type",
                nullptr, nullptr, k, (char const **)choices, attributes,
                0, 0, 0, FILLTYPE+1, nullptr, nullptr, nullptr, nullptr);
        g_help_mode = old_help_mode;
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

    if (SPHERE)
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
    if (!(g_raytrace_format != raytrace_formats::none && !SPHERE))
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

    if (SPHERE)
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
    g_help_mode = help_labels::HELP3DPARMS;
    k = fullscreen_prompt(s, k, prompts3d, uvalues, 0, nullptr);
    g_help_mode = old_help_mode;
    if (k < 0)
    {
        goto restart_1;
    }

    k = 0;
    if (!(g_raytrace_format != raytrace_formats::none && !SPHERE))
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
    char const *prompts3d[13];
    fullscreenvalues uvalues[13];

    int k;

    // defaults go here

    k = -1;

    if (ILLUMINE || g_raytrace_format != raytrace_formats::none)
    {
        prompts3d[++k] = "X value light vector";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = XLIGHT    ;

        prompts3d[++k] = "Y value light vector";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = YLIGHT    ;

        prompts3d[++k] = "Z value light vector";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = ZLIGHT    ;

        if (g_raytrace_format == raytrace_formats::none)
        {
            prompts3d[++k] = "Light Source Smoothing Factor";
            uvalues[k].type = 'i';
            uvalues[k].uval.ival = LIGHTAVG  ;

            prompts3d[++k] = "Ambient";
            uvalues[k].type = 'i';
            uvalues[k].uval.ival = g_ambient;
        }
    }

    if (g_targa_out && g_raytrace_format == raytrace_formats::none)
    {
        prompts3d[++k] = "Haze Factor        (0 - 100, '0' disables)";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = g_haze;

        if (!g_targa_overlay)
        {
            check_writefile(g_light_name, ".tga");
        }
        prompts3d[++k] = "Targa File Name  (Assume .tga)";
        uvalues[k].type = 's';
        std::strcpy(uvalues[k].uval.sval, g_light_name.c_str());

        prompts3d[++k] = "Back Ground Color (0 - 255)";
        uvalues[k].type = '*';

        prompts3d[++k] = "   Red";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = (int)g_background_color[0];

        prompts3d[++k] = "   Green";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = (int)g_background_color[1];

        prompts3d[++k] = "   Blue";
        uvalues[k].type = 'i';
        uvalues[k].uval.ival = (int)g_background_color[2];

        prompts3d[++k] = "Overlay Targa File? (Y/N)";
        uvalues[k].type = 'y';
        uvalues[k].uval.ch.val = g_targa_overlay ? 1 : 0;

    }

    prompts3d[++k] = "";

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP3DLIGHT;
    k = fullscreen_prompt("Light Source Parameters", k, prompts3d, uvalues, 0, nullptr);
    g_help_mode = old_help_mode;
    if (k < 0)
    {
        return true;
    }

    k = 0;
    if (ILLUMINE)
    {
        XLIGHT   = uvalues[k++].uval.ival;
        YLIGHT   = uvalues[k++].uval.ival;
        ZLIGHT   = uvalues[k++].uval.ival;
        if (g_raytrace_format == raytrace_formats::none)
        {
            LIGHTAVG = uvalues[k++].uval.ival;
            g_ambient  = uvalues[k++].uval.ival;
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
        g_haze  =  uvalues[k++].uval.ival;
        if (g_haze >= 100)
        {
            g_haze = 100;
        }
        if (g_haze <= 0)
        {
            g_haze = 0;
        }
        g_light_name = uvalues[k++].uval.sval;
        /* In case light_name conflicts with an existing name it is checked again in line3d */
        k++;
        g_background_color[0] = (char)(uvalues[k++].uval.ival % 255);
        g_background_color[1] = (char)(uvalues[k++].uval.ival % 255);
        g_background_color[2] = (char)(uvalues[k++].uval.ival % 255);
        g_targa_overlay = uvalues[k].uval.ch.val != 0;
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
        merge_pathnames(buff, funnyglasses_map_name, cmd_file::AT_CMD_LINE);
    }

    while (true)
    {
        if (askflag)
        {
            help_labels const old_help_mode = g_help_mode;
            g_help_mode = help_labels::NONE;
            i = field_prompt("Enter name of .MAP file to use,\n"
                             "or '*' to use palette from the image to be loaded.",
                             nullptr, buff, 60, nullptr);
            g_help_mode = old_help_mode;
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
    char const *prompts3d[10];

    fullscreenvalues uvalues[10];

    int k;

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
        std::strcpy(funnyglasses_map_name, g_glasses1_map.c_str());
    }
    else if (g_glasses_type == 2)
    {
        if (FILLTYPE == -1)
        {
            std::strcpy(funnyglasses_map_name, "grid.map");
        }
        else
        {
            std::string glasses2_map{g_glasses1_map};
            glasses2_map.replace(glasses2_map.find('1'), 1, "2");
            std::strcpy(funnyglasses_map_name, glasses2_map.c_str());
        }
    }

    k = -1;
    prompts3d[++k] = "Interocular distance (as % of screen)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_eye_separation;

    prompts3d[++k] = "Convergence adjust (positive = spread greater)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_converge_x_adjust;

    prompts3d[++k] = "Left  red image crop (% of screen)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_red_crop_left;

    prompts3d[++k] = "Right red image crop (% of screen)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_red_crop_right;

    prompts3d[++k] = "Left  blue image crop (% of screen)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_blue_crop_left;

    prompts3d[++k] = "Right blue image crop (% of screen)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_blue_crop_right;

    prompts3d[++k] = "Red brightness factor (%)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_red_bright;

    prompts3d[++k] = "Blue brightness factor (%)";
    uvalues[k].type = 'i';
    uvalues[k].uval.ival = g_blue_bright;

    if (g_glasses_type == 1 || g_glasses_type == 2)
    {
        prompts3d[++k] = "Map File name";
        uvalues[k].type = 's';
        std::strcpy(uvalues[k].uval.sval, funnyglasses_map_name);
    }

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP3DGLASSES;
    k = fullscreen_prompt("Funny Glasses Parameters", k+1, prompts3d, uvalues, 0, nullptr);
    g_help_mode = old_help_mode;
    if (k < 0)
    {
        return true;
    }

    k = 0;
    g_eye_separation   =  uvalues[k++].uval.ival;
    g_converge_x_adjust         =  uvalues[k++].uval.ival;
    g_red_crop_left   =  uvalues[k++].uval.ival;
    g_red_crop_right  =  uvalues[k++].uval.ival;
    g_blue_crop_left  =  uvalues[k++].uval.ival;
    g_blue_crop_right =  uvalues[k++].uval.ival;
    g_red_bright      =  uvalues[k++].uval.ival;
    g_blue_bright     =  uvalues[k++].uval.ival;

    if (g_glasses_type == 1 || g_glasses_type == 2)
    {
        std::strcpy(funnyglasses_map_name, uvalues[k].uval.sval);
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

    help_labels const old_help_mode = g_help_mode;
    g_help_mode = help_labels::HELP3DFRACT;
    int i = builder.prompt("3D Parameters", 0, nullptr);
    g_help_mode = old_help_mode;

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
    g_glasses_type = builder.read_int_number();;
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
