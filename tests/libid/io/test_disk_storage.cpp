// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/disk_storage.h>

#include "MockDriver.h"

#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <tuple>
#include <vector>

using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

class TestDiskStorage : public Test
{
protected:
    void SetUp() override
    {
        end_disk();
        init_memory();
        m_directory = std::filesystem::temp_directory_path() /
            ("id-disk-storage-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(m_directory);
        g_temp_dir = m_directory;
    }

    void TearDown() override
    {
        EXPECT_NO_THROW(end_disk());
        EXPECT_TRUE(exit_check().empty());
        std::filesystem::remove_all(m_directory);
    }

    void open(const int width, const int height, const int colors, const MemoryLocation location,
        const std::vector<Byte> &header = {})
    {
        std::uint64_t blocks{};
        ASSERT_TRUE(disk_video_memory_blocks(width, height, colors, static_cast<int>(header.size()), blocks));
        const MemoryHandle backing{memory_alloc(
            DISK_STORAGE_BLOCK_SIZE, blocks, location, header.empty() ? std::filesystem::path{} : targa_disk_path())};
        start_disk_storage(width, height, colors, backing, header);
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    std::filesystem::path m_directory;
};

class TestDiskStorageCache : public TestDiskStorage, public WithParamInterface<std::tuple<int, MemoryLocation>>
{
};

TEST_P(TestDiskStorageCache, packedPixelsSurviveEvictionWithoutUi)
{
    const int colors{std::get<0>(GetParam())};
    const MemoryLocation location{std::get<1>(GetParam())};
    constexpr int ROWS{2200};
    open(DISK_STORAGE_BLOCK_SIZE, ROWS, colors, location);
    ASSERT_TRUE(g_disk_flag);
    EXPECT_EQ(location, disk_video_memory_type());

    for (int row = 0; row < ROWS; ++row)
    {
        if (row % 3 == 0)
        {
            EXPECT_EQ(0, read_disk_pixel(0, row));
        }
        else
        {
            write_disk_pixel(0, row, 1 + row % (colors - 1));
            write_disk_pixel(DISK_STORAGE_BLOCK_SIZE - 1, row, colors - 1);
        }
    }
    for (int row = 0; row < ROWS; ++row)
    {
        EXPECT_EQ(row % 3 == 0 ? 0 : 1 + row % (colors - 1), read_disk_pixel(0, row)) << row;
        EXPECT_EQ(row % 3 == 0 ? 0 : colors - 1, read_disk_pixel(DISK_STORAGE_BLOCK_SIZE - 1, row)) << row;
    }
}

INSTANTIATE_TEST_SUITE_P(Formats, TestDiskStorageCache,
    Combine(Values(2, 4, 16, 256), Values(MemoryLocation::MEMORY, MemoryLocation::DISK)));

TEST_F(TestDiskStorage, queueBytesRoundTripAtCacheBoundaries)
{
    open(DISK_STORAGE_BLOCK_SIZE, 2, 256, MemoryLocation::MEMORY);
    const std::array<Byte, 4> input{12, 34, 56, 78};
    std::array<Byte, 4> output{};

    EXPECT_TRUE(to_mem_disk(DISK_STORAGE_BLOCK_SIZE - 4, input.size(), input.data()));
    EXPECT_TRUE(to_mem_disk(DISK_STORAGE_BLOCK_SIZE, input.size(), input.data()));
    EXPECT_TRUE(from_mem_disk(DISK_STORAGE_BLOCK_SIZE - 4, output.size(), output.data()));
    EXPECT_EQ(input, output);
    output.fill(0);
    EXPECT_TRUE(from_mem_disk(DISK_STORAGE_BLOCK_SIZE, output.size(), output.data()));
    EXPECT_EQ(input, output);
    EXPECT_FALSE(to_mem_disk(DISK_STORAGE_BLOCK_SIZE - 2, input.size(), input.data()));
    EXPECT_FALSE(from_mem_disk(-1, output.size(), output.data()));
}

TEST_F(TestDiskStorage, progressIsDataWithoutPresentation)
{
    open(16, 8, 256, MemoryLocation::MEMORY);
    EXPECT_EQ(DiskStorageOperation::NONE, disk_storage_progress().operation);

    write_disk_pixel(1, 3, 77);
    EXPECT_EQ(DiskStorageOperation::WRITE, disk_storage_progress().operation);
    EXPECT_EQ(3, disk_storage_progress().row);
    EXPECT_EQ(77, read_disk_pixel(1, 3));
    EXPECT_EQ(DiskStorageOperation::READ, disk_storage_progress().operation);
    EXPECT_EQ(3, disk_storage_progress().row);
}

TEST_F(TestDiskStorage, failedInitializationReleasesBackingAndAllowsFreshSetup)
{
    const MemoryHandle backing{memory_alloc(1, 1, MemoryLocation::DISK)};

    EXPECT_THROW(start_disk_storage(16, 16, 256, backing), std::logic_error);

    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::NOWHERE, disk_video_memory_type());
    EXPECT_TRUE(exit_check().empty());
    EXPECT_TRUE(std::filesystem::is_empty(m_directory));
    open(16, 16, 256, MemoryLocation::MEMORY);
    write_disk_pixel(15, 15, 42);
    EXPECT_EQ(42, read_disk_pixel(15, 15));
}

TEST_F(TestDiskStorage, invalidHandleDoesNotPublishStorage)
{
    EXPECT_THROW(start_disk_storage(16, 16, 256, MemoryHandle{256}), std::logic_error);
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::NOWHERE, disk_video_memory_type());
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestDiskStorage, truncatedTargaHeaderClosesTheSource)
{
    const std::filesystem::path path{m_directory / "short.tga"};
    std::ofstream(path, std::ios::binary) << "short";
    std::FILE *file{std::fopen(path.string().c_str(), "rb")};
    ASSERT_NE(nullptr, file);

    EXPECT_THROW(read_targa_header(file, 18), MemoryError);

    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(std::filesystem::remove(path));
}

