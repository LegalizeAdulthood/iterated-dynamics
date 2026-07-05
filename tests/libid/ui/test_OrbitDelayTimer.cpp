// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/OrbitDelayTimer.h>

#include <gtest/gtest.h>

#include <chrono>

using namespace id::ui;

namespace id::test
{

namespace
{

constexpr unsigned long DELAY_UNITS = 500;
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

} // namespace id::test
