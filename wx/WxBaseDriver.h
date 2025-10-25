// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/driver_types.h>

#include "gui/Screen.h"
#include "misc/Driver.h"

#include <string>
#include <vector>

namespace id::misc
{

class WxBaseDriver : public Driver
{
public:
    explicit WxBaseDriver(const char *name) :
        m_name(name),
        m_description("wxWidgets")
    {
    }

    WxBaseDriver(const WxBaseDriver &) = delete;
    WxBaseDriver(WxBaseDriver &&) = delete;
    ~WxBaseDriver() override = default;
    WxBaseDriver &operator=(const WxBaseDriver &) = delete;
    WxBaseDriver &operator=(WxBaseDriver &&) = delete;

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
    void set_video_mode(const engine::VideoInfo &mode) override;
    void put_string(int row, int col, int attr, const char *msg) override;
    void scroll_up(int top, int bot) override;
    void move_cursor(int row, int col) override;
    void set_attr(int row, int col, int attr, int count) override;
    void stack_screen() override;
    void unstack_screen() override;
    void discard_screen() override;
    int init_fm() override;
    void buzzer(Buzzer kind) override;
    void sound_off() override;
    bool sound_on(int frequency) override;
    void mute() override;
    bool is_disk() const override;
    int key_cursor(int row, int col) override;
    int wait_key_pressed(bool timeout) override;
    int get_char_attr() override;
    void put_char_attr(int char_attr) override;
    void delay(int ms) override;
    void set_keyboard_timeout(int ms) override;
    void debug_text(const char *text) override;
    void get_cursor_pos(int &x, int &y) const override;
    bool validate_mode(const engine::VideoInfo &mode) override;
    void get_max_screen(int &width, int &height) override;
    void pause() override;
    void resume() override;
    void schedule_alarm(int secs) override;
    void create_window() override;
    void save_graphics() override;
    void restore_graphics() override;
    bool is_text() override;
    void set_for_text() override;
    void set_for_graphics() override;
    void set_clear() override;
    void flush() override;
    void check_memory() override;
    bool get_filename(const char *hdg, const char *type_desc, const char *type_wildcard,
        std::string &result_filename) override;

protected:
    struct TextLocation
    {
        int row{};
        int col{};
    };
    std::string m_name;
    std::string m_description;

    /* key_buffer
    *
    * When we peeked ahead and saw a keypress, stash it here for later
    * feeding to our caller.
    */
    mutable int m_key_buffer{};

    std::vector<gui::Screen> m_saved_screens;
    std::vector<TextLocation> m_saved_cursor;
    TextLocation m_cursor;
};

} // namespace id::misc
