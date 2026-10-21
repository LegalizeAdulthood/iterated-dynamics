// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "MockDriver.h"

#include <engine/pixel_limits.h>
#include <engine/LogicalScreen.h>
#include <engine/VideoInfo.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <ui/diskvid.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <array>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <limits>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using testing::Return;

namespace id::test
{

namespace
{

class DiskVideoState
{
public:
    DiskVideoState(Driver *driver, const DebugFlags debug_flag) :
        m_saved_driver(g_driver, driver),
        m_saved_debug_flag(g_debug_flag, debug_flag),
        m_saved_temp_dir(g_temp_dir, std::filesystem::temp_directory_path())
    {
        init_memory();
    }

    ~DiskVideoState()
    {
        if (g_disk_flag)
        {
            end_disk();
        }
        exit_check();
    }

private:
    ValueSaver<Driver *> m_saved_driver;
    ValueSaver<DebugFlags> m_saved_debug_flag;
    ValueSaver<std::filesystem::path> m_saved_temp_dir;
};

} // namespace

TEST(TestDiskVideo, memoryBlocksAcceptGifMaximum)
{
    std::uint64_t blocks{};

    EXPECT_TRUE(disk_video_memory_blocks(GIF_MAX_PIXELS, GIF_MAX_PIXELS, 256, 0, blocks));
    EXPECT_EQ((static_cast<std::uint64_t>(GIF_MAX_PIXELS) * GIF_MAX_PIXELS + 2047) / 2048, blocks);
}

TEST(TestDiskVideo, memoryBlocksRejectInvalidSizes)
{
    std::uint64_t blocks{};

    EXPECT_FALSE(disk_video_memory_blocks(0, 1, 256, 0, blocks));
    EXPECT_FALSE(disk_video_memory_blocks(1, 0, 256, 0, blocks));
    EXPECT_FALSE(disk_video_memory_blocks(1, 1, 1, 0, blocks));
    EXPECT_FALSE(disk_video_memory_blocks(1, 1, 256, -1, blocks));
    EXPECT_FALSE(disk_video_memory_blocks(std::numeric_limits<std::int64_t>::max(), 2, 256, 0, blocks));
}

TEST(TestDiskVideo, memoryBlocksIncludeHeaderAndCachePadding)
{
    std::uint64_t blocks{};

    ASSERT_TRUE(disk_video_memory_blocks(6, 2, 256, 18, blocks));
    EXPECT_EQ(2U, blocks);
    ASSERT_TRUE(disk_video_memory_blocks(2048, 1, 256, 18, blocks));
    EXPECT_EQ(2U, blocks);
    ASSERT_TRUE(disk_video_memory_blocks(2049, 1, 256, 18, blocks));
    EXPECT_EQ(3U, blocks);
}

TEST(TestDiskVideo, commonStartDiskUsesMemoryWhenAvailable)
{
    MockDriver driver;
    DiskVideoState state{&driver, DebugFlags::NONE};
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));

    EXPECT_EQ(0, common_start_disk(16, 16, 256));

    EXPECT_TRUE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::MEMORY, disk_video_memory_type());
    disk_write_pixel(15, 15, 77);
    EXPECT_EQ(77, disk_read_pixel(15, 15));
}

TEST(TestDiskVideo, commonStartDiskUsesDiskWhenForced)
{
    MockDriver driver;
    DiskVideoState state{&driver, DebugFlags::FORCE_MEMORY_FROM_DISK};
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));

    EXPECT_EQ(0, common_start_disk(64, 64, 256));

    EXPECT_TRUE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::DISK, disk_video_memory_type());
    disk_write_pixel(63, 63, 99);
    EXPECT_EQ(0, disk_read_pixel(0, 0));
    EXPECT_EQ(99, disk_read_pixel(63, 63));
}

TEST(TestDiskVideo, targaCachePreservesHeaderAndPixelsAcrossBackingBlocks)
{
    MockDriver driver;
    DiskVideoState state{&driver, DebugFlags::NONE};
    ValueSaver screen{g_logical_screen, LogicalScreen{2, 2, 0, 0, 1.0, 1.0}};
    ValueSaver colors{g_colors, 256};
    EXPECT_CALL(driver, is_disk()).WillRepeatedly(Return(false));
    const std::filesystem::path path{std::filesystem::temp_directory_path() /
        ("id-targa-cache-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".tga")};
    std::FILE *file{std::fopen(path.string().c_str(), "w+b")};
    ASSERT_NE(nullptr, file);
    std::array<Byte, 18> header{};
    header.fill(13);
    ASSERT_EQ(header.size(), std::fwrite(header.data(), 1, header.size(), file));
    set_targa_disk_path(path);
    ASSERT_EQ(0, targa_start_disk(file, static_cast<int>(header.size())));

    targa_write_disk(0, 0, 10, 20, 30);
    targa_write_disk(1, 1, 40, 50, 60);
    end_disk();

    EXPECT_EQ(30U, std::filesystem::file_size(path));
    std::ifstream input{path, std::ios::binary};
    std::array<Byte, 30> bytes{};
    input.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
    EXPECT_EQ(13, bytes[0]);
    EXPECT_EQ(13, bytes[17]);
    EXPECT_EQ(30, bytes[18]);
    EXPECT_EQ(20, bytes[19]);
    EXPECT_EQ(10, bytes[20]);
    EXPECT_EQ(60, bytes[27]);
    EXPECT_EQ(50, bytes[28]);
    EXPECT_EQ(40, bytes[29]);
    input.close();
    std::filesystem::remove(path);
}

} // namespace id::test
