// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "MockDriver.h"

#include <engine/pixel_limits.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <ui/diskvid.h>

#include <gtest/gtest.h>

#include <cstdint>
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

} // namespace id::test
