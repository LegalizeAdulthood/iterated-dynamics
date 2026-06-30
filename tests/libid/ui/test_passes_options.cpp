// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/passes_options.h>

#include "MockDriver.h"

#include <engine/orbit.h>
#include <engine/sticky_orbits.h>
#include <engine/UserData.h>
#include <fractals/lorenz.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestPassesOptions : public Test
{
protected:
    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
    ValueSaver<int> m_saved_orbit_delay{g_orbit_delay, 100};
    ValueSaver<int> m_saved_orbit_skip_points{g_orbit_skip_points, -32767};
    ValueSaver<long> m_saved_orbit_interval{g_orbit_interval, 1L};
    ValueSaver<bool> m_saved_keep_screen_coords{g_keep_screen_coords, false};
    ValueSaver<OrbitDrawMode> m_saved_draw_mode{g_draw_mode, OrbitDrawMode::RECTANGLE};
    ValueSaver<bool> m_saved_set_orbit_corners{g_set_orbit_corners, false};
};

TEST_F(TestPassesOptions, displaysOrbitDelayInsteadOfSkipPoints)
{
    g_user.periodicity_value = 1;
    std::vector<std::string> output;

    EXPECT_CALL(m_driver, set_clear());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _))
        .Times(AnyNumber())
        .WillRepeatedly(Invoke([&output](int, int, int, const char *text) { output.emplace_back(text); }));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, passes_options());

    EXPECT_THAT(output, Contains(StrEq("100   ")));
    EXPECT_EQ(100, g_orbit_delay);
    EXPECT_EQ(100, g_orbit_skip_points);
}

} // namespace id::test
