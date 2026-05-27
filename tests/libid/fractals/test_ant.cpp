// SPDX-License-Identifier: GPL-3.0-only
//
#include "fractals/Ant.h"

#include "engine/calcfrac.h"
#include "misc/ValueSaver.h"
#include "misc/version.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <string>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;

namespace id::test
{

class AntRuleParamSaver
{
public:
    AntRuleParamSaver()
    {
        std::copy(&g_params[0], &g_params[MAX_PARAMS], m_params.begin());
        m_param_text = g_param_text;
    }
    ~AntRuleParamSaver()
    {
        std::copy(m_params.begin(), m_params.end(), &g_params[0]);
        g_param_text = m_param_text;
    }

private:
    std::array<double, MAX_PARAMS> m_params{};
    std::array<std::string, MAX_PARAMS> m_param_text;
};

class TestAntRuleText : public testing::Test
{
protected:
    void SetUp() override
    {
        std::fill(&g_params[0], &g_params[MAX_PARAMS], 0.0);
        g_param_text.fill({});
        g_params[0] = 1100.0;
    }

    AntRuleParamSaver m_saved_params;
    ValueSaver<Version> m_saved_version{g_version, parse_legacy_version(1730)};
};

TEST_F(TestAntRuleText, legacyFractintResetUsesCompactNumericRule)
{
    g_version = parse_legacy_version(1730);

    EXPECT_EQ("1100", ant_rule_text());
}

TEST_F(TestAntRuleText, idVersionOneZeroUsesDriftedFixedNumericRule)
{
    g_version = Version{1, 0, 0, 0, false};

    EXPECT_EQ("1100.00000000000000000", ant_rule_text());
}

TEST_F(TestAntRuleText, idVersionOneThreeTwoUsesDriftedFixedNumericRule)
{
    g_version = Version{1, 3, 2, 0, false};

    EXPECT_EQ("1100.00000000000000000", ant_rule_text());
}

TEST_F(TestAntRuleText, idVersionOneThreeThreeUsesCompactNumericRule)
{
    g_version = Version{1, 3, 3, 0, false};

    EXPECT_EQ("1100", ant_rule_text());
}

TEST_F(TestAntRuleText, integerTextRuleOverridesDriftedNumericRule)
{
    g_version = Version{1, 0, 0, 0, false};
    g_param_text[0] = "101001011001";

    EXPECT_EQ("101001011001", ant_rule_text());
}

} // namespace id::test
