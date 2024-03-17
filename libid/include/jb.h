#pragma once

extern float                 g_eyes_fp;
extern int                   g_julibrot_3d_mode;
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

extern bool JulibrotSetup();
extern bool JulibrotfpSetup();
extern int jb_per_pixel();
extern int jbfp_per_pixel();
extern int zline(long, long);
extern int zlinefp(double, double);
extern int Std4dFractal();
extern int Std4dfpFractal();
