#pragma once

struct VIDEOINFO;

extern int                   g_row_count;       // row-counter for decoder and out_line
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern int                   g_video_start_x;
extern int                   g_video_start_y;

#if defined(XFRACT)
extern bool                  g_fake_lut;
extern bool                  g_x_zoom_waiting;
#endif
