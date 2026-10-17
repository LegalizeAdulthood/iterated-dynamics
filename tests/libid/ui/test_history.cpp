// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include <ui/history.h>

#include "MockDriver.h"
#include "test_data.h"

#include <engine/calcfrac.h>
#include <engine/fractal_type.h>
#include <engine/ImageRegion.h>
#include <engine/spindac.h>
#include <engine/UserData.h>
#include <fractals/fractalp.h>
#include <io/CurrentPathSaver.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <math/big.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ostream>
#include <sstream>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::test::data;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestHistoryState : public Test
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_max_image_history{g_max_image_history, 3};
    ValueSaver<int> m_saved_history_ptr{g_history_ptr, -1};
    ValueSaver<bool> m_saved_history_flag{g_history_flag, false};
    ValueSaver<Version> m_saved_version{g_version, parse_legacy_version(1730)};
    ValueSaver<Version> m_saved_file_version{g_file_version, parse_legacy_version(1730)};
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<FractalSpecific *> m_saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<DebugFlags> m_saved_debug_flag{g_debug_flag, DebugFlags::NONE};
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<long> m_saved_iterations{g_max_iterations, 150};
    ValueSaver<double> m_saved_parameter{g_params[0], 0.0};
    ValueSaver<unsigned char> m_saved_palette{g_dac_box[0][0], 0};
    ValueSaver<bool> m_saved_legacy_float{g_user.legacy_float, false};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{-2.0, -1.5}, {1.0, 1.5}, {-2.0, -1.5}}};
};

void TestHistoryState::SetUp()
{
    history_init();
}

class TestHistory : public TestHistoryState
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    CurrentPathSaver m_current_path{ID_TEST_DATA_DIR};
};

void TestHistory::SetUp()
{
    TestHistoryState::SetUp();
    clear_save_library();
    set_save_library(ID_TEST_DATA_DIR);
}

void TestHistory::TearDown()
{
    clear_save_library();
}

struct HistoryChange
{
    const char *name;
    void (*apply)();
    bool changed;
};

void PrintTo(const HistoryChange &value, std::ostream *str)
{
    *str << value.name;
}

static const HistoryChange HISTORY_CHANGES[]{
    {"unchanged", [] {}, false},
    {"iterations", [] { g_max_iterations = 300; }, true},
    {"coordinates", [] { g_image_region.m_min.x = -1.25; }, true},
    {"parameter", [] { g_params[0] = 1.25; }, true},
    {"fractal type", [] { set_fractal_type(FractalType::JULIA); }, true},
    {"palette", [] { g_dac_box[0][0] = 31; }, true},
    {"version", [] { g_version = parse_legacy_version(1800); }, true},
    {"file version", [] { g_file_version = parse_legacy_version(1800); }, true},
    {"legacy float", [] { g_user.legacy_float = true; }, true},
};

class TestHistoryInsertion : public TestHistoryState, public WithParamInterface<HistoryChange>
{
};

TEST_P(TestHistoryInsertion, recordsChangesAndSkipsDuplicates)
{
    save_history_info();
    ASSERT_EQ(0, g_history_ptr);
    GetParam().apply();

    save_history_info();

    const int expected = GetParam().changed ? 1 : 0;
    EXPECT_EQ(expected, g_history_ptr);
    EXPECT_FALSE(g_history_flag);

    save_history_info();

    EXPECT_EQ(expected, g_history_ptr);
}

TEST_P(TestHistoryInsertion, historyNavigationSuppressesOnlyOneSave)
{
    save_history_info();
    GetParam().apply();
    g_history_flag = true;

    save_history_info();

    EXPECT_EQ(0, g_history_ptr);
    EXPECT_FALSE(g_history_flag);

    save_history_info();

    EXPECT_EQ(GetParam().changed ? 1 : 0, g_history_ptr);
}

INSTANTIATE_TEST_SUITE_P(ImageStates, TestHistoryInsertion, ValuesIn(HISTORY_CHANGES));

