// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/trig_fns.h>

#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/parser.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <string>
#include <ostream>

using namespace id::fractals;
using namespace id::misc;

namespace id::engine
{

class TestTrigFns : public testing::Test
{
protected:
    ValueSaver<FractalType> m_saved_fractal_type{g_fractal_type, FractalType::FORMULA};
    ValueSaver<const FractalSpecific *> m_saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<int> m_saved_max_function{g_formula.max_function, 0};
    ValueSaver<TrigFn> m_saved_trig_index0{g_trig_index[0], TrigFn::SIN};
    ValueSaver<TrigFn> m_saved_trig_index1{g_trig_index[1], TrigFn::COS};
    ValueSaver<TrigFn> m_saved_trig_index2{g_trig_index[2], TrigFn::TAN};
    ValueSaver<TrigFn> m_saved_trig_index3{g_trig_index[3], TrigFn::COTAN};
};

struct TrigDisplayInput
{
    const char *name;
    int count;
    const char *details;
    const char *parameter;
};

void PrintTo(const TrigDisplayInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestTrigFnsDisplay : public TestTrigFns, public testing::WithParamInterface<TrigDisplayInput>
{
protected:
    void SetUp() override;
};

void TestTrigFnsDisplay::SetUp()
{
    TestTrigFns::SetUp();
    g_formula.max_function = GetParam().count;
}

TEST_P(TestTrigFnsDisplay, rendersFunctionDetails)
{
    EXPECT_EQ(GetParam().details, trig_details());
}

TEST_P(TestTrigFnsDisplay, rendersFunctionParameter)
{
    EXPECT_EQ(GetParam().parameter, get_function_param());
}

INSTANTIATE_TEST_SUITE_P(FunctionCounts, TestTrigFnsDisplay,
    testing::Values(TrigDisplayInput{"Empty", 0, "", ""}, TrigDisplayInput{"OneFunction", 1, "sin", " function=sin"},
        TrigDisplayInput{"FourFunctions", 4, "sin/cos/tan/cotan", " function=sin/cos/tan/cotan"}),
    [](const testing::TestParamInfo<TrigDisplayInput> &info) { return info.param.name; });

} // namespace id::engine
