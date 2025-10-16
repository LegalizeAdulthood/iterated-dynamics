// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <misc/Driver.h>

#include <gmock/gmock.h>

namespace id::misc::test
{

class MockDriver : public testing::StrictMock<Driver>
{
public:
    MOCK_METHOD(const std::string &, get_name, (), (const, override));
    MOCK_METHOD(const std::string &, get_description, (), (const, override));
    MOCK_METHOD(bool, init, (int *, char **), (override));
    MOCK_METHOD(bool, validate_mode, (const ui::VideoInfo &), (override));
    MOCK_METHOD(void, get_max_screen, (int &, int &), (override));
    MOCK_METHOD(void, terminate, (), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, resume, (), (override));
    MOCK_METHOD(void, schedule_alarm, (int), (override));
    MOCK_METHOD(void, create_window, (), (override));
    MOCK_METHOD(bool, resize, (), (override));
    MOCK_METHOD(void, redraw, (), (override));
    MOCK_METHOD(void, read_palette, (), (override));
    MOCK_METHOD(void, write_palette, (), (override));
    MOCK_METHOD(int, read_pixel, (int, int), (override));
    MOCK_METHOD(void, write_pixel, (int, int, int), (override));
    MOCK_METHOD(void, read_span, (int, int, int, Byte *), (override));
    MOCK_METHOD(void, write_span, (int, int, int, Byte *), (override));
    MOCK_METHOD(void, set_line_mode, (int), (override));
    MOCK_METHOD(void, draw_line, (int, int, int, int, int), (override));
    MOCK_METHOD(void, display_string, (int, int, int, int, char const *), (override));
    MOCK_METHOD(void, save_graphics, (), (override));
    MOCK_METHOD(void, restore_graphics, (), (override));
    MOCK_METHOD(int, get_key, (), (override));
    MOCK_METHOD(int, key_cursor, (int, int), (override));
    MOCK_METHOD(int, key_pressed, (), (override));
    MOCK_METHOD(int, wait_key_pressed, (bool), (override));
    MOCK_METHOD(void, unget_key, (int), (override));
    MOCK_METHOD(void, shell, (), (override));
    MOCK_METHOD(void, set_video_mode, (const ui::VideoInfo &), (override));
    MOCK_METHOD(void, put_string, (int, int, int, char const *), (override));
    MOCK_METHOD(bool, is_text, (), (override));
    MOCK_METHOD(void, set_for_text, (), (override));
    MOCK_METHOD(void, set_for_graphics, (), (override));
    MOCK_METHOD(void, set_clear, (), (override));
    MOCK_METHOD(void, move_cursor, (int, int), (override));
    MOCK_METHOD(void, hide_text_cursor, (), (override));
    MOCK_METHOD(void, set_attr, (int, int, int, int), (override));
    MOCK_METHOD(void, scroll_up, (int, int), (override));
    MOCK_METHOD(void, stack_screen, (), (override));
    MOCK_METHOD(void, unstack_screen, (), (override));
    MOCK_METHOD(void, discard_screen, (), (override));
    MOCK_METHOD(int, init_fm, (), (override));
    MOCK_METHOD(void, buzzer, (Buzzer), (override));
    MOCK_METHOD(bool, sound_on, (int), (override));
    MOCK_METHOD(void, sound_off, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, is_disk, (), (const, override));
    MOCK_METHOD(int, get_char_attr, (), (override));
    MOCK_METHOD(void, put_char_attr, (int), (override));
    MOCK_METHOD(void, delay, (int), (override));
    MOCK_METHOD(void, set_keyboard_timeout, (int), (override));
    MOCK_METHOD(void, flush, (), (override));
    MOCK_METHOD(void, debug_text, (const char *text), (override));
    MOCK_METHOD(void, get_cursor_pos, (int &x, int &y), (const, override));
    MOCK_METHOD(void, check_memory, (), (override));
    MOCK_METHOD(bool, get_filename, (const char *, const char *, const char *, std::string &), (override));
};

} // namespace id::misc::test