class TestHistoryWraparound : public TestHistoryState, public WithParamInterface<int>
{
};

TEST_P(TestHistoryWraparound, wrapsChangedStatesWithoutConsumingSlotsForDuplicates)
{
    g_max_image_history = GetParam();
    history_init();
    save_history_info();
    ASSERT_EQ(0, g_history_ptr);
    g_max_iterations = 300;

    save_history_info();

    ASSERT_EQ(1, g_history_ptr);
    g_max_iterations = 450;
    save_history_info();
    ASSERT_EQ(GetParam() == 2 ? 0 : 2, g_history_ptr);
    g_max_iterations = 600;
    save_history_info();
    const int expected = GetParam() == 2 ? 1 : 0;
    EXPECT_EQ(expected, g_history_ptr);

    save_history_info();

    EXPECT_EQ(expected, g_history_ptr);
}

INSTANTIATE_TEST_SUITE_P(Capacities, TestHistoryWraparound, Values(2, 3));

static std::string read_file_contents(const std::filesystem::path &path)
{
    const std::ifstream file{path};
    std::ostringstream str;
    str << file.rdbuf();
    return str.str();
}

TEST_F(TestHistory, restoresVersionAndFileVersion)
{
    g_version = parse_legacy_version(1730);
    g_file_version = parse_legacy_version(2004);
    save_history_info();
    set_fractal_type(FractalType::POPCORN);
    complete_fractal_type_selection(FractalType::MANDEL, true);
    ASSERT_EQ(current_id_version(), g_version);
    ASSERT_EQ(current_id_version(), g_file_version);
    g_max_iterations = 300;
    save_history_info();
    ASSERT_EQ(1, g_history_ptr);

    EXPECT_CALL(m_driver, write_palette()).Times(2);
    EXPECT_CALL(m_driver, delay(255)).Times(2);

    restore_history_info(0);

    EXPECT_EQ(parse_legacy_version(1730), g_version);
    EXPECT_EQ(parse_legacy_version(2004), g_file_version);
    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_EQ(150, g_max_iterations);

    restore_history_info(1);

    EXPECT_EQ(current_id_version(), g_version);
    EXPECT_EQ(current_id_version(), g_file_version);
    EXPECT_EQ(FractalType::POPCORN, g_fractal_type);
    EXPECT_EQ(300, g_max_iterations);
}

TEST_F(TestHistory, dumpsVersionAndFileVersion)
{
    const std::filesystem::path history_file{std::filesystem::path{ID_TEST_DATA_DIR} / "debug/history.json"};
    std::filesystem::remove(history_file);
    g_debug_flag = DebugFlags::HISTORY_DUMP_JSON;
    g_version = Version{1, 2, 3, 4, false};
    g_file_version = parse_legacy_version(2004);

    save_history_info();

    const std::string text{read_file_contents(history_file)};
    EXPECT_THAT(text, HasSubstr(R"json("version":{"major":1,"minor":2,"patch":3,"tweak":4,"legacy":0})json"));
    EXPECT_THAT(text, HasSubstr(R"json("file_version":{"major":20,"minor":4,"patch":0,"tweak":0,"legacy":1})json"));
    EXPECT_THAT(text, Not(HasSubstr(R"json("release":)json")));
}

class TestHistoryLegacyFloat : public TestHistory, public WithParamInterface<bool>
{
};

TEST_P(TestHistoryLegacyFloat, restoresLegacyFloat)
{
    g_user.legacy_float = GetParam();
    save_history_info();
    g_user.legacy_float = !GetParam();

    EXPECT_CALL(m_driver, write_palette());
    EXPECT_CALL(m_driver, delay(255));

    restore_history_info(g_history_ptr);

    EXPECT_EQ(GetParam(), g_user.legacy_float);
}

INSTANTIATE_TEST_SUITE_P(LegacyFloatChoices, TestHistoryLegacyFloat, Bool());

} // namespace id::test
