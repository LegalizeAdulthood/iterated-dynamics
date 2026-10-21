// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <io/library.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>
#include <ui/request_memory.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestRequestMemory : public Test
{
protected:
    void SetUp() override
    {
        init_memory();
        m_directory = std::filesystem::temp_directory_path() /
            ("id-request-memory-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directory(m_directory);
        g_temp_dir = m_directory / "missing";
        set_save_library(m_directory);
    }

    void TearDown() override
    {
        EXPECT_TRUE(exit_check().empty());
        clear_save_library();
        std::filesystem::remove_all(m_directory);
    }

    void expect_dialogs(const int count, const bool problem = true)
    {
        EXPECT_CALL(m_driver, stack_screen()).Times(count);
        EXPECT_CALL(m_driver, unstack_screen()).Times(count);
        EXPECT_CALL(m_driver, move_cursor(_, _)).Times(count);
        EXPECT_CALL(m_driver, hide_text_cursor()).Times(count);
        EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(count);
        EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM)).Times(problem ? count : 0);
        EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    std::filesystem::path m_directory;
};

TEST_F(TestRequestMemory, successDoesNotPollInput)
{
    const MemoryHandle handle{request_memory(1, 16, MemoryLocation::MEMORY)};

    EXPECT_TRUE(handle);
    memory_release(handle);
}

TEST_F(TestRequestMemory, retryOccursAfterReturningToUiAndCorrectingTheFailure)
{
    expect_dialogs(1);
    EXPECT_CALL(m_driver, get_key())
        .WillOnce(Invoke(
            []
            {
                EXPECT_TRUE(exit_check().empty());
                std::filesystem::create_directory(g_temp_dir);
                return ID_KEY_ENTER;
            }));

    const MemoryHandle handle{request_memory(1, 4096, MemoryLocation::DISK)};

    EXPECT_EQ(1, handle.index);
    EXPECT_EQ(MemoryLocation::DISK, memory_type(handle));
    memory_release(handle);
    EXPECT_TRUE(std::filesystem::is_empty(g_temp_dir));
}

TEST_F(TestRequestMemory, cancellationAbandonsRequestWithoutLeakingAndAllowsLaterRequest)
{
    expect_dialogs(1);
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    EXPECT_FALSE(request_memory(1, 4096, MemoryLocation::DISK));

    EXPECT_TRUE(exit_check().empty());
    std::filesystem::create_directory(g_temp_dir);
    const MemoryHandle handle{request_memory(1, 4096, MemoryLocation::DISK)};
    EXPECT_EQ(1, handle.index);
    memory_release(handle);
}

TEST_F(TestRequestMemory, batchFailureReportsAndCancelsWithoutInput)
{
    g_init_batch = BatchMode::NORMAL;

    EXPECT_FALSE(request_memory(1, 4096, MemoryLocation::DISK));

    EXPECT_EQ(BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE, g_init_batch);
    std::ifstream file{m_directory / "debug/stopmsg.txt"};
    ASSERT_TRUE(file.good());
    const std::string message{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    EXPECT_THAT(message, HasSubstr("Allocating 4096 bytes of disk failed"));
}

TEST_F(TestRequestMemory, uiOwnsAllocationStatistics)
{
    const std::filesystem::path file{m_directory / "backing.dat"};
    std::ofstream(file, std::ios::binary) << "header";
    g_debug_flag = DebugFlags::DISPLAY_MEMORY_STATISTICS;
    expect_dialogs(2, false);
    EXPECT_CALL(m_driver, get_key()).WillRepeatedly(Return(ID_KEY_ENTER));
    EXPECT_CALL(m_driver, put_string(_, _, _, HasSubstr("Asked for memory, allocated 64 bytes of disk")));

    const MemoryHandle handle{request_memory(1, 64, MemoryLocation::MEMORY, file)};

    EXPECT_EQ(MemoryLocation::DISK, memory_type(handle));
    memory_release(handle);
    EXPECT_TRUE(std::filesystem::exists(file));
}

} // namespace id::test
