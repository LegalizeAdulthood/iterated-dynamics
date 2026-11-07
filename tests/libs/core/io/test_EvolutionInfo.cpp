// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/EvolutionInfo.h>

#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <tuple>

using namespace id::io;
using namespace testing;

namespace id::test
{

TEST(TestEvolutionInfo, selfEquality)
{
    const EvolutionInfo original{};

    EXPECT_EQ(original, original);
    EXPECT_FALSE(original != original);
}

class EvolutionComparisonTest : public Test
{
protected:
    const EvolutionInfo m_original{};
    EvolutionInfo m_changed{m_original};
};

using SignedEvolutionMember = std::int16_t EvolutionInfo::*;

struct EvolutionSignedField
{
    const char *name;
    SignedEvolutionMember member;
};

void PrintTo(const EvolutionSignedField &value, std::ostream *str)
{
    *str << value.name;
}

class TestEvolutionInfoSignedField : public EvolutionComparisonTest, public WithParamInterface<EvolutionSignedField>
{
};

TEST_P(TestEvolutionInfoSignedField, changedFieldIsUnequal)
{
    m_changed.*GetParam().member = -1;

    EXPECT_NE(m_original, m_changed);
    EXPECT_FALSE(m_original == m_changed);
}

INSTANTIATE_TEST_SUITE_P(SignedFields, TestEvolutionInfoSignedField,
    Values(EvolutionSignedField{"Evolving", &EvolutionInfo::evolving},
        EvolutionSignedField{"ImageGridSize", &EvolutionInfo::image_grid_size},
        EvolutionSignedField{"DiscreteXOffset", &EvolutionInfo::discrete_x_parameter_offset},
        EvolutionSignedField{"DiscreteYOffset", &EvolutionInfo::discrete_y_parameter_offset},
        EvolutionSignedField{"ParameterX", &EvolutionInfo::px}, EvolutionSignedField{"ParameterY", &EvolutionInfo::py},
        EvolutionSignedField{"ScreenXOffset", &EvolutionInfo::screen_x_offset},
        EvolutionSignedField{"ScreenYOffset", &EvolutionInfo::screen_y_offset},
        EvolutionSignedField{"Count", &EvolutionInfo::count}),
    [](const TestParamInfo<EvolutionSignedField> &info) { return info.param.name; });

using UnsignedEvolutionMember = std::uint16_t EvolutionInfo::*;

struct EvolutionUnsignedField
{
    const char *name;
    UnsignedEvolutionMember member;
};

void PrintTo(const EvolutionUnsignedField &value, std::ostream *str)
{
    *str << value.name;
}

class TestEvolutionInfoUnsignedField : public EvolutionComparisonTest, public WithParamInterface<EvolutionUnsignedField>
{
};

TEST_P(TestEvolutionInfoUnsignedField, changedFieldIsUnequal)
{
    m_changed.*GetParam().member = 65535;

    EXPECT_NE(m_original, m_changed);
}

INSTANTIATE_TEST_SUITE_P(UnsignedFields, TestEvolutionInfoUnsignedField,
    Values(EvolutionUnsignedField{"RandomSeed", &EvolutionInfo::this_generation_random_seed},
        EvolutionUnsignedField{"Width", &EvolutionInfo::x_dots},
        EvolutionUnsignedField{"Height", &EvolutionInfo::y_dots}),
    [](const TestParamInfo<EvolutionUnsignedField> &info) { return info.param.name; });

class TestEvolutionInfoMutation : public EvolutionComparisonTest, public WithParamInterface<int>
{
};

TEST_P(TestEvolutionInfoMutation, changedMutationIsUnequal)
{
    m_changed.mutate[GetParam()] = 1;

    EXPECT_NE(m_original, m_changed) << GetParam();
}

INSTANTIATE_TEST_SUITE_P(MutationIndices, TestEvolutionInfoMutation, Range<int>(0, NUM_GENES),
    [](const TestParamInfo<int> &info) { return "Gene" + std::to_string(info.param); });

struct EvolutionCoordinateField
{
    const char *name;
    double EvolutionInfo::*member;
};

void PrintTo(const EvolutionCoordinateField &value, std::ostream *str)
{
    *str << value.name;
}

struct EvolutionCoordinateDelta
{
    const char *name;
    double value;
    bool equal;
};

void PrintTo(const EvolutionCoordinateDelta &value, std::ostream *str)
{
    *str << value.name;
}

using EvolutionCoordinateInput = std::tuple<EvolutionCoordinateField, EvolutionCoordinateDelta>;

class TestEvolutionInfoCoordinate : public EvolutionComparisonTest, public WithParamInterface<EvolutionCoordinateInput>
{
};

TEST_P(TestEvolutionInfoCoordinate, honorsCoordinateTolerance)
{
    const EvolutionCoordinateField &field{std::get<0>(GetParam())};
    const EvolutionCoordinateDelta &delta{std::get<1>(GetParam())};

    m_changed.*field.member = delta.value;

    if (delta.equal)
    {
        EXPECT_EQ(m_original, m_changed);
    }
    else
    {
        EXPECT_NE(m_original, m_changed);
    }
}

INSTANTIATE_TEST_SUITE_P(CoordinateInputs, TestEvolutionInfoCoordinate,
    Combine(Values(EvolutionCoordinateField{"XRange", &EvolutionInfo::x_parameter_range},
                EvolutionCoordinateField{"YRange", &EvolutionInfo::y_parameter_range},
                EvolutionCoordinateField{"XOffset", &EvolutionInfo::x_parameter_offset},
                EvolutionCoordinateField{"YOffset", &EvolutionInfo::y_parameter_offset}),
        Values(EvolutionCoordinateDelta{"WithinTolerance", 1.0e-7, true},
            EvolutionCoordinateDelta{"OutsideTolerance", 1.0e-5, false})),
    [](const TestParamInfo<EvolutionCoordinateInput> &info)
    { return std::string{std::get<0>(info.param).name} + std::get<1>(info.param).name; });

TEST_F(EvolutionComparisonTest, mutationMagnitudeIsComparedExactly)
{
    m_changed.max_random_mutation = 1.0e-7;

    EXPECT_NE(m_original, m_changed);
}

TEST(TestEvolutionInfo, ignoresReservedFields)
{
    const EvolutionInfo original{};
    EvolutionInfo changed{original};
    for (std::int16_t &value : changed.future)
    {
        value = -1;
    }

    EXPECT_EQ(original, changed);
    EXPECT_FALSE(original != changed);
}

} // namespace id::test
