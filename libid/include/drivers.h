#pragma once

#include <string>

enum class buzzer_codes
{
    COMPLETE = 0,
    INTERRUPT = 1,
    PROBLEM = 2
};

/*------------------------------------------------------------
 * Driver Methods:
 *
 * init
 * Initialize the driver and return non-zero on success.
 *
 * terminate
 * schedule_alarm
 *
 * window
 * Do whatever is necessary to open up a window after the screen size
 * has been set.  In a windowed environment, the window may provide
 * scroll bars to pan a cropped view across the screen.
 *
 * resize
 * redraw
 * read_palette, write_palette
 * read_pixel, write_pixel
 * read_span, write_span
 * set_line_mode
 * draw_line
 * get_key
 * key_cursor
 * key_pressed
 * wait_key_pressed
 * unget_key
 * shell
 * set_video_mode
 * put_string
 * set_for_text, set_for_graphics, set_clear
 * move_cursor
 * hide_text_cursor
 * set_attr
 * scroll_up
 * stack_screen, unstack_screen, discard_screen
 * get_char_attr, put_char_attr
 */
struct Driver
{
    char const *name;                       // name of driver
    char const *description;                // driver description
    bool (*init)(Driver *drv, int *argc, char **argv); // init the driver
    bool (*validate_mode)(Driver *drv, VIDEOINFO *mode); // validate a id.cfg mode
    void (*get_max_screen)(Driver *drv, int *xmax, int *ymax); // find max screen extents
    void (*terminate)(Driver *drv);         // shutdown the driver
    void (*pause)(Driver *drv);             // pause this driver
    void (*resume)(Driver *drv);            // resume this driver
    void (*schedule_alarm)(Driver *drv, int secs); // refresh alarm
    void (*window)(Driver *drv);            // creates a window
    bool (*resize)(Driver *drv);            // handles window resize.
    void (*redraw)(Driver *drv);            // redraws the screen
    int (*read_palette)(Driver *drv);       // read palette into g_dac_box
    int (*write_palette)(Driver *drv);      // write g_dac_box into palette
    int (*read_pixel)(Driver *drv, int x, int y); // reads a single pixel
    void (*write_pixel)(Driver *drv, int x, int y, int color); // writes a single pixel
    void (*read_span)(Driver *drv, int y, int x, int lastx, BYTE *pixels); // reads a span of pixel
    void (*write_span)(Driver *drv, int y, int x, int lastx, BYTE *pixels); // writes a span of pixels
    void (*get_truecolor)(Driver *drv, int x, int y, int *r, int *g, int *b, int *a);
    void (*put_truecolor)(Driver *drv, int x, int y, int r, int g, int b, int a);
    void (*set_line_mode)(Driver *drv, int mode); // set copy/xor line
    void (*draw_line)(Driver *drv, int x1, int y1, int x2, int y2, int color); // draw line
    void (*display_string)(Driver *drv, int x, int y, int fg, int bg, char const *text); // draw string in graphics mode
    void (*save_graphics)(Driver *drv);     // save graphics
    void (*restore_graphics)(Driver *drv);  // restore graphics
    int (*get_key)(Driver *drv);            // poll or block for a key
    int (*key_cursor)(Driver *drv, int row, int col);
    int (*key_pressed)(Driver *drv);
    int (*wait_key_pressed)(Driver *drv, int timeout);
    void (*unget_key)(Driver *drv, int key);
    void (*shell)(Driver *drv);             // invoke a command shell
    void (*set_video_mode)(Driver *drv, VIDEOINFO *mode);
    void (*put_string)(Driver *drv, int row, int col, int attr, char const *msg);
    void (*set_for_text)(Driver *drv);      // set for text mode & save gfx
    void (*set_for_graphics)(Driver *drv);  // restores graphics and data
    void (*set_clear)(Driver *drv);         // clears text screen
    // text screen functions
    void (*move_cursor)(Driver *drv, int row, int col);
    void (*hide_text_cursor)(Driver *drv);
    void (*set_attr)(Driver *drv, int row, int col, int attr, int count);
    void (*scroll_up)(Driver *drv, int top, int bot);
    void (*stack_screen)(Driver *drv);
    void (*unstack_screen)(Driver *drv);
    void (*discard_screen)(Driver *drv);
    // sound routines
    int (*init_fm)(Driver *drv);
    void (*buzzer)(Driver *drv, buzzer_codes kind);
    bool (*sound_on)(Driver *drv, int frequency);
    void (*sound_off)(Driver *drv);
    void (*mute)(Driver *drv);
    bool (*diskp)(Driver *drv);
    int (*get_char_attr)(Driver *drv);
    void (*put_char_attr)(Driver *drv, int char_attr);
    void (*delay)(Driver *drv, int ms);
    void (*set_keyboard_timeout)(Driver *drv, int ms);
    void (*flush)(Driver *drv);
};

