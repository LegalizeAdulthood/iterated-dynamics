// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
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
#include <ui/thinking.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <ostream>
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

struct SlideShowKeyInput
{
    const char *name;
    const char *token;
    int expected;
};

void PrintTo(const SlideShowKeyInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSlideShowKeyTokens : public TestWithParam<SlideShowKeyInput>
{
};

TEST_P(TestSlideShowKeyTokens, decodesKeyToken)
{
    EXPECT_EQ(GetParam().expected, get_slide_show_key_code(GetParam().token));
}

INSTANTIATE_TEST_SUITE_P(Token, TestSlideShowKeyTokens,
    Values(SlideShowKeyInput{"UpperControlS", "^S", ID_KEY_CTL_S},
        SlideShowKeyInput{"LowerControlS", "^s", ID_KEY_CTL_S},
        SlideShowKeyInput{"ControlBackslash", "^\\", ID_KEY_CTL_BACKSLASH}, SlideShowKeyInput{"BareCaret", "^", -1},
        SlideShowKeyInput{"LongCaret", "^SS", -1}, SlideShowKeyInput{"NumericCaret", "^1", -1},
        SlideShowKeyInput{"Named", "ENTER", ID_KEY_ENTER}),
    [](const TestParamInfo<SlideShowKeyInput> &info) { return info.param.name; });

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

TEST(TestSlideShow, thinkingWaitDeliversKeyOnlyWhileThinkingIsActive)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "thinking.key"};
    write_text_file(input_path, "THINKWAIT x\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));
    EXPECT_CALL(driver, stack_screen());
    EXPECT_CALL(driver, set_clear());
    EXPECT_CALL(driver, put_string(_, _, _, _)).Times(2);
    EXPECT_CALL(driver, unstack_screen());

    EXPECT_EQ(0, slide_show());
    EXPECT_EQ(0, slide_show());
    thinking("preparation");
    EXPECT_EQ('x', slide_show());

    thinking_end();
    stop_slide_show();
}

TEST(TestSlideShow, thinkingWaitAcceptsNamedKeyWhenThinkingIsAlreadyActive)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "thinking.key"};
    write_text_file(input_path, "THINKWAIT ENTER\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));
    EXPECT_CALL(driver, stack_screen());
    EXPECT_CALL(driver, set_clear());
    EXPECT_CALL(driver, put_string(_, _, _, _)).Times(2);
    EXPECT_CALL(driver, unstack_screen());

    thinking("preparation");
    EXPECT_EQ(ID_KEY_ENTER, slide_show());

    thinking_end();
    stop_slide_show();
}

struct SlideShowRowWaitInput
{
    const char *name;
    int row;
    CalcStatus status;
};

void PrintTo(const SlideShowRowWaitInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSlideShowRowWait : public TestWithParam<SlideShowRowWaitInput>
{
};

TEST_P(TestSlideShowRowWait, waitsUntilCalculationReachesTarget)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "row.key"};
    write_text_file(input_path, "ROWWAIT 2 x\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_coordinate{g_current_row, GetParam().row};
    ValueSaver saved_status{g_calc_status, GetParam().status};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));

    EXPECT_EQ(0, slide_show());
    g_current_row = 2;
    g_calc_status = CalcStatus::IN_PROGRESS;
    EXPECT_EQ('x', slide_show());

    stop_slide_show();
}

INSTANTIATE_TEST_SUITE_P(BlockedState, TestSlideShowRowWait,
    Values(SlideShowRowWaitInput{"BeforeStart", 0, CalcStatus::IN_PROGRESS},
        SlideShowRowWaitInput{"BeforeTarget", 1, CalcStatus::IN_PROGRESS},
        SlideShowRowWaitInput{"NotCalculating", 2, CalcStatus::RESUMABLE}),
    [](const TestParamInfo<SlideShowRowWaitInput> &info) { return info.param.name; });

TEST(TestSlideShow, rowWaitAcceptsANamedKeyAtTheCurrentRow)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "row.key"};
    write_text_file(input_path, "ROWWAIT 1 ENTER\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_row{g_current_row, 1};
    ValueSaver saved_status{g_calc_status, CalcStatus::IN_PROGRESS};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));

    EXPECT_EQ(ID_KEY_ENTER, slide_show());

    stop_slide_show();
}

