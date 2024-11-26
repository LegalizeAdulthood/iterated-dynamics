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
    breadth_first,
    depth_first,
    random_walk,
    random_run
};

enum class Minor
{
    left_first,
    right_first
};

extern Minor                 g_inverse_julia_minor_method;
extern bool                  g_keep_screen_coords;
extern Major                 g_major_method;
extern long                  g_max_count;
extern double                g_orbit_corner_3_x;
extern double                g_orbit_corner_3_y;
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
    case Major::breadth_first:
        return "breadth";
    case Major::depth_first:
        return "depth";
    case Major::random_walk:
        return "walk";
    case Major::random_run:
        return "run";
    }
}

constexpr const char *to_string(Minor value)
{
    return value == Minor::left_first ? "left" : "right";
}

bool orbit3d_long_setup();
bool orbit3d_float_setup();
int lorenz3d_long_orbit(long *, long *, long *);
int lorenz3d1_float_orbit(double *, double *, double *);
int lorenz3d_float_orbit(double *, double *, double *);
int lorenz3d3_float_orbit(double *, double *, double *);
int lorenz3d4_float_orbit(double *, double *, double *);
int henon_float_orbit(double *, double *, double *);
int henon_long_orbit(long *, long *, long *);
int inverse_julia_orbit(double *, double *, double *);
int m_inverse_julia_orbit();
int l_inverse_julia_orbit();
int inverse_julia_per_image();
int rossler_float_orbit(double *, double *, double *);
int pickover_float_orbit(double *, double *, double *);
int ginger_bread_float_orbit(double *, double *, double *);
int rossler_long_orbit(long *, long *, long *);
int kam_torus_float_orbit(double *, double *, double *);
int kam_torus_long_orbit(long *, long *, long *);
int hopalong2d_float_orbit(double *, double *, double *);
int chip2d_float_orbit(double *, double *, double *);
int quadrup_two2d_float_orbit(double *, double *, double *);
int three_ply2d_float_orbit(double *, double *, double *);
int martin2d_float_orbit(double *, double *, double *);
int orbit2d_float();
int orbit2d_long();
int funny_glasses_call(int (*)());
int ifs();
int orbit3d_float();
int orbit3d_long();
int icon_float_orbit(double *, double *, double *);
int latoo_float_orbit(double *, double *, double *);
bool setup_convert_to_screen(Affine *);
int plot_orbits2d_setup();
int plot_orbits2d_float();
int dynam_float(double *x, double *y, double *z);
int mandel_cloud_float(double *x, double *y, double *z);
int dynam2d_float();
bool dynam2d_float_setup();
