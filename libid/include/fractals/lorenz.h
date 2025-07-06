// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

struct Affine
{
    // weird order so a,b,e and c,d,f are vectors
    double a;
    double b;
    double e;
    double c;
    double d;
    double f;
};

enum class Major
{
    BREADTH_FIRST,
    DEPTH_FIRST,
    RANDOM_WALK,
    RANDOM_RUN
};
inline int operator+(Major value)
{
    return static_cast<int>(value);
}

enum class Minor
{
    LEFT_FIRST,
    RIGHT_FIRST
};
inline int operator+(Minor value)
{
    return static_cast<int>(value);
}

extern Minor                 g_inverse_julia_minor_method;
extern bool                  g_keep_screen_coords;
extern Major                 g_major_method;
extern long                  g_max_count;
extern double                g_orbit_corner_3rd_x;
extern double                g_orbit_corner_3rd_y;
extern double                g_orbit_corner_max_x;
extern double                g_orbit_corner_max_y;
extern double                g_orbit_corner_min_x;
extern double                g_orbit_corner_min_y;
extern long                  g_orbit_interval;
extern bool                  g_set_orbit_corners;

constexpr const char *to_string(Major value)
{
    switch(value)
    {
    default:
    case Major::BREADTH_FIRST:
        return "breadth";
    case Major::DEPTH_FIRST:
        return "depth";
    case Major::RANDOM_WALK:
        return "walk";
    case Major::RANDOM_RUN:
        return "run";
    }
}

constexpr const char *to_string(Minor value)
{
    return value == Minor::LEFT_FIRST ? "left" : "right";
}

bool orbit3d_per_image();
int lorenz3d1_orbit(double *x, double *y, double *z);
int lorenz3d_orbit(double *x, double *y, double *z);
int lorenz3d3_orbit(double *x, double *y, double *z);
int lorenz3d4_orbit(double *x, double *y, double *z);
int henon_orbit(double *x, double *y, double *z);
int inverse_julia_orbit();
int rossler_orbit(double *x, double *y, double *z);
int pickover_orbit(double *x, double *y, double *z);
int ginger_bread_orbit(double *x, double *y, double *z);
int kam_torus_orbit(double *r, double *s, double *z);
int hopalong2d_orbit(double *x, double *y, double *z);
int chip2d_orbit(double *x, double *y, double *z);
int quadrup_two2d_orbit(double *x, double *y, double *z);
int three_ply2d_orbit(double *x, double *y, double *z);
int martin2d_orbit(double *x, double *y, double *z);
int orbit2d_type();
int funny_glasses_call(int (*calc)());
int ifs_type();
int orbit3d_type();
int icon_orbit(double *x, double *y, double *z);
int latoo_orbit(double *x, double *y, double *z);
bool setup_convert_to_screen(Affine *scrn_cnvt);
int plot_orbits2d_setup();
int plot_orbits2d_float();
int dynamic_orbit(double *x, double *y, double *z);
int mandel_cloud_orbit(double *x, double *y, double *z);
int dynamic2d_type();
bool dynamic2d_per_image();
