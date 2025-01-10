// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/comments.h>

#include <engine/id_data.h>
#include <ui/id_keys.h>
#include <ui/video_mode.h>
#include <version.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>
#include <ctime>
#include <iomanip>
#include <sstream>

using namespace testing;

namespace
{

struct TestComments : Test
{
    ~TestComments() override = default;

protected:
    void SetUp() override
    {
        init_comments();
        m_saved_calc_time = g_calc_time;
        m_saved_version = g_release;
        m_saved_x_dots = g_logical_screen_x_dots;
        m_saved_y_dots = g_logical_screen_y_dots;
        m_saved_video_key = g_video_entry.key;
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
        g_video_entry.key = m_saved_video_key;
        g_logical_screen_y_dots = m_saved_y_dots;
        g_logical_screen_x_dots = m_saved_x_dots;
        g_release = m_saved_version;
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
    int m_saved_version{};
    int m_saved_x_dots{};
    int m_saved_y_dots{};
    int m_saved_video_key{};
};

} // namespace

TEST_F(TestComments, expandCpu)
{
    std::strcpy(g_par_comment[0], "; rendered on $cpu$ CPU");
    StrictMock<MockFunction<std::string()>> get_cpu_id;
    g_get_cpu_id = get_cpu_id.AsStdFunction();
    EXPECT_CALL(get_cpu_id, Call()).WillOnce(Return("Intel Core i9"));

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("; rendered on Intel Core i9 CPU", result);
}

TEST_F(TestComments, emptyString)
{
    const std::string &result{expand_command_comment(0)};

    EXPECT_TRUE(result.empty());
}

TEST_F(TestComments, replaceUnderscoresWithSpaces)
{
    std::strcpy(g_par_comment[0], "every_word_separated_with_underscore");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("every word separated with underscore", result);
}

TEST_F(TestComments, escapedUnderscore)
{
    std::strcpy(g_par_comment[0], R"(escaped\_underscore)");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("escaped_underscore", result);
}

TEST_F(TestComments, escapedDollar)
{
    std::strcpy(g_par_comment[0], R"(escaped\$_dollar)");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("escaped$ dollar", result);
}

TEST_F(TestComments, escapedBackslash)
{
    std::strcpy(g_par_comment[0], R"(escaped\\_backslash)");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ(R"(escaped\ backslash)", result);
}

TEST_F(TestComments, expandZeroCalcTime)
{
    g_calc_time = 0;
    std::strcpy(g_par_comment[0], "$calctime$");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("  0:00:00.00", result);
}

TEST_F(TestComments, expandHundredthsSecondsCalcTime)
{
    g_calc_time = 42;
    std::strcpy(g_par_comment[0], "$calctime$");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("  0:00:00.42", result);
}

TEST_F(TestComments, expandSecondsCalcTime)
{
    g_calc_time = 42 * 100 + 42;
    std::strcpy(g_par_comment[0], "$calctime$");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("  0:00:42.42", result);
}

TEST_F(TestComments, expandMinutesCalcTime)
{
    g_calc_time = (42 * 60 + 42) * 100 + 42;
    std::strcpy(g_par_comment[0], "$calctime$");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("  0:42:42.42", result);
}

TEST_F(TestComments, expandHoursCalcTime)
{
    g_calc_time = ((4 * 60 + 42) * 60 + 42) * 100 + 42;
    std::strcpy(g_par_comment[0], "$calctime$");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("  4:42:42.42", result);
}

TEST_F(TestComments, expandDaysCalcTime)
{
    g_calc_time = (((4 * 24 + 4) * 60 + 42) * 60 + 42) * 100 + 42;
    std::strcpy(g_par_comment[0], "$calctime$");
    
    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("100:42:42.42", result);
}

TEST_F(TestComments, expandVersion)
{
    g_release = 1964;
    std::strcpy(g_par_comment[0], "$version$");

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("1964", result);
}

TEST_F(TestComments, expandPatch)
{
    std::strcpy(g_par_comment[0], "$patch$");

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ(std::to_string(ID_VERSION_PATCH), result);
}

TEST_F(TestComments, expandXDots)
{
    g_logical_screen_x_dots = 1964;    
    std::strcpy(g_par_comment[0], "$xdots$");

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("1964", result);
}

TEST_F(TestComments, expandYDots)
{
    g_logical_screen_y_dots = 1964;    
    std::strcpy(g_par_comment[0], "$ydots$");

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("1964", result);
}

TEST_F(TestComments, expandVidKey)
{
    g_video_entry.key = ID_KEY_F6;
    std::strcpy(g_par_comment[0], "$vidkey$");

    const std::string &result{expand_command_comment(0)};

    EXPECT_EQ("F6", result);
}

TEST_F(TestComments, expandYear)
{
    std::strcpy(g_par_comment[0], "$year$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ(std::to_string(TEST_YEAR), result);
}

TEST_F(TestComments, expandMonth)
{
    std::strcpy(g_par_comment[0], "$month$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ("Dec", result);
}

TEST_F(TestComments, expandDay)
{
    std::strcpy(g_par_comment[0], "$day$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ(std::to_string(TEST_DAY), result);
}

TEST_F(TestComments, expandHour)
{
    std::strcpy(g_par_comment[0], "$hour$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ(std::to_string(TEST_HOUR), result);
}

TEST_F(TestComments, expandMinute)
{
    std::strcpy(g_par_comment[0], "$min$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ(std::to_string(TEST_MINUTE), result);
}

TEST_F(TestComments, expandTime)
{
    std::strcpy(g_par_comment[0], "$time$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ(
        std::to_string(TEST_HOUR) + ':' + std::to_string(TEST_MINUTE) + ':' + std::to_string(TEST_SECOND),
        result);
}

TEST_F(TestComments, expandDate)
{
    std::strcpy(g_par_comment[0], "$date$");

    const std::string &result{expand_command_comment(0, m_test_time)};

    EXPECT_EQ("Dec " + std::to_string(TEST_DAY) + ", " + std::to_string(TEST_YEAR), result);
}
