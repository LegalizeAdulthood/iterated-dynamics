// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/slideshw.h>

#include <ui/id_keys.h>

#include "MockDriver.h"

#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <io/check_write_file.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <ui/text_screen.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

static std::string test_name()
{
    const TestInfo *info = UnitTest::GetInstance()->current_test_info();
    std::string name{info->test_suite_name()};
    name += '_';
    name += info->name();
    std::replace_if(name.begin(), name.end(), [](const unsigned char ch) { return std::isalnum(ch) == 0; }, '_');
    return name;
}

class TempDir
{
public:
    TempDir() :
        m_path{fs::temp_directory_path() / ("id-slideshw-" + test_name())}
    {
        std::error_code ignored;
        fs::remove_all(m_path, ignored);
        fs::create_directories(m_path);
    }

    ~TempDir()
    {
        std::error_code ignored;
        fs::remove_all(m_path, ignored);
    }

    const fs::path &path() const
    {
        return m_path;
    }

private:
    fs::path m_path;
};

static void write_text_file(const fs::path &path, const std::string &text)
{
    fs::create_directories(path.parent_path());
    std::ofstream file{path, std::ios::binary};
    file << text;
}

static std::string read_text_file(const fs::path &path)
{
    const std::ifstream file{path, std::ios::binary};
    std::ostringstream contents;
    contents << file.rdbuf();
    std::string result{contents.str()};
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
    return result;
}

TEST(TestSlideShow, controlKeyCodeUsesCaretSyntax)
{
    EXPECT_EQ(ID_KEY_CTL_S, get_slide_show_key_code("^S"));
    EXPECT_EQ(ID_KEY_CTL_S, get_slide_show_key_code("^s"));
    EXPECT_EQ(ID_KEY_CTL_BACKSLASH, get_slide_show_key_code("^\\"));
}

TEST(TestSlideShow, invalidCaretSyntaxIsNotAKeyCode)
{
    EXPECT_EQ(-1, get_slide_show_key_code("^"));
    EXPECT_EQ(-1, get_slide_show_key_code("^SS"));
    EXPECT_EQ(-1, get_slide_show_key_code("^1"));
}

TEST(TestSlideShow, namedKeyCodeStillWorks)
{
    EXPECT_EQ(ID_KEY_ENTER, get_slide_show_key_code("ENTER"));
}

TEST(TestSlideShow, recordShowWritesNamedMnemonic)
{
    TempDir temp_dir;
    const fs::path output_path{temp_dir.path() / "key" / "recorded.key"};

    ValueSaver saved_auto_name{g_auto_name, "recorded.key"};
    ValueSaver saved_overwrite{g_overwrite_file, true};
    ValueSaver saved_save_dir{g_save_dir, temp_dir.path()};

    stop_slide_show();
    record_show(ID_KEY_CTL_RIGHT_ARROW);
    stop_slide_show();

    EXPECT_EQ("CTRL_RIGHT\n", read_text_file(output_path));
}

TEST(TestSlideShow, slideShowGotoResumesAtTarget)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "goto.key"};
    write_text_file(input_path, "GOTO target\nignored: 65\ntarget: ENTER\n");

    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_busy{g_busy, false};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};

    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));

    EXPECT_EQ(ID_KEY_ENTER, slide_show());

    stop_slide_show();
}

TEST(TestSlideShow, slideShowMessageDisplaysLineAfterSecondsArgument)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "message.key"};
    write_text_file(input_path, "MESSAGE 0 hello\n");

    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_busy{g_busy, false};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};

    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));
    EXPECT_CALL(driver, is_text()).WillOnce(Return(true));
    EXPECT_CALL(driver, move_cursor(0, _)).Times(160);
    EXPECT_CALL(driver, get_char_attr()).Times(80).WillRepeatedly(Return(7));
    EXPECT_CALL(driver, put_string(0, 0, 7, StrEq(" hello")));
    EXPECT_CALL(driver, hide_text_cursor());
    EXPECT_CALL(driver, put_char_attr(7)).Times(80);

    EXPECT_EQ(0, slide_show());

    stop_slide_show();
}

TEST(TestSlideShow, slideShowUnknownTokenDisplaysErrorMessage)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "error.key"};
    write_text_file(input_path, "BOGUS\n");

    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_busy{g_busy, false};
    ValueSaver saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver saved_first_init{g_first_init, false};
    ValueSaver saved_init_batch{g_init_batch, BatchMode::NONE};
    ValueSaver saved_text_row{g_text_row, 4};

    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));
    EXPECT_CALL(driver, stack_screen());
    EXPECT_CALL(driver, move_cursor(4, 0));
    EXPECT_CALL(driver, put_string(4, 0, 7, StrEq("Slideshow error:\nCan't understand BOGUS")));
    EXPECT_CALL(driver, put_string(6, 0, 7, StrEq("Any key to continue...")));
    EXPECT_CALL(driver, set_attr(4, 0, _, _));
    EXPECT_CALL(driver, hide_text_cursor());
    EXPECT_CALL(driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(driver, key_pressed()).WillOnce(Return(0));
    EXPECT_CALL(driver, get_key()).WillOnce(Return(ID_KEY_ENTER));
    EXPECT_CALL(driver, unstack_screen());

    EXPECT_EQ(0, slide_show());

    stop_slide_show();
}

} // namespace id::test
