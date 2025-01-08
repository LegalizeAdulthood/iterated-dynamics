// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class FractalType;

enum class Julibrot3DMode
{
    MONOCULAR = 0,
    LEFT_EYE = 1,
    RIGHT_EYE = 2,
    RED_BLUE = 3
};

constexpr const char *to_string(Julibrot3DMode value)
{
    switch (value)
    {
    default:
    case Julibrot3DMode::MONOCULAR:
        return "monocular";
    case Julibrot3DMode::LEFT_EYE:
        return "lefteye";
    case Julibrot3DMode::RIGHT_EYE:
        return "righteye";
    case Julibrot3DMode::RED_BLUE:
        return "red-blue";
    }
}

extern bool                  g_julibrot;
extern float                 g_eyes_fp;
extern Julibrot3DMode        g_julibrot_3d_mode;
extern float                 g_julibrot_depth_fp;
extern float                 g_julibrot_dist_fp;
extern float                 g_julibrot_height_fp;
extern float                 g_julibrot_origin_fp;
extern float                 g_julibrot_width_fp;
extern double                g_julibrot_x_max;
extern double                g_julibrot_x_min;
extern double                g_julibrot_y_max;
extern double                g_julibrot_y_min;
extern int                   g_julibrot_z_dots;
extern FractalType           g_new_orbit_type;
extern const char *          g_julibrot_3d_options[];

bool julibrot_setup();
bool julibrot_fp_setup();
int jb_per_pixel();
int jb_fp_per_pixel();
int z_line(long x, long y);
int z_line_fp(double x, double y);
int std_4d_fractal();
int std_4d_fp_fractal();
