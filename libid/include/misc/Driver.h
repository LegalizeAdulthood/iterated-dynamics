// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <cassert>
#include <filesystem>
#include <string>

namespace id::ui
{
struct VideoInfo;
}

namespace id::misc
{

enum class Buzzer
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

    virtual const std::string &get_name() const = 0;                          // name of driver
    virtual const std::string &get_description() const = 0;                   // driver description
    virtual bool init(int *argc, char **argv) = 0;                            // init the driver
    virtual bool validate_mode(const ui::VideoInfo &mode) = 0;                // validate a id.cfg mode
    virtual void get_max_screen(int &width, int &height) = 0;                 // find max screen extents
    virtual void terminate() = 0;                                             // shutdown the driver
    virtual void pause() = 0;                                                 // pause this driver
    virtual void resume() = 0;                                                // resume this driver
    virtual void schedule_alarm(int secs) = 0;                                // refresh alarm
    virtual void create_window() = 0;                                         // creates a window
    virtual bool resize() = 0;                                                // handles window resize.
    virtual void read_palette() = 0;                                          // read palette into g_dac_box
    virtual void write_palette() = 0;                                         // write g_dac_box into palette
    virtual int read_pixel(int x, int y) = 0;                                 // reads a single pixel
    virtual void write_pixel(int x, int y, int color) = 0;                    // writes a single pixel
    virtual void draw_line(int x1, int y1, int x2, int y2, int color) = 0;    // draw line
    virtual void display_string(
        int x, int y, int fg, int bg, const char *text) = 0;                  // draw string in graphics mode
    virtual void save_graphics() = 0;                                         // save graphics
    virtual void restore_graphics() = 0;                                      // restore graphics
    virtual int get_key() = 0;                                                // poll or block for a key
    virtual int key_cursor(int row, int col) = 0;                             //
    virtual int key_pressed() = 0;                                            //
    virtual int wait_key_pressed(bool timeout) = 0;                           //
    virtual void unget_key(int key) = 0;                                      //
    virtual void shell() = 0;                                                 // invoke a command shell
    virtual void set_video_mode(const ui::VideoInfo &mode) = 0;               //
    virtual void put_string(int row, int col, int attr, const char *msg) = 0; //
    virtual bool is_text() = 0;                                               //
    virtual void set_for_text() = 0;                                          // set for text mode & save gfx
    virtual void set_for_graphics() = 0;                                      // restores graphics and data
    virtual void set_clear() = 0;                                             // clears text screen
    virtual void move_cursor(int row, int col) = 0;                           // text screen functions
    virtual void hide_text_cursor() = 0;                                      //
    virtual void set_attr(int row, int col, int attr, int count) = 0;         //
    virtual void scroll_up(int top, int bot) = 0;                             //
    virtual void stack_screen() = 0;                                          //
    virtual void unstack_screen() = 0;                                        //
    virtual void discard_screen() = 0;                                        //
    virtual int init_fm() = 0;                                                // sound routines
    virtual void buzzer(Buzzer kind) = 0;                                     //
    virtual bool sound_on(int frequency) = 0;                                 //
    virtual void sound_off() = 0;                                             //
    virtual void mute() = 0;                                                  //
    virtual bool is_disk() const = 0;                                         // is a disk driver?
    virtual int get_char_attr() = 0;                                          //
    virtual void put_char_attr(int char_attr) = 0;                            //
    virtual void delay(int ms) = 0;                                           //
    virtual void set_keyboard_timeout(int ms) = 0;                            //
    virtual void flush() = 0;                                                 //
    virtual void debug_text(const char *text) = 0;         // Emit debug text (no EOL assumed)
    virtual void get_cursor_pos(int &x, int &y) const = 0; // get cursor position within frame
    virtual void check_memory() = 0;                       // check memory for corrupted heap
    virtual bool get_filename(
        const char *hdg, const char *type_desc, const char *type_wildcard, std::string &result_filename) = 0;
};

