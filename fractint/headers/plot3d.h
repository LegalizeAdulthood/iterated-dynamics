#if !defined(PLOT_3D_H)
#define PLOT_3D_H

extern void cdecl draw_line(int, int, int, int, int);
extern void _fastcall plot_3d_superimpose_16(int, int, int);
extern void _fastcall plot_3d_superimpose_256(int, int, int);
extern void _fastcall plot_ifs_3d_superimpose_256(int, int, int);
extern void _fastcall plot_3d_alternate(int, int, int);
extern void plot_setup();

#endif
