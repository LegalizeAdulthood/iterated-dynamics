#pragma once

#include "port.h"

#include <cassert>
#include <string>

struct VIDEOINFO;

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
class Driver
{
public:
    virtual ~Driver() = default;

    virtual const std::string &get_name() const = 0;                                 // name of driver
    virtual const std::string &get_description() const = 0;                          // driver description
    virtual bool init(int *argc, char **argv) = 0;                                   // init the driver
    virtual bool validate_mode(VIDEOINFO *mode) = 0;                                 // validate a id.cfg mode
    virtual void get_max_screen(int &xmax, int &ymax) = 0;                           // find max screen extents
    virtual void terminate() = 0;                                                    // shutdown the driver
    virtual void pause() = 0;                                                        // pause this driver
    virtual void resume() = 0;                                                       // resume this driver
    virtual void schedule_alarm(int secs) = 0;                                       // refresh alarm
    virtual void create_window() = 0;                                                // creates a window
    virtual bool resize() = 0;                                                       // handles window resize.
    virtual void redraw() = 0;                                                       // redraws the screen
    virtual int read_palette() = 0;                                                  // read palette into g_dac_box
    virtual int write_palette() = 0;                                                 // write g_dac_box into palette
    virtual int read_pixel(int x, int y) = 0;                                        // reads a single pixel
    virtual void write_pixel(int x, int y, int color) = 0;                           // writes a single pixel
    virtual void read_span(int y, int x, int lastx, BYTE *pixels) = 0;               // reads a span of pixel
    virtual void write_span(int y, int x, int lastx, BYTE *pixels) = 0;              // writes a span of pixels
    virtual void get_truecolor(int x, int y, int *r, int *g, int *b, int *a) = 0;    //
    virtual void put_truecolor(int x, int y, int r, int g, int b, int a) = 0;        //
    virtual void set_line_mode(int mode) = 0;                                        // set copy/xor line
    virtual void draw_line(int x1, int y1, int x2, int y2, int color) = 0;           // draw line
    virtual void display_string(int x, int y, int fg, int bg, char const *text) = 0; // draw string in graphics mode
    virtual void save_graphics() = 0;                                                // save graphics
    virtual void restore_graphics() = 0;                                             // restore graphics
    virtual int get_key() = 0;                                                       // poll or block for a key
    virtual int key_cursor(int row, int col) = 0;                                    //
    virtual int key_pressed() = 0;                                                   //
    virtual int wait_key_pressed(int timeout) = 0;                                   //
    virtual void unget_key(int key) = 0;                                             //
    virtual void shell() = 0;                                                        // invoke a command shell
    virtual void set_video_mode(VIDEOINFO *mode) = 0;                                //
    virtual void put_string(int row, int col, int attr, char const *msg) = 0;        //
    virtual bool is_text() = 0;                                                      //
    virtual void set_for_text() = 0;                                                 // set for text mode & save gfx
    virtual void set_for_graphics() = 0;                                             // restores graphics and data
    virtual void set_clear() = 0;                                                    // clears text screen
    virtual void move_cursor(int row, int col) = 0;                                  // text screen functions
    virtual void hide_text_cursor() = 0;                                             //
    virtual void set_attr(int row, int col, int attr, int count) = 0;                //
    virtual void scroll_up(int top, int bot) = 0;                                    //
    virtual void stack_screen() = 0;                                                 //
    virtual void unstack_screen() = 0;                                               //
    virtual void discard_screen() = 0;                                               //
    virtual int init_fm() = 0;                                                       // sound routines
    virtual void buzzer(buzzer_codes kind) = 0;                                      //
    virtual bool sound_on(int frequency) = 0;                                        //
    virtual void sound_off() = 0;                                                    //
    virtual void mute() = 0;                                                         //
    virtual bool diskp() = 0;                                                        // is a disk driver?
    virtual int get_char_attr() = 0;                                                 //
    virtual void put_char_attr(int char_attr) = 0;                                   //
    virtual void delay(int ms) = 0;                                                  //
    virtual void set_keyboard_timeout(int ms) = 0;                                   //
    virtual void flush() = 0;                                                        //
    virtual void debug_text(const char *text) = 0;                                   // Emit debug text (no EOL assumed)
};

