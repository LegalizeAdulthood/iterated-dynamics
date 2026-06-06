// SPDX-License-Identifier: GPL-3.0-only
//
#include "X11Frame.h"

#include "misc/Driver.h"

namespace id::misc
{

namespace
{

enum
{
    DEFAULT_WINDOW_WIDTH = 640,
    DEFAULT_WINDOW_HEIGHT = 480,
};

class X11Driver : public Driver
{
public:
    X11Driver();
    ~X11Driver() override = default;

    const std::string &get_name() const override;
    const std::string &get_description() const override;
    bool init(int *argc, char **argv) override;
    bool validate_mode(const engine::VideoInfo &mode) override;
    void get_max_screen(int &width, int &height) override;
    void terminate() override;
    void pause() override;
    void resume() override;
    void schedule_alarm(int secs) override;
    void create_window() override;
    bool resize() override;
    void read_palette() override;
    void write_palette() override;
    int read_pixel(int x, int y) override;
    void write_pixel(int x, int y, int color) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void display_string(int x, int y, int fg, int bg, const char *text) override;
    void save_graphics() override;
    void restore_graphics() override;
    int get_key() override;
    int key_cursor(int row, int col) override;
    int key_pressed() override;
    int wait_key_pressed(bool timeout) override;
    void unget_key(int key) override;
    void shell() override;
    void set_video_mode(const engine::VideoInfo &mode) override;
    void put_string(int row, int col, int attr, const char *msg) override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    void set_clear() override;
    void move_cursor(int row, int col) override;
    void hide_text_cursor() override;
    void set_attr(int row, int col, int attr, int count) override;
    void scroll_up(int top, int bot) override;
    void stack_screen() override;
    void unstack_screen() override;
    void discard_screen() override;
    int init_fm() override;
    void buzzer(Buzzer kind) override;
    bool sound_on(int frequency) override;
    void sound_off() override;
    void mute() override;
    bool is_disk() const override;
    int get_char_attr() override;
    void put_char_attr(int char_attr) override;
    void delay(int ms) override;
    void set_keyboard_timeout(int ms) override;
    void flush() override;
    void debug_text(const char *text) override;
    void get_cursor_pos(int &x, int &y) const override;
    void check_memory() override;
    bool get_filename(
        const char *hdg, const char *type_desc, const char *type_wildcard, std::string &result_filename) override;

private:
    X11Frame m_frame;
    const std::string m_name;
    const std::string m_description;
};

X11Driver::X11Driver() :
    m_name("x11"),
    m_description("X11")
{
}

const std::string &X11Driver::get_name() const
{
    return m_name;
}

const std::string &X11Driver::get_description() const
{
    return m_description;
}

bool X11Driver::init(int * /*argc*/, char ** /*argv*/)
{
    return m_frame.init("Iterated Dynamics");
}

bool X11Driver::validate_mode(const engine::VideoInfo & /*mode*/)
{
    return false;
}

void X11Driver::get_max_screen(int &width, int &height)
{
    m_frame.get_max_screen(width, height);
}

void X11Driver::terminate()
{
    m_frame.terminate();
}

void X11Driver::pause()
{
    m_frame.pause();
}

void X11Driver::resume()
{
    m_frame.resume();
}

void X11Driver::schedule_alarm(int /*secs*/)
{
}

void X11Driver::create_window()
{
    m_frame.create_window(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
}

bool X11Driver::resize()
{
    return m_frame.resize(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
}

void X11Driver::read_palette()
{
}

void X11Driver::write_palette()
{
}

int X11Driver::read_pixel(int /*x*/, int /*y*/)
{
    return 0;
}

void X11Driver::write_pixel(int /*x*/, int /*y*/, int /*color*/)
{
}

void X11Driver::draw_line(int /*x1*/, int /*y1*/, int /*x2*/, int /*y2*/, int /*color*/)
{
}

void X11Driver::display_string(int /*x*/, int /*y*/, int /*fg*/, int /*bg*/, const char * /*text*/)
{
}

void X11Driver::save_graphics()
{
}

void X11Driver::restore_graphics()
{
}

int X11Driver::get_key()
{
    m_frame.pump_messages();
    return 0;
}

int X11Driver::key_cursor(int /*row*/, int /*col*/)
{
    return 0;
}

int X11Driver::key_pressed()
{
    m_frame.pump_messages();
    return 0;
}

int X11Driver::wait_key_pressed(bool /*timeout*/)
{
    m_frame.pump_messages();
    return 0;
}

void X11Driver::unget_key(int /*key*/)
{
}

void X11Driver::shell()
{
}

void X11Driver::set_video_mode(const engine::VideoInfo & /*mode*/)
{
}

void X11Driver::put_string(int /*row*/, int /*col*/, int /*attr*/, const char * /*msg*/)
{
}

bool X11Driver::is_text()
{
    return true;
}

void X11Driver::set_for_text()
{
}

void X11Driver::set_for_graphics()
{
}

void X11Driver::set_clear()
{
}

void X11Driver::move_cursor(int /*row*/, int /*col*/)
{
}

void X11Driver::hide_text_cursor()
{
}

void X11Driver::set_attr(int /*row*/, int /*col*/, int /*attr*/, int /*count*/)
{
}

void X11Driver::scroll_up(int /*top*/, int /*bot*/)
{
}

void X11Driver::stack_screen()
{
}

void X11Driver::unstack_screen()
{
}

void X11Driver::discard_screen()
{
}

int X11Driver::init_fm()
{
    return 0;
}

void X11Driver::buzzer(Buzzer /*kind*/)
{
}

bool X11Driver::sound_on(int /*frequency*/)
{
    return false;
}

void X11Driver::sound_off()
{
}

void X11Driver::mute()
{
}

bool X11Driver::is_disk() const
{
    return false;
}

int X11Driver::get_char_attr()
{
    return 0;
}

void X11Driver::put_char_attr(int /*char_attr*/)
{
}

void X11Driver::delay(int /*ms*/)
{
    m_frame.pump_messages();
}

void X11Driver::set_keyboard_timeout(int /*ms*/)
{
}

void X11Driver::flush()
{
    m_frame.pump_messages();
}

void X11Driver::debug_text(const char * /*text*/)
{
}

void X11Driver::get_cursor_pos(int &x, int &y) const
{
    x = 0;
    y = 0;
}

void X11Driver::check_memory()
{
}

bool X11Driver::get_filename(
    const char * /*hdg*/, const char * /*type_desc*/, const char * /*type_wildcard*/, std::string & /*result_filename*/)
{
    return false;
}

X11Driver s_x11_driver;

} // namespace

Driver *get_x11_driver()
{
    return &s_x11_driver;
}

} // namespace id::misc
