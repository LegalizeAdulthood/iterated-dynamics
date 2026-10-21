// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/color_state.h"
#include "geometry/line3d.h"
#include "geometry/plot3d.h"

#include <array>
#include <string>

namespace id::ui
{

struct ThreeDSettings
{
    ThreeDSettings();
    void apply() const;
    void reset_geometry();

    bool preview{};
    bool show_box{};
    int preview_factor{};
    bool sphere{};
    geometry::GlassesType glasses_type{};
    geometry::StereoImage which_image{};
    geometry::RayTraceFormat raytrace_format{};
    bool brief{};
    std::string raytrace_filename{};
    bool targa_out{};
    bool gray_flag{};
    geometry::FillType fill_type{};
    int x_rot{};
    int y_rot{};
    int z_rot{};
    int x_scale{};
    int y_scale{};
    int sphere_phi_min{};
    int sphere_phi_max{};
    int sphere_theta_min{};
    int sphere_theta_max{};
    int sphere_radius{};
    int rough{};
    int water_line{};
    int viewer_z{};
    int shift_x{};
    int shift_y{};
    int randomize_3d{};
    int light_x{};
    int light_y{};
    int light_z{};
    int light_avg{};
    int ambient{};
    int haze{};
    math::Point2i adjust_3d{};
    std::array<int, 2> transparent_color_3d{};
    std::string light_name{};
    std::array<Byte, 3> background_color{};
    bool targa_overlay{};
    int eye_separation{};
    int converge_x_adjust{};
    int red_crop_left{};
    int red_crop_right{};
    int blue_crop_left{};
    int blue_crop_right{};
    int red_bright{};
    int blue_bright{};
    bool map_set{};
    std::string map_name{};
    engine::ColorState color_state{};
    std::string last_map_name;
    std::string funny_glasses_map_name;
};

} // namespace id::ui
