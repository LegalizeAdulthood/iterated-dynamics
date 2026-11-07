// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/VideoInfo.h>

#include <ui/comments.h>

#include <config/port.h>

#include <engine/calcfrac.h>
#include <engine/command_comments.h>
#include <engine/LogicalScreen.h>
#include <misc/version.h>
#include <ui/id_keys.h>
#include <ui/video_mode.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

using namespace id::config;
using namespace id::engine;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

struct TestComments : Test
{
    ~TestComments() override = default;

protected:
    void SetUp() override
    {
        init_comments();
        m_saved_calc_time = g_calc_time;
        m_saved_version = g_version;
        m_saved_x_dots = g_logical_screen.x_dots;
        m_saved_y_dots = g_logical_screen.y_dots;
        m_saved_video_key = g_video_entry_key;
        g_calc_time = 0;
        std::tm tm{};
        tm.tm_year = TEST_YEAR - 1900; // years since 1900
        tm.tm_mon = TEST_MONTH;
        tm.tm_mday = TEST_DAY;
        tm.tm_hour = TEST_HOUR;
        tm.tm_min = TEST_MINUTE;
        tm.tm_sec = TEST_SECOND;
        m_test_time = mktime(&tm);
    }
    void TearDown() override
    {
        init_comments();
        g_video_entry_key = m_saved_video_key;
        g_logical_screen.y_dots = m_saved_y_dots;
        g_logical_screen.x_dots = m_saved_x_dots;
        g_version = m_saved_version;
        g_calc_time = m_saved_calc_time;
    }

    static constexpr int TEST_YEAR{1984};
    static constexpr int TEST_MONTH{11};
    static constexpr int TEST_DAY{12};
    static constexpr int TEST_HOUR{20};
    static constexpr int TEST_MINUTE{45};
    static constexpr int TEST_SECOND{32};
    std::time_t m_test_time{};

private:
    long m_saved_calc_time{};
    Version m_saved_version{};
    int m_saved_x_dots{};
    int m_saved_y_dots{};
    int m_saved_video_key{};
};

TEST_F(TestComments, expandCpu)
{
    g_par_comment[0] = "; rendered on $cpu$ CPU";
    StrictMock<MockFunction<std::string()>> get_cpu_id;
    g_get_cpu_id = get_cpu_id.AsStdFunction();
    EXPECT_CALL(get_cpu_id, Call()).WillOnce(Return("Intel Core i9"));

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("; rendered on Intel Core i9 CPU", result);
}

struct CommentTextInput
{
    const char *name;
    const char *input;
    std::string expected;
};

void PrintTo(const CommentTextInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLiteralCommentInputs : public TestComments, public WithParamInterface<CommentTextInput>
{
};

TEST_P(TestLiteralCommentInputs, expandsLiteralAndEscapedText)
{
    g_par_comment[0] = GetParam().input;

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Text, TestLiteralCommentInputs,
    Values(CommentTextInput{"Empty", "", ""},
        CommentTextInput{"Underscores", "every_word_separated_with_underscore", "every word separated with underscore"},
        CommentTextInput{"EscapedUnderscore", R"(escaped\_underscore)", "escaped_underscore"},
        CommentTextInput{"EscapedDollar", R"(escaped\$_dollar)", "escaped$ dollar"},
        CommentTextInput{"EscapedBackslash", R"(escaped\\_backslash)", R"(escaped\ backslash)"}),
    [](const TestParamInfo<CommentTextInput> &info) { return info.param.name; });

TEST_F(TestComments, parseCommentsSplitsSlashDelimitedValues)
{
    parse_comments("main/second//fourth");

    EXPECT_THAT(g_par_comment, ElementsAre("main", "second", "", "fourth"));
}

TEST_F(TestComments, parseCommentsTruncatesToLegacyLimit)
{
    const std::string comment(MAX_COMMENT_LEN + 10, 'x');

    parse_comments(comment + "/tail");

    EXPECT_EQ(std::string(MAX_COMMENT_LEN - 1, 'x'), g_par_comment[0]);
    EXPECT_EQ("tail", g_par_comment[1]);
}

struct CommentCalculationTimeInput
{
    const char *name;
    long centiseconds;
    const char *expected;
};

void PrintTo(const CommentCalculationTimeInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestCommentCalculationTime : public TestComments, public WithParamInterface<CommentCalculationTimeInput>
{
};

TEST_P(TestCommentCalculationTime, expandsElapsedTime)
{
    g_calc_time = GetParam().centiseconds;
    g_par_comment[0] = "$calctime$";

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Duration, TestCommentCalculationTime,
    Values(CommentCalculationTimeInput{"Zero", 0, "  0:00:00.00"},
        CommentCalculationTimeInput{"Hundredths", 42, "  0:00:00.42"},
        CommentCalculationTimeInput{"Seconds", 42 * 100 + 42, "  0:00:42.42"},
        CommentCalculationTimeInput{"Minutes", (42 * 60 + 42) * 100 + 42, "  0:42:42.42"},
        CommentCalculationTimeInput{"Hours", ((4 * 60 + 42) * 60 + 42) * 100 + 42, "  4:42:42.42"},
        CommentCalculationTimeInput{"Days", (((4 * 24 + 4) * 60 + 42) * 60 + 42) * 100 + 42, "100:42:42.42"}),
    [](const TestParamInfo<CommentCalculationTimeInput> &info) { return info.param.name; });

class TestCommentMetadata : public TestComments, public WithParamInterface<CommentTextInput>
{
protected:
    void SetUp() override;
};

void TestCommentMetadata::SetUp()
{
    TestComments::SetUp();
    g_version = parse_legacy_version(1730);
    g_logical_screen.x_dots = 1964;
    g_logical_screen.y_dots = 1964;
    g_video_entry_key = ID_KEY_F6;
}

TEST_P(TestCommentMetadata, expandsMetadataToken)
{
    g_par_comment[0] = GetParam().input;

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Token, TestCommentMetadata,
    Values(CommentTextInput{"Version", "$version$", to_string(current_id_version())},
        CommentTextInput{"Patch", "$patch$", std::to_string(current_id_version().patch)},
        CommentTextInput{"XDots", "$xdots$", "1964"}, CommentTextInput{"YDots", "$ydots$", "1964"},
        CommentTextInput{"VideoKey", "$vidkey$", "F6"}),
    [](const TestParamInfo<CommentTextInput> &info) { return info.param.name; });

class TestCommentDateTime : public TestComments, public WithParamInterface<CommentTextInput>
{
};

TEST_P(TestCommentDateTime, expandsTokenAtTheFixedFixtureTime)
{
    g_par_comment[0] = GetParam().input;

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Token, TestCommentDateTime,
    Values(CommentTextInput{"Year", "$year$", "1984"}, CommentTextInput{"Month", "$month$", "Dec"},
        CommentTextInput{"Day", "$day$", "12"}, CommentTextInput{"Hour", "$hour$", "20"},
        CommentTextInput{"Minute", "$min$", "45"}, CommentTextInput{"Time", "$time$", "20:45:32"},
        CommentTextInput{"Date", "$date$", "Dec 12, 1984"}),
    [](const TestParamInfo<CommentTextInput> &info) { return info.param.name; });

} // namespace id::test
