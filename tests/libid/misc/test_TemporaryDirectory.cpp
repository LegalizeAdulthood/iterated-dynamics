// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "TemporaryDirectory.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <system_error>

namespace id::test
{

class TestTemporaryDirectory : public testing::Test
{
protected:
    void SetUp() override;
    void add_contents();

    std::optional<TemporaryDirectory> m_directory;
};

void TestTemporaryDirectory::SetUp()
{
    m_directory.emplace();
    m_directory->create("id-temporary-directory-");
}

void TestTemporaryDirectory::add_contents()
{
    std::filesystem::create_directory(m_directory->path() / "nested");
    std::ofstream{m_directory->path() / "nested" / "file.txt"} << "contents";
}

TEST_F(TestTemporaryDirectory, createsAnEmptyDirectory)
{
    EXPECT_TRUE(std::filesystem::is_directory(m_directory->path()));
    EXPECT_TRUE(std::filesystem::is_empty(m_directory->path()));
}

TEST_F(TestTemporaryDirectory, simultaneousOwnersUseDifferentDirectories)
{
    TemporaryDirectory other;
    other.create("id-temporary-directory-");

    EXPECT_NE(m_directory->path(), other.path());
    EXPECT_TRUE(std::filesystem::is_directory(m_directory->path()));
    EXPECT_TRUE(std::filesystem::is_directory(other.path()));
}

TEST_F(TestTemporaryDirectory, explicitRemovalDeletesContents)
{
    add_contents();
    const std::filesystem::path path{m_directory->path()};
    ASSERT_TRUE(std::filesystem::exists(path / "nested" / "file.txt"));

    m_directory->remove();

    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(TestTemporaryDirectory, destructionDeletesContents)
{
    add_contents();
    const std::filesystem::path path{m_directory->path()};
    ASSERT_TRUE(std::filesystem::exists(path / "nested" / "file.txt"));

    m_directory.reset();

    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(TestTemporaryDirectory, errorCodeRemovalDeletesContents)
{
    add_contents();
    const std::filesystem::path path{m_directory->path()};
    ASSERT_TRUE(std::filesystem::exists(path / "nested" / "file.txt"));
    std::error_code error{std::make_error_code(std::errc::permission_denied)};

    m_directory->remove(error);

    EXPECT_FALSE(error);
    EXPECT_FALSE(std::filesystem::exists(path));
}

TEST_F(TestTemporaryDirectory, inactiveErrorCodeRemovalClearsPreviousError)
{
    TemporaryDirectory other;
    std::error_code error{std::make_error_code(std::errc::permission_denied)};

    other.remove(error);

    EXPECT_FALSE(error);
}

TEST_F(TestTemporaryDirectory, refusesToReplaceAnOwnedDirectory)
{
    EXPECT_THROW(m_directory->create("id-temporary-directory-"), std::logic_error);
}

TEST_F(TestTemporaryDirectory, rejectsPrefixesContainingParentPaths)
{
    TemporaryDirectory other;

    EXPECT_THROW(other.create("../id-temporary-directory-"), std::logic_error);
}

} // namespace id::test
