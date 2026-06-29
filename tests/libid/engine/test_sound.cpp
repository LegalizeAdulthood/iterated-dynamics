// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/orbit.h>
#include <engine/sound.h>

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

namespace
{

bool input_pending()
{
    return true;
}

bool input_not_pending()
{
    return false;
}

} // namespace

class TestSound : public Test
{
protected:
    void TearDown() override
    {
        set_sound_input_pending(m_saved_input_pending);
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<int> m_saved_orbit_save_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_saved_orbit_delay{g_orbit_delay, 0};
    SoundInputPendingFn m_saved_input_pending{set_sound_input_pending(nullptr)};
};

TEST_F(TestSound, pendingInputSuppressesSpeakerTone)
{
    set_sound_input_pending(input_pending);

    write_sound(440);
}

TEST_F(TestSound, noPendingInputPlaysSpeakerTone)
{
    set_sound_input_pending(input_not_pending);

    EXPECT_CALL(m_driver, sound_on(440)).WillOnce(Return(false));

    write_sound(440);
}

} // namespace id::test
