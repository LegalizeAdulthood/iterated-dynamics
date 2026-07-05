// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/OrbitDelayTimer.h>
#include <ui/standard_orbit_plot.h>

#include <gtest/gtest.h>

#include <chrono>

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
    void SetUp() override
    {
        reset_orbit_delay_timer();
        s_input_check_count = 0;
    }
};

TEST_F(TestOrbitDelayTimer, armedDelayWaitsUntilDeadline)
{
    EXPECT_FALSE(wait_for_orbit_delay(DELAY_UNITS, no_input_pending));
    s_input_check_count = 0;

    const std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};

    EXPECT_FALSE(wait_for_orbit_delay(DELAY_UNITS, no_input_pending));

    const std::chrono::steady_clock::duration elapsed{std::chrono::steady_clock::now() - start};
    EXPECT_GE(elapsed, MINIMUM_DELAY);
    EXPECT_GT(s_input_check_count, 0);
}

TEST_F(TestOrbitDelayTimer, inputCallbackWakesPendingDelay)
{
    EXPECT_FALSE(wait_for_orbit_delay(DELAY_UNITS, no_input_pending));
    s_input_check_count = 0;

    EXPECT_TRUE(wait_for_orbit_delay(DELAY_UNITS, input_pending));

    EXPECT_GT(s_input_check_count, 0);
}

TEST_F(TestOrbitDelayTimer, shortDelayDoesNotPumpInputDuringSpin)
{
    EXPECT_FALSE(wait_for_orbit_delay(SHORT_DELAY_UNITS, no_input_pending));
    s_input_check_count = 0;

    EXPECT_FALSE(wait_for_orbit_delay(SHORT_DELAY_UNITS, no_input_pending));

    EXPECT_EQ(s_input_check_count, 0);
}

TEST_F(TestOrbitDelayTimer, displayOrbitDelayUses72HzMinimum)
{
    EXPECT_EQ(display_orbit_delay_units(0), 0);
    EXPECT_EQ(display_orbit_delay_units(MIN_DISPLAY_DELAY_UNITS - 1), 0);
    EXPECT_EQ(display_orbit_delay_units(MIN_DISPLAY_DELAY_UNITS), MIN_DISPLAY_DELAY_UNITS);
}

} // namespace id::test
