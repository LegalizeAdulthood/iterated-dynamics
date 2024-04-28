#pragma once

#include "frame.h"
#include "win_text.h"

#define WIN32_MAXSCREENS 10

class Win32BaseDriver : public Driver
{
public:
    Win32BaseDriver(const char *name, const char *description) :
        m_name(name),
        m_description(description)
    {
    }
    ~Win32BaseDriver() override = default;

    const std::string &get_name() const override
    {
        return m_name;
    }
    const std::string &get_description() const override
    {
        return m_description;
    }
    void shell() override;
    int key_pressed() override;
    void terminate() override;
    bool init(int *argc, char **argv) override;
    void unget_key(int key) override;
    int get_key() override;
    void hide_text_cursor() override;
    void set_video_mode(VIDEOINFO *mode) override;
    void put_string(int row, int col, int attr, const char *msg) override;
    void scroll_up(int top, int bot) override;
    void move_cursor(int row, int col) override;
    void set_attr(int row, int col, int attr, int count) override;
    void stack_screen() override;
    void unstack_screen() override;
    void discard_screen() override;
    int init_fm() override;
    void buzzer(buzzer_codes kind) override;
    void sound_off() override;
    bool sound_on(int frequency) override;
    void mute() override;
    bool diskp() override;
    int key_cursor(int row, int col) override;
    int wait_key_pressed(int timeout) override;
    int get_char_attr() override;
    void put_char_attr(int char_attr) override;
    void delay(int ms) override;
    void get_truecolor(int x, int y, int *r, int *g, int *b, int *a) override;
    void put_truecolor(int x, int y, int r, int g, int b, int a) override;
    void set_keyboard_timeout(int ms) override;

protected:
    std::string m_name;
    std::string m_description;

    WinText wintext;

    /* key_buffer
    *
    * When we peeked ahead and saw a keypress, stash it here for later
    * feeding to our caller.
    */
    mutable int key_buffer{};

    int screen_count{-1};
    BYTE *saved_screens[WIN32_MAXSCREENS]{};
    int saved_cursor[WIN32_MAXSCREENS+1]{};
    bool cursor_shown{};
    int cursor_row{};
    int cursor_col{};
};
