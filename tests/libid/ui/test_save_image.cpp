// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/save_image.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>
#include <ui/evolve.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestEvolutionSaveSnapshot : public Test
{
protected:
    void SetUp() override;

    ValueSaver<EvolutionModeFlags> m_saved_mode{
        g_evolving, EvolutionModeFlags::FIELD_MAP | EvolutionModeFlags::NO_GROUT};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::RESUMABLE};
    ValueSaver<bool> m_saved_have_info{g_have_evolve_info, false};
    ValueSaver<EvolutionInfo> m_saved_info{g_evolve_info, EvolutionInfo{}};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{8, 6, 2, 1, 7, 5}};
    ValueSaver<int> m_saved_grid{g_evolve_image_grid_size, 3};
    ValueSaver<unsigned int> m_saved_seed{g_evolve_this_generation_random_seed, 0x1ABCDU};
    ValueSaver<double> m_saved_mutation{g_evolve_max_random_mutation, 0.25};
    ValueSaver<double> m_saved_x_range{g_evolve_x_parameter_range, 4.0};
    ValueSaver<double> m_saved_y_range{g_evolve_y_parameter_range, 3.0};
    ValueSaver<double> m_saved_x_offset{g_evolve_x_parameter_offset, -2.0};
    ValueSaver<double> m_saved_y_offset{g_evolve_y_parameter_offset, -1.5};
    ValueSaver<char> m_saved_discrete_x{g_evolve_discrete_x_parameter_offset, static_cast<char>(-7)};
    ValueSaver<char> m_saved_discrete_y{g_evolve_discrete_y_parameter_offset, static_cast<char>(9)};
    ValueSaver<int> m_saved_px{g_evolve_param_grid_x, 1};
    ValueSaver<int> m_saved_py{g_evolve_param_grid_y, 2};
    ValueSaver<Variations> m_saved_gene{g_gene_bank[2].mutate, Variations::Y};
    ValueSaver<Variations> m_saved_last_gene{g_gene_bank[NUM_GENES - 1].mutate, Variations::WEIGHTED_RANDOM};
};

void TestEvolutionSaveSnapshot::SetUp()
{
    g_evolve_info.evolving = 1;
    g_evolve_info.image_grid_size = 5;
    g_evolve_info.this_generation_random_seed = 0x9876;
    g_evolve_info.max_random_mutation = 0.5;
    g_evolve_info.x_parameter_range = 8;
    g_evolve_info.y_parameter_range = 6;
    g_evolve_info.x_parameter_offset = -4;
    g_evolve_info.y_parameter_offset = -3;
    g_evolve_info.discrete_x_parameter_offset = -11;
    g_evolve_info.discrete_y_parameter_offset = 13;
    g_evolve_info.px = 3;
    g_evolve_info.py = 4;
    g_evolve_info.screen_x_offset = 5;
    g_evolve_info.screen_y_offset = 6;
    g_evolve_info.x_dots = 0xFFFF;
    g_evolve_info.y_dots = 0x8000;
    g_evolve_info.count = 17;
    std::fill(std::begin(g_evolve_info.mutate), std::end(g_evolve_info.mutate), -1);
    g_evolve_info.future[0] = -123;
    g_evolve_info.future[std::size(g_evolve_info.future) - 1] = 456;
}

class TestDisabledEvolutionSnapshot : public TestEvolutionSaveSnapshot, public WithParamInterface<EvolutionModeFlags>
{
};

TEST_P(TestDisabledEvolutionSnapshot, omitsTheRecordWithoutFieldMap)
{
    g_evolving = GetParam();
    g_have_evolve_info = true;

    EXPECT_FALSE(capture_evolution_info());
}

INSTANTIATE_TEST_SUITE_P(Modes, TestDisabledEvolutionSnapshot,
    Values(EvolutionModeFlags::NONE, EvolutionModeFlags::RAND_WALK, EvolutionModeFlags::RAND_PARAM,
        EvolutionModeFlags::NO_GROUT, EvolutionModeFlags::PARAM_BOX));

class TestLiveEvolutionSnapshot : public TestEvolutionSaveSnapshot,
                                  public WithParamInterface<std::pair<bool, CalcStatus>>
{
};

TEST_P(TestLiveEvolutionSnapshot, capturesLiveControlsAndCurrentGeneMutations)
{
    g_have_evolve_info = GetParam().first;
    g_calc_status = GetParam().second;

    const std::optional<EvolutionInfo> info{capture_evolution_info()};

    ASSERT_TRUE(info);
    EXPECT_EQ(9, info->evolving);
    EXPECT_EQ(3, info->image_grid_size);
    EXPECT_EQ(0xABCD, info->this_generation_random_seed);
    EXPECT_EQ(9, info->count);
    EXPECT_EQ(0.25, info->max_random_mutation);
    EXPECT_EQ(4.0, info->x_parameter_range);
    EXPECT_EQ(3.0, info->y_parameter_range);
    EXPECT_EQ(-2.0, info->x_parameter_offset);
    EXPECT_EQ(-1.5, info->y_parameter_offset);
    EXPECT_EQ(-7, info->discrete_x_parameter_offset);
    EXPECT_EQ(9, info->discrete_y_parameter_offset);
    EXPECT_EQ(1, info->px);
    EXPECT_EQ(2, info->py);
    EXPECT_EQ(2, info->screen_x_offset);
    EXPECT_EQ(1, info->screen_y_offset);
    EXPECT_EQ(8, info->x_dots);
    EXPECT_EQ(6, info->y_dots);
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EXPECT_EQ(static_cast<std::int16_t>(g_gene_bank[i].mutate), info->mutate[i]);
    }
    EXPECT_TRUE(
        std::all_of(std::begin(info->future), std::end(info->future), [](const auto value) { return value == 0; }));
}

INSTANTIATE_TEST_SUITE_P(States, TestLiveEvolutionSnapshot,
    Values(std::make_pair(false, CalcStatus::RESUMABLE), std::make_pair(false, CalcStatus::IN_PROGRESS),
        std::make_pair(false, CalcStatus::COMPLETED), std::make_pair(true, CalcStatus::COMPLETED)));

class TestCachedEvolutionSnapshot : public TestEvolutionSaveSnapshot, public WithParamInterface<CalcStatus>
{
};

TEST_P(TestCachedEvolutionSnapshot, retainsCachedProgressAndRefreshesEveryGeneMutation)
{
    g_have_evolve_info = true;
    g_calc_status = GetParam();
    EvolutionInfo expected{g_evolve_info};
    for (int i = 0; i < NUM_GENES; ++i)
    {
        expected.mutate[i] = static_cast<std::int16_t>(g_gene_bank[i].mutate);
    }

    const std::optional<EvolutionInfo> info{capture_evolution_info()};

    ASSERT_TRUE(info);
    EXPECT_EQ(expected, *info);
    EXPECT_EQ(17, info->count);
    EXPECT_EQ(-123, info->future[0]);
    EXPECT_EQ(456, info->future[std::size(info->future) - 1]);
}

INSTANTIATE_TEST_SUITE_P(States, TestCachedEvolutionSnapshot, Values(CalcStatus::RESUMABLE, CalcStatus::IN_PROGRESS));

} // namespace id::test
