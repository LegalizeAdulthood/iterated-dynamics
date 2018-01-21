#pragma once
#if !defined(STEREO_H)
#define STEREO_H

#include <string>

extern int                   g_auto_stereo_depth;
extern double                g_auto_stereo_width;
extern char                  g_calibrate;
extern bool                  g_gray_flag;
extern bool                  g_image_map;
extern std::string           g_stereo_map_filename;

extern bool do_AutoStereo();
extern int outline_stereo(BYTE *, int);

#endif
