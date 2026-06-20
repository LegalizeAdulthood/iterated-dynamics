// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/trig_fns.h>

#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/parser.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <string>

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

TEST_F(TestTrigFns, trigDetailsEmptyWhenNoFunctions)
{
    EXPECT_EQ("", trig_details());
    EXPECT_EQ("", get_function_param());
}

TEST_F(TestTrigFns, trigDetailsRendersOneFunction)
{
    g_formula.max_function = 1;

    EXPECT_EQ("sin", trig_details());
    EXPECT_EQ(" function=sin", get_function_param());
}

TEST_F(TestTrigFns, trigDetailsRendersMultipleFunctions)
{
    g_formula.max_function = 4;

    EXPECT_EQ("sin/cos/tan/cotan", trig_details());
    EXPECT_EQ(" function=sin/cos/tan/cotan", get_function_param());
}

} // namespace id::engine
