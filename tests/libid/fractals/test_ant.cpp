// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "fractals/Ant.h"

#include "FractalParamSaver.h"

#include "engine/calcfrac.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <ostream>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::test
{

struct AntRuleInput
{
    const char *name;
    Version version;
    double numeric_rule;
    std::string text_rule;
    const char *expected;
};

void PrintTo(const AntRuleInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestAntRuleText : public testing::Test, public testing::WithParamInterface<AntRuleInput>
{
protected:
    void SetUp() override;

    FractalParamSaver m_saved_params;
    ValueSaver<Version> m_saved_version{g_version, parse_legacy_version(1730)};
};

void TestAntRuleText::SetUp()
{
    std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0);
    g_param_text.fill({});
    g_params[0] = GetParam().numeric_rule;
    g_param_text[0] = GetParam().text_rule;
    g_version = GetParam().version;
}

TEST_P(TestAntRuleText, usesVersionCompatibleRuleText)
{
    EXPECT_EQ(GetParam().expected, ant_rule_text());
}

INSTANTIATE_TEST_SUITE_P(Versions, TestAntRuleText,
    testing::Values(AntRuleInput{"LegacyFractintReset", parse_legacy_version(1730), 1100.0, {}, "1100"},
        AntRuleInput{"IdOneZero", Version{1, 0, 0, 0, false}, 1100.0, {}, "1100.00000000000000000"},
        AntRuleInput{"IdOneThreeTwo", Version{1, 3, 2, 0, false}, 1100.0, {}, "1100.00000000000000000"},
        AntRuleInput{"IdOneThreeThree", Version{1, 3, 3, 0, false}, 1100.0, {}, "1100"},
        AntRuleInput{
            "TextOverridesDriftedNumericRule", Version{1, 0, 0, 0, false}, 1100.0, "101001011001", "101001011001"}),
    [](const testing::TestParamInfo<AntRuleInput> &info) { return info.param.name; });

} // namespace id::test
