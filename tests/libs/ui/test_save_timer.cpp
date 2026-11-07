// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/save_timer.h>

#include <engine/calcfrac.h>
#include <io/save_timer.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <ctime>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;

namespace id::test
{

class TestSaveTimer : public testing::Test
{
protected:
    static constexpr long START{100};
    static constexpr long MINUTE{60 * CLOCKS_PER_SEC};

    SaveTimer m_timer;
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::IN_PROGRESS};
    ValueSaver<Passes> m_saved_passes{g_passes, Passes::SEQUENTIAL_SCAN};
    ValueSaver<int> m_saved_row{g_row, 5};
    ValueSaver<TimedSave> m_saved_timed_save{g_timed_save, TimedSave::NONE};
};

TEST_F(TestSaveTimer, disabledTimerLeavesPendingSaveUntouched)
{
    g_timed_save = TimedSave::STARTED;

    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    m_timer.start(0, START);
    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    EXPECT_EQ(TimedSave::STARTED, g_timed_save);
}

TEST_F(TestSaveTimer, stoppedTimerLeavesPendingSaveUntouched)
{
    m_timer.start(1, START);
    g_timed_save = TimedSave::STARTED;

    m_timer.stop();

    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    EXPECT_EQ(TimedSave::STARTED, g_timed_save);
}

TEST_F(TestSaveTimer, completedCalculationSavesAtTheDeadline)
{
    g_calc_status = CalcStatus::COMPLETED;
    m_timer.start(1, START);

    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE - 1));
    EXPECT_TRUE(m_timer.auto_save_needed(START + MINUTE));
    EXPECT_EQ(TimedSave::STARTED, g_timed_save);
}

TEST_F(TestSaveTimer, pendingSaveIsReportedBeforeTheDeadline)
{
    m_timer.start(1, START);
    g_timed_save = TimedSave::STARTED;

    EXPECT_TRUE(m_timer.auto_save_needed(START));
}

TEST_F(TestSaveTimer, finalSaveIsNotReportedAsANewRequest)
{
    m_timer.start(1, START);
    g_timed_save = TimedSave::FINAL;

    EXPECT_FALSE(m_timer.auto_save_needed(START));
    EXPECT_EQ(TimedSave::FINAL, g_timed_save);
}

TEST_F(TestSaveTimer, negativeIntervalUsesItsMagnitudeWithoutChangingResaveState)
{
    ValueSaver saved_resave{g_resave_flag, TimedSave::FINAL};
    ValueSaver saved_started_resaves{g_started_resaves, true};
    g_calc_status = CalcStatus::COMPLETED;
    m_timer.start(-2, START);

    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    EXPECT_TRUE(m_timer.auto_save_needed(START + 2 * MINUTE));
    EXPECT_EQ(TimedSave::FINAL, g_resave_flag);
    EXPECT_TRUE(g_started_resaves);
}

TEST_F(TestSaveTimer, restartClearsTheOldDeadlineAndPendingRow)
{
    m_timer.start(1, START);
    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));

    m_timer.start(2, START + MINUTE);
    ++g_row;

    EXPECT_FALSE(m_timer.auto_save_needed(START + 3 * MINUTE - 1));
    EXPECT_FALSE(m_timer.auto_save_needed(START + 3 * MINUTE));
    ++g_row;
    EXPECT_TRUE(m_timer.auto_save_needed(START + 3 * MINUTE));
}

TEST_F(TestSaveTimer, zeroIntervalDisablesAPendingRow)
{
    m_timer.start(1, START);
    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));

    m_timer.start(0, START + MINUTE);
    ++g_row;

    EXPECT_FALSE(m_timer.auto_save_needed(START + 2 * MINUTE));
    EXPECT_EQ(TimedSave::NONE, g_timed_save);
}

class TestSaveTimerRowBoundary : public TestSaveTimer, public testing::WithParamInterface<Passes>
{
};

TEST_P(TestSaveTimerRowBoundary, expirationWaitsForTheNextRow)
{
    g_passes = GetParam();
    m_timer.start(1, START);

    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE - 1));
    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE + 1));
    ++g_row;
    EXPECT_TRUE(m_timer.auto_save_needed(START + MINUTE + 1));
    EXPECT_EQ(TimedSave::STARTED, g_timed_save);
}

INSTANTIATE_TEST_SUITE_P(
    Passes, TestSaveTimerRowBoundary, testing::Values(Passes::SEQUENTIAL_SCAN, Passes::SOLID_GUESS));

class TestSaveTimerOtherPasses : public TestSaveTimer, public testing::WithParamInterface<Passes>
{
};

TEST_P(TestSaveTimerOtherPasses, waitsForCalculationCompletion)
{
    g_passes = GetParam();
    m_timer.start(1, START);

    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    ++g_row;
    EXPECT_FALSE(m_timer.auto_save_needed(START + MINUTE));
    g_calc_status = CalcStatus::COMPLETED;
    EXPECT_TRUE(m_timer.auto_save_needed(START + MINUTE));
}

INSTANTIATE_TEST_SUITE_P(Passes, TestSaveTimerOtherPasses,
    testing::Values(
        Passes::NONE, Passes::BOUNDARY_TRACE, Passes::THREE_D, Passes::TESSERAL, Passes::DIFFUSION, Passes::ORBITS));

} // namespace id::test