/* Define the drivers to be included in the compilation:
    HAVE_CURSES_DRIVER      Curses based disk driver (no current implementation)
    HAVE_X11_DRIVER         X11 code path
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
void load_driver(Driver *drv, int *argc, char **argv);
int init_drivers(int *argc, char **argv);
void add_video_mode(Driver *drv, VIDEOINFO *mode);
void close_drivers();
Driver *driver_find_by_name(char const *name);

extern Driver *g_driver;            // current driver in use

void driver_set_video_mode(VIDEOINFO *mode);

inline bool driver_validate_mode(VIDEOINFO *mode)
{
    return g_driver->validate_mode(mode);
}
inline void driver_get_max_screen(int *xmax, int *ymax)
{
    assert(xmax != nullptr);
    assert(ymax != nullptr);
    g_driver->get_max_screen(*xmax, *ymax);
}
inline void driver_terminate()
{
    g_driver->terminate();
}
inline void driver_schedule_alarm(int secs)
{
    g_driver->schedule_alarm(secs);
}
inline void driver_create_window()
{
    g_driver->create_window();
}
inline bool driver_resize()
{
    return g_driver->resize();
}
inline void driver_redraw()
{
    g_driver->redraw();
}
inline int driver_read_palette()
{
    return g_driver->read_palette();
}
inline int driver_write_palette()
{
    return g_driver->write_palette();
}
inline int driver_read_pixel(int x, int y)
{
    return g_driver->read_pixel(x, y);
}
inline void driver_write_pixel(int x, int y, int color)
{
    g_driver->write_pixel(x, y, color);
}
inline void driver_read_span(int y, int x, int lastx, BYTE *pixels)
{
    g_driver->read_span(y, x, lastx, pixels);
}
inline void driver_write_span(int y, int x, int lastx, BYTE *pixels)
{
    g_driver->write_span(y, x, lastx, pixels);
}
inline void driver_set_line_mode(int mode)
{
    g_driver->set_line_mode(mode);
}
inline void driver_draw_line(int x1, int y1, int x2, int y2, int color)
{
    g_driver->draw_line(x1, y1, x2, y2, color);
}
inline void driver_get_truecolor(int x, int y, int *r, int *g, int *b, int *a)
{
    g_driver->get_truecolor(x, y, r, g, b, a);
}
inline void driver_put_truecolor(int x, int y, int r, int g, int b, int a)
{
    g_driver->put_truecolor(x, y, r, g, b, a);
}
inline int driver_get_key()
{
    return g_driver->get_key();
}
inline void driver_display_string(int x, int y, int fg, int bg, char const *text)
{
    g_driver->display_string(x, y, fg, bg, text);
}
inline void driver_save_graphics()
{
    g_driver->save_graphics();
}
inline void driver_restore_graphics()
{
    g_driver->restore_graphics();
}
inline int driver_key_cursor(int row, int col)
{
    return g_driver->key_cursor(row, col);
}
inline int driver_key_pressed()
{
    return g_driver->key_pressed();
}
inline int driver_wait_key_pressed(int timeout)
{
    return g_driver->wait_key_pressed(timeout);
}
inline void driver_unget_key(int key)
{
    g_driver->unget_key(key);
}
inline void driver_shell()
{
    g_driver->shell();
}
inline void driver_put_string(int row, int col, int attr, char const *msg)
{
    g_driver->put_string(row, col, attr, msg);
}
inline void driver_put_string(int row, int col, int attr, const std::string &msg)
{
    g_driver->put_string(row, col, attr, msg.c_str());
}
inline bool driver_is_text()
{
    return g_driver->is_text();
}
inline void driver_set_for_text()
{
    g_driver->set_for_text();
}
inline void driver_set_for_graphics()
{
    g_driver->set_for_graphics();
}
inline void driver_set_clear()
{
    g_driver->set_clear();
}
inline void driver_move_cursor(int row, int col)
{
    g_driver->move_cursor(row, col);
}
inline void driver_hide_text_cursor()
{
    g_driver->hide_text_cursor();
}
inline void driver_set_attr(int row, int col, int attr, int count)
{
    g_driver->set_attr(row, col, attr, count);
}
inline void driver_scroll_up(int top, int bot)
{
    g_driver->scroll_up(top, bot);
}
inline void driver_stack_screen()
{
    g_driver->stack_screen();
}
inline void driver_unstack_screen()
{
    g_driver->unstack_screen();
}
inline void driver_discard_screen()
{
    g_driver->discard_screen();
}
inline int driver_init_fm()
{
    return g_driver->init_fm();
}
inline void driver_buzzer(buzzer_codes kind)
{
    g_driver->buzzer(kind);
}
inline bool driver_sound_on(int freq)
{
    return g_driver->sound_on(freq);
}
inline void driver_sound_off()
{
    g_driver->sound_off();
}
inline void driver_mute()
{
    g_driver->mute();
}
inline bool driver_diskp()
{
    return g_driver->diskp();
}
inline int driver_get_char_attr()
{
    return g_driver->get_char_attr();
}
inline void driver_put_char_attr(int char_attr)
{
    g_driver->put_char_attr(char_attr);
}
inline void driver_delay(int ms)
{
    g_driver->delay(ms);
}
inline void driver_set_keyboard_timeout(int ms)
{
    g_driver->set_keyboard_timeout(ms);
}
inline void driver_flush()
{
    g_driver->flush();
}
inline void driver_debug_text(const char *text)
{
    g_driver->debug_text(text);
}
// guarantees EOL
inline void driver_debug_line(const char *line)
{
    std::string text{line};
    if (text.back() != '\n')
    {
        text += '\n';
    }
    g_driver->debug_text(text.c_str());
}
