// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/ShowDotController.h>

#include <MockDriver.h>
#include <PendingDriverKey.h>
#include <ui/TestRender.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/NullSoundOutput.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/SoundOutput.h>
#include <engine/StandardFractal.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <fractals/lyapunov.h>
#include <misc/ValueSaver.h>
#include <ui/ImageRenderContext.h>
#include <ui/KeyboardHandler.h>
#include <ui/LyapunovController.h>
#include <ui/SoundSession.h>
#include <ui/StandardFractalController.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

std::function<int()> s_calculate;

int calculate_pixel()
{
    return s_calculate();
}

bool initialize_image()
{
    return true;
}

} // namespace

class TestShowDotController : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        m_pixels.resize(WIDTH * HEIGHT);
        for (int i = 0; i < WIDTH * HEIGHT; ++i)
        {
            m_pixels[i] = 1 + i % 13;
        }
        set_normal_dot();
        reset_calc_interrupted();
        m_specific.flags = FractalFlags::NONE;
        m_specific.symmetry = SymmetryType::NONE;
        m_specific.per_image = initialize_image;
        m_specific.calc_type = calculate_pixel;
        g_dispatch = FractalDispatch{m_specific};
        s_calculate = [this]
        {
            EXPECT_THAT(m_pixels, Each(Ne(15)));
            ++m_calculations;
            if (m_yield_once)
            {
                m_yield_once = false;
                return CALC_YIELDED;
            }
            g_color = 7;
            g_plot(g_col, g_row, g_color);
            if (m_calculations == m_interrupt_after)
            {
                m_key = 'x';
            }
            return g_color;
        };
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * WIDTH + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int col, int row, int color) { m_pixels.at(row * WIDTH + col) = color; }));
        EXPECT_CALL(m_driver, flush()).WillRepeatedly(Invoke([this] { ++m_flushes; }));
        EXPECT_CALL(m_driver, delay(0)).Times(AnyNumber());
        expect_pending_driver_key(m_driver, m_key);
        EXPECT_CALL(m_driver, unget_key('x')).WillRepeatedly(Invoke([this](int key) { m_key = key; }));
    }

    void TearDown() override
    {
        s_calculate = {};
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
        end_resume();
        reset_calc_interrupted();
    }

    void clear_image()
    {
        std::fill(m_pixels.begin(), m_pixels.end(), 0);
        m_key = 0;
    }

    static constexpr int WIDTH{16};
    static constexpr int HEIGHT{12};
    MockDriver m_driver;
    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<LogicalScreen> m_saved_screen{
        g_logical_screen, LogicalScreen{WIDTH, HEIGHT, 0, 0, WIDTH - 1, HEIGHT - 1}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, WIDTH};
    ValueSaver<int> m_saved_height{g_screen_y_dots, HEIGHT};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{WIDTH - 1, HEIGHT - 1}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{WIDTH - 1, HEIGHT - 1}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 15};
    ValueSaver<int> m_saved_size{g_size_dot, 0};
    ValueSaver<AutoShowDot> m_saved_auto{g_auto_show_dot, AutoShowDot::NONE};
    ValueSaver<int> m_saved_delay{g_orbit_delay, 0};
    ValueSaver<bool> m_saved_overlay{g_show_orbit, false};
    ValueSaver<int> m_saved_fill{g_fill_color, -1};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<SymmetryType> m_saved_force{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<SymmetryType> m_saved_symmetry{g_symmetry};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_quick{g_quick_calc, false};
    ValueSaver<bool> m_saved_three_pass{g_three_pass, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<int> m_saved_interval{g_keyboard_check_interval};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, std::make_shared<NullSoundOutput>()};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels;
    int m_calculations{};
    int m_flushes{};
    int m_interrupt_after{-1};
    int m_key{};
    bool m_yield_once{};
};

