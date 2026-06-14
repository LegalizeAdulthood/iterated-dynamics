// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/tab_display.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/LogicalScreen.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/lorenz.h>
#include <io/encoder.h>
#include <math/big.h>
#include <misc/ValueSaver.h>
#include <ui/id_keys.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TabDisplayParamSaver
{
public:
    TabDisplayParamSaver()
    {
        std::copy(&g_params[0], &g_params[MAX_PARAMS], m_params.begin());
    }

    ~TabDisplayParamSaver()
    {
        std::copy(m_params.begin(), m_params.end(), &g_params[0]);
    }

private:
    std::array<double, MAX_PARAMS> m_params{};
};

class TestTabDisplay : public Test
{
protected:
    void SetUp() override
    {
        std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0);
        g_image_region = {{-2.0, -1.5}, {1.0, 1.5}, {-2.0, 1.5}};
        g_logical_screen = {640, 480, 0, 0, 639.0, 479.0};
        g_video_entry.x_dots = 0;
        g_screen_aspect = DEFAULT_ASPECT;
        g_aspect_drift = DEFAULT_ASPECT_DRIFT;
    }

    MockDriver m_driver;
    const std::string m_driver_name{"mock"};
    const std::string m_driver_description{"test driver"};
    TabDisplayParamSaver m_param_saver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<FractalSpecific *> m_saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<CalcStatus> m_saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<Passes> m_saved_passes{g_passes, Passes::NONE};
    ValueSaver<ImageRegion> m_saved_image_region{g_image_region};
    ValueSaver<LogicalScreen> m_saved_logical_screen{g_logical_screen};
    ValueSaver<VideoInfo> m_saved_video_entry{g_video_entry};
    ValueSaver<float> m_saved_screen_aspect{g_screen_aspect};
    ValueSaver<float> m_saved_aspect_drift{g_aspect_drift};
    ValueSaver<long> m_saved_calc_time{g_calc_time, 123};
    ValueSaver<long> m_saved_color_iter{g_color_iter, 12};
    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 150};
    ValueSaver<double> m_saved_magnitude_limit{g_magnitude_limit, 4.0};
    ValueSaver<BatchMode> m_saved_init_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<BFMathType> m_saved_bf_math{g_bf_math, BFMathType::NONE};
    ValueSaver<Display3DMode> m_saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<std::filesystem::path> m_saved_save_filename{g_save_filename, "test.gif"};
    ValueSaver<Inversion> m_saved_inversion{g_inversion, Inversion{}};
    ValueSaver<UserData> m_saved_user{g_user, UserData{}};
};

TEST_F(TestTabDisplay, displaysMainStatusScreen)
{
    std::vector<std::string> output;

    EXPECT_CALL(m_driver, stack_screen());
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _))
        .Times(AnyNumber())
        .WillRepeatedly(Invoke([&output](int, int, int, const char *text) { output.emplace_back(text); }));
    EXPECT_CALL(m_driver, get_name()).WillRepeatedly(ReturnRef(m_driver_name));
    EXPECT_CALL(m_driver, get_description()).WillRepeatedly(ReturnRef(m_driver_description));
    EXPECT_CALL(m_driver, hide_text_cursor());
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));
    EXPECT_CALL(m_driver, unstack_screen());

    EXPECT_EQ(0, tab_display());

    EXPECT_THAT(output, Contains(StrEq("Fractal type:")));
    EXPECT_THAT(output, Contains(StrEq("Image completed")));
    EXPECT_THAT(output, Contains(StrEq("Savename: ")));
    EXPECT_THAT(output, Contains(StrEq("test.gif")));
    EXPECT_THAT(output, Contains(StrEq("Driver: mock, test driver")));
    EXPECT_THAT(output, Contains(StrEq("Current (Max) Iteration: ")));
    EXPECT_THAT(output, Contains(StrEq("12 (150)")));
    EXPECT_THAT(output, Contains(StrEq("4.000000")));
}

} // namespace id::test