#define STD_DRIVER_STRUCT(name_, desc_) \
  { \
    #name_, desc_, \
    name_##_init, \
    name_##_validate_mode, \
    name_##_get_max_screen, \
    name_##_terminate, \
    name_##_pause, \
    name_##_resume, \
    name_##_schedule_alarm, \
    name_##_window, \
    name_##_resize, \
    name_##_redraw, \
    name_##_read_palette, \
    name_##_write_palette, \
    name_##_read_pixel, \
    name_##_write_pixel, \
    name_##_read_span, \
    name_##_write_span, \
    name_##_get_truecolor, \
    name_##_put_truecolor, \
    name_##_set_line_mode, \
    name_##_draw_line, \
    name_##_display_string, \
    name_##_save_graphics, \
    name_##_restore_graphics, \
    name_##_get_key, \
    name_##_key_cursor, \
    name_##_key_pressed, \
    name_##_wait_key_pressed, \
    name_##_unget_key, \
    name_##_shell, \
    name_##_set_video_mode, \
    name_##_put_string, \
    name_##_set_for_text, \
    name_##_set_for_graphics, \
    name_##_set_clear, \
    name_##_move_cursor, \
    name_##_hide_text_cursor, \
    name_##_set_attr, \
    name_##_scroll_up, \
    name_##_stack_screen, \
    name_##_unstack_screen, \
    name_##_discard_screen, \
    name_##_init_fm, \
    name_##_buzzer, \
    name_##_sound_on, \
    name_##_sound_off, \
    name_##_mute, \
    name_##_diskp, \
    name_##_get_char_attr, \
    name_##_put_char_attr, \
    name_##_delay, \
    name_##_set_keyboard_timeout, \
    name_##_flush \
  }
/* Define the drivers to be included in the compilation:
    HAVE_CURSES_DRIVER      Curses based disk driver
    HAVE_X11_DRIVER         XFractint code path
    HAVE_GDI_DRIVER         Win32 GDI driver
    HAVE_WIN32_DISK_DRIVER  Win32 disk driver
*/
#if defined(XFRACT)
#define HAVE_X11_DRIVER         1
#define HAVE_GDI_DRIVER         0
#define HAVE_WIN32_DISK_DRIVER  0
#endif
#if defined(_WIN32)
#define HAVE_X11_DRIVER         0
#define HAVE_GDI_DRIVER         1
#define HAVE_WIN32_DISK_DRIVER  1
#endif
int init_drivers(int *argc, char **argv);
void add_video_mode(Driver *drv, VIDEOINFO *mode);
void close_drivers();
Driver *driver_find_by_name(char const *name);

extern Driver *g_driver;            // current driver in use

void driver_set_video_mode(VIDEOINFO *mode);

