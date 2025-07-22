// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>

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


namespace id::fractals
{

/*
 * This is the routine called to perform a time-discrete dynamical
 * system image.
 * The starting positions are taken by stepping across the image in steps
 * of parameter1 pixels.  maxit differential equation steps are taken, with
 * a step size of parameter2.
 */
class Dynamic2D
{
public:
    Dynamic2D();
    Dynamic2D(const Dynamic2D &rhs) = delete;
    Dynamic2D(Dynamic2D &&rhs) = delete;
    ~Dynamic2D();
    Dynamic2D &operator=(const Dynamic2D &rhs) = delete;
    Dynamic2D &operator=(Dynamic2D &&rhs) = delete;

    void resume();
    void suspend();
    bool done() const;
    void iterate();

private:
    std::FILE *m_fp{};
    Affine m_cvt;
    double m_x{};
    double m_y{};
    double m_z{};
    double *m_p0{&m_x};
    double *m_p1{&m_y};
    const double *m_sound_var{};
    long m_count{-1};
    int m_color{};
    int m_old_row{-1};
    int m_old_col{-1};
    int m_x_step{-1};
    int m_y_step{0}; // The starting position step number
    // Our pixel position on the screen
    double m_x_pixel{};
    double m_y_pixel{};
    bool m_keep_going{};
};

} // namespace id::fractals

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
bool dynamic2d_per_image();