TEST_F(TestShowDotController, disabledMarkerDoesNotTouchImage)
{
    g_show_dot = -1;
    const std::vector<int> original{m_pixels};
    ShowDotController marker;

    EXPECT_FALSE(marker.enabled());
    marker.show({0, 0});
    marker.restore();

    EXPECT_EQ(original, m_pixels);
}

class TestMarkerGeometry : public TestShowDotController
{
protected:
    void SetUp() override;

    std::vector<int> m_original;
};

void TestMarkerGeometry::SetUp()
{
    TestShowDotController::SetUp();
    m_original = m_pixels;
}

struct MarkerCornerInput
{
    const char *name;
    Point2i point;
    Point2i origin;
    std::array<const char *, 4> shape;
};

void PrintTo(const MarkerCornerInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMarkerCorner : public TestMarkerGeometry, public WithParamInterface<MarkerCornerInput>
{
protected:
    void SetUp() override;
};

void TestMarkerCorner::SetUp()
{
    TestMarkerGeometry::SetUp();
    g_size_dot = 128;
}

TEST_P(TestMarkerCorner, restoresThePointAndTriangle)
{
    std::vector<int> expected{m_original};
    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            if (GetParam().shape[row][col] == '#')
            {
                expected[(GetParam().origin.y + row) * WIDTH + GetParam().origin.x + col] = 15;
            }
        }
    }
    ShowDotController marker;

    marker.show(GetParam().point);

    EXPECT_EQ(expected, m_pixels);
    marker.restore();
    EXPECT_EQ(m_original, m_pixels);
}

INSTANTIATE_TEST_SUITE_P(Corners, TestMarkerCorner,
    Values(MarkerCornerInput{"TopLeft", {0, 0}, {0, 0}, {"#...", "##..", "###.", "####"}},
        MarkerCornerInput{"TopRight", {15, 0}, {12, 0}, {"...#", "..##", ".###", "####"}},
        MarkerCornerInput{"BottomRight", {15, 11}, {12, 8}, {"####", ".###", "..##", "...#"}},
        MarkerCornerInput{"BottomLeft", {0, 11}, {0, 8}, {"####", "###.", "##..", "#..."}}),
    [](const TestParamInfo<MarkerCornerInput> &info) { return info.param.name; });

using MarkerPlot = void (*)(int, int, int);

struct MarkerSymmetryInput
{
    const char *name;
    MarkerPlot plot;
};

