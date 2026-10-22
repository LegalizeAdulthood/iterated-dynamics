// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/ImageRenderOperation.h>

#include "MockDriver.h"

#include <fractals/fractalp.h>
#include <misc/ValueSaver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

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
};

TEST_F(TestImageRenderOperation, requiresSelectionBeforeAccessOrFinish)
{
    ImageRenderOperation operation;

    EXPECT_THROW(operation.standard(), std::logic_error);
    EXPECT_THROW(operation.lyapunov(), std::logic_error);
    EXPECT_THROW(operation.non_standard(), std::logic_error);
    EXPECT_THROW(operation.finish(false), std::logic_error);
}

TEST_F(TestImageRenderOperation, standardAndFastMandelbrotSelectStandardWithoutStartingWork)
{
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, _)).Times(0);
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(0);
    for (const CalcType calculate : {standard_fractal_type, calc_mandelbrot_type})
    {
        m_specific.calc_type = calculate;
        ImageRenderOperation operation;

        EXPECT_EQ(ImageRenderOperation::Renderer::STANDARD, operation.select());
        EXPECT_FALSE(operation.standard().done());
        EXPECT_THROW(operation.lyapunov(), std::logic_error);
        EXPECT_THROW(operation.non_standard(), std::logic_error);
        EXPECT_THROW(operation.select(), std::logic_error);
    }
}

TEST_F(TestImageRenderOperation, lyapunovSelectionOwnsItsCalculation)
{
    m_specific.calc_type = lyapunov_type;
    ImageRenderOperation operation;

    EXPECT_EQ(ImageRenderOperation::Renderer::LYAPUNOV, operation.select());
    EXPECT_FALSE(operation.lyapunov().done());
    EXPECT_THROW(operation.standard(), std::logic_error);
    EXPECT_THROW(operation.non_standard(), std::logic_error);
}

TEST_F(TestImageRenderOperation, standaloneSelectionDoesNotInvokeTheRenderer)
{
    m_specific.calc_type = []() -> int { throw std::logic_error("Unexpected renderer call"); };
    ImageRenderOperation operation;

    EXPECT_EQ(ImageRenderOperation::Renderer::NON_STANDARD, operation.select());
    EXPECT_TRUE(operation.non_standard().done());
    EXPECT_THROW(operation.standard(), std::logic_error);
    EXPECT_THROW(operation.lyapunov(), std::logic_error);
}

} // namespace
} // namespace id::test
