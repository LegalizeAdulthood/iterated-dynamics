// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/disk_storage.h>

#include "MockDriver.h"
#include "TemporaryDirectory.h"

#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
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
        m_directory.create("id-disk-storage-");
        g_temp_dir = m_directory.path();
    }

    void TearDown() override
    {
        EXPECT_NO_THROW(end_disk());
        EXPECT_TRUE(exit_check().empty());
        m_directory.remove();
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
    TemporaryDirectory m_directory;
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

class TestDiskStorageQueue : public TestDiskStorage
{
protected:
    void SetUp() override;

    const std::array<Byte, 4> m_input{12, 34, 56, 78};
    std::array<Byte, 4> m_output{};
};

void TestDiskStorageQueue::SetUp()
{
    TestDiskStorage::SetUp();
    open(DISK_STORAGE_BLOCK_SIZE, 2, 256, MemoryLocation::MEMORY);
}

class TestDiskStorageQueueBoundary : public TestDiskStorageQueue, public WithParamInterface<int>
{
};

TEST_P(TestDiskStorageQueueBoundary, bytesRoundTripAtCacheBoundary)
{
    EXPECT_TRUE(to_mem_disk(GetParam(), m_input.size(), m_input.data()));
    EXPECT_TRUE(from_mem_disk(GetParam(), m_output.size(), m_output.data()));

    EXPECT_EQ(m_input, m_output);
}

INSTANTIATE_TEST_SUITE_P(CacheOffsets, TestDiskStorageQueueBoundary,
    Values(DISK_STORAGE_BLOCK_SIZE - 4, DISK_STORAGE_BLOCK_SIZE),
    [](const TestParamInfo<int> &info) { return info.param == DISK_STORAGE_BLOCK_SIZE ? "NextBlock" : "EndOfBlock"; });

TEST_F(TestDiskStorageQueue, rejectsCrossBlockWrite)
{
    EXPECT_FALSE(to_mem_disk(DISK_STORAGE_BLOCK_SIZE - 2, m_input.size(), m_input.data()));
}

TEST_F(TestDiskStorageQueue, rejectsNegativeOffsetRead)
{
    EXPECT_FALSE(from_mem_disk(-1, m_output.size(), m_output.data()));
}

class TestDiskStorageProgress : public TestDiskStorage
{
protected:
    void SetUp() override;
};

void TestDiskStorageProgress::SetUp()
{
    TestDiskStorage::SetUp();
    open(16, 8, 256, MemoryLocation::MEMORY);
}

TEST_F(TestDiskStorageProgress, initialProgressHasNoOperation)
{
    EXPECT_EQ(DiskStorageOperation::NONE, disk_storage_progress().operation);
}

TEST_F(TestDiskStorageProgress, writeProgressReportsTheWrittenRow)
{
    write_disk_pixel(1, 3, 77);

    EXPECT_EQ(DiskStorageOperation::WRITE, disk_storage_progress().operation);
    EXPECT_EQ(3, disk_storage_progress().row);
}

class TestDiskStorageReadProgress : public TestDiskStorageProgress
{
protected:
    void SetUp() override;
};

void TestDiskStorageReadProgress::SetUp()
{
    TestDiskStorageProgress::SetUp();
    write_disk_pixel(1, 3, 77);
}

TEST_F(TestDiskStorageReadProgress, readProgressReportsTheReadRow)
{
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
    EXPECT_TRUE(std::filesystem::is_empty(m_directory.path()));
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

class TestDiskStorageTarga : public TestDiskStorage
{
protected:
    using FilePtr = std::unique_ptr<std::FILE, decltype(&std::fclose)>;

    void SetUp() override;
    void write_source(const std::string &contents) const;
    FilePtr open_source() const;
    void open_backing(int width, int height);

    std::filesystem::path m_path;
};

void TestDiskStorageTarga::SetUp()
{
    TestDiskStorage::SetUp();
    m_path = m_directory.path() / "backing.tga";
    write_source("012345678901234567");
}

void TestDiskStorageTarga::write_source(const std::string &contents) const
{
    std::ofstream output{m_path, std::ios::binary};
    output << contents;
    output.close();
    if (!output)
    {
        throw std::runtime_error("Can't create Targa backing test input");
    }
}

TestDiskStorageTarga::FilePtr TestDiskStorageTarga::open_source() const
{
    FilePtr source{std::fopen(m_path.string().c_str(), "rb"), std::fclose};
    if (!source)
    {
        throw std::runtime_error("Can't open Targa backing test input");
    }
    return source;
}

void TestDiskStorageTarga::open_backing(const int width, const int height)
{
    const std::vector<Byte> header{read_targa_header(open_source().release(), 18)};
    set_targa_disk_path(m_path);
    open(width, height, 256, MemoryLocation::DISK, header);
}

TEST_F(TestDiskStorageTarga, truncatedTargaHeaderClosesTheSource)
{
    write_source("short");

    EXPECT_THROW(read_targa_header(open_source().release(), 18), MemoryError);

    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(std::filesystem::remove(m_path));
}

TEST_F(TestDiskStorageTarga, failedBackingReadCanBeClosedWithoutUi)
{
    open_backing(DISK_STORAGE_BLOCK_SIZE, 1);
    std::filesystem::resize_file(m_path, 1);

    EXPECT_THROW(read_disk_pixel(0, 0), MemoryError);
    EXPECT_NO_THROW(end_disk());

    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::NOWHERE, disk_video_memory_type());
    EXPECT_EQ(DiskStorageOperation::NONE, disk_storage_progress().operation);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_TRUE(std::filesystem::exists(m_path));
}

TEST_F(TestDiskStorageTarga, selectingAnotherTargaPathDoesNotChangeTheActiveBackingFile)
{
    const std::filesystem::path next{m_directory.path() / "next.tga"};
    std::ofstream(next, std::ios::binary) << "keep";
    open_backing(6, 2);
    write_targa_pixel(0, 0, 10, 20, 30);

    set_targa_disk_path(next);
    end_disk();

    EXPECT_EQ(30U, std::filesystem::file_size(m_path));
    EXPECT_EQ(4U, std::filesystem::file_size(next));
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestDiskStorageTarga, failedDirtyTargaFlushReleasesStorageAndAllowsRepeatedClose)
{
    open_backing(DISK_STORAGE_BLOCK_SIZE, 8);
    write_targa_pixel(0, 7, 10, 20, 30);
    read_disk_pixel(0, 0);
    std::filesystem::resize_file(m_path, 0);

    EXPECT_THROW(end_disk(), MemoryError);

    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::NOWHERE, disk_video_memory_type());
    EXPECT_EQ(DiskStorageOperation::NONE, disk_storage_progress().operation);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_NO_THROW(end_disk());
}

} // namespace id::test
