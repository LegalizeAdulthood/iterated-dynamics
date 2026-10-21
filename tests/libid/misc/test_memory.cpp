// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

class TestMemory : public Test
{
protected:
    void SetUp() override
    {
        init_memory();
        m_directory = std::filesystem::temp_directory_path() /
            ("id-memory-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(m_directory);
        g_temp_dir = m_directory;
    }

    void TearDown() override
    {
        exit_check();
        std::filesystem::remove_all(m_directory);
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    std::filesystem::path m_directory;
};

class TestMemoryTransfer : public TestMemory, public WithParamInterface<MemoryLocation>
{
};

TEST_P(TestMemoryTransfer, roundTripsLargeTransfersAndFillsAcrossBlocks)
{
    const MemoryHandle handle{memory_alloc(1, 70003, GetParam())};
    std::vector<Byte> input(70000);
    for (size_t i = 0; i < input.size(); ++i)
    {
        input[i] = static_cast<Byte>(i % 251);
    }
    std::vector<Byte> output(input.size());

    handle.from_memory(input.data(), 1, input.size(), 3);
    handle.to_memory(output.data(), 1, output.size(), 3);

    EXPECT_EQ(input, output);
    handle.set(37, 1, output.size(), 3);
    handle.to_memory(output.data(), 1, output.size(), 3);
    EXPECT_THAT(output, Each(37));
    EXPECT_EQ(70003U, handle.size());
    memory_release(handle);
    EXPECT_TRUE(exit_check().empty());
}

TEST_P(TestMemoryTransfer, rejectsInvalidTransfersWithoutInputOrChangingStorage)
{
    const MemoryHandle handle{memory_alloc(1, 4, GetParam())};
    std::array<Byte, 4> buffer{1, 2, 3, 4};
    handle.from_memory(buffer.data(), 1, buffer.size(), 0);

    EXPECT_THROW(handle.from_memory(buffer.data(), 0, 1, 0), std::logic_error);
    EXPECT_THROW(handle.to_memory(buffer.data(), 1, 0, 0), std::logic_error);
    EXPECT_THROW(handle.set(0, 2, std::numeric_limits<std::uint64_t>::max(), 0), std::logic_error);
    EXPECT_THROW(handle.to_memory(buffer.data(), 2, 1, std::numeric_limits<std::uint64_t>::max()), std::logic_error);
    EXPECT_THROW(handle.from_memory(buffer.data(), 1, 1, 4), std::logic_error);
    EXPECT_THROW(handle.to_memory(buffer.data(), 1, 4, 1), std::logic_error);
    EXPECT_THROW(handle.set(0, 1, 1, 5), std::logic_error);
    EXPECT_THROW(handle.from_memory(nullptr, 1, 1, 0), std::logic_error);
    EXPECT_THROW(handle.to_memory(nullptr, 1, 1, 0), std::logic_error);

    buffer.fill(0);
    handle.to_memory(buffer.data(), 1, buffer.size(), 0);
    EXPECT_THAT(buffer, ElementsAre(1, 2, 3, 4));
    memory_release(handle);
}

INSTANTIATE_TEST_SUITE_P(Locations, TestMemoryTransfer, Values(MemoryLocation::MEMORY, MemoryLocation::DISK));

TEST_F(TestMemory, rejectsInvalidHandlesWithoutInput)
{
    std::array<Byte, 4> buffer{};
    const MemoryHandle empty;
    const MemoryHandle invalid{256};

    EXPECT_THROW(empty.from_memory(buffer.data(), 1, 4, 0), std::logic_error);
    EXPECT_THROW(invalid.to_memory(buffer.data(), 1, 4, 0), std::logic_error);
    EXPECT_THROW(invalid.set(0, 1, 4, 0), std::logic_error);
    EXPECT_THROW(memory_release(invalid), std::logic_error);
    EXPECT_NO_THROW(memory_release(empty));
}

TEST_F(TestMemory, rejectsInvalidAllocationSizesWithoutPublishingHandles)
{
    EXPECT_THROW(memory_alloc(0, 1, MemoryLocation::MEMORY), std::logic_error);
    EXPECT_THROW(memory_alloc(1, 0, MemoryLocation::MEMORY), std::logic_error);
    EXPECT_THROW(memory_alloc(2, std::numeric_limits<std::uint64_t>::max(), MemoryLocation::MEMORY), std::logic_error);
    EXPECT_THROW(memory_alloc(1, 1, MemoryLocation::NOWHERE), std::logic_error);
    EXPECT_THROW(memory_alloc(1, 1, static_cast<MemoryLocation>(-1)), std::logic_error);
    EXPECT_TRUE(exit_check().empty());

    const MemoryHandle valid{memory_alloc(1, 1, MemoryLocation::MEMORY)};
    EXPECT_EQ(1, valid.index);
    memory_release(valid);
}

TEST_F(TestMemory, exhaustedHandleTableCanBeRetriedAfterRelease)
{
    std::vector<MemoryHandle> handles;
    for (int i = 1; i < 256; ++i)
    {
        handles.push_back(memory_alloc(1, 1, MemoryLocation::MEMORY));
    }

    EXPECT_THROW(memory_alloc(1, 1, MemoryLocation::MEMORY), MemoryError);

    memory_release(handles.back());
    const MemoryHandle retried{memory_alloc(1, 1, MemoryLocation::MEMORY)};
    EXPECT_EQ(handles.back().index, retried.index);
    memory_release(retried);
    handles.pop_back();
    for (const MemoryHandle handle : handles)
    {
        memory_release(handle);
    }
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestMemory, diskAllocationFailureCanBeRetriedWithoutLeakedHandles)
{
    g_temp_dir = m_directory / "missing";

    EXPECT_THROW(memory_alloc(1, 4096, MemoryLocation::DISK), MemoryError);
    EXPECT_TRUE(exit_check().empty());

    std::filesystem::create_directory(g_temp_dir);
    const MemoryHandle handle{memory_alloc(1, 4096, MemoryLocation::DISK)};
    EXPECT_EQ(1, handle.index);
    memory_release(handle);
    EXPECT_TRUE(std::filesystem::is_empty(g_temp_dir));
}

TEST_F(TestMemory, shortReadReportsFailureAndAllowsRepairAndRetry)
{
    const std::filesystem::path file{m_directory / "backing.dat"};
    std::ofstream(file, std::ios::binary) << "1234";
    const MemoryHandle handle{memory_alloc(1, 4, MemoryLocation::DISK, file)};
    std::filesystem::resize_file(file, 1);
    std::array<Byte, 4> buffer{};

    EXPECT_THROW(handle.to_memory(buffer.data(), 1, 4, 0), MemoryError);

    const std::array<Byte, 4> repaired{4, 3, 2, 1};
    handle.from_memory(repaired.data(), 1, repaired.size(), 0);
    handle.to_memory(buffer.data(), 1, buffer.size(), 0);
    EXPECT_EQ(repaired, buffer);
    memory_release(handle);
    EXPECT_TRUE(std::filesystem::exists(file));
    EXPECT_EQ(4U, std::filesystem::file_size(file));
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestMemory, releaseUsesTheAllocatedPathRatherThanCurrentTempDirectory)
{
    const MemoryHandle handle{memory_alloc(1, 16, MemoryLocation::DISK)};
    ASSERT_FALSE(std::filesystem::is_empty(m_directory));
    g_temp_dir = m_directory / "elsewhere";

    memory_release(handle);

    EXPECT_TRUE(std::filesystem::is_empty(m_directory));
}

TEST_F(TestMemory, shutdownReturnsReportsAfterReleasingAllResources)
{
    memory_alloc(1, 16, MemoryLocation::MEMORY);
    memory_alloc(1, 16, MemoryLocation::DISK);

    EXPECT_THAT(exit_check(),
        ElementsAre("Error - not all memory released, I'll get it.", "Memory type memory still allocated.  Handle = 1.",
            "Memory type disk still allocated.  Handle = 2."));

    EXPECT_TRUE(std::filesystem::is_empty(m_directory));
    EXPECT_TRUE(exit_check().empty());
    const MemoryHandle handle{memory_alloc(1, 16, MemoryLocation::MEMORY)};
    EXPECT_EQ(1, handle.index);
    memory_release(handle);
}

} // namespace id::test
