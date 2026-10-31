// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <CommandTestFile.h>
#include <TemporaryDirectory.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace id::test
{

class TestCommandTestFile : public testing::Test
{
protected:
    void SetUp() override;
    void write_input();

    TemporaryDirectory m_directory;
    std::optional<CommandTestFile> m_file;
};

void TestCommandTestFile::SetUp()
{
    m_directory.create("id-command-file-");
    m_file.emplace(m_directory.path() / "input.par");
}

void TestCommandTestFile::write_input()
{
    m_file->write("entry { maxiter=222 }");
}

TEST_F(TestCommandTestFile, openInputWritesBinaryBytesAndRewinds)
{
    constexpr char contents[]{"first\r\nsecond\0last"};
    const std::string_view text{contents, sizeof(contents) - 1};
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> file{m_file->open_input(text), std::fclose};
    std::string actual(text.size(), '\0');

    ASSERT_EQ(actual.size(), std::fread(actual.data(), 1, actual.size(), file.get()));

    EXPECT_EQ(text, actual);
    EXPECT_EQ(EOF, std::fgetc(file.get()));
}

TEST_F(TestCommandTestFile, writeStoresBinaryBytes)
{
    constexpr char contents[]{"first\r\nsecond\0last"};
    const std::string_view text{contents, sizeof(contents) - 1};

    m_file->write(text);

    std::ifstream file{m_file->path(), std::ios::binary};
    ASSERT_TRUE(file.is_open());
    const std::string actual{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    EXPECT_EQ(text, actual);
}

TEST_F(TestCommandTestFile, writeClosesTheFile)
{
    write_input();

    EXPECT_TRUE(m_file->remove());
}

TEST_F(TestCommandTestFile, writeReplacesExistingContents)
{
    write_input();

    m_file->write("new");

    EXPECT_EQ(3U, std::filesystem::file_size(m_file->path()));
}

TEST_F(TestCommandTestFile, removalLeavesDirectoryAndOtherFilesIntact)
{
    write_input();
    CommandTestFile other{m_directory.path() / "other.par"};
    {
        const std::unique_ptr<std::FILE, decltype(&std::fclose)> file{
            other.open_input("entry { maxiter=333 }"), std::fclose};
    }

    EXPECT_TRUE(m_file->remove());

    EXPECT_FALSE(std::filesystem::exists(m_file->path()));
    EXPECT_TRUE(std::filesystem::is_directory(m_directory.path()));
    EXPECT_TRUE(std::filesystem::exists(other.path()));
}

TEST_F(TestCommandTestFile, removalReturnsFalseForMissingFile)
{
    EXPECT_FALSE(m_file->remove());
}

TEST_F(TestCommandTestFile, destructionRemovesItsFile)
{
    write_input();
    const std::filesystem::path path{m_file->path()};
    ASSERT_TRUE(std::filesystem::exists(path));

    m_file.reset();

    EXPECT_FALSE(std::filesystem::exists(path));
    EXPECT_TRUE(std::filesystem::is_directory(m_directory.path()));
}

} // namespace id::test
