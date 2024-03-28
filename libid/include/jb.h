#pragma once

enum class fractal_type;

enum class julibrot_3d_mode
{
    MONOCULAR = 0,
    LEFT_EYE = 1,
    RIGHT_EYE = 2,
    RED_BLUE = 3
};

constexpr const char *to_string(julibrot_3d_mode value)
{
    switch (value)
    {
    default:
    case julibrot_3d_mode::MONOCULAR:
        return "monocular";
    case julibrot_3d_mode::LEFT_EYE:
        return "lefteye";
    case julibrot_3d_mode::RIGHT_EYE:
        return "righteye";
    case julibrot_3d_mode::RED_BLUE:
        return "red-blue";
    }
}

extern bool                  g_julibrot;
extern float                 g_eyes_fp;
extern julibrot_3d_mode      g_julibrot_3d_mode;
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
extern fractal_type          g_new_orbit_type;
extern const char *          g_julibrot_3d_options[];

bool JulibrotSetup();
bool JulibrotfpSetup();
int jb_per_pixel();
int jbfp_per_pixel();
int zline(long, long);
int zlinefp(double, double);
int Std4dFractal();
int Std4dfpFractal();
