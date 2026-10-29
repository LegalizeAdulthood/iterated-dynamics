// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <io/disk_storage.h>
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
#include <ostream>
#include <string>
#include <vector>

using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

namespace
{

std::filesystem::space_info one_byte_available(const std::filesystem::path &, std::error_code &error)
{
    error.clear();
    return {1024 * 1024, 1, 1};
}

std::filesystem::space_info no_bytes_available(const std::filesystem::path &, std::error_code &error)
{
    error.clear();
    return {1024 * 1024, 0, 0};
}

} // namespace

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

class TestLargeMemoryTransfer : public TestMemory, public WithParamInterface<MemoryLocation>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void release_handle();

    MemoryHandle m_handle;
    std::vector<Byte> m_input;
    std::vector<Byte> m_output;
};

void TestLargeMemoryTransfer::SetUp()
{
    TestMemory::SetUp();
    m_handle = memory_alloc(1, 70003, GetParam());
    m_input.resize(70000);
    m_output.resize(m_input.size());
    for (std::size_t i = 0; i < m_input.size(); ++i)
    {
        m_input[i] = static_cast<Byte>(i % 251);
    }
}

void TestLargeMemoryTransfer::TearDown()
{
    if (m_handle)
    {
        memory_release(m_handle);
    }
    TestMemory::TearDown();
}

void TestLargeMemoryTransfer::release_handle()
{
    memory_release(m_handle);
    m_handle = {};
}

TEST_P(TestLargeMemoryTransfer, roundTripsAcrossBlocks)
{
    m_handle.from_memory(m_input.data(), 1, m_input.size(), 3);
    m_handle.to_memory(m_output.data(), 1, m_output.size(), 3);

    EXPECT_EQ(m_input, m_output);
    release_handle();
    EXPECT_TRUE(exit_check().empty());
}

TEST_P(TestLargeMemoryTransfer, fillsAcrossBlocks)
{
    m_handle.from_memory(m_input.data(), 1, m_input.size(), 3);

    m_handle.set(37, 1, m_output.size(), 3);
    m_handle.to_memory(m_output.data(), 1, m_output.size(), 3);

    EXPECT_THAT(m_output, Each(37));
    EXPECT_EQ(70003U, m_handle.size());
    release_handle();
    EXPECT_TRUE(exit_check().empty());
}

INSTANTIATE_TEST_SUITE_P(Locations, TestLargeMemoryTransfer, Values(MemoryLocation::MEMORY, MemoryLocation::DISK));

class TestInvalidMemoryTransfer : public TestMemory
{
protected:
    void TearDown() override;
    void prepare_storage(MemoryLocation location);
    void expect_unchanged_storage();

    MemoryHandle m_handle;
    std::array<Byte, 4> m_buffer{1, 2, 3, 4};
};

void TestInvalidMemoryTransfer::TearDown()
{
    if (m_handle)
    {
        memory_release(m_handle);
    }
    TestMemory::TearDown();
}

void TestInvalidMemoryTransfer::prepare_storage(const MemoryLocation location)
{
    m_handle = memory_alloc(1, 4, location);
    m_handle.from_memory(m_buffer.data(), 1, m_buffer.size(), 0);
}

void TestInvalidMemoryTransfer::expect_unchanged_storage()
{
    m_buffer.fill(0);
    m_handle.to_memory(m_buffer.data(), 1, m_buffer.size(), 0);
    EXPECT_THAT(m_buffer, ElementsAre(1, 2, 3, 4));
}

struct InvalidBufferTransferInput
{
    std::string name;
    MemoryLocation location;
    U16 size;
    std::uint64_t count;
    std::uint64_t offset;
    bool null_buffer;
};

