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

extern bool orbit3dlongsetup();
extern bool orbit3dfloatsetup();
extern int lorenz3dlongorbit(long *, long *, long *);
extern int lorenz3d1floatorbit(double *, double *, double *);
extern int lorenz3dfloatorbit(double *, double *, double *);
extern int lorenz3d3floatorbit(double *, double *, double *);
extern int lorenz3d4floatorbit(double *, double *, double *);
extern int henonfloatorbit(double *, double *, double *);
extern int henonlongorbit(long *, long *, long *);
extern int inverse_julia_orbit(double *, double *, double *);
extern int Minverse_julia_orbit();
extern int Linverse_julia_orbit();
extern int inverse_julia_per_image();
extern int rosslerfloatorbit(double *, double *, double *);
extern int pickoverfloatorbit(double *, double *, double *);
extern int gingerbreadfloatorbit(double *, double *, double *);
extern int rosslerlongorbit(long *, long *, long *);
extern int kamtorusfloatorbit(double *, double *, double *);
extern int kamtoruslongorbit(long *, long *, long *);
extern int hopalong2dfloatorbit(double *, double *, double *);
extern int chip2dfloatorbit(double *, double *, double *);
extern int quadruptwo2dfloatorbit(double *, double *, double *);
extern int threeply2dfloatorbit(double *, double *, double *);
extern int martin2dfloatorbit(double *, double *, double *);
extern int orbit2dfloat();
extern int orbit2dlong();
extern int funny_glasses_call(int (*)());
extern int ifs();
extern int orbit3dfloat();
extern int orbit3dlong();
extern int iconfloatorbit(double *, double *, double *);
extern int latoofloatorbit(double *, double *, double *);
extern bool setup_convert_to_screen(affine *);
extern int plotorbits2dsetup();
extern int plotorbits2dfloat();
