#include <load_config.h>
#include <drivers.h>

#include "test_config_data.h"

#include <video_mode.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

class MockDriver : public StrictMock<Driver>
{
public:
    MOCK_METHOD(const std::string &, get_name, (), (const, override));
    MOCK_METHOD(const std::string &, get_description, (), (const, override));
    MOCK_METHOD(bool, init, (int *, char **), (override));
    MOCK_METHOD(bool, validate_mode, (VIDEOINFO *), (override));
    MOCK_METHOD(void, get_max_screen, (int &, int &), (override));
    MOCK_METHOD(void, terminate, (), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, resume, (), (override));
    MOCK_METHOD(void, schedule_alarm, (int), (override));
    MOCK_METHOD(void, create_window, (), (override));
    MOCK_METHOD(bool, resize, (), (override));
    MOCK_METHOD(void, redraw, (), (override));
    MOCK_METHOD(int, read_palette, (), (override));
    MOCK_METHOD(int, write_palette, (), (override));
    MOCK_METHOD(int, read_pixel, (int, int), (override));
    MOCK_METHOD(void, write_pixel, (int, int, int), (override));
    MOCK_METHOD(void, read_span, (int, int, int, BYTE *), (override));
    MOCK_METHOD(void, write_span, (int, int, int, BYTE *), (override));
    MOCK_METHOD(void, get_truecolor, (int, int, int *, int *, int *, int *), (override));
    MOCK_METHOD(void, put_truecolor, (int, int, int, int, int, int), (override));
    MOCK_METHOD(void, set_line_mode, (int), (override));
    MOCK_METHOD(void, draw_line, (int, int, int, int, int), (override));
    MOCK_METHOD(void, display_string, (int, int, int, int, char const *), (override));
    MOCK_METHOD(void, save_graphics, (), (override));
    MOCK_METHOD(void, restore_graphics, (), (override));
    MOCK_METHOD(int, get_key, (), (override));
    MOCK_METHOD(int, key_cursor, (int, int), (override));
    MOCK_METHOD(int, key_pressed, (), (override));
    MOCK_METHOD(int, wait_key_pressed, (int), (override));
    MOCK_METHOD(void, unget_key, (int), (override));
    MOCK_METHOD(void, shell, (), (override));
    MOCK_METHOD(void, set_video_mode, (VIDEOINFO *), (override));
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
    MOCK_METHOD(void, buzzer, (buzzer_codes), (override));
    MOCK_METHOD(bool, sound_on, (int), (override));
    MOCK_METHOD(void, sound_off, (), (override));
    MOCK_METHOD(void, mute, (), (override));
    MOCK_METHOD(bool, diskp, (), (override));
    MOCK_METHOD(int, get_char_attr, (), (override));
    MOCK_METHOD(void, put_char_attr, (int), (override));
    MOCK_METHOD(void, delay, (int), (override));
    MOCK_METHOD(void, set_keyboard_timeout, (int), (override));
    MOCK_METHOD(void, flush, (), (override));
};

TEST(TestDriver, loadClose)
{
    MockDriver gdi;
    EXPECT_CALL(gdi, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, terminate());

    load_driver(&gdi, nullptr, nullptr);

    EXPECT_EQ(&gdi, g_driver);

    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

TEST(TestLoadConfig, gdiDisk)
{
    MockDriver gdi;
    const std::string gdi_name{"gdi"};
    MockDriver disk;
    const std::string disk_name{"disk"};
    ExpectationSet init_gdi = EXPECT_CALL(gdi, init(nullptr, nullptr)).WillOnce(Return(true));
    ExpectationSet init_disk = EXPECT_CALL(disk, init(nullptr, nullptr)).WillOnce(Return(true));
    EXPECT_CALL(gdi, get_name()).WillRepeatedly(ReturnRef(gdi_name));
    EXPECT_CALL(gdi, validate_mode(NotNull())).After(init_gdi).WillRepeatedly(Return(true));
    EXPECT_CALL(disk, get_name()).WillRepeatedly(ReturnRef(disk_name));
    EXPECT_CALL(disk, validate_mode(NotNull())).After(init_disk).WillRepeatedly(Return(true));
    EXPECT_CALL(gdi, terminate());
    EXPECT_CALL(disk, terminate());
    load_driver(&gdi, nullptr, nullptr);
    load_driver(&disk, nullptr, nullptr);

    load_config(ID_TEST_CONFIG_FILE);

    ASSERT_EQ(2, g_video_table_len);
    const VIDEOINFO &gdi_mode{g_video_table[0]};
    EXPECT_STREQ(ID_TEST_GDI_COMMENT, gdi_mode.comment);
    EXPECT_EQ(ID_TEST_GDI_FN_KEY, gdi_mode.keynum);
    EXPECT_EQ(ID_TEST_GDI_WIDTH, gdi_mode.xdots);
    EXPECT_EQ(ID_TEST_GDI_HEIGHT, gdi_mode.ydots);
    EXPECT_EQ(ID_TEST_GDI_COLORS, gdi_mode.colors);
    EXPECT_EQ(&gdi, gdi_mode.driver);
    const VIDEOINFO &disk_mode{g_video_table[1]};
    EXPECT_STREQ(ID_TEST_DISK_COMMENT, disk_mode.comment);
    EXPECT_EQ(ID_TEST_DISK_FN_KEY, disk_mode.keynum);
    EXPECT_EQ(ID_TEST_DISK_WIDTH, disk_mode.xdots);
    EXPECT_EQ(ID_TEST_DISK_HEIGHT, disk_mode.ydots);
    EXPECT_EQ(ID_TEST_DISK_COLORS, disk_mode.colors);
    EXPECT_EQ(&disk, disk_mode.driver);

    close_drivers();
}
