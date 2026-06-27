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

#include <algorithm>
#include <array>
#include <limits>
#include <optional>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;

namespace id::test
{

using TrigFunction = void (*)();
constexpr std::size_t CHAOTIC_HISTOGRAM_BUCKETS{8};
using ChaoticHistogram = std::array<int, CHAOTIC_HISTOGRAM_BUCKETS>;

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

struct StableBifurcationSequenceCase
{
    const char *name{};
    BifurcationSequenceInput input{};
    std::vector<double> expected_populations;
    double tolerance{};
};

struct ChaoticBifurcationSummary
{
    double minimum{};
    double maximum{};
    double mean{};
    double variance{};
    ChaoticHistogram histogram{};
};

struct ChaoticBifurcationSummaryCase
{
    const char *name{};
    BifurcationSequenceInput input{};
    int burn_in{};
    double histogram_minimum{};
    double histogram_maximum{};
    ChaoticBifurcationSummary expected{};
    double value_tolerance{};
    int histogram_tolerance{};
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

static void expect_populations_near(
    const BifurcationSequence &result, const std::vector<double> &expected_populations, const double tolerance)
{
    ASSERT_EQ(expected_populations.size(), result.populations.size());
    for (std::size_t index = 0; index < expected_populations.size(); ++index)
    {
        EXPECT_NEAR(expected_populations[index], result.populations[index], tolerance) << "population " << index;
    }
}

static ChaoticBifurcationSummary summarize_sequence(const BifurcationSequence &sequence, const int burn_in,
    const double histogram_minimum, const double histogram_maximum)
{
    ChaoticBifurcationSummary summary{};
    summary.minimum = std::numeric_limits<double>::max();
    summary.maximum = std::numeric_limits<double>::lowest();

    const std::size_t start{static_cast<std::size_t>(burn_in)};
    const double count{static_cast<double>(sequence.populations.size() - start)};
    const double histogram_width{
        (histogram_maximum - histogram_minimum) / static_cast<double>(CHAOTIC_HISTOGRAM_BUCKETS)};
    double sum{};
    double sum_of_squares{};

    for (std::size_t index = start; index < sequence.populations.size(); ++index)
    {
        const double population{sequence.populations[index]};
        summary.minimum = std::min(summary.minimum, population);
        summary.maximum = std::max(summary.maximum, population);
        sum += population;
        sum_of_squares += population * population;

        int bucket{static_cast<int>((population - histogram_minimum) / histogram_width)};
        bucket = std::max(0, std::min(bucket, static_cast<int>(CHAOTIC_HISTOGRAM_BUCKETS - 1)));
        ++summary.histogram[static_cast<std::size_t>(bucket)];
    }

    summary.mean = sum / count;
    summary.variance = sum_of_squares / count - summary.mean * summary.mean;

    return summary;
}

static void expect_summary_near(const ChaoticBifurcationSummary &expected, const ChaoticBifurcationSummary &actual,
    const double value_tolerance, const int histogram_tolerance)
{
    EXPECT_NEAR(expected.minimum, actual.minimum, value_tolerance);
    EXPECT_NEAR(expected.maximum, actual.maximum, value_tolerance);
    EXPECT_NEAR(expected.mean, actual.mean, value_tolerance);
    EXPECT_NEAR(expected.variance, actual.variance, value_tolerance);

    for (std::size_t bucket = 0; bucket < expected.histogram.size(); ++bucket)
    {
        EXPECT_NEAR(static_cast<double>(expected.histogram[bucket]), static_cast<double>(actual.histogram[bucket]),
            static_cast<double>(histogram_tolerance))
            << "bucket " << bucket;
    }
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

TEST(TestBifurcationStableSequence, followsReferencePrefixes)
{
    const std::vector<StableBifurcationSequenceCase> tests{
        StableBifurcationSequenceCase{"Bifurcation",
            {bifurc_verhulst_trig_orbit, TrigFn::IDENT, 0.5, 0.2, std::nullopt, 4},
            {0.28, 0.3808, 0.49869568000000003, 0.6236948293746688}, 1.0e-12},
        StableBifurcationSequenceCase{"BifLambda", {bifurc_lambda_trig_orbit, TrigFn::IDENT, 1.5, 0.2, std::nullopt, 4},
            {0.24000000000000005, 0.27360000000000007, 0.29811456000000003, 0.31386340367400961}, 1.0e-12},
        StableBifurcationSequenceCase{"BifMay", {bifurc_may_orbit, TrigFn::SIN, 2.0, 1.0, 3.0, 4},
            {0.25, 0.256, 0.25840507734968382, 0.25934008734078989}, 1.0e-10},
        StableBifurcationSequenceCase{"BifStewart",
            {bifurc_stewart_trig_orbit, TrigFn::IDENT, 0.8, 0.5, std::nullopt, 4},
            {-0.8, -0.48799999999999988, -0.80948480000000012, -0.47578748685516781}, 1.0e-12},
        StableBifurcationSequenceCase{"BifPlusSinPi",
            {bifurc_add_trig_pi_orbit, TrigFn::SIN, 0.1, 0.2, std::nullopt, 4},
            {0.25877852522924732, 0.33141216542207341, 0.41771135392690084, 0.51438836012500488}, 1.0e-10},
        StableBifurcationSequenceCase{"BifEqualSinPi",
            {bifurc_set_trig_pi_orbit, TrigFn::SIN, 0.9, 0.2, std::nullopt, 4},
            {0.52900672706322582, 0.89626570041163567, 0.28813764047421686, 0.70789997473289534}, 1.0e-10}};

    for (const StableBifurcationSequenceCase &test : tests)
    {
        SCOPED_TRACE(test.name);
        const BifurcationSequence result{run_bifurcation_sequence(test.input)};

        expect_populations_near(result, test.expected_populations, test.tolerance);
        EXPECT_NEAR(test.expected_populations.back(), result.populations.back(), test.tolerance);
        EXPECT_FALSE(result.bailout_step.has_value());
    }
}

TEST(TestBifurcationChaoticSequence, followsSummaryStatistics)
{
    constexpr int BURN_IN{512};
    constexpr int SAMPLE_COUNT{4096};
    const std::vector<ChaoticBifurcationSummaryCase> tests{
        ChaoticBifurcationSummaryCase{"BifMay",
            {bifurc_may_orbit, TrigFn::SIN, 50.0, 0.66, 7.0, BURN_IN + SAMPLE_COUNT}, BURN_IN, 0.0, 3.0,
            {0.011659782830271741, 2.8326161383664665, 0.9307299406817845, 0.80112311740260389,
                ChaoticHistogram{1487, 762, 474, 429, 169, 176, 241, 358}},
            0.08, 250},
        ChaoticBifurcationSummaryCase{"BifPlusSinPi",
            {bifurc_add_trig_pi_orbit, TrigFn::SIN, 1.4, 0.66, std::nullopt, BURN_IN + SAMPLE_COUNT}, BURN_IN, 0.0, 2.0,
            {0.063655873023445553, 1.9363294864874447, 0.9997575286269117, 0.38783794226994606,
                ChaoticHistogram{613, 679, 392, 361, 372, 397, 671, 611}},
            0.08, 250},
        ChaoticBifurcationSummaryCase{"BifEqualSinPi",
            {bifurc_set_trig_pi_orbit, TrigFn::SIN, 1.2, 0.66, std::nullopt, BURN_IN + SAMPLE_COUNT}, BURN_IN, -1.2,
            1.2,
            {-1.1999991466632438, 1.1999996150746453, -0.00021277050287789141, 0.59111613857773837,
                ChaoticHistogram{758, 394, 426, 481, 454, 403, 417, 763}},
            0.08, 250}};

    for (const ChaoticBifurcationSummaryCase &test : tests)
    {
        SCOPED_TRACE(test.name);
        const BifurcationSequence result{run_bifurcation_sequence(test.input)};

        ASSERT_EQ(static_cast<std::size_t>(test.input.iterations), result.populations.size());
        EXPECT_FALSE(result.bailout_step.has_value());
        const ChaoticBifurcationSummary summary{
            summarize_sequence(result, test.burn_in, test.histogram_minimum, test.histogram_maximum)};
        expect_summary_near(test.expected, summary, test.value_tolerance, test.histogram_tolerance);
    }
}

} // namespace id::test
