#pragma once

struct VIDEOINFO;

extern int                   g_color_bright;    // brightest color in palette
extern int                   g_color_dark;      // darkest color in palette
extern int                   g_color_medium;    // nearest to medbright grey in palette
extern long                  g_l_init_x;
extern long                  g_l_init_y;
extern int                   g_row_count;       // row-counter for decoder and out_line
extern long                  g_save_base;
extern long                  g_save_ticks;
extern int                   g_text_cbase;      // g_text_col is relative to this
extern int                   g_text_col;        // current column in text mode
extern int                   g_text_rbase;      // g_text_row is relative to this
extern int                   g_text_row;        // current row in text mode
extern int                   g_vesa_x_res;
extern int                   g_vesa_y_res;
extern int                   g_video_start_x;
extern int                   g_video_start_y;

#if defined(XFRACT)
extern bool                  g_fake_lut;
extern bool                  g_x_zoom_waiting;
#endif