void PrintTo(const MarkerSymmetryInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestOrdinaryMarkerSymmetry : public TestMarkerGeometry, public WithParamInterface<MarkerSymmetryInput>
{
protected:
    void SetUp() override;
};

void TestOrdinaryMarkerSymmetry::SetUp()
{
    TestMarkerGeometry::SetUp();
    g_size_dot = 128;
    g_i_stop_pt = {7, 5};
    g_plot = GetParam().plot;
}

TEST_P(TestOrdinaryMarkerSymmetry, restoresCopiesWithoutAssumingTheirOriginalColors)
{
    ShowDotController marker;

    marker.show({3, 3});
    EXPECT_NE(m_original, m_pixels);
    g_plot = put_color_a;
    marker.restore();

    EXPECT_EQ(m_original, m_pixels);
}

INSTANTIATE_TEST_SUITE_P(Symmetries, TestOrdinaryMarkerSymmetry,
    Values(MarkerSymmetryInput{"Horizontal", sym_plot2}, MarkerSymmetryInput{"Vertical", sym_plot2y},
        MarkerSymmetryInput{"Origin", sym_plot2j}, MarkerSymmetryInput{"BothAxes", sym_plot4},
        MarkerSymmetryInput{"HorizontalBasin", sym_plot2_basin}, MarkerSymmetryInput{"BothAxesBasin", sym_plot4_basin}),
    [](const TestParamInfo<MarkerSymmetryInput> &info) { return info.param.name; });

class TestPeriodicMarkerSymmetry : public TestMarkerGeometry, public WithParamInterface<MarkerSymmetryInput>
{
protected:
    void SetUp() override;

    ValueSaver<int> m_saved_period{g_pi_in_pixels, 4};
};

void TestPeriodicMarkerSymmetry::SetUp()
{
    TestMarkerGeometry::SetUp();
    g_i_stop_pt = {3, 5};
    g_plot = GetParam().plot;
}

TEST_P(TestPeriodicMarkerSymmetry, restoresPeriodicCopiesAndOverlappingAxisPixels)
{
    ShowDotController marker;

    marker.show({0, 0});
    EXPECT_EQ(15, m_pixels[0]);
    marker.restore();

    EXPECT_EQ(m_original, m_pixels);
}

INSTANTIATE_TEST_SUITE_P(Symmetries, TestPeriodicMarkerSymmetry,
    Values(MarkerSymmetryInput{"Periodic", sym_pi_plot}, MarkerSymmetryInput{"PeriodicOrigin", sym_pi_plot2j},
        MarkerSymmetryInput{"PeriodicBothAxes", sym_pi_plot4j}, MarkerSymmetryInput{"BothAxes", sym_plot4}),
    [](const TestParamInfo<MarkerSymmetryInput> &info) { return info.param.name; });

TEST_F(TestShowDotController, destructionRestoresAnOutstandingMarker)
{
    const std::vector<int> original{m_pixels};
    {
        ShowDotController marker;
        marker.show({3, 3});
        EXPECT_THROW(marker.show({4, 3}), std::logic_error);
    }
    EXPECT_EQ(original, m_pixels);
}

struct MarkerPassInput
{
    const char *name;
    CalcMode mode;
};

void PrintTo(const MarkerPassInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestMarkerPass : public TestShowDotController, public WithParamInterface<MarkerPassInput>
{
protected:
    void SetUp() override;
};

void TestMarkerPass::SetUp()
{
    TestShowDotController::SetUp();
    clear_image();
    g_std_calc_mode = GetParam().mode;
    g_resuming = false;
    g_size_dot = 128;
}

TEST_P(TestMarkerPass, readsOnlyRestoredImageData)
{
    ShowDotController marker;
    StandardFractal calculation;
    calculation.resume();

    for (int chunks = 0; chunks < 1000 && !calculation.done(); ++chunks)
    {
        marker.iterate(calculation);
    }

    EXPECT_TRUE(calculation.done());
    EXPECT_THAT(m_pixels, Each(7));
}

INSTANTIATE_TEST_SUITE_P(Passes, TestMarkerPass,
    Values(MarkerPassInput{"OnePass", CalcMode::ONE_PASS}, MarkerPassInput{"TwoPass", CalcMode::TWO_PASS},
        MarkerPassInput{"BoundaryTrace", CalcMode::BOUNDARY_TRACE},
        MarkerPassInput{"SolidGuess", CalcMode::SOLID_GUESS}, MarkerPassInput{"Diffusion", CalcMode::DIFFUSION},
        MarkerPassInput{"Tesseral", CalcMode::TESSERAL}),
    [](const TestParamInfo<MarkerPassInput> &info) { return info.param.name; });

class TestMarkerCalculation : public TestShowDotController
{
protected:
    void SetUp() override;
    void TearDown() override;
    void prepare_calculation();

    std::unique_ptr<ShowDotController> m_marker;
    std::unique_ptr<StandardFractal> m_calculation;
};

void TestMarkerCalculation::SetUp()
{
    TestShowDotController::SetUp();
    clear_image();
}

void TestMarkerCalculation::TearDown()
{
    m_calculation.reset();
    m_marker.reset();
    TestShowDotController::TearDown();
}

void TestMarkerCalculation::prepare_calculation()
{
    m_marker = std::make_unique<ShowDotController>();
    m_calculation = std::make_unique<StandardFractal>();
    m_calculation->resume();
}

TEST_F(TestMarkerCalculation, zeroDelayRetainsTheOrdinaryScanlineBatch)
{
    prepare_calculation();

    m_marker->iterate(*m_calculation);

    EXPECT_TRUE(m_calculation->done());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_EQ(0, m_flushes);
}

TEST_F(TestMarkerCalculation, delayHappensWithTheMarkerVisibleButCalculationSeesRestoredPixels)
{
    g_orbit_delay = 1;
    EXPECT_CALL(m_driver, delay(0)).Times(2).WillRepeatedly(Invoke([this] { EXPECT_EQ(15, m_pixels[0]); }));
    prepare_calculation();

    m_marker->iterate(*m_calculation);

    EXPECT_EQ(1, m_calculations);
    EXPECT_EQ(1, m_flushes);
    EXPECT_EQ(7, m_pixels[0]);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
}

TEST_F(TestMarkerCalculation, unfinishedPixelDoesNotRepeatItsMarkerOrDelay)
{
    g_orbit_delay = 1;
    m_yield_once = true;
    prepare_calculation();

    m_marker->iterate(*m_calculation);
    EXPECT_EQ(1, m_flushes);
    EXPECT_THAT(m_pixels, Each(0));
    m_marker->iterate(*m_calculation);

    EXPECT_EQ(2, m_calculations);
    EXPECT_EQ(1, m_flushes);
    EXPECT_EQ(7, m_pixels[0]);
}

TEST_F(TestShowDotController, menuInterruptionAndResumeLeaveNoMarkerBehind)
{
    clear_image();
    g_orbit_delay = 1;
    g_size_dot = 128;
    m_interrupt_after = 5;

    EXPECT_EQ(-1, render_standard_image());
    EXPECT_EQ('x', m_key);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    m_key = 0;
    g_resuming = true;

    EXPECT_EQ(0, render_standard_image());

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestShowDotController, parameterRestartUsesFreshMarkerState)
{
    clear_image();
    g_orbit_delay = 1;
    m_interrupt_after = 5;
    ASSERT_EQ(-1, render_standard_image());
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    clear_image();
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_size_dot = 128;
    ValueSaver saved_iterations{g_max_iterations, 300L};

    EXPECT_EQ(0, render_standard_image());

    EXPECT_THAT(m_pixels, Each(7));
}

class TestImageRenderContext : public TestShowDotController
{
protected:
    void SetUp() override;
};

void TestImageRenderContext::SetUp()
{
    TestShowDotController::SetUp();
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = []
    {
        g_dispatch.set_calc_type(calculate_pixel);
        return true;
    };
    g_dispatch = FractalDispatch{m_specific};
}

class TestPreparedImageRenderContext : public TestImageRenderContext
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::unique_ptr<SoundSession> m_sound;
    std::unique_ptr<ImageRenderContext> m_rendering;
};

void TestPreparedImageRenderContext::SetUp()
{
    TestImageRenderContext::SetUp();
    m_sound = std::make_unique<SoundSession>(g_sound, SoundSession::Ownership::BORROWED);
    m_rendering = std::make_unique<ImageRenderContext>(*m_sound);
    ASSERT_FALSE(m_rendering->done());
    ASSERT_EQ(0, m_calculations);
}

void TestPreparedImageRenderContext::TearDown()
{
    m_rendering.reset();
    m_sound.reset();
    TestImageRenderContext::TearDown();
}

TEST_F(TestPreparedImageRenderContext, iterateRequiresStart)
{
    EXPECT_THROW(m_rendering->iterate(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

TEST_F(TestPreparedImageRenderContext, resultRequiresCompletion)
{
    EXPECT_THROW(m_rendering->result(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

TEST_F(TestPreparedImageRenderContext, suspendRequiresStart)
{
    EXPECT_THROW(m_rendering->suspend(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

class TestStartedImageRenderContext : public TestPreparedImageRenderContext
{
protected:
    void SetUp() override;
};

void TestStartedImageRenderContext::SetUp()
{
    TestPreparedImageRenderContext::SetUp();
    m_rendering->start();
    ASSERT_FALSE(m_rendering->done());
    ASSERT_EQ(0, m_calculations);
}

TEST_F(TestStartedImageRenderContext, duplicateStartIsRejected)
{
    EXPECT_THROW(m_rendering->start(), std::logic_error);
}

TEST_F(TestStartedImageRenderContext, explicitSuspensionCompletesTheController)
{
    m_rendering->suspend();

    EXPECT_TRUE(m_rendering->done());
    EXPECT_EQ(0, m_rendering->result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestImageRenderContext, outerStepRetainsTheExistingScanlineBatch)
{
    clear_image();
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    ImageRenderContext rendering{sound};
    rendering.start();

    rendering.iterate();

    ASSERT_TRUE(rendering.done());
    EXPECT_EQ(0, rendering.result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
    rendering.iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestImageRenderContext, interruptedOperationReleasesHandlersAndCanResume)
{
    clear_image();
    g_orbit_delay = 1;
    m_interrupt_after = 5;
    EXPECT_EQ(-1, render_image());

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    EXPECT_THAT(m_pixels, Each(Ne(15)));

    m_key = 0;
    EXPECT_EQ(0, render_image());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestImageRenderContext, abandoningAnActiveContextAbortsAndReleasesPresentation)
{
    clear_image();
    g_orbit_delay = 1;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    {
        ImageRenderContext rendering{sound};
        rendering.start();
        rendering.iterate();
        EXPECT_FALSE(rendering.done());
    }

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

class TestStandardFractalController : public TestShowDotController
{
};

class TestPreparedStandardController : public TestStandardFractalController
{
protected:
    void SetUp() override;
    void TearDown() override;
    void prepare_controller();

    std::unique_ptr<StandardFractal> m_calculation;
    std::unique_ptr<SoundSession> m_sound;
    std::unique_ptr<StandardFractalController> m_controller;
};

void TestPreparedStandardController::SetUp()
{
    TestStandardFractalController::SetUp();
    prepare_controller();
}

void TestPreparedStandardController::TearDown()
{
    m_controller.reset();
    m_sound.reset();
    m_calculation.reset();
    TestStandardFractalController::TearDown();
}

void TestPreparedStandardController::prepare_controller()
{
    m_calculation = std::make_unique<StandardFractal>();
    m_sound = std::make_unique<SoundSession>(g_sound, SoundSession::Ownership::BORROWED);
    m_controller = std::make_unique<StandardFractalController>(*m_calculation, *m_sound);
    ASSERT_FALSE(m_controller->done());
    ASSERT_EQ(0, m_calculations);
}

class TestSteppedStandardController : public TestPreparedStandardController
{
protected:
    void SetUp() override;
};

void TestSteppedStandardController::SetUp()
{
    TestStandardFractalController::SetUp();
    clear_image();
    g_orbit_delay = 1;
    prepare_controller();
}

class TestSuspendingStandardController : public TestSteppedStandardController
{
protected:
    void SetUp() override;
};

void TestSuspendingStandardController::SetUp()
{
    g_size_dot = 128;
    TestSteppedStandardController::SetUp();
}

TEST_F(TestPreparedStandardController, iterateRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

TEST_F(TestPreparedStandardController, resultRequiresCompletion)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

TEST_F(TestPreparedStandardController, suspendRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

class TestStartedStandardController : public TestPreparedStandardController
{
protected:
    void SetUp() override;
};

void TestStartedStandardController::SetUp()
{
    TestPreparedStandardController::SetUp();
    m_controller->start();
    ASSERT_FALSE(m_controller->done());
    ASSERT_EQ(0, m_calculations);
}

TEST_F(TestStartedStandardController, duplicateStartIsRejected)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

TEST_F(TestStartedStandardController, explicitSuspensionCompletesTheController)
{
    m_controller->suspend();

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
}

TEST_F(TestSteppedStandardController, callerAdvancesExistingChunksToCompletion)
{
    m_controller->start();

    m_controller->iterate();

    EXPECT_EQ(1, m_calculations);
    EXPECT_FALSE(m_controller->done());
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !m_controller->done(); ++chunks)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(0, m_controller->result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
    m_controller->iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestStandardFractalController, smallImageDoesNotIntroduceExtraYields)
{
    clear_image();
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    StandardFractal calculation;
    SoundSession sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{calculation, sound};
    controller.start();

    controller.iterate();

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSuspendingStandardController, explicitSuspensionRestoresMarkerAndResumesSavedWork)
{
    m_controller->start();
    for (int chunks = 0; chunks < 100 && m_calculations == 0; ++chunks)
    {
        ASSERT_FALSE(m_controller->done());
        m_controller->iterate();
    }
    ASSERT_GT(m_calculations, 0);

    m_controller->suspend();

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    m_controller.reset();
    m_calculation.reset();
    StandardFractal resumed;
    SoundSession resumed_sound{g_sound, SoundSession::Ownership::BORROWED};
    StandardFractalController controller{resumed, resumed_sound};
    controller.start();
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSteppedStandardController, interruptedStepReleasesHandlersBeforeReturning)
{
    m_interrupt_after = 5;
    m_controller->start();

    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !m_controller->done(); ++chunks)
    {
        m_controller->iterate();
    }

    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestSteppedStandardController, failedStepAbortsCalculationAndReleasesPresentation)
{
    s_calculate = []() -> int { throw std::runtime_error("Calculation failed"); };
    m_controller->start();
    bool failed{};

    for (int chunks = 0; chunks < 100 && !failed; ++chunks)
    {
        try
        {
            m_controller->iterate();
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_STREQ("Calculation failed", error.what());
            failed = true;
        }
    }

    EXPECT_TRUE(failed);
    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_THAT(m_pixels, Each(0));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

class TestLyapunovController : public TestShowDotController
{
protected:
    void SetUp() override
    {
        TestShowDotController::SetUp();
        g_inside_method = ColorMethod::COLOR;
        g_fractal_type = FractalType::LYAPUNOV;
        m_specific.type = FractalType::LYAPUNOV;
    }
};

class TestPreparedLyapunovController : public TestLyapunovController
{
protected:
    void SetUp() override;
    void TearDown() override;
    void prepare_controller();

    std::unique_ptr<Lyapunov> m_lyapunov;
    std::unique_ptr<LyapunovController> m_controller;
};

void TestPreparedLyapunovController::SetUp()
{
    TestLyapunovController::SetUp();
    prepare_controller();
}

void TestPreparedLyapunovController::TearDown()
{
    m_controller.reset();
    m_lyapunov.reset();
    TestLyapunovController::TearDown();
}

void TestPreparedLyapunovController::prepare_controller()
{
    m_lyapunov = std::make_unique<Lyapunov>();
    m_controller = std::make_unique<LyapunovController>(*m_lyapunov);
    ASSERT_FALSE(m_controller->done());
    ASSERT_EQ(0, m_calculations);
}

class TestSteppedLyapunovController : public TestPreparedLyapunovController
{
protected:
    void SetUp() override;
};

void TestSteppedLyapunovController::SetUp()
{
    TestLyapunovController::SetUp();
    clear_image();
    g_orbit_delay = 1;
    prepare_controller();
}

class TestSuspendingLyapunovController : public TestSteppedLyapunovController
{
protected:
    void SetUp() override;
};

void TestSuspendingLyapunovController::SetUp()
{
    g_size_dot = 128;
    TestSteppedLyapunovController::SetUp();
}

TEST_F(TestPreparedLyapunovController, iterateRequiresStart)
{
    EXPECT_THROW(m_controller->iterate(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

TEST_F(TestPreparedLyapunovController, resultRequiresCompletion)
{
    EXPECT_THROW(m_controller->result(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

TEST_F(TestPreparedLyapunovController, suspendRequiresStart)
{
    EXPECT_THROW(m_controller->suspend(), std::logic_error);

    EXPECT_EQ(0, m_calculations);
}

class TestStartedLyapunovController : public TestPreparedLyapunovController
{
protected:
    void SetUp() override;
};

void TestStartedLyapunovController::SetUp()
{
    TestPreparedLyapunovController::SetUp();
    m_controller->start();
    ASSERT_FALSE(m_controller->done());
    ASSERT_EQ(0, m_calculations);
}

TEST_F(TestStartedLyapunovController, duplicateStartIsRejected)
{
    EXPECT_THROW(m_controller->start(), std::logic_error);
}

TEST_F(TestStartedLyapunovController, explicitSuspensionCompletesTheController)
{
    m_controller->suspend();

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestSteppedLyapunovController, callerAdvancesExistingMarkerChunksToCompletion)
{
    m_controller->start();

    m_controller->iterate();

    EXPECT_EQ(1, m_calculations);
    EXPECT_FALSE(m_controller->done());
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !m_controller->done(); ++chunks)
    {
        m_controller->iterate();
    }
    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(0, m_controller->result());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
    m_controller->iterate();
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

class TestZeroDelayLyapunovController : public TestPreparedLyapunovController, public WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestZeroDelayLyapunovController::SetUp()
{
    TestLyapunovController::SetUp();
    clear_image();
    m_calculations = 0;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_show_dot = GetParam();
    prepare_controller();
}

TEST_P(TestZeroDelayLyapunovController, retainsSingleChunkCompletion)
{
    m_controller->start();

    m_controller->iterate();

    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(0, m_controller->result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

INSTANTIATE_TEST_SUITE_P(Markers, TestZeroDelayLyapunovController, Values(-1, 15),
    [](const TestParamInfo<int> &info) { return info.param < 0 ? "Disabled" : "Enabled"; });

TEST_F(TestSuspendingLyapunovController, explicitSuspensionRestoresMarkerAndResumesSavedWork)
{
    m_controller->start();
    m_controller->iterate();
    ASSERT_EQ(1, m_calculations);

    m_controller->suspend();

    EXPECT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    m_controller.reset();
    m_lyapunov.reset();
    Lyapunov resumed;
    LyapunovController controller{resumed};
    controller.start();
    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !controller.done(); ++chunks)
    {
        controller.iterate();
    }

    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_EQ(WIDTH * HEIGHT, m_calculations);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestSteppedLyapunovController, interruptedStepReleasesHandlersBeforeReturning)
{
    m_interrupt_after = 5;
    m_controller->start();

    for (int chunks = 0; chunks < WIDTH * HEIGHT + 20 && !m_controller->done(); ++chunks)
    {
        m_controller->iterate();
    }

    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ('x', m_key);
    EXPECT_THAT(m_pixels, Each(Ne(15)));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestSteppedLyapunovController, orbitToggleDoesNotInterruptCalculation)
{
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    m_controller->start();
    m_controller->iterate();

    EXPECT_TRUE(dispatch_keyboard_key('o'));
    EXPECT_TRUE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_TRUE(dispatch_keyboard_key('O'));
    EXPECT_FALSE(g_show_orbit);
    EXPECT_FALSE(calc_interrupt_requested());
    EXPECT_FALSE(m_controller->done());

    m_controller->suspend();

    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestSteppedLyapunovController, failedStepAbortsCalculationAndReleasesPresentation)
{
    s_calculate = []() -> int { throw std::runtime_error("Calculation failed"); };
    m_controller->start();

    EXPECT_THROW(m_controller->iterate(), std::runtime_error);

    ASSERT_TRUE(m_controller->done());
    EXPECT_EQ(-1, m_controller->result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_THAT(m_pixels, Each(0));
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

} // namespace id::test