inline bool driver_validate_mode(VIDEOINFO *mode)
{
    return (*g_driver->validate_mode)(g_driver, mode);
}
inline void driver_get_max_screen(int *xmax, int *ymax)
{
    (*g_driver->get_max_screen)(g_driver, xmax, ymax);
}
inline void driver_terminate()
{
    (*g_driver->terminate)(g_driver);
}
inline void driver_schedule_alarm(int secs)
{
    (*g_driver->schedule_alarm)(g_driver, secs);
}
inline void driver_window()
{
    (*g_driver->window)(g_driver);
}
inline bool driver_resize()
{
    return (*g_driver->resize)(g_driver);
}
inline void driver_redraw()
{
    (*g_driver->redraw)(g_driver);
}
inline int driver_read_palette()
{
    return (*g_driver->read_palette)(g_driver);
}
inline int driver_write_palette()
{
    return (*g_driver->write_palette)(g_driver);
}
inline int driver_read_pixel(int x, int y)
{
    return (*g_driver->read_pixel)(g_driver, x, y);
}
inline void driver_write_pixel(int x, int y, int color)
{
    (*g_driver->write_pixel)(g_driver, x, y, color);
}
inline void driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
    (*g_driver->read_span)(g_driver, y, x, lastx, pixels);
}
inline void driver_write_span(int y, int x, int lastx, BYTE *pixels)
{
    (*g_driver->write_span)(g_driver, y, x, lastx, pixels);
}
inline void driver_set_line_mode(int mode)
{
    (*g_driver->set_line_mode)(g_driver, mode);
}
inline void driver_draw_line(int x1, int y1, int x2, int y2, int color)
{
    (*g_driver->draw_line)(g_driver, x1, y1, x2, y2, color);
}
inline void driver_get_truecolor(int x, int y, int *r, int *g, int *b, int *a)
{
    (*g_driver->get_truecolor)(g_driver, x, y, r, g, b, a);
}
inline void driver_put_truecolor(int x, int y, int r, int g, int b, int a)
{
    (*g_driver->put_truecolor)(g_driver, x, y, r, g, b, a);
}
inline int driver_get_key()
{
    return (*g_driver->get_key)(g_driver);
}
inline void driver_display_string(int x, int y, int fg, int bg, char const *text)
{
    (*g_driver->display_string)(g_driver, x, y, fg, bg, text);
}
inline void driver_save_graphics()
{
    (*g_driver->save_graphics)(g_driver);
}
inline void driver_restore_graphics()
{
    (*g_driver->restore_graphics)(g_driver);
}
inline int driver_key_cursor(int row, int col)
{
    return (*g_driver->key_cursor)(g_driver, row, col);
}
inline int driver_key_pressed()
{
    return (*g_driver->key_pressed)(g_driver);
}
inline int driver_wait_key_pressed(int timeout)
{
    return (*g_driver->wait_key_pressed)(g_driver, timeout);
}
inline void driver_unget_key(int key)
{
    (*g_driver->unget_key)(g_driver, key);
}
inline void driver_shell()
{
    (*g_driver->shell)(g_driver);
}
inline void driver_put_string(int row, int col, int attr, char const *msg)
{
    (*g_driver->put_string)(g_driver, row, col, attr, msg);
}
inline void driver_put_string(int row, int col, int attr, const std::string &msg)
{
    (*g_driver->put_string)(g_driver, row, col, attr, msg.c_str());
}
inline void driver_set_for_text()
{
    (*g_driver->set_for_text)(g_driver);
}
inline void driver_set_for_graphics()
{
    (*g_driver->set_for_graphics)(g_driver);
}
inline void driver_set_clear()
{
    (*g_driver->set_clear)(g_driver);
}
inline void driver_move_cursor(int row, int col)
{
    (*g_driver->move_cursor)(g_driver, row, col);
}
inline void driver_hide_text_cursor()
{
    (*g_driver->hide_text_cursor)(g_driver);
}
inline void driver_set_attr(int row, int col, int attr, int count)
{
    (*g_driver->set_attr)(g_driver, row, col, attr, count);
}
inline void driver_scroll_up(int top, int bot)
{
    (*g_driver->scroll_up)(g_driver, top, bot);
}
inline void driver_stack_screen()
{
    (*g_driver->stack_screen)(g_driver);
}
inline void driver_unstack_screen()
{
    (*g_driver->unstack_screen)(g_driver);
}
inline void driver_discard_screen()
{
    (*g_driver->discard_screen)(g_driver);
}
inline int driver_init_fm()
{
    return (*g_driver->init_fm)(g_driver);
}
inline void driver_buzzer(buzzer_codes kind)
{
    (*g_driver->buzzer)(g_driver, kind);
}
inline bool driver_sound_on(int freq)
{
    return (*g_driver->sound_on)(g_driver, freq);
}
inline void driver_sound_off()
{
    (*g_driver->sound_off)(g_driver);
}
inline void driver_mute()
{
    (*g_driver->mute)(g_driver);
}
inline bool driver_diskp()
{
    return (*g_driver->diskp)(g_driver);
}
inline int driver_get_char_attr()
{
    return (*g_driver->get_char_attr)(g_driver);
}
inline void driver_put_char_attr(int char_attr)
{
    (*g_driver->put_char_attr)(g_driver, char_attr);
}
inline void driver_delay(int ms)
{
    (*g_driver->delay)(g_driver, ms);
}
inline void driver_set_keyboard_timeout(int ms)
{
    (*g_driver->set_keyboard_timeout)(g_driver, ms);
}
inline void driver_flush()
{
    (*g_driver->flush)(g_driver);
}