void PrintTo(const InvalidBufferTransferInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InvalidBufferTransferInput> invalid_memory_write_inputs()
{
    std::vector<InvalidBufferTransferInput> inputs;
    for (const MemoryLocation location : {MemoryLocation::MEMORY, MemoryLocation::DISK})
    {
        const std::string prefix{location == MemoryLocation::MEMORY ? "Memory" : "Disk"};
        inputs.push_back({prefix + "ZeroUnit", location, 0, 1, 0, false});
        inputs.push_back({prefix + "PastEnd", location, 1, 1, 4, false});
        inputs.push_back({prefix + "NullBuffer", location, 1, 1, 0, true});
    }
    return inputs;
}

class TestInvalidMemoryWrites : public TestInvalidMemoryTransfer, public WithParamInterface<InvalidBufferTransferInput>
{
protected:
    void SetUp() override;
};

void TestInvalidMemoryWrites::SetUp()
{
    TestMemory::SetUp();
    prepare_storage(GetParam().location);
}

TEST_P(TestInvalidMemoryWrites, rejectsInvalidRangeWithoutChangingStorage)
{
    EXPECT_THROW(m_handle.from_memory(GetParam().null_buffer ? nullptr : m_buffer.data(), GetParam().size,
                     GetParam().count, GetParam().offset),
        std::logic_error);

    expect_unchanged_storage();
}

INSTANTIATE_TEST_SUITE_P(Arguments, TestInvalidMemoryWrites, ValuesIn(invalid_memory_write_inputs()),
    [](const TestParamInfo<InvalidBufferTransferInput> &info) { return info.param.name; });

std::vector<InvalidBufferTransferInput> invalid_memory_read_inputs()
{
    std::vector<InvalidBufferTransferInput> inputs;
    for (const MemoryLocation location : {MemoryLocation::MEMORY, MemoryLocation::DISK})
    {
        const std::string prefix{location == MemoryLocation::MEMORY ? "Memory" : "Disk"};
        inputs.push_back({prefix + "ZeroCount", location, 1, 0, 0, false});
        inputs.push_back({prefix + "OffsetOverflow", location, 2, 1, std::numeric_limits<std::uint64_t>::max(), false});
        inputs.push_back({prefix + "PastEnd", location, 1, 4, 1, false});
        inputs.push_back({prefix + "NullBuffer", location, 1, 1, 0, true});
    }
    return inputs;
}

class TestInvalidMemoryReads : public TestInvalidMemoryTransfer, public WithParamInterface<InvalidBufferTransferInput>
{
protected:
    void SetUp() override;
};

void TestInvalidMemoryReads::SetUp()
{
    TestMemory::SetUp();
    prepare_storage(GetParam().location);
}

TEST_P(TestInvalidMemoryReads, rejectsInvalidRangeWithoutChangingStorage)
{
    EXPECT_THROW(m_handle.to_memory(GetParam().null_buffer ? nullptr : m_buffer.data(), GetParam().size,
                     GetParam().count, GetParam().offset),
        std::logic_error);

    expect_unchanged_storage();
}

INSTANTIATE_TEST_SUITE_P(Arguments, TestInvalidMemoryReads, ValuesIn(invalid_memory_read_inputs()),
    [](const TestParamInfo<InvalidBufferTransferInput> &info) { return info.param.name; });

struct InvalidMemoryFillInput
{
    std::string name;
    MemoryLocation location;
    U16 size;
    std::uint64_t count;
    std::uint64_t offset;
};

void PrintTo(const InvalidMemoryFillInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<InvalidMemoryFillInput> invalid_memory_fill_inputs()
{
    std::vector<InvalidMemoryFillInput> inputs;
    for (const MemoryLocation location : {MemoryLocation::MEMORY, MemoryLocation::DISK})
    {
        const std::string prefix{location == MemoryLocation::MEMORY ? "Memory" : "Disk"};
        inputs.push_back({prefix + "SizeOverflow", location, 2, std::numeric_limits<std::uint64_t>::max(), 0});
        inputs.push_back({prefix + "PastEnd", location, 1, 1, 5});
    }
    return inputs;
}

class TestInvalidMemoryFills : public TestInvalidMemoryTransfer, public WithParamInterface<InvalidMemoryFillInput>
{
protected:
    void SetUp() override;
};

void TestInvalidMemoryFills::SetUp()
{
    TestMemory::SetUp();
    prepare_storage(GetParam().location);
}

TEST_P(TestInvalidMemoryFills, rejectsInvalidRangeWithoutChangingStorage)
{
    EXPECT_THROW(m_handle.set(0, GetParam().size, GetParam().count, GetParam().offset), std::logic_error);

    expect_unchanged_storage();
}

INSTANTIATE_TEST_SUITE_P(Arguments, TestInvalidMemoryFills, ValuesIn(invalid_memory_fill_inputs()),
    [](const TestParamInfo<InvalidMemoryFillInput> &info) { return info.param.name; });

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

class TestMemoryBackingFile : public TestMemory, public WithParamInterface<int>
{
};

TEST_P(TestMemoryBackingFile, availableSpaceOnlyNeedsToCoverMissingBytes)
{
    constexpr U16 BLOCK_SIZE{DISK_STORAGE_BLOCK_SIZE};
    constexpr std::uint64_t REQUESTED_BYTES{2 * BLOCK_SIZE};
    const std::size_t existing_bytes{static_cast<std::size_t>(static_cast<int>(REQUESTED_BYTES) + GetParam())};
    const std::filesystem::path file{m_directory / "backing.dat"};
    const std::vector<Byte> input(existing_bytes, 37);
    {
        std::ofstream stream{file, std::ios::binary};
        stream.write(reinterpret_cast<const char *>(input.data()), input.size());
    }

    const DiskSpaceFn disk_space{GetParam() < 0 ? one_byte_available : no_bytes_available};
    const MemoryHandle handle{memory_alloc(BLOCK_SIZE, 2, MemoryLocation::DISK, file, disk_space)};

    EXPECT_EQ(REQUESTED_BYTES, handle.size());
    std::vector<Byte> output(std::min<std::size_t>(existing_bytes, REQUESTED_BYTES));
    handle.to_memory(output.data(), 1, output.size(), 0);
    EXPECT_THAT(output, Each(37));
    memory_release(handle);
    EXPECT_EQ(std::max<std::uint64_t>(existing_bytes, REQUESTED_BYTES), std::filesystem::file_size(file));
    EXPECT_TRUE(exit_check().empty());
}

INSTANTIATE_TEST_SUITE_P(SmallerEqualLarger, TestMemoryBackingFile, Values(-1, 0, 1));

TEST_F(TestMemory, insufficientSpaceForBackingFileGrowthDoesNotChangeItOrLeakHandles)
{
    const std::filesystem::path file{m_directory / "backing.dat"};
    std::ofstream(file, std::ios::binary) << "1234";

    EXPECT_THROW(memory_alloc(1, 6, MemoryLocation::DISK, file, one_byte_available), MemoryError);

    EXPECT_EQ(4U, std::filesystem::file_size(file));
    EXPECT_TRUE(exit_check().empty());
    const MemoryHandle handle{memory_alloc(1, 5, MemoryLocation::DISK, file, one_byte_available)};
    EXPECT_EQ(1, handle.index);
    memory_release(handle);
    EXPECT_EQ(5U, std::filesystem::file_size(file));
}

TEST_F(TestMemory, newTemporaryFileRequiresItsFullSizeInAvailableSpace)
{
    EXPECT_THROW(memory_alloc(1, 2, MemoryLocation::DISK, {}, one_byte_available), MemoryError);
    EXPECT_TRUE(exit_check().empty());
    EXPECT_TRUE(std::filesystem::is_empty(m_directory));

    const MemoryHandle handle{memory_alloc(1, 1, MemoryLocation::DISK, {}, one_byte_available)};
    EXPECT_EQ(1, handle.index);
    memory_release(handle);
    EXPECT_TRUE(std::filesystem::is_empty(m_directory));
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
