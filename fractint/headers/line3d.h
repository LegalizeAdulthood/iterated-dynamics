#if !defined(LINE_3D_H)
#define LINE_3D_H

#include <string>
#include "3d.h"

extern std::string g_light_name;
extern VECTOR g_view;

extern const int g_bad_value;
extern int out_line_3d(BYTE const *pixels, int line_length);
extern int targa_color(int, int, int);
extern int start_disk_targa(const std::string &file_name2, FILE *Source, bool overlay_file);

#endif
