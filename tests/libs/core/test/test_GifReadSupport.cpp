// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <GifReadSupport.h>
#include <TemporaryDirectory.h>

#include <io/GifWriter.h>

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

namespace id::test
{

class TestGifReadSupport : public testing::Test
{
protected:
    void SetUp() override;

    TemporaryDirectory m_directory;
    std::filesystem::path m_path;
};

void TestGifReadSupport::SetUp()
{
    m_directory.create("id-gif-read-");
    m_path = m_directory.path() / "input.gif";
}

TEST_F(TestGifReadSupport, missingInputReportsFailure)
{
    EXPECT_THROW(read_gif(m_path), std::runtime_error);
}

TEST_F(TestGifReadSupport, decodeFailureClosesInput)
{
    io::GifWriter writer{m_path, {1, 1}, {{0, 0, 0}, {255, 255, 255}}};
    ASSERT_TRUE(writer.write_pixels({1}));
    ASSERT_TRUE(writer.finish());
    // Remove the trailer; giflib 5.2.2 corrupts its image table on truncated pixel data.
    std::filesystem::resize_file(m_path, std::filesystem::file_size(m_path) - 1);

    EXPECT_THROW(read_gif(m_path), std::runtime_error);

    EXPECT_TRUE(std::filesystem::remove(m_path));
}

} // namespace id::test
