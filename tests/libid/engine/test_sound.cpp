// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/OrbitPlot.h>
#include <engine/sound.h>
#include <engine/wait_until.h>

#include "MockDriver.h"

#include <fractals/lorenz.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

class TestSound : public Test
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_orbit_save_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_saved_orbit_delay{g_orbit_delay, 0};
};

TEST_F(TestSound, pendingInputDoesNotSuppressSpeakerTone)
{
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, sound_on(440)).WillOnce(Return(false));

    write_sound(440);
}

TEST_F(TestSound, supportedSoundWaitDoesNotPollInput)
{
    g_orbit_delay = 20;
    reset_wait_until();
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, sound_on(440)).Times(2).WillRepeatedly(Return(true));

    write_sound(440);
    write_sound(440);
    reset_wait_until();
}

} // namespace id::test
