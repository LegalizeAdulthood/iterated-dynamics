// SPDX-License-Identifier: GPL-3.0-only
//
#include <HelpSource.h>
#include <messages.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace hc::test
{

class TestHelpSource : public testing::Test
{
protected:
    void SetUp() override
    {
        m_test_dir = test_dir();
        std::error_code ignored;
        std::filesystem::remove_all(m_test_dir, ignored);
        std::filesystem::create_directories(m_test_dir);

        reset_globals();
    }

    void TearDown() override
    {
        if (g_src.swap_file != nullptr)
        {
            std::fclose(g_src.swap_file);
            g_src.swap_file = nullptr;
        }
        g_src = HelpSource{};
        g_errors = 0;
        g_warnings = 0;
        g_src_line = -1;
        g_current_src_filename.clear();

        std::error_code ignored;
        std::filesystem::remove_all(m_test_dir, ignored);
    }

    void read_source(const std::string &text, const Mode mode = Mode::COMPILE)
    {
        m_source_path = m_test_dir / "test.src";
        std::ofstream src{m_source_path};
        ASSERT_TRUE(src) << "Unable to create " << m_source_path;
        src << text;
        src.close();

        testing::internal::CaptureStdout();
        read_src(m_source_path.string(), mode);
        m_output = testing::internal::GetCapturedStdout();
    }

    std::filesystem::path test_dir() const
    {
        const testing::TestInfo *info = testing::UnitTest::GetInstance()->current_test_info();
        std::string name{info->test_suite_name()};
        name += '_';
        name += info->name();
        std::replace_if(name.begin(), name.end(), [](const unsigned char ch) { return std::isalnum(ch) == 0; }, '_');
        return std::filesystem::temp_directory_path() / name;
    }

    void reset_globals()
    {
        g_src = HelpSource{};
        g_src.buffer.resize(BUFFER_SIZE);
        g_src.curr = g_src.buffer.data();
        g_errors = 0;
        g_warnings = 0;
        g_quiet_mode = true;
        g_src_line = -1;
        g_current_src_filename.clear();

        const std::filesystem::path swap_path{m_test_dir / "test.swap"};
        g_src.swap_file = std::fopen(swap_path.string().c_str(), "w+b");
        ASSERT_NE(nullptr, g_src.swap_file) << "Unable to create " << swap_path;
    }

    std::filesystem::path m_test_dir;
    std::filesystem::path m_source_path;
    std::string m_output;
};

TEST(TestLabels, indexLabelLeast)
{
    const Label idx{INDEX_LABEL, 0, 0, 0};
    const Label other{"HELP_OTHER", 0, 0, 0};

    EXPECT_TRUE(idx < other);
    EXPECT_FALSE(other < idx);
}

TEST(TestLabels, labelsSortByName)
{
    const Label smaller{"aardvark", 0, 0, 0};
    const Label bigger{"zebra", 0, 0, 0};

    EXPECT_TRUE(smaller < bigger);
    EXPECT_FALSE(bigger < smaller);
}

TEST_F(TestHelpSource, findTopicTitleTrimsSpacesAndOptionalQuotes)
{
    Topic topic{};
    topic.title = "Quoted Topic";
    topic.title_len = static_cast<unsigned>(topic.title.length());
    g_src.topics.push_back(topic);

    EXPECT_EQ(0, find_topic_title("  Quoted Topic   "));
    EXPECT_EQ(0, find_topic_title("  \"Quoted Topic\"   "));
    EXPECT_EQ(-1, find_topic_title("  \"Missing Topic\"   "));
}

TEST_F(TestHelpSource, readSourceParsesQuotedDocContentsAndFormatsToc)
{
    read_source("~DocContents\n"
                "{1.   , 0, \"Quoted Topic\", \"Second Topic\", FF}\n"
                "{1.1  , 1, LabelTopic}\n"
                "~Topic=Quoted Topic\n"
                "First body.\n"
                "~Topic=Second Topic\n"
                "Second body.\n"
                "~Topic=LabelTopic\n"
                "Label body.\n");

    EXPECT_EQ(0, g_errors);
    EXPECT_EQ(0, g_warnings);
    ASSERT_EQ(3U, g_src.contents.size());

    const Content &entry{g_src.contents[1]};
    EXPECT_EQ("1.", entry.id);
    EXPECT_EQ(0, entry.indent);
    EXPECT_EQ("Quoted Topic", entry.name);
    ASSERT_EQ(2, entry.num_topic);
    EXPECT_FALSE(entry.is_label[0]);
    EXPECT_EQ("Quoted Topic", entry.topic_name[0]);
    EXPECT_FALSE(entry.is_label[1]);
    EXPECT_EQ("Second Topic", entry.topic_name[1]);
    EXPECT_NE(0, entry.flags & CF_NEW_PAGE);

    ASSERT_FALSE(g_src.topics.empty());
    const Topic &toc_topic{g_src.topics[0]};
    const std::string toc_text{toc_topic.get_topic_text(), toc_topic.text_len};
    EXPECT_THAT(toc_text, testing::HasSubstr("Quoted Topic"));
    EXPECT_THAT(toc_text, testing::HasSubstr("LabelTopic"));
    EXPECT_THAT(toc_text, testing::HasSubstr("..."));
}

TEST_F(TestHelpSource, readSourceWarnsForLongTopicTitle)
{
    const std::string long_title(61, 'T');
    read_source("~Topic=" + long_title + "\nBody.\n");

    EXPECT_EQ(0, g_errors);
    EXPECT_EQ(1, g_warnings);
    ASSERT_EQ(1U, g_src.topics.size());
    EXPECT_EQ(long_title, g_src.topics[0].title);
}

TEST_F(TestHelpSource, readSourceReportsEmptyLabelAndWarnsForLongLabel)
{
    const std::string long_label(33, 'L');
    read_source("~Topic=Base\n"
                "~Label=\n"
                "~Label=" +
        long_label +
        "\n"
        "Body.\n");

    EXPECT_EQ(1, g_errors);
    EXPECT_EQ(1, g_warnings);
    EXPECT_EQ(nullptr, g_src.find_label(""));
    ASSERT_NE(nullptr, g_src.find_label(long_label.c_str()));
}

} // namespace hc::test