void load_driver(Driver *drv, int *argc, char **argv);
int init_drivers(int *argc, char **argv);
void add_video_mode(Driver *drv, ui::VideoInfo *mode);
void close_drivers();
Driver *driver_find_by_name(const char *name);

extern Driver *g_driver; // current driver in use

void driver_set_video_mode(const ui::VideoInfo &mode);

inline void driver_get_max_screen(int &width, int &height)
{
    g_driver->get_max_screen(width, height);
}

inline void driver_terminate()
{
    g_driver->terminate();
}

inline void driver_schedule_alarm(const int secs)
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

inline void driver_read_palette()
{
    g_driver->read_palette();
}

inline void driver_write_palette()
{
    g_driver->write_palette();
}

inline int driver_read_pixel(const int x, const int y)
{
    return g_driver->read_pixel(x, y);
}

inline void driver_write_pixel(const int x, const int y, const int color)
{
    g_driver->write_pixel(x, y, color);
}

inline void driver_draw_line(const int x1, const int y1, const int x2, const int y2, const int color)
{
    g_driver->draw_line(x1, y1, x2, y2, color);
}

inline int driver_get_key()
{
    return g_driver->get_key();
}

inline void driver_display_string(const int x, const int y, const int fg, const int bg, const char *text)
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

inline int driver_key_cursor(const int row, const int col)
{
    return g_driver->key_cursor(row, col);
}

inline int driver_key_pressed()
{
    return g_driver->key_pressed();
}

inline int driver_wait_key_pressed(const bool timeout)
{
    return g_driver->wait_key_pressed(timeout);
}

inline void driver_unget_key(const int key)
{
    g_driver->unget_key(key);
}

inline void driver_shell()
{
    g_driver->shell();
}

inline void driver_put_string(const int row, const int col, const int attr, const char *msg)
{
    g_driver->put_string(row, col, attr, msg);
}

inline void driver_put_string(const int row, const int col, const int attr, const std::string &msg)
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

inline void driver_move_cursor(const int row, const int col)
{
    g_driver->move_cursor(row, col);
}

inline void driver_hide_text_cursor()
{
    g_driver->hide_text_cursor();
}

inline void driver_set_attr(const int row, const int col, const int attr, const int count)
{
    g_driver->set_attr(row, col, attr, count);
}

inline void driver_scroll_up(const int top, const int bot)
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

inline void driver_buzzer(const Buzzer kind)
{
    g_driver->buzzer(kind);
}

inline bool driver_sound_on(const int freq)
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

inline bool driver_is_disk()
{
    return g_driver->is_disk();
}

inline int driver_get_char_attr()
{
    return g_driver->get_char_attr();
}

inline void driver_put_char_attr(const int char_attr)
{
    g_driver->put_char_attr(char_attr);
}

inline void driver_delay(const int ms)
{
    g_driver->delay(ms);
}

inline void driver_set_keyboard_timeout(const int ms)
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

inline void driver_debug_text(const std::string &text)
{
    driver_debug_text(text.c_str());
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

inline void driver_debug_line(const std::string &line)
{
    driver_debug_line(line.c_str());
}

inline void driver_get_cursor_pos(int &x, int &y)
{
    g_driver->get_cursor_pos(x, y);
}

inline void driver_check_memory()
{
    g_driver->check_memory();
}

inline bool driver_get_filename(
    const char *hdg, const char *type_desc, const char *type_wildcard, std::string &result_filename)
{
    return g_driver->get_filename(hdg, type_desc, type_wildcard, result_filename);
}

inline bool driver_get_filename(
    const char *hdg, const char *type_desc, const char *type_wildcard, std::filesystem::path &result_path)
{
    std::string result_filename = result_path.string();
    const bool result = g_driver->get_filename(hdg, type_desc, type_wildcard, result_filename);
    result_path = result_filename;
    return result;
}

} // namespace id::misc
