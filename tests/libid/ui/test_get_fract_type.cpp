// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include <ui/get_fract_type.h>

#include "MockDriver.h"

#include <engine/cmdfiles.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/Potential.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractalp.h>
#include <fractals/julibrot.h>
#include <fractals/lorenz.h>
#include <helpdefs.h>
#include <io/loadfile.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <ui/id_keys.h>
#include <ui/menu_handler.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <deque>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{
namespace
{

class TestFractalTypeDialog : public TestWithParam<bool>
{
protected:
    void SetUp() override;
    void TearDown() override;
    void queue_type();
    void select(bool accepted);
    void expect_version(const Version &version);

    MockDriver m_driver;
    std::deque<int> m_keys;

private:
    FractalSpecific *m_target{get_fractal_specific(FractalType::POPCORN)};
    std::array<double, MAX_PARAMS> m_params{};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<HelpLabels> m_saved_help{m_target->help_formula, HelpLabels::NONE};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<UserData> m_saved_user{g_user};
    ValueSaver<Viewport> m_saved_viewport{g_viewport, Viewport{}};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<Inversion> m_saved_inversion{g_inversion, Inversion{}};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<Display3DMode> m_saved_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_saved_julibrot{g_julibrot, false};
    ValueSaver<int> m_saved_adapter{g_adapter, 0};
    ValueSaver<int> m_saved_init_mode{g_init_mode, 0};
    ValueSaver<decltype(g_param_text)> m_saved_param_text{g_param_text};
};

void TestFractalTypeDialog::SetUp()
{
    std::copy_n(g_params, MAX_PARAMS, m_params.begin());
    ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET, cmd_arg("reset=1730", CmdFile::AT_AFTER_STARTUP));
    set_fractal_type(FractalType::MANDEL);
    g_user = UserData{-1, 0, 1, CalcMode::ONE_PASS, 0, false};
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
    EXPECT_CALL(m_driver, move_cursor(_, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillRepeatedly(Return(1));
    EXPECT_CALL(m_driver, get_key())
        .WillRepeatedly(
            [this]() -> int
            {
                if (m_keys.empty())
                {
                    ADD_FAILURE() << "Unexpected request for a type-selection key";
                    return ID_KEY_ESC;
                }
                const int key = m_keys.front();
                m_keys.pop_front();
                return key;
            });
}

void TestFractalTypeDialog::TearDown()
{
    std::copy(m_params.begin(), m_params.end(), g_params);
}

void TestFractalTypeDialog::queue_type()
{
    const std::string name{m_target->name};
    m_keys.assign(name.begin(), name.end());
    m_keys.push_back(ID_KEY_ENTER);
}

void TestFractalTypeDialog::select(const bool accepted)
{
    if (GetParam())
    {
        EXPECT_CALL(m_driver, stack_screen());
        if (accepted)
        {
            EXPECT_CALL(m_driver, discard_screen());
        }
        else
        {
            EXPECT_CALL(m_driver, unstack_screen());
        }
        MainContext context{};
        EXPECT_EQ(accepted ? MainState::IMAGE_START : MainState::NOTHING, request_fractal_type(context));
    }
    else
    {
        EXPECT_EQ(accepted ? 0 : -1, get_fract_type());
    }
    EXPECT_TRUE(m_keys.empty());
}

void TestFractalTypeDialog::expect_version(const Version &version)
{
    EXPECT_EQ(version, g_version);
    EXPECT_EQ(version, g_file_version);
}

TEST_P(TestFractalTypeDialog, acceptedSelectionResetsBothVersions)
{
    queue_type();
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ENTER));

    select(true);

    EXPECT_EQ(FractalType::POPCORN, g_fractal_type);
    expect_version(current_id_version());
}

TEST_P(TestFractalTypeDialog, canceledTypeSelectionPreservesBothVersions)
{
    m_keys.push_back(ID_KEY_ESC);

    select(false);

    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    expect_version(parse_legacy_version(1730));
}

TEST_P(TestFractalTypeDialog, canceledParametersPreserveBothVersions)
{
    queue_type();
    m_keys.push_back(ID_KEY_ESC);
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ESC));

    select(false);

    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    expect_version(parse_legacy_version(1730));
}

TEST_P(TestFractalTypeDialog, editingParametersPreservesBothVersions)
{
    set_fractal_type(FractalType::POPCORN);
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, get_fract_params(true));

    expect_version(parse_legacy_version(1730));
}

INSTANTIATE_TEST_SUITE_P(EntryPoints, TestFractalTypeDialog, Bool());

} // namespace
} // namespace id::test
