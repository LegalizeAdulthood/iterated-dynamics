// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/restore_image.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/Viewport.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>
#include <ui/evolve.h>
#include <ui/save_image.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <tuple>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestEvolutionRestore : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    EvolutionInfo m_info{};
    GeneBase m_genes[NUM_GENES]{};
    ValueSaver<EvolutionModeFlags> m_saved_mode{g_evolving, EvolutionModeFlags::FIELD_MAP};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::RESUMABLE};
    ValueSaver<bool> m_saved_have_info{g_have_evolve_info, true};
    ValueSaver<EvolutionInfo> m_saved_info{g_evolve_info, EvolutionInfo{}};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{1, 1, 0, 0, 0, 0}};
    ValueSaver<Viewport> m_saved_viewport{g_viewport, Viewport{}};
    ValueSaver<int> m_saved_grid{g_evolve_image_grid_size};
    ValueSaver<unsigned int> m_saved_seed{g_evolve_this_generation_random_seed};
    ValueSaver<double> m_saved_mutation{g_evolve_max_random_mutation};
    ValueSaver<double> m_saved_x_range{g_evolve_x_parameter_range};
    ValueSaver<double> m_saved_y_range{g_evolve_y_parameter_range};
    ValueSaver<double> m_saved_x_offset{g_evolve_x_parameter_offset};
    ValueSaver<double> m_saved_y_offset{g_evolve_y_parameter_offset};
    ValueSaver<double> m_saved_new_x_offset{g_evolve_new_x_parameter_offset};
    ValueSaver<double> m_saved_new_y_offset{g_evolve_new_y_parameter_offset};
    ValueSaver<char> m_saved_discrete_x{g_evolve_discrete_x_parameter_offset};
    ValueSaver<char> m_saved_discrete_y{g_evolve_discrete_y_parameter_offset};
    ValueSaver<char> m_saved_new_discrete_x{g_evolve_new_discrete_x_parameter_offset};
    ValueSaver<char> m_saved_new_discrete_y{g_evolve_new_discrete_y_parameter_offset};
    ValueSaver<int> m_saved_px{g_evolve_param_grid_x};
    ValueSaver<int> m_saved_py{g_evolve_param_grid_y};
    ValueSaver<double> m_saved_dist_x{g_evolve_dist_per_x};
    ValueSaver<double> m_saved_dist_y{g_evolve_dist_per_y};
};

void TestEvolutionRestore::SetUp()
{
    copy_genes_from_bank(m_genes);
    m_info.evolving = 9;
    m_info.image_grid_size = 3;
    m_info.this_generation_random_seed = 0xABCD;
    m_info.max_random_mutation = 0.25;
    m_info.x_parameter_range = 4;
    m_info.y_parameter_range = 3;
    m_info.x_parameter_offset = -2;
    m_info.y_parameter_offset = -1.5;
    m_info.discrete_x_parameter_offset = -7;
    m_info.discrete_y_parameter_offset = 9;
    m_info.px = 1;
    m_info.py = 2;
    m_info.screen_x_offset = 2;
    m_info.screen_y_offset = 1;
    m_info.x_dots = 8;
    m_info.y_dots = 6;
    m_info.count = 4;
    for (int i = 0; i < NUM_GENES; ++i)
    {
        m_info.mutate[i] = static_cast<std::int16_t>(i % static_cast<int>(Variations::NUM));
    }
}

void TestEvolutionRestore::TearDown()
{
    copy_genes_to_bank(m_genes);
}

TEST_F(TestEvolutionRestore, absentRecordDisablesEvolutionAndInvalidatesCachedProgress)
{
    g_viewport.enabled = true;

    apply_evolution_info(std::nullopt);

    EXPECT_EQ(EvolutionModeFlags::NONE, g_evolving);
    EXPECT_FALSE(g_have_evolve_info);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(1, g_logical_screen.x_dots);
    EXPECT_TRUE(g_viewport.enabled);
    EXPECT_FALSE(capture_evolution_info());
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EXPECT_EQ(m_genes[i].mutate, g_gene_bank[i].mutate) << i;
    }
}