struct SlideShowFastWaitInput
{
    const char *name;
    const char *command;
};

void PrintTo(const SlideShowFastWaitInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSlideShowFastWait : public TestWithParam<SlideShowFastWaitInput>
{
};

TEST_P(TestSlideShowFastWait, preservesFastTypingAfterWait)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "fast.key"};
    write_text_file(input_path, std::string{"FAST\n"} + GetParam().command + "\nENTER\nENTER\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver saved_busy{g_busy, false};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_)).Times(AnyNumber());

    EXPECT_EQ(0, slide_show());
    EXPECT_EQ(0, slide_show());
    EXPECT_EQ(ID_KEY_ENTER, slide_show());
    EXPECT_EQ(ID_KEY_ENTER, slide_show());

    stop_slide_show();
}

INSTANTIATE_TEST_SUITE_P(Command, TestSlideShowFastWait,
    Values(SlideShowFastWaitInput{"TimedWait", "WAIT 0"}, SlideShowFastWaitInput{"CalculationWait", "CALCWAIT"}),
    [](const TestParamInfo<SlideShowFastWaitInput> &info) { return info.param.name; });

struct SlideShowIterationWaitInput
{
    const char *name;
    long iteration;
    CalcStatus status;
};

void PrintTo(const SlideShowIterationWaitInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSlideShowIterationWait : public TestWithParam<SlideShowIterationWaitInput>
{
};

TEST_P(TestSlideShowIterationWait, waitsUntilCalculationReachesTarget)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "iteration.key"};
    write_text_file(input_path, "ITERWAIT 512 x\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_coordinate{g_color_iter, GetParam().iteration};
    ValueSaver saved_status{g_calc_status, GetParam().status};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));

    EXPECT_EQ(0, slide_show());
    g_color_iter = 512;
    g_calc_status = CalcStatus::IN_PROGRESS;
    EXPECT_EQ('x', slide_show());

    stop_slide_show();
}

INSTANTIATE_TEST_SUITE_P(BlockedState, TestSlideShowIterationWait,
    Values(SlideShowIterationWaitInput{"BeforeStart", 0, CalcStatus::IN_PROGRESS},
        SlideShowIterationWaitInput{"BeforeTarget", 511, CalcStatus::IN_PROGRESS},
        SlideShowIterationWaitInput{"NotCalculating", 512, CalcStatus::COMPLETED}),
    [](const TestParamInfo<SlideShowIterationWaitInput> &info) { return info.param.name; });

TEST(TestSlideShow, iterationWaitAcceptsANamedKeyAtTheCurrentIteration)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "iteration.key"};
    write_text_file(input_path, "ITERWAIT 512 ENTER\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_iteration{g_color_iter, 512L};
    ValueSaver saved_status{g_calc_status, CalcStatus::IN_PROGRESS};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_));

    EXPECT_EQ(ID_KEY_ENTER, slide_show());

    stop_slide_show();
}

TEST(TestSlideShow, stoppingSlideShowDiscardsThePendingIterationWait)
{
    TempDir temp_dir;
    const fs::path input_path{temp_dir.path() / "iteration.key"};
    write_text_file(input_path, "ITERWAIT 512 x\n");
    MockDriver driver;
    ValueSaver saved_driver{g_driver, static_cast<Driver *>(&driver)};
    ValueSaver saved_auto_name{g_auto_name, input_path};
    ValueSaver saved_iteration{g_color_iter, 0L};
    ValueSaver saved_status{g_calc_status, CalcStatus::IN_PROGRESS};
    stop_slide_show();
    ValueSaver saved_slides{g_slides, SlidesMode::PLAY};
    EXPECT_CALL(driver, set_keyboard_timeout(_)).Times(2);

    EXPECT_EQ(0, slide_show());
    stop_slide_show();
    write_text_file(input_path, "ENTER\n");
    g_slides = SlidesMode::PLAY;
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
