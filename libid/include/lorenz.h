// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

struct affine
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

bool orbit3dlongsetup();
bool orbit3dfloatsetup();
int lorenz3dlongorbit(long *, long *, long *);
int lorenz3d1floatorbit(double *, double *, double *);
int lorenz3dfloatorbit(double *, double *, double *);
int lorenz3d3floatorbit(double *, double *, double *);
int lorenz3d4floatorbit(double *, double *, double *);
int henonfloatorbit(double *, double *, double *);
int henonlongorbit(long *, long *, long *);
int inverse_julia_orbit(double *, double *, double *);
int Minverse_julia_orbit();
int Linverse_julia_orbit();
int inverse_julia_per_image();
int rosslerfloatorbit(double *, double *, double *);
int pickoverfloatorbit(double *, double *, double *);
int gingerbreadfloatorbit(double *, double *, double *);
int rosslerlongorbit(long *, long *, long *);
int kamtorusfloatorbit(double *, double *, double *);
int kamtoruslongorbit(long *, long *, long *);
int hopalong2dfloatorbit(double *, double *, double *);
int chip2dfloatorbit(double *, double *, double *);
int quadruptwo2dfloatorbit(double *, double *, double *);
int threeply2dfloatorbit(double *, double *, double *);
int martin2dfloatorbit(double *, double *, double *);
int orbit2dfloat();
int orbit2dlong();
int funny_glasses_call(int (*)());
int ifs();
int orbit3dfloat();
int orbit3dlong();
int iconfloatorbit(double *, double *, double *);
int latoofloatorbit(double *, double *, double *);
bool setup_convert_to_screen(affine *);
int plotorbits2dsetup();
int plotorbits2dfloat();
