// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/OrbitDelayTimer.h>
#include <ui/standard_orbit_plot.h>

#include <gtest/gtest.h>

#include <chrono>
#include <ostream>

using namespace id::ui;

namespace id::test
{

namespace
{

constexpr unsigned long DELAY_UNITS = 500;
constexpr unsigned long SHORT_DELAY_UNITS = 1;
constexpr unsigned long DISPLAY_REFRESH_HZ = 72;
constexpr unsigned long MIN_DISPLAY_DELAY_UNITS = (1000000UL + DISPLAY_REFRESH_HZ * ORBIT_DELAY_UNIT_MICROSECONDS - 1) /
    (DISPLAY_REFRESH_HZ * ORBIT_DELAY_UNIT_MICROSECONDS);
constexpr std::chrono::milliseconds MINIMUM_DELAY{40};

int s_input_check_count{};

bool no_input_pending()
{
    ++s_input_check_count;
    return false;
}

bool input_pending()
{
    ++s_input_check_count;
    return true;
}

} // namespace

class TestOrbitDelayTimer : public testing::Test
{
protected:
    void SetUp() override;
    void arm(unsigned long delay);
};

void TestOrbitDelayTimer::SetUp()
{
    reset_orbit_delay_timer();
    s_input_check_count = 0;
}

void TestOrbitDelayTimer::arm(const unsigned long delay)
{
    ASSERT_FALSE(wait_for_orbit_delay(delay, no_input_pending));
    s_input_check_count = 0;
}

class TestLongOrbitDelayTimer : public TestOrbitDelayTimer
{
protected:
    void SetUp() override;
};

void TestLongOrbitDelayTimer::SetUp()
{
    TestOrbitDelayTimer::SetUp();
    arm(DELAY_UNITS);
}

class TestShortOrbitDelayTimer : public TestOrbitDelayTimer
{
protected:
    void SetUp() override;
};

void TestShortOrbitDelayTimer::SetUp()
{
    TestOrbitDelayTimer::SetUp();
    arm(SHORT_DELAY_UNITS);
}

TEST_F(TestLongOrbitDelayTimer, armedDelayWaitsUntilDeadline)
{
    const std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};

    EXPECT_FALSE(wait_for_orbit_delay(DELAY_UNITS, no_input_pending));

    const std::chrono::steady_clock::duration elapsed{std::chrono::steady_clock::now() - start};
    EXPECT_GE(elapsed, MINIMUM_DELAY);
    EXPECT_GT(s_input_check_count, 0);
}

TEST_F(TestLongOrbitDelayTimer, inputCallbackWakesPendingDelay)
{
    EXPECT_TRUE(wait_for_orbit_delay(DELAY_UNITS, input_pending));

    EXPECT_GT(s_input_check_count, 0);
}

TEST_F(TestShortOrbitDelayTimer, shortDelayDoesNotPumpInputDuringSpin)
{
    EXPECT_FALSE(wait_for_orbit_delay(SHORT_DELAY_UNITS, no_input_pending));

    EXPECT_EQ(s_input_check_count, 0);
}

struct DisplayOrbitDelayInput
{
    const char *name;
    unsigned long delay;
    unsigned long expected;
};

void PrintTo(const DisplayOrbitDelayInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestDisplayOrbitDelay : public TestOrbitDelayTimer, public testing::WithParamInterface<DisplayOrbitDelayInput>
{
};

TEST_P(TestDisplayOrbitDelay, uses72HzMinimum)
{
    EXPECT_EQ(GetParam().expected, display_orbit_delay_units(GetParam().delay));
}

INSTANTIATE_TEST_SUITE_P(Thresholds, TestDisplayOrbitDelay,
    testing::Values(DisplayOrbitDelayInput{"Zero", 0, 0},
        DisplayOrbitDelayInput{"BelowMinimum", MIN_DISPLAY_DELAY_UNITS - 1, 0},
        DisplayOrbitDelayInput{"AtMinimum", MIN_DISPLAY_DELAY_UNITS, MIN_DISPLAY_DELAY_UNITS}),
    [](const testing::TestParamInfo<DisplayOrbitDelayInput> &info) { return info.param.name; });

} // namespace id::test