class TestEvolutionRestoreProgress : public TestEvolutionRestore, public WithParamInterface<std::tuple<CalcStatus, int>>
{
};

TEST_P(TestEvolutionRestoreProgress, restoresControlsAndPreservesProgressWhenSaved)
{
    g_calc_status = std::get<0>(GetParam());
    m_info.count = static_cast<std::int16_t>(std::get<1>(GetParam()));
    const bool resumable{g_calc_status != CalcStatus::COMPLETED && m_info.count != 9};

    apply_evolution_info(m_info);

    EXPECT_EQ(resumable ? CalcStatus::RESUMABLE : CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(resumable, g_have_evolve_info);
    if (resumable)
    {
        EXPECT_EQ(m_info, g_evolve_info);
    }
    EXPECT_EQ(EvolutionModeFlags::FIELD_MAP | EvolutionModeFlags::NO_GROUT, g_evolving);
    EXPECT_TRUE(g_viewport.enabled);
    EXPECT_EQ(8, g_logical_screen.x_dots);
    EXPECT_EQ(6, g_logical_screen.y_dots);
    EXPECT_EQ(2, g_logical_screen.x_offset);
    EXPECT_EQ(1, g_logical_screen.y_offset);
    EXPECT_EQ(3, g_evolve_image_grid_size);
    EXPECT_EQ(0xABCDU, g_evolve_this_generation_random_seed);
    EXPECT_EQ(0.25, g_evolve_max_random_mutation);
    EXPECT_EQ(4, g_evolve_x_parameter_range);
    EXPECT_EQ(3, g_evolve_y_parameter_range);
    EXPECT_EQ(-2, g_evolve_x_parameter_offset);
    EXPECT_EQ(-1.5, g_evolve_y_parameter_offset);
    EXPECT_EQ(-2, g_evolve_new_x_parameter_offset);
    EXPECT_EQ(-1.5, g_evolve_new_y_parameter_offset);
    EXPECT_EQ(-7, g_evolve_discrete_x_parameter_offset);
    EXPECT_EQ(9, g_evolve_discrete_y_parameter_offset);
    EXPECT_EQ(-7, g_evolve_new_discrete_x_parameter_offset);
    EXPECT_EQ(9, g_evolve_new_discrete_y_parameter_offset);
    EXPECT_EQ(1, g_evolve_param_grid_x);
    EXPECT_EQ(2, g_evolve_param_grid_y);
    EXPECT_EQ(2, g_evolve_dist_per_x);
    EXPECT_EQ(1.5, g_evolve_dist_per_y);
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EXPECT_EQ(m_info.mutate[i], static_cast<int>(g_gene_bank[i].mutate)) << i;
        EXPECT_EQ(m_genes[i].addr, g_gene_bank[i].addr) << i;
        EXPECT_EQ(m_genes[i].vary_fn, g_gene_bank[i].vary_fn) << i;
    }
    EvolutionInfo expected{m_info};
    if (!resumable)
    {
        expected.count = 9;
    }
    const std::optional<EvolutionInfo> saved{capture_evolution_info()};
    ASSERT_TRUE(saved);
    EXPECT_EQ(expected, *saved);
}

INSTANTIATE_TEST_SUITE_P(States, TestEvolutionRestoreProgress,
    Combine(Values(CalcStatus::PARAMS_CHANGED, CalcStatus::IN_PROGRESS, CalcStatus::RESUMABLE,
                CalcStatus::NON_RESUMABLE, CalcStatus::COMPLETED),
        Values(4, 9)));

TEST_F(TestEvolutionRestore, presentDisabledModeDisablesTheViewport)
{
    g_viewport.enabled = true;
    m_info.evolving = 0;

    apply_evolution_info(m_info);

    EXPECT_EQ(EvolutionModeFlags::NONE, g_evolving);
    EXPECT_FALSE(g_viewport.enabled);
}

} // namespace id::test
