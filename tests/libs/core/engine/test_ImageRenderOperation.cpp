// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/ImageRenderOperation.h>

#include <MockDriver.h>

#include <fractals/fractalp.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ostream>
#include <stdexcept>

using namespace id::engine;
using namespace id::fractals;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{
namespace
{

class TestImageRenderOperation : public Test
{
protected:
    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ImageRenderOperation m_operation;
};

TEST_F(TestImageRenderOperation, standardAccessRequiresSelection)
{
    EXPECT_THROW(m_operation.standard(), std::logic_error);
}

TEST_F(TestImageRenderOperation, lyapunovAccessRequiresSelection)
{
    EXPECT_THROW(m_operation.lyapunov(), std::logic_error);
}

TEST_F(TestImageRenderOperation, standaloneAccessRequiresSelection)
{
    EXPECT_THROW(m_operation.non_standard(), std::logic_error);
}

TEST_F(TestImageRenderOperation, finishRequiresSelection)
{
    EXPECT_THROW(m_operation.finish(false), std::logic_error);
}

struct StandardRenderInput
{
    const char *name;
    CalcType calculate;
};

void PrintTo(const StandardRenderInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestStandardRenderSelection : public TestImageRenderOperation, public WithParamInterface<StandardRenderInput>
{
protected:
    void SetUp() override;
};

void TestStandardRenderSelection::SetUp()
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, _)).Times(0);
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(0);
    m_specific.calc_type = GetParam().calculate;
    EXPECT_EQ(ImageRenderOperation::Renderer::STANDARD, m_operation.select());
}

TEST_P(TestStandardRenderSelection, selectionDoesNotStartWork)
{
    EXPECT_FALSE(m_operation.standard().done());
}

TEST_P(TestStandardRenderSelection, rejectsLyapunovAccess)
{
    EXPECT_THROW(m_operation.lyapunov(), std::logic_error);
}

TEST_P(TestStandardRenderSelection, rejectsStandaloneAccess)
{
    EXPECT_THROW(m_operation.non_standard(), std::logic_error);
}

TEST_P(TestStandardRenderSelection, rejectsDuplicateSelection)
{
    EXPECT_THROW(m_operation.select(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(Calculators, TestStandardRenderSelection,
    Values(StandardRenderInput{"Standard", standard_fractal_type},
        StandardRenderInput{"FastMandelbrot", calc_mandelbrot_type}),
    [](const TestParamInfo<StandardRenderInput> &info) { return info.param.name; });

class TestLyapunovRenderSelection : public TestImageRenderOperation
{
protected:
    void SetUp() override;
};

void TestLyapunovRenderSelection::SetUp()
{
    m_specific.calc_type = lyapunov_type;
    EXPECT_EQ(ImageRenderOperation::Renderer::LYAPUNOV, m_operation.select());
}

TEST_F(TestLyapunovRenderSelection, selectionOwnsItsCalculation)
{
    EXPECT_FALSE(m_operation.lyapunov().done());
}

TEST_F(TestLyapunovRenderSelection, rejectsStandardAccess)
{
    EXPECT_THROW(m_operation.standard(), std::logic_error);
}

TEST_F(TestLyapunovRenderSelection, rejectsStandaloneAccess)
{
    EXPECT_THROW(m_operation.non_standard(), std::logic_error);
}

class TestStandaloneRenderSelection : public TestImageRenderOperation
{
protected:
    void SetUp() override;
};

void TestStandaloneRenderSelection::SetUp()
{
    m_specific.calc_type = []() -> int { throw std::logic_error("Unexpected renderer call"); };
    EXPECT_EQ(ImageRenderOperation::Renderer::NON_STANDARD, m_operation.select());
}

TEST_F(TestStandaloneRenderSelection, selectionDoesNotInvokeTheRenderer)
{
    EXPECT_TRUE(m_operation.non_standard().done());
}

TEST_F(TestStandaloneRenderSelection, rejectsStandardAccess)
{
    EXPECT_THROW(m_operation.standard(), std::logic_error);
}

TEST_F(TestStandaloneRenderSelection, rejectsLyapunovAccess)
{
    EXPECT_THROW(m_operation.lyapunov(), std::logic_error);
}

} // namespace
} // namespace id::test
