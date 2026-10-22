// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/ThreeDSettings.h"

#include "engine/spindac.h"
#include "geometry/stereo_params.h"
#include "io/loadmap.h"

#include <algorithm>

using namespace id::engine;
using namespace id::geometry;
using namespace id::io;

namespace id::ui
{

ThreeDSettings::ThreeDSettings()
{
    preview = g_preview;
    show_box = g_show_box;
    preview_factor = g_preview_factor;
    sphere = g_sphere;
    glasses_type = g_glasses_type;
    which_image = g_which_image;
    raytrace_format = g_raytrace_format;
    brief = g_brief;
    raytrace_filename = g_raytrace_filename;
    targa_out = g_targa_out;
    gray_flag = g_gray_flag;
    fill_type = g_fill_type;
    x_rot = g_x_rot;
    y_rot = g_y_rot;
    z_rot = g_z_rot;
    x_scale = g_x_scale;
    y_scale = g_y_scale;
    sphere_phi_min = g_sphere_phi_min;
    sphere_phi_max = g_sphere_phi_max;
    sphere_theta_min = g_sphere_theta_min;
    sphere_theta_max = g_sphere_theta_max;
    sphere_radius = g_sphere_radius;
    rough = g_rough;
    water_line = g_water_line;
    viewer_z = g_viewer_z;
    shift_x = g_shift_x;
    shift_y = g_shift_y;
    randomize_3d = g_randomize_3d;
    light_x = g_light_x;
    light_y = g_light_y;
    light_z = g_light_z;
    light_avg = g_light_avg;
    ambient = g_ambient;
    haze = g_haze;
    adjust_3d = g_adjust_3d;
    std::copy_n(g_transparent_color_3d, transparent_color_3d.size(), transparent_color_3d.begin());
    light_name = g_light_name;
    std::copy_n(g_background_color, background_color.size(), background_color.begin());
    targa_overlay = g_targa_overlay;
    eye_separation = g_eye_separation;
    converge_x_adjust = g_converge_x_adjust;
    red_crop_left = g_red_crop_left;
    red_crop_right = g_red_crop_right;
    blue_crop_left = g_blue_crop_left;
    blue_crop_right = g_blue_crop_right;
    red_bright = g_red_bright;
    blue_bright = g_blue_bright;
    map_set = g_map_set;
    map_name = g_map_name;
    color_state = g_color_state;
    last_map_name = g_last_map_name;
}

void ThreeDSettings::apply() const
{
    g_preview = preview;
    g_show_box = show_box;
    g_preview_factor = preview_factor;
    g_sphere = sphere;
    g_glasses_type = glasses_type;
    g_which_image = which_image;
    g_raytrace_format = raytrace_format;
    g_brief = brief;
    g_raytrace_filename = raytrace_filename;
    g_targa_out = targa_out;
    g_gray_flag = gray_flag;
    g_fill_type = fill_type;
    g_x_rot = x_rot;
    g_y_rot = y_rot;
    g_z_rot = z_rot;
    g_x_scale = x_scale;
    g_y_scale = y_scale;
    g_sphere_phi_min = sphere_phi_min;
    g_sphere_phi_max = sphere_phi_max;
    g_sphere_theta_min = sphere_theta_min;
    g_sphere_theta_max = sphere_theta_max;
    g_sphere_radius = sphere_radius;
    g_rough = rough;
    g_water_line = water_line;
    g_viewer_z = viewer_z;
    g_shift_x = shift_x;
    g_shift_y = shift_y;
    g_randomize_3d = randomize_3d;
    g_light_x = light_x;
    g_light_y = light_y;
    g_light_z = light_z;
    g_light_avg = light_avg;
    g_ambient = ambient;
    g_haze = haze;
    g_adjust_3d = adjust_3d;
    std::copy(transparent_color_3d.begin(), transparent_color_3d.end(), g_transparent_color_3d);
    g_light_name = light_name;
    std::copy(background_color.begin(), background_color.end(), g_background_color);
    g_targa_overlay = targa_overlay;
    g_eye_separation = eye_separation;
    g_converge_x_adjust = converge_x_adjust;
    g_red_crop_left = red_crop_left;
    g_red_crop_right = red_crop_right;
    g_blue_crop_left = blue_crop_left;
    g_blue_crop_right = blue_crop_right;
    g_red_bright = red_bright;
    g_blue_bright = blue_bright;
    g_map_set = map_set;
    g_map_name = map_name;
    g_color_state = color_state;
    g_last_map_name = last_map_name;
}

void ThreeDSettings::reset_geometry()
{
    rough = 30;
    water_line = 0;
    viewer_z = 0;
    shift_x = 0;
    shift_y = 0;
    adjust_3d.x = 0;
    adjust_3d.y = 0;
    light_avg = 0;
    ambient = 20;
    randomize_3d = 0;
    haze = 0;
    background_color[0] = 51;
    background_color[1] = 153;
    background_color[2] = 200;
    if (sphere)
    {
        sphere_phi_min = 180;
        sphere_phi_max = 0;
        sphere_theta_min = -90;
        sphere_theta_max = 90;
        sphere_radius = 100;
        fill_type = FillType::SURFACE_INTERPOLATED;
        light_x = 1;
        light_y = 1;
        light_z = 1;
    }
    else
    {
        x_rot = 60;
        y_rot = 30;
        z_rot = 0;
        x_scale = 90;
        y_scale = 90;
        fill_type = FillType::POINTS;
        light_x = 1;
        light_y = -1;
        light_z = 1;
    }
}

} // namespace id::ui
