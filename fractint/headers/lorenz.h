#if !defined(LORENZ_H)
#define LORENZ_H

extern int orbit_3d_setup();
extern int orbit_3d_setup_fp();
extern int lorenz_3d_orbit(long *, long *, long *);
extern int lorenz_3d_orbit_fp(double *, double *, double *);
extern int lorenz_3d1_orbit_fp(double *, double *, double *);
extern int lorenz_3d3_orbit_fp(double *, double *, double *);
extern int lorenz_3d4_orbit_fp(double *, double *, double *);
extern int henon_orbit_fp(double *, double *, double *);
extern int henon_orbit(long *, long *, long *);
extern int inverse_julia_orbit(double *, double *, double *);
extern int Minverse_julia_orbit();
extern int Linverse_julia_orbit();
extern int inverse_julia_per_image();
extern int rossler_orbit_fp(double *, double *, double *);
extern int pickover_orbit_fp(double *, double *, double *);
extern int gingerbread_orbit_fp(double *, double *, double *);
extern int rossler_orbit(long *, long *, long *);
extern int kam_torus_orbit_fp(double *, double *, double *);
extern int kam_torus_orbit(long *, long *, long *);
extern int hopalong_2d_orbit_fp(double *, double *, double *);
extern int chip_2d_orbit_fp(double *, double *, double *);
extern int quadrup_two_2d_orbit_fp(double *, double *, double *);
extern int three_ply_2d_orbit_fp(double *, double *, double *);
extern int martin_2d_orbit_fp(double *, double *, double *);
extern int orbit_2d_fp();
extern int orbit_2d();
extern int funny_glasses_call(int (*)());
extern int ifs();
extern int orbit_3d_fp();
extern int orbit_3d();
extern int icon_orbit_fp(double *, double *, double *);  /* dmf */
extern int latoo_orbit_fp(double *, double *, double *);  /* hb */
extern int setup_convert_to_screen(struct affine *);
extern int plot_orbits_2d_setup();
extern int plotorbits2dfloat();

#endif
