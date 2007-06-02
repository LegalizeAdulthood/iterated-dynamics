#if !defined(LINE_3D_H)
#define LINE_3D_H

extern int out_line_3d(BYTE *pixels, int line_length);
extern int targa_color(int, int, int);
extern int start_disk1(char *file_name2, FILE *Source, bool overlay_file);
extern void line_3d_free();

#endif