TEST_F(TestDiskStorage, failedBackingReadCanBeClosedWithoutUi)
{
    const std::filesystem::path path{m_directory / "backing.tga"};
    std::ofstream(path, std::ios::binary) << "012345678901234567";
    std::FILE *file{std::fopen(path.string().c_str(), "rb")};
    ASSERT_NE(nullptr, file);
    const std::vector<Byte> header{read_targa_header(file, 18)};
    set_targa_disk_path(path);
    open(DISK_STORAGE_BLOCK_SIZE, 1, 256, MemoryLocation::DISK, header);
    std::filesystem::resize_file(path, 1);

    EXPECT_THROW(read_disk_pixel(0, 0), MemoryError);
    EXPECT_NO_THROW(end_disk());

    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::NOWHERE, disk_video_memory_type());
    EXPECT_EQ(DiskStorageOperation::NONE, disk_storage_progress().operation);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_TRUE(std::filesystem::exists(path));
}

TEST_F(TestDiskStorage, selectingAnotherTargaPathDoesNotChangeTheActiveBackingFile)
{
    const std::filesystem::path active{m_directory / "active.tga"};
    const std::filesystem::path next{m_directory / "next.tga"};
    std::ofstream(active, std::ios::binary) << "012345678901234567";
    std::ofstream(next, std::ios::binary) << "keep";
    std::FILE *file{std::fopen(active.string().c_str(), "rb")};
    ASSERT_NE(nullptr, file);
    const std::vector<Byte> header{read_targa_header(file, 18)};
    set_targa_disk_path(active);
    open(6, 2, 256, MemoryLocation::DISK, header);
    write_targa_pixel(0, 0, 10, 20, 30);

    set_targa_disk_path(next);
    end_disk();

    EXPECT_EQ(30U, std::filesystem::file_size(active));
    EXPECT_EQ(4U, std::filesystem::file_size(next));
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
}

} // namespace id::test
