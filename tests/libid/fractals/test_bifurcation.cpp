// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/Bifurcation.h"

#include "engine/calcfrac.h"
#include "engine/trig_fns.h"
#include "fractals/bif_may.h"
#include "fractals/fractalp.h"
#include "fractals/population.h"
#include "math/arg.h"
#include "math/fixed_pt.h"
#include "misc/ValueSaver.h"

#include <gtest/gtest.h>

#include <optional>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

using TrigFunction = void (*)();

struct BifurcationSequenceInput
{
    OrbitCalc orbit_calc{};
    TrigFn trig{TrigFn::SIN};
    double rate{};
    double seed_population{};
    std::optional<double> may_beta;
    int iterations{};
};

struct BifurcationSequence
{
    std::vector<double> populations;
    std::optional<int> bailout_step;
};

static BifurcationSequence run_bifurcation_sequence(const BifurcationSequenceInput &input)
{
    Arg stack{};
    ValueSaver saved_arg1{g_arg1, &stack};
    ValueSaver saved_arg2{g_arg2, &stack};
    ValueSaver saved_trig_index{g_trig_index[0], input.trig};
    ValueSaver<TrigFunction> saved_trig_fn{g_d_trig0};
    ValueSaver saved_population{g_population, input.seed_population};
    ValueSaver saved_rate{g_rate, input.rate};
    ValueSaver saved_overflow{g_overflow, false};
    ValueSaver saved_tmp_z{g_tmp_z};
    ValueSaver saved_may_beta{g_params[2]};

    set_trig_pointers(0);
    if (input.may_beta.has_value())
    {
        set_bifurc_may_beta(*input.may_beta);
    }

    BifurcationSequence result;
    result.populations.reserve(input.iterations);
    for (int step = 0; step < input.iterations; ++step)
    {
        const int bailed = input.orbit_calc();
        result.populations.push_back(g_population);
        if (bailed)
        {
            result.bailout_step = step;
            break;
        }
    }

    return result;
}

TEST(TestBifurcationSequence, recordsPopulationValues)
{
    const BifurcationSequence result{
        run_bifurcation_sequence({bifurc_lambda_trig_orbit, TrigFn::IDENT, 2.0, 0.25, std::nullopt, 3})};

    ASSERT_EQ(3U, result.populations.size());
    EXPECT_DOUBLE_EQ(0.375, result.populations[0]);
    EXPECT_DOUBLE_EQ(0.46875, result.populations[1]);
    EXPECT_DOUBLE_EQ(0.498046875, result.populations[2]);
    EXPECT_FALSE(result.bailout_step.has_value());
}

TEST(TestBifurcationSequence, recordsBailoutStep)
{
    const BifurcationSequence result{
        run_bifurcation_sequence({bifurc_lambda_trig_orbit, TrigFn::IDENT, 100001.0, 2.0, std::nullopt, 10})};

    ASSERT_EQ(1U, result.populations.size());
    EXPECT_DOUBLE_EQ(-200002.0, result.populations[0]);
    ASSERT_TRUE(result.bailout_step.has_value());
    EXPECT_EQ(0, *result.bailout_step);
}

TEST(TestBifurcationSequence, initializesMayBeta)
{
    const BifurcationSequence result{run_bifurcation_sequence({bifurc_may_orbit, TrigFn::SIN, 2.0, 1.0, 3.0, 1})};

    ASSERT_EQ(1U, result.populations.size());
    EXPECT_DOUBLE_EQ(0.25, result.populations[0]);
    EXPECT_FALSE(result.bailout_step.has_value());
}

} // namespace id::test
