// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/pixel.h>
#include <engine/StandardPass.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/engine_timer.h>
#include <engine/log_map.h>
#include <engine/LogicalScreen.h>
#include <engine/NullSoundOutput.h>
#include <engine/OrbitPlot.h>
#include <engine/Potential.h>
#include <engine/resume.h>
#include <engine/show_dot.h>
#include <engine/spindac.h>
#include <engine/StandardFractal.h>
#include <engine/VideoInfo.h>
#include <engine/work_list.h>
#include <fractals/fractalp.h>
#include <fractals/lyapunov.h>
#include <io/disk_storage.h>
#include <misc/debug_flags.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <ctime>
#include <initializer_list>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

namespace
{

int unexpected_calculation()
{
    throw std::logic_error{"Pixel routing must not dispatch calculation"};
}

struct PixelPassCase
{
    const char *name;
    CalcMode mode;
    Point2i next;
};

constexpr PixelPassCase PIXEL_PASSES[]{
    {"OnePass", CalcMode::ONE_PASS, {1, 0}},
    {"TwoPass", CalcMode::TWO_PASS, {2, 0}},
    {"BoundaryTrace", CalcMode::BOUNDARY_TRACE, {1, 0}},
    {"SolidGuess", CalcMode::SOLID_GUESS, {4, 0}},
    {"Diffusion", CalcMode::DIFFUSION, {4, 4}},
    {"Tesseral", CalcMode::TESSERAL, {1, 0}},
};

std::vector<Point2i> s_calculated;
int s_pending_result{};
bool s_checkerboard{};
int s_color{7};

bool initialize_image()
{
    return true;
}

int calculate_selected_pixel()
{
    if (s_pending_result != 0)
    {
        const int result{s_pending_result};
        s_pending_result = 0;
        return result;
    }
    s_calculated.push_back({g_col, g_row});
    g_color = s_checkerboard ? 1 + (g_col + g_row) % 2 : s_color;
    g_plot(g_col, g_row, g_color);
    return g_color;
}

int calculate_log_map_pixel()
{
    if (s_calculated.size() == 4 && s_pending_result != 0)
    {
        const int result{s_pending_result};
        s_pending_result = 0;
        return result;
    }
    s_calculated.push_back({g_col, g_row});
    g_real_color_iter = 10 + (g_col + g_row) % 3;
    g_color = 7;
    g_plot(g_col, g_row, g_color);
    return g_color;
}

void PrintTo(const PixelPassCase &value, std::ostream *out)
{
    *out << value.name;
}

} // namespace

class TestStandardPass : public Test
{
protected:
    void SetUp() override
    {
        std::copy_n(g_work_list, MAX_CALC_WORK, m_saved_work.begin());
        set_frame(8, 8);
        g_dispatch.set_calc_type(unexpected_calculation);
        set_normal_dot();
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int col, int row) { return m_pixels.at(row * m_width + col); }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke(
                [this](int col, int row, int color)
                {
                    m_pixels.at(row * m_width + col) = color;
                    ++m_plot_count;
                }));
        EXPECT_CALL(m_driver, key_pressed()).Times(0);
        EXPECT_CALL(m_driver, get_key()).Times(0);
    }

    void TearDown() override
    {
        std::copy(m_saved_work.begin(), m_saved_work.end(), g_work_list);
    }

    bool select_next(StandardPass &pass)
    {
        for (int chunks = 0; chunks < 16; ++chunks)
        {
            if (pass.select_pixel())
            {
                return true;
            }
        }
        return false;
    }

    void set_frame(int width, int height)
    {
        m_width = width;
        g_screen_x_dots = width;
        g_screen_y_dots = height;
        g_logical_screen = {width, height, 0, 0, width - 1.0, height - 1.0};
        g_start_pt = {0, 0};
        g_stop_pt = {width - 1, height - 1};
        g_i_start_pt = g_start_pt;
        g_i_stop_pt = g_stop_pt;
        g_begin_pt = g_start_pt;
        m_pixels.assign(width * height, 0);
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{8, 8, 0, 0, 7, 7}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 8};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 8};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<Point2i> m_saved_start{g_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_stop{g_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_i_start{g_i_start_pt, Point2i{0, 0}};
    ValueSaver<Point2i> m_saved_i_stop{g_i_stop_pt, Point2i{7, 7}};
    ValueSaver<Point2i> m_saved_begin{g_begin_pt, Point2i{0, 0}};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<int> m_saved_current_col{g_current_column};
    ValueSaver<int> m_saved_current_row{g_current_row};
    ValueSaver<int> m_saved_current_pass{g_current_pass};
    ValueSaver<int> m_saved_total_passes{g_total_passes};
    ValueSaver<int> m_saved_color{g_color};
    ValueSaver<int> m_saved_fill_color{g_fill_color, -1};
    ValueSaver<int> m_saved_show_dot{g_show_dot, 0};
    ValueSaver<int> m_saved_inside_color{g_inside_color, 0};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::ITER};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::ITER};
    ValueSaver<int> m_saved_work_count{g_num_work_list, 0};
    ValueSaver<int> m_saved_work_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_stop_pass{g_stop_pass, 0};
    ValueSaver<bool> m_saved_three_pass{g_three_pass, false};
    ValueSaver<bool> m_saved_quick{g_quick_calc, false};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<bool> m_saved_periodicity{g_reset_periodicity};
    ValueSaver<Passes> m_saved_passes{g_passes};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::NONE};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<decltype(g_put_color)> m_saved_put_color{g_put_color, put_color_a};
    std::array<WorkList, MAX_CALC_WORK> m_saved_work{};
    std::vector<int> m_pixels;
    int m_width{};
    int m_plot_count{};
};

class TestStandardPassPixel : public TestStandardPass, public WithParamInterface<PixelPassCase>
{
};

TEST_P(TestStandardPassPixel, selectionIsRetainedWithoutCalculationOrPlotting)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    g_col = 99;
    g_row = 99;

    pass.begin_chunk();
    pass.select(GetParam().mode);
    ASSERT_TRUE(pass.select_pixel());

    EXPECT_EQ(GetParam().mode, pass.calc_mode());
    EXPECT_FALSE(pass.done());
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
    EXPECT_EQ(0, g_col);
    EXPECT_EQ(0, g_row);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassPixel, completionAdvancesTheActivePassOnce)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    g_plot(0, 0, 13);
    g_color = 99;
    g_col = 99;
    g_row = 99;

    pass.complete_pixel(13);

    EXPECT_EQ(13, g_color);
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ(GetParam().next, pass.selected_pixel());
    if (GetParam().mode == CalcMode::TWO_PASS)
    {
        EXPECT_EQ(13, m_pixels[1]);
        EXPECT_EQ(13, m_pixels[8]);
        EXPECT_EQ(13, m_pixels[9]);
    }
    if (GetParam().mode == CalcMode::DIFFUSION)
    {
        EXPECT_EQ(13, m_pixels[7 * 8 + 7]);
    }
}

class TestStandardPassInstance : public TestStandardPass
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::optional<StandardPass> m_pass;
};

void TestStandardPassInstance::SetUp()
{
    TestStandardPass::SetUp();
    m_pass.emplace();
}

void TestStandardPassInstance::TearDown()
{
    m_pass.reset();
    TestStandardPass::TearDown();
}

enum class StandardPassGuardState
{
    INITIAL,
    COMPLETED,
    RESET,
    CHANGED,
};

struct StandardPassGuardCase
{
    const char *name;
    StandardPassGuardState state;
};

struct StandardPassGuardInput
{
    std::string name;
    PixelPassCase pass;
    StandardPassGuardState state;
};

void PrintTo(const StandardPassGuardInput &value, std::ostream *str)
{
    *str << value.name;
}

static std::vector<StandardPassGuardInput> pixel_guard_inputs(const std::initializer_list<StandardPassGuardCase> states)
{
    std::vector<StandardPassGuardInput> inputs;
    for (const PixelPassCase &pass : PIXEL_PASSES)
    {
        for (const StandardPassGuardCase &state : states)
        {
            inputs.push_back({std::string{pass.name} + "_" + state.name, pass, state.state});
        }
    }
    return inputs;
}

class TestStandardPassSelectionGuard : public TestStandardPassInstance,
                                       public WithParamInterface<StandardPassGuardInput>
{
protected:
    void SetUp() override;
    void verify_continuation();
};

void TestStandardPassSelectionGuard::SetUp()
{
    TestStandardPassInstance::SetUp();
    const StandardPassGuardInput &input{GetParam()};
    m_pass->select(input.pass.mode);
    if (input.state == StandardPassGuardState::INITIAL)
    {
        return;
    }
    ASSERT_TRUE(select_next(*m_pass));
    if (input.state == StandardPassGuardState::RESET)
    {
        m_pass->reset();
        return;
    }
    g_plot(0, 0, 13);
    if (input.state == StandardPassGuardState::COMPLETED)
    {
        g_color = 99;
        g_col = 99;
        g_row = 99;
    }
    m_pass->complete_pixel(13);
    if (input.state == StandardPassGuardState::CHANGED)
    {
        ASSERT_TRUE(select_next(*m_pass));
        EXPECT_EQ(input.pass.next, m_pass->selected_pixel());
        const CalcMode other{input.pass.mode == CalcMode::ONE_PASS ? CalcMode::TWO_PASS : CalcMode::ONE_PASS};
        m_pass->select(other);
    }
}

void TestStandardPassSelectionGuard::verify_continuation()
{
    const StandardPassGuardInput &input{GetParam()};
    if (input.state == StandardPassGuardState::COMPLETED || input.state == StandardPassGuardState::RESET)
    {
        m_pass->select(input.pass.mode);
    }
    ASSERT_TRUE(select_next(*m_pass));
    const Point2i expected{input.state == StandardPassGuardState::COMPLETED ? input.pass.next : Point2i{0, 0}};
    EXPECT_EQ(expected, m_pass->selected_pixel());
}

class TestStandardPassAccessorGuard : public TestStandardPassSelectionGuard
{
};

TEST_P(TestStandardPassAccessorGuard, rejectionPreservesContinuation)
{
    EXPECT_THROW(m_pass->selected_pixel(), std::logic_error);

    verify_continuation();
}

INSTANTIATE_TEST_SUITE_P(PixelStates, TestStandardPassAccessorGuard,
    ValuesIn(pixel_guard_inputs(
        {{"Initial", StandardPassGuardState::INITIAL}, {"Completed", StandardPassGuardState::COMPLETED},
            {"Reset", StandardPassGuardState::RESET}, {"PassChanged", StandardPassGuardState::CHANGED}})),
    [](const TestParamInfo<StandardPassGuardInput> &info) { return info.param.name; });

class TestStandardPassCompletionGuard : public TestStandardPassSelectionGuard
{
};

TEST_P(TestStandardPassCompletionGuard, rejectionPreservesContinuation)
{
    EXPECT_THROW(m_pass->complete_pixel(7), std::logic_error);

    verify_continuation();
}

INSTANTIATE_TEST_SUITE_P(PixelStates, TestStandardPassCompletionGuard,
    ValuesIn(pixel_guard_inputs({{"Initial", StandardPassGuardState::INITIAL},
        {"Completed", StandardPassGuardState::COMPLETED}, {"Reset", StandardPassGuardState::RESET}})),
    [](const TestParamInfo<StandardPassGuardInput> &info) { return info.param.name; });

class TestStandardPassResetSelection : public TestStandardPassSelectionGuard
{
};

TEST_P(TestStandardPassResetSelection, hasNoPixelRequest)
{
    EXPECT_FALSE(m_pass->select_pixel());
}

INSTANTIATE_TEST_SUITE_P(PixelStates, TestStandardPassResetSelection,
    ValuesIn(pixel_guard_inputs({{"Reset", StandardPassGuardState::RESET}})),
    [](const TestParamInfo<StandardPassGuardInput> &info) { return info.param.name; });

TEST_P(TestStandardPassPixel, resetDiscardsPendingSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));

    pass.reset();

    EXPECT_EQ(CalcMode::NONE, pass.calc_mode());
    EXPECT_TRUE(pass.done());
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

TEST_P(TestStandardPassPixel, changingPassDiscardsPendingSelection)
{
    StandardPass pass;
    pass.select(GetParam().mode);
    ASSERT_TRUE(select_next(pass));
    g_plot(0, 0, 13);
    pass.complete_pixel(13);
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ(GetParam().next, pass.selected_pixel());
    const CalcMode other{GetParam().mode == CalcMode::ONE_PASS ? CalcMode::TWO_PASS : CalcMode::ONE_PASS};

    pass.select(other);

    EXPECT_EQ(other, pass.calc_mode());
    ASSERT_TRUE(select_next(pass));
    EXPECT_EQ((Point2i{0, 0}), pass.selected_pixel());
}

INSTANTIATE_TEST_SUITE_P(PixelPasses, TestStandardPassPixel, ValuesIn(PIXEL_PASSES),
    [](const TestParamInfo<PixelPassCase> &info) -> std::string { return info.param.name; });

class TestEmptyStandardPass : public TestStandardPassInstance
{
};

TEST_F(TestEmptyStandardPass, hasNoCalculationMode)
{
    EXPECT_EQ(CalcMode::NONE, m_pass->calc_mode());
}

TEST_F(TestEmptyStandardPass, hasNoPixelRequest)
{
    EXPECT_FALSE(m_pass->select_pixel());
}

TEST_F(TestEmptyStandardPass, rejectsSelectedPixelAccess)
{
    EXPECT_THROW(m_pass->selected_pixel(), std::logic_error);
}

TEST_F(TestEmptyStandardPass, rejectsPixelCompletion)
{
    EXPECT_THROW(m_pass->complete_pixel(7), std::logic_error);
}

TEST_F(TestEmptyStandardPass, iterationIsAlreadyComplete)
{
    EXPECT_TRUE(m_pass->iterate());
}

struct OrbitPassInput
{
    const char *name;
    CalcMode mode;
};

void PrintTo(const OrbitPassInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestStandardPassOrbit : public TestStandardPassInstance, public WithParamInterface<OrbitPassInput>
{
protected:
    void SetUp() override;
};

void TestStandardPassOrbit::SetUp()
{
    TestStandardPassInstance::SetUp();
    m_pass->select(GetParam().mode);
}

TEST_P(TestStandardPassOrbit, rejectsPixelSelection)
{
    EXPECT_THROW(m_pass->select_pixel(), std::logic_error);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassOrbit, rejectsBeginningPixelChunk)
{
    EXPECT_THROW(m_pass->begin_chunk(), std::logic_error);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassOrbit, rejectsPixelCompletionQuery)
{
    EXPECT_THROW(m_pass->done(), std::logic_error);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassOrbit, rejectsSelectedPixelAccess)
{
    EXPECT_THROW(m_pass->selected_pixel(), std::logic_error);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassOrbit, rejectsPixelCompletion)
{
    EXPECT_THROW(m_pass->complete_pixel(7), std::logic_error);
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardPassOrbit, retainsSelectedModeWithoutPlotting)
{
    EXPECT_EQ(GetParam().mode, m_pass->calc_mode());
    EXPECT_EQ(0, m_plot_count);
}

INSTANTIATE_TEST_SUITE_P(OrbitModes, TestStandardPassOrbit,
    Values(OrbitPassInput{"SynchronousOrbit", CalcMode::SYNCHRONOUS_ORBIT}, OrbitPassInput{"Orbit", CalcMode::ORBIT}),
    [](const TestParamInfo<OrbitPassInput> &info) { return info.param.name; });

TEST_F(TestStandardPass, suspensionBeforeSynchronousTraversalQueuesWorkWithoutStarting)
{
    StandardPass pass;
    pass.select(CalcMode::SYNCHRONOUS_ORBIT);

    pass.suspend();

    ASSERT_EQ(1, g_num_work_list);
    EXPECT_EQ(g_start_pt, g_work_list[0].start);
    EXPECT_EQ(g_stop_pt, g_work_list[0].stop);
    EXPECT_EQ(g_begin_pt, g_work_list[0].begin);
    EXPECT_TRUE(pass.iterate());
    pass.suspend();
    EXPECT_EQ(1, g_num_work_list);
    EXPECT_EQ(0, m_plot_count);
}

class TestStandardFractal : public TestStandardPass
{
protected:
    void SetUp() override
    {
        TestStandardPass::SetUp();
        s_calculated.clear();
        s_pending_result = 0;
        s_checkerboard = false;
        s_color = 7;
        m_specific.flags = FractalFlags::NONE;
        m_specific.symmetry = SymmetryType::NONE;
        m_specific.per_image = initialize_image;
        m_specific.calc_type = calculate_selected_pixel;
        g_dispatch = FractalDispatch{m_specific};
    }

    void TearDown() override
    {
        end_resume();
        TestStandardPass::TearDown();
    }

    bool select_next(StandardFractal &calculation)
    {
        for (int chunks = 0; chunks < 100 && !calculation.done(); ++chunks)
        {
            if (calculation.select_pixel())
            {
                return true;
            }
        }
        return false;
    }

    FractalSpecific m_specific{*get_fractal_specific(FractalType::MANDEL)};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific, &m_specific};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<SymmetryType> m_saved_force_symmetry{g_force_symmetry, SymmetryType::NONE};
    ValueSaver<SymmetryType> m_saved_image_symmetry{g_symmetry};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<long> m_saved_log_map{g_log_map_flag, 0};
    ValueSaver<long> m_saved_distance_estimator{g_distance_estimator, 0};
    ValueSaver<int> m_saved_marker{g_show_dot, -1};
    ValueSaver<bool> m_saved_overlay{g_show_orbit, false};
    ValueSaver<int> m_saved_check_interval{g_keyboard_check_interval};
};

class TestRenderLifecycle : public TestStandardFractal
{
protected:
    ValueSaver<long> m_saved_time{g_calc_time, 0};
    ValueSaver<long> m_saved_timer_start{g_engine_timer_start, 0};
    ValueSaver<long> m_saved_interval{g_timer_interval, 0};
    ValueSaver<bool> m_saved_start_orbit{g_start_show_orbit, false};
    ValueSaver<bool> m_saved_true_color{g_true_color, false};
};

class AutoLogMapFixture : public TestStandardFractal
{
protected:
    void initialize_log_map(long log_map);

    ValueSaver<long> m_saved_max_iterations{g_max_iterations, 128};
    ValueSaver<long> m_saved_real_iterations{g_real_color_iter};
    ValueSaver<bool> m_saved_auto_log_map{g_log_map_auto_calculate, false};
    const std::vector<Point2i> m_perimeter{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}, {7, 1},
        {7, 2}, {7, 3}, {7, 4}, {7, 5}, {7, 6}, {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, {1, 7},
        {2, 7}, {3, 7}, {4, 7}, {5, 7}, {6, 7}};
};

void AutoLogMapFixture::initialize_log_map(const long log_map)
{
    TestStandardFractal::SetUp();
    g_log_map_flag = log_map;
    m_specific.calc_type = calculate_log_map_pixel;
    g_dispatch = FractalDispatch{m_specific};
}

class TestAutoLogMap : public AutoLogMapFixture, public WithParamInterface<long>
{
protected:
    void SetUp() override;
};

void TestAutoLogMap::SetUp()
{
    initialize_log_map(GetParam());
}

class TestAutoLogMapYield : public TestAutoLogMap
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::optional<StandardFractal> m_calculation;
};

void TestAutoLogMapYield::SetUp()
{
    TestAutoLogMap::SetUp();
    s_pending_result = CALC_YIELDED;
    m_calculation.emplace();
    m_calculation->resume();
    m_calculation->iterate();
    ASSERT_EQ(4U, s_calculated.size());
    ASSERT_FALSE(m_calculation->done());
}

void TestAutoLogMapYield::TearDown()
{
    m_calculation.reset();
    TestAutoLogMap::TearDown();
}

TEST_P(TestAutoLogMapYield, yieldPreservesTheSampleAndPerimeterOrder)
{
    StandardFractal &calculation{*m_calculation};

    ASSERT_EQ(4U, s_calculated.size());
    EXPECT_EQ(GetParam(), g_log_map_flag);
    EXPECT_EQ(10L, g_max_iterations);
    EXPECT_FALSE(calculation.done());

    calculation.iterate();

    EXPECT_EQ(m_perimeter, s_calculated);
    EXPECT_EQ(GetParam() > 0 ? 10L : -10L, g_log_map_flag);
    EXPECT_EQ(128L, g_max_iterations);
    EXPECT_FALSE(calculation.done());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(m_perimeter.size() + 64, s_calculated.size());
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_P(TestAutoLogMapYield, samplingRejectsSelectedPixelAccess)
{
    EXPECT_THROW(m_calculation->selected_pixel(), std::logic_error);
}

INSTANTIATE_TEST_SUITE_P(LogMapSigns, TestAutoLogMapYield, Values(2L, -2L),
    [](const TestParamInfo<long> &info) { return info.param > 0 ? "Positive" : "Negative"; });

TEST_P(TestAutoLogMap, interruptionSavesUnfinishedSamplingForResume)
{
    s_pending_result = CALC_INTERRUPTED;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_FALSE(g_resume_data.empty());
    ASSERT_EQ(4U, s_calculated.size());
    EXPECT_EQ(GetParam(), g_log_map_flag);
    EXPECT_EQ(128L, g_max_iterations);
    g_resuming = true;
    StandardFractal resumed;
    resumed.resume();

    resumed.iterate();

    EXPECT_EQ(m_perimeter, s_calculated);
    EXPECT_EQ(GetParam() > 0 ? 10L : -10L, g_log_map_flag);
    EXPECT_EQ(128L, g_max_iterations);
    resumed.iterate();
    EXPECT_TRUE(resumed.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_P(TestAutoLogMap, abandoningSamplingRestoresTheIterationLimit)
{
    s_pending_result = CALC_YIELDED;
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    ASSERT_EQ(10L, g_max_iterations);

    calculation.abort();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(128L, g_max_iterations);
}

struct AutoLogMapResumeInput
{
    std::string name;
    long log_map;
    CalcMode mode;
    bool other_work;
};

void PrintTo(const AutoLogMapResumeInput &value, std::ostream *out)
{
    *out << value.name;
}

std::vector<AutoLogMapResumeInput> auto_log_map_resume_inputs()
{
    std::vector<AutoLogMapResumeInput> inputs;
    for (const long sign : {2L, -2L})
    {
        for (const OrbitPassInput &pass :
            {OrbitPassInput{"OnePass", CalcMode::ONE_PASS}, OrbitPassInput{"TwoPass", CalcMode::TWO_PASS},
                OrbitPassInput{"SynchronousOrbit", CalcMode::SYNCHRONOUS_ORBIT}})
        {
            for (const bool queued : {false, true})
            {
                inputs.push_back({std::string{sign > 0 ? "Positive_" : "Negative_"} + pass.name +
                        (queued ? "_QueuedWork" : "_NoQueuedWork"),
                    sign, pass.mode, queued});
            }
        }
    }
    return inputs;
}

class TestAutoLogMapResume : public AutoLogMapFixture, public WithParamInterface<AutoLogMapResumeInput>
{
protected:
    void SetUp() override;
};

void TestAutoLogMapResume::SetUp()
{
    initialize_log_map(GetParam().log_map);
    g_dispatch.set_orbit_calc([] { return 1; });
}

TEST_P(TestAutoLogMapResume, suspensionAfterSamplingPreservesUnstartedTraversal)
{
    const AutoLogMapResumeInput &input{GetParam()};
    g_std_calc_mode = input.mode;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_resuming = false;
    g_log_map_flag = input.log_map;
    m_pixels.assign(64, -1);
    StandardFractal baseline;
    baseline.resume();
    for (int chunks = 0; chunks < 1000 && !baseline.done(); ++chunks)
    {
        baseline.iterate();
    }
    ASSERT_TRUE(baseline.done());
    ASSERT_EQ(CalcStatus::COMPLETED, g_calc_status);
    ASSERT_THAT(m_pixels, Each(Ge(0)));
    const std::vector<int> expected{m_pixels};

    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_resuming = false;
    g_log_map_flag = input.log_map;
    s_calculated.clear();
    StandardFractal calculation;
    calculation.resume();
    for (int chunks = 0; chunks < 100 && g_log_map_flag == input.log_map; ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_EQ(input.log_map > 0 ? 10L : -10L, g_log_map_flag);
    ASSERT_EQ(m_perimeter, s_calculated);
    ASSERT_FALSE(calculation.done());
    ASSERT_EQ(0, g_num_work_list);
    const WorkList active{g_start_pt, g_stop_pt, g_begin_pt, g_work_symmetry, g_work_pass};
    if (input.other_work)
    {
        ASSERT_FALSE(add_work_list({1, 1}, {3, 3}, {1, 1}, 1, 0));
    }

    calculation.suspend();

    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());
    ASSERT_EQ(input.other_work ? 2 : 1, g_num_work_list);
    EXPECT_EQ(active.start, g_work_list[0].start);
    EXPECT_EQ(active.stop, g_work_list[0].stop);
    EXPECT_EQ(active.begin, g_work_list[0].begin);
    EXPECT_EQ(active.pass, g_work_list[0].pass);
    EXPECT_EQ(active.symmetry, g_work_list[0].symmetry);
    ASSERT_EQ(2, start_resume());
    int saved_count{};
    std::array<WorkList, MAX_CALC_WORK> saved_work{};
    get_resume_len(sizeof(saved_count), &saved_count, sizeof(g_work_list), saved_work.data(), 0);
    EXPECT_EQ(g_num_work_list, saved_count);
    EXPECT_EQ(active.start, saved_work[0].start);
    EXPECT_EQ(active.stop, saved_work[0].stop);
    EXPECT_EQ(active.begin, saved_work[0].begin);
    EXPECT_EQ(active.pass, saved_work[0].pass);
    EXPECT_EQ(active.symmetry, saved_work[0].symmetry);
    m_pixels.assign(64, -1);
    s_calculated.clear();
    g_resuming = true;
    StandardFractal resumed;
    resumed.resume();
    for (int chunks = 0; chunks < 1000 && !resumed.done(); ++chunks)
    {
        resumed.iterate();
    }
    ASSERT_TRUE(resumed.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(128L, g_max_iterations);
    EXPECT_EQ(input.log_map > 0 ? 10L : -10L, g_log_map_flag);
    EXPECT_THAT(m_pixels, Each(Ge(0)));
    EXPECT_EQ(expected, m_pixels);
}

INSTANTIATE_TEST_SUITE_P(ResumeInputs, TestAutoLogMapResume, ValuesIn(auto_log_map_resume_inputs()),
    [](const TestParamInfo<AutoLogMapResumeInput> &info) { return info.param.name; });
INSTANTIATE_TEST_SUITE_P(LogMapSigns, TestAutoLogMap, Values(2L, -2L));

struct StickyLogMapResumeInput
{
    std::string name;
    long log_map;
    OrbitDrawMode draw_mode;
    bool other_work;
};

void PrintTo(const StickyLogMapResumeInput &input, std::ostream *out)
{
    *out << input.name;
}

std::vector<StickyLogMapResumeInput> sticky_log_map_resume_inputs()
{
    std::vector<StickyLogMapResumeInput> inputs;
    for (const long sign : {2L, -2L})
    {
        for (const OrbitDrawMode mode : {OrbitDrawMode::RECTANGLE, OrbitDrawMode::LINE})
        {
            for (const bool queued : {false, true})
            {
                inputs.push_back({std::string{sign > 0 ? "Positive_" : "Negative_"} +
                        (mode == OrbitDrawMode::RECTANGLE ? "Rectangle_" : "Line_") +
                        (queued ? "QueuedWork" : "NoQueuedWork"),
                    sign, mode, queued});
            }
        }
    }
    return inputs;
}

class TestStickyLogMapResume : public AutoLogMapFixture, public WithParamInterface<StickyLogMapResumeInput>
{
protected:
    void SetUp() override;
    void start_after_sampling(StandardFractal &calculation);
    void complete(StandardFractal &calculation);

    static constexpr int WIDTH{8};
    static constexpr int HEIGHT{6};
    static constexpr int SAMPLE_COUNT{2 * (WIDTH + HEIGHT - 2)};
    ValueSaver<OrbitDrawMode> m_saved_draw_mode{g_draw_mode};
    ValueSaver<ImageRegion> m_saved_region{g_image_region};
    ValueSaver<bool> m_saved_keep_coords{g_keep_screen_coords, false};
    ValueSaver<long> m_saved_orbit_interval{g_orbit_interval, 1};
    ValueSaver<int> m_saved_skip_points{g_orbit_skip_points, 0};
    ValueSaver<int> m_saved_periodicity_check{g_periodicity_check, 0};
    ValueSaver<DComplex> m_saved_new_z{g_new_z};
    ValueSaver<SoundOutputPtr> m_saved_sound{g_sound, std::make_shared<NullSoundOutput>()};
};

void TestStickyLogMapResume::SetUp()
{
    initialize_log_map(GetParam().log_map);
    set_frame(WIDTH, HEIGHT);
    g_std_calc_mode = CalcMode::ORBIT;
    g_draw_mode = GetParam().draw_mode;
    g_inside_color = 7;
    g_image_region.m_min = {0.0, 0.0};
    g_image_region.m_max = {2.0, 2.0};
    g_image_region.m_3rd = {0.0, 0.0};
    m_specific.calc_type = standard_fractal_type;
    m_specific.per_image = []
    {
        g_dispatch.set_calc_type(calculate_log_map_pixel);
        return true;
    };
    m_specific.per_pixel = []
    {
        s_calculated.push_back({g_col, g_row});
        return 0;
    };
    m_specific.orbit_calc = []
    {
        g_new_z = {1.0 + g_col * 0.1, 1.0 + g_row * 0.1};
        return 0;
    };
    g_dispatch = FractalDispatch{m_specific};
}

void TestStickyLogMapResume::start_after_sampling(StandardFractal &calculation)
{
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    g_resuming = false;
    g_log_map_flag = GetParam().log_map;
    m_pixels.assign(WIDTH * HEIGHT, -1);
    s_calculated.clear();
    ASSERT_TRUE(calculation.resume().empty());
    for (int chunks = 0; chunks < 100 && g_log_map_flag == GetParam().log_map; ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_EQ(GetParam().log_map > 0 ? 10L : -10L, g_log_map_flag);
    ASSERT_EQ(SAMPLE_COUNT, s_calculated.size());
    ASSERT_FALSE(calculation.done());
    ASSERT_EQ(0, g_num_work_list);
    if (GetParam().other_work)
    {
        ASSERT_FALSE(add_work_list({1, 1}, {5, 3}, {1, 1}, 0, 0));
    }
}

void TestStickyLogMapResume::complete(StandardFractal &calculation)
{
    for (int chunks = 0; chunks < 1000 && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    ASSERT_EQ(CalcStatus::COMPLETED, g_calc_status);
}

TEST_P(TestStickyLogMapResume, suspensionAfterSamplingPreservesUnstartedTraversal)
{
    StandardFractal baseline;
    ASSERT_NO_FATAL_FAILURE(start_after_sampling(baseline));
    const std::vector<Point2i> expected_samples{s_calculated};
    ASSERT_NO_FATAL_FAILURE(complete(baseline));
    ASSERT_GT(s_calculated.size(), SAMPLE_COUNT);
    const std::vector<Point2i> expected_seeds{s_calculated.begin() + SAMPLE_COUNT, s_calculated.end()};
    const std::vector<int> expected_pixels{m_pixels};

    StandardFractal calculation;
    ASSERT_NO_FATAL_FAILURE(start_after_sampling(calculation));
    ASSERT_EQ(expected_samples, s_calculated);
    const WorkList active{g_start_pt, g_stop_pt, g_begin_pt, g_work_symmetry, g_work_pass};
    const int plots_before_suspension{m_plot_count};

    calculation.suspend();

    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());
    ASSERT_EQ(GetParam().other_work ? 2 : 1, g_num_work_list);
    EXPECT_EQ(active.start, g_work_list[0].start);
    EXPECT_EQ(active.stop, g_work_list[0].stop);
    EXPECT_EQ(active.begin, g_work_list[0].begin);
    EXPECT_EQ(active.pass, g_work_list[0].pass);
    EXPECT_EQ(active.symmetry, g_work_list[0].symmetry);
    EXPECT_EQ(expected_samples, s_calculated);
    EXPECT_EQ(plots_before_suspension, m_plot_count);

    s_calculated.clear();
    g_resuming = true;
    StandardFractal resumed;
    ASSERT_TRUE(resumed.resume().empty());
    ASSERT_NO_FATAL_FAILURE(complete(resumed));

    EXPECT_EQ(expected_seeds, s_calculated);
    EXPECT_EQ(expected_pixels, m_pixels);
    EXPECT_EQ(128L, g_max_iterations);
    EXPECT_EQ(GetParam().log_map > 0 ? 10L : -10L, g_log_map_flag);
}

INSTANTIATE_TEST_SUITE_P(ResumeInputs, TestStickyLogMapResume, ValuesIn(sticky_log_map_resume_inputs()),
    [](const TestParamInfo<StickyLogMapResumeInput> &info) { return info.param.name; });

TEST_F(TestRenderLifecycle, completionAccountsForElapsedImageTimeOnce)
{
    StandardFractal calculation;
    EXPECT_TRUE(calculation.resume().empty());
    ASSERT_TRUE(select_next(calculation));
    EXPECT_TRUE(calculation.calculate_pixel());
    g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;
    for (int chunks = 0; chunks < 100 && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);

    calculation.finish();

    EXPECT_GE(g_calc_time, 100L);
    const long completed_time{g_calc_time};
    calculation.finish();
    EXPECT_EQ(completed_time, g_calc_time);
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestRenderLifecycle, suspensionAndResumeAccumulateDistinctIntervals)
{
    StandardFractal calculation;
    EXPECT_TRUE(calculation.resume().empty());
    ASSERT_TRUE(select_next(calculation));
    EXPECT_TRUE(calculation.calculate_pixel());
    g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;

    calculation.suspend();

    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_GE(g_calc_time, 100L);
    const long suspended_time{g_calc_time};
    calculation.finish();
    EXPECT_EQ(suspended_time, g_calc_time);

    StandardFractal resumed;
    EXPECT_TRUE(resumed.resume().empty());
    EXPECT_EQ(suspended_time, g_calc_time);
    ASSERT_TRUE(select_next(resumed));
    g_engine_timer_start = static_cast<long>(std::clock()) - 2 * CLOCKS_PER_SEC;
    EXPECT_TRUE(resumed.calculate_pixel());
    for (int chunks = 0; chunks < 100 && !resumed.done(); ++chunks)
    {
        resumed.iterate();
    }
    ASSERT_TRUE(resumed.done());
    resumed.finish();

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_GE(g_calc_time, suspended_time + 200);
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestRenderLifecycle, parameterChangeDiscardsSuspendedTraversalAndTiming)
{
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    calculation.calculate_pixel();
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    ASSERT_FALSE(g_resume_data.empty());
    g_calc_time = 123;
    g_calc_status = CalcStatus::PARAMS_CHANGED;
    s_calculated.clear();

    StandardFractal restarted;
    restarted.resume();

    EXPECT_FALSE(g_resuming);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0L, g_calc_time);
    ASSERT_TRUE(select_next(restarted));
    EXPECT_EQ((Point2i{0, 0}), restarted.selected_pixel());
    restarted.abort();
}

TEST_F(TestRenderLifecycle, abandoningSetupDoesNotChargeAStaleTimer)
{
    g_timer_interval = 12345;
    {
        StandardFractal calculation;
        EXPECT_TRUE(calculation.resume().empty());
    }

    EXPECT_EQ(0L, g_calc_time);
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
}

TEST_F(TestRenderLifecycle, finishingUnfinishedWorkRestoresDispatchAndDiscardsResumeData)
{
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    EXPECT_TRUE(calculation.calculate_pixel());

    calculation.finish();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(calculate_selected_pixel, g_dispatch.calc_type());
}

TEST_F(TestRenderLifecycle, failureRestoresDispatchAndDiscardsPartialWork)
{
    g_dispatch.set_per_image(
        []() -> bool
        {
            g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;
            throw std::runtime_error("Calculation setup failed");
        });
    const FractalDispatch dispatch{g_dispatch};

    EXPECT_THROW(
        {
            StandardFractal calculation;
            calculation.resume();
            calculation.iterate();
        },
        std::runtime_error);

    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_TRUE(g_log_map_table.empty());
    EXPECT_EQ(dispatch.calc_type(), g_dispatch.calc_type());
    EXPECT_EQ(dispatch.per_image(), g_dispatch.per_image());
    EXPECT_GE(g_calc_time, 100L);
}

TEST_F(TestRenderLifecycle, lyapunovUsesTheCommonSetupAndCleanup)
{
    g_colors = 2;
    g_log_map_flag = 1;
    g_inside_method = ColorMethod::COLOR;
    Lyapunov calculation;

    EXPECT_THAT(calculation.resume(), ElementsAre("Need at least 16 colors to use logmap"));
    EXPECT_EQ(0L, g_log_map_flag);
    for (int chunks = 0; chunks < 100 && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    g_engine_timer_start = static_cast<long>(std::clock()) - CLOCKS_PER_SEC;
    calculation.finish();

    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_GE(g_calc_time, 100L);
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestRenderLifecycle, activeCalculationCannotBeStartedAgain)
{
    StandardFractal calculation;
    calculation.resume();

    EXPECT_THROW(calculation.resume(), std::logic_error);

    calculation.abort();
}

class TestStandardFractalInstance : public TestStandardFractal
{
protected:
    void TearDown() override;
    void start_calculation(CalcMode mode);
    void finish_calculation();

    std::optional<StandardFractal> m_calculation;
};

void TestStandardFractalInstance::TearDown()
{
    m_calculation.reset();
    TestStandardFractal::TearDown();
}

void TestStandardFractalInstance::start_calculation(const CalcMode mode)
{
    g_std_calc_mode = mode;
    m_calculation.emplace();
    m_calculation->resume();
}

void TestStandardFractalInstance::finish_calculation()
{
    for (int chunks = 0; chunks < 1000 && !m_calculation->done(); ++chunks)
    {
        while (m_calculation->select_pixel())
        {
            ASSERT_LT(s_calculated.size(), 10000U);
            ASSERT_TRUE(m_calculation->calculate_pixel());
        }
    }
    ASSERT_TRUE(m_calculation->done());
}

class TestStandardFractalPixel : public TestStandardFractalInstance, public WithParamInterface<PixelPassCase>
{
};

TEST_P(TestStandardFractalPixel, selectedCoordinateIsObservableBeforeCalculation)
{
    start_calculation(GetParam().mode);
    StandardFractal &calculation{*m_calculation};

    ASSERT_TRUE(select_next(calculation));
    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    g_col = 99;
    g_row = 99;
    ASSERT_TRUE(calculation.select_pixel());

    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_EQ(0, m_plot_count);
}

TEST_P(TestStandardFractalPixel, completionUsesTheSelectedCoordinateAndAdvances)
{
    start_calculation(GetParam().mode);
    StandardFractal &calculation{*m_calculation};
    ASSERT_TRUE(select_next(calculation));
    g_col = 99;
    g_row = 99;
    ASSERT_TRUE(calculation.select_pixel());

    ASSERT_TRUE(calculation.calculate_pixel());

    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}));
    ASSERT_TRUE(select_next(calculation));
    EXPECT_EQ(GetParam().next, calculation.selected_pixel());
}

TEST_P(TestStandardFractalPixel, unfinishedCalculationRetainsSelectionAndContinuation)
{
    g_std_calc_mode = GetParam().mode;
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    s_pending_result = CALC_YIELDED;

    EXPECT_FALSE(calculation.calculate_pixel());

    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_EQ(0, m_plot_count);
    ASSERT_TRUE(calculation.select_pixel());
    ASSERT_TRUE(calculation.calculate_pixel());
    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}));
    ASSERT_TRUE(select_next(calculation));
    EXPECT_EQ(GetParam().next, calculation.selected_pixel());
}

TEST_P(TestStandardFractalPixel, interruptedCalculationDoesNotCompleteSelectedPixel)
{
    g_std_calc_mode = GetParam().mode;
    StandardFractal calculation;
    calculation.resume();
    ASSERT_TRUE(select_next(calculation));
    s_pending_result = CALC_INTERRUPTED;

    EXPECT_FALSE(calculation.calculate_pixel());

    EXPECT_EQ((Point2i{0, 0}), calculation.selected_pixel());
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_EQ(0, m_plot_count);
    EXPECT_FALSE(calculation.done());
}

TEST_P(TestStandardFractalPixel, separateOperationsCompleteTheImage)
{
    start_calculation(GetParam().mode);

    finish_calculation();

    EXPECT_TRUE(m_calculation->done());
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_THAT(m_pixels, Each(7));
}

enum class StandardFractalGuardState
{
    BEFORE_SELECTION,
    CONSUMED,
    IMAGE_COMPLETE,
};

struct StandardFractalGuardCase
{
    const char *name;
    StandardFractalGuardState state;
};

struct StandardFractalGuardInput
{
    std::string name;
    PixelPassCase pass;
    StandardFractalGuardState state;
};

void PrintTo(const StandardFractalGuardInput &value, std::ostream *str)
{
    *str << value.name;
}

static std::vector<StandardFractalGuardInput> fractal_guard_inputs(
    const std::initializer_list<StandardFractalGuardCase> states)
{
    std::vector<StandardFractalGuardInput> inputs;
    for (const PixelPassCase &pass : PIXEL_PASSES)
    {
        for (const StandardFractalGuardCase &state : states)
        {
            inputs.push_back({std::string{pass.name} + "_" + state.name, pass, state.state});
        }
    }
    return inputs;
}

class TestStandardFractalPixelGuard : public TestStandardFractalInstance,
                                      public WithParamInterface<StandardFractalGuardInput>
{
protected:
    void SetUp() override;
};

void TestStandardFractalPixelGuard::SetUp()
{
    TestStandardFractalInstance::SetUp();
    const StandardFractalGuardInput &input{GetParam()};
    start_calculation(input.pass.mode);
    if (input.state == StandardFractalGuardState::CONSUMED)
    {
        ASSERT_TRUE(select_next(*m_calculation));
        g_col = 99;
        g_row = 99;
        ASSERT_TRUE(m_calculation->select_pixel());
        ASSERT_TRUE(m_calculation->calculate_pixel());
    }
    else if (input.state == StandardFractalGuardState::IMAGE_COMPLETE)
    {
        ASSERT_NO_FATAL_FAILURE(finish_calculation());
    }
}

class TestStandardFractalPixelCalculationGuard : public TestStandardFractalPixelGuard
{
};

TEST_P(TestStandardFractalPixelCalculationGuard, rejectionPreservesCalculationState)
{
    EXPECT_THROW(m_calculation->calculate_pixel(), std::logic_error);

    if (GetParam().state == StandardFractalGuardState::IMAGE_COMPLETE)
    {
        EXPECT_TRUE(m_calculation->done());
    }
    else
    {
        ASSERT_TRUE(select_next(*m_calculation));
        const Point2i expected{
            GetParam().state == StandardFractalGuardState::CONSUMED ? GetParam().pass.next : Point2i{0, 0}};
        EXPECT_EQ(expected, m_calculation->selected_pixel());
    }
}

INSTANTIATE_TEST_SUITE_P(PixelStates, TestStandardFractalPixelCalculationGuard,
    ValuesIn(fractal_guard_inputs({{"BeforeSelection", StandardFractalGuardState::BEFORE_SELECTION},
        {"Consumed", StandardFractalGuardState::CONSUMED},
        {"ImageComplete", StandardFractalGuardState::IMAGE_COMPLETE}})),
    [](const TestParamInfo<StandardFractalGuardInput> &info) { return info.param.name; });

class TestStandardFractalPixelAccessGuard : public TestStandardFractalPixelGuard
{
};

TEST_P(TestStandardFractalPixelAccessGuard, completedImageRejectsSelectedPixelAccess)
{
    EXPECT_THROW(m_calculation->selected_pixel(), std::logic_error);
    EXPECT_TRUE(m_calculation->done());
}

INSTANTIATE_TEST_SUITE_P(PixelStates, TestStandardFractalPixelAccessGuard,
    ValuesIn(fractal_guard_inputs({{"ImageComplete", StandardFractalGuardState::IMAGE_COMPLETE}})),
    [](const TestParamInfo<StandardFractalGuardInput> &info) { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(PixelPasses, TestStandardFractalPixel, ValuesIn(PIXEL_PASSES),
    [](const TestParamInfo<PixelPassCase> &info) -> std::string { return info.param.name; });

class TestStandardFractalScanline : public TestStandardFractal, public WithParamInterface<OrbitPassInput>
{
};

TEST_P(TestStandardFractalScanline, scanlineCalculationRemainsBatched)
{
    g_std_calc_mode = GetParam().mode;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
    EXPECT_THAT(m_pixels, Each(7));
}

INSTANTIATE_TEST_SUITE_P(ScanlineModes, TestStandardFractalScanline,
    Values(OrbitPassInput{"OnePass", CalcMode::ONE_PASS}, OrbitPassInput{"TwoPass", CalcMode::TWO_PASS}),
    [](const TestParamInfo<OrbitPassInput> &info) { return info.param.name; });

TEST_F(TestStandardFractal, scanlineCalculationAcceptsZeroColor)
{
    g_std_calc_mode = CalcMode::TWO_PASS;
    s_color = 0;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(0, g_color);
    EXPECT_THAT(m_pixels, Each(0));
}

TEST_F(TestStandardFractal, boundaryScanKeepsItsCalculationChunk)
{
    set_frame(600, 1);
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    s_checkerboard = true;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(512U, s_calculated.size());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(600U, s_calculated.size());
}

TEST_F(TestStandardFractal, boundaryOutlineKeepsItsCalculationChunk)
{
    set_frame(600, 2);
    g_std_calc_mode = CalcMode::BOUNDARY_TRACE;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(258U, s_calculated.size());
}

TEST_F(TestStandardFractal, solidGuessKeepsSeedCalculationsBatchedByRow)
{
    set_frame(16, 12);
    g_std_calc_mode = CalcMode::SOLID_GUESS;
    StandardFractal calculation;
    calculation.resume();
    calculation.iterate();
    EXPECT_TRUE(s_calculated.empty());

    calculation.iterate();

    EXPECT_THAT(s_calculated, ElementsAre(Point2i{0, 0}, Point2i{4, 0}, Point2i{8, 0}, Point2i{12, 0}));
    s_calculated.clear();
    calculation.iterate();
    EXPECT_THAT(s_calculated,
        ElementsAre(Point2i{0, 4}, Point2i{4, 4}, Point2i{8, 4}, Point2i{12, 4}, Point2i{14, 2}, Point2i{14, 0}));
    EXPECT_EQ(0, g_current_row);
    s_calculated.clear();
    calculation.iterate();
    EXPECT_THAT(s_calculated,
        ElementsAre(Point2i{0, 8}, Point2i{4, 8}, Point2i{8, 8}, Point2i{12, 8}, Point2i{14, 6}, Point2i{14, 4}));
    EXPECT_EQ(4, g_current_row);
}

TEST_F(TestStandardFractal, diffusionKeepsItsCounterChunk)
{
    set_frame(64, 32);
    g_std_calc_mode = CalcMode::DIFFUSION;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(1024U, s_calculated.size());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(2048U, s_calculated.size());
    EXPECT_THAT(m_pixels, Each(7));
}

TEST_F(TestStandardFractal, tesseralKeepsBatchedEdges)
{
    set_frame(600, 4);
    g_std_calc_mode = CalcMode::TESSERAL;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_EQ(512U, s_calculated.size());
    EXPECT_FALSE(calculation.done());
    calculation.iterate();
    EXPECT_EQ(1112U, s_calculated.size());
    calculation.iterate();
    EXPECT_EQ(1204U, s_calculated.size());
    EXPECT_FALSE(calculation.done());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_THAT(m_pixels, Each(7));
}

class TestStandardPotential : public TestStandardFractal
{
protected:
    void SetUp() override;
    void TearDown() override;
    void open_storage();
    void request_storage(CalcMode mode = CalcMode::ONE_PASS);

    std::optional<StandardFractal> m_calculation;
};

void TestStandardPotential::SetUp()
{
    TestStandardFractal::SetUp();
    end_disk();
    init_memory();
    g_potential.flag = true;
    g_potential.store_16bit = true;
}

void TestStandardPotential::TearDown()
{
    m_calculation.reset();
    end_disk();
    EXPECT_TRUE(exit_check().empty());
    TestStandardFractal::TearDown();
}

void TestStandardPotential::open_storage()
{
    start_disk_storage(8, 16, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    g_disk_16_bit = true;
}

void TestStandardPotential::request_storage(const CalcMode mode)
{
    g_std_calc_mode = mode;
    m_calculation.emplace();
    m_calculation->resume();
    m_calculation->iterate();
    ASSERT_TRUE(m_calculation->potential_storage_required());
}

TEST_F(TestStandardPotential, storageRequirementStopsBeforeWorkOrInput)
{
    g_std_calc_mode = CalcMode::SOLID_GUESS;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_TRUE(calculation.potential_storage_required());
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ(CalcMode::SOLID_GUESS, g_std_calc_mode);
    EXPECT_TRUE(s_calculated.empty());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(0, m_plot_count);
    calculation.iterate();
    EXPECT_FALSE(calculation.select_pixel());
    EXPECT_TRUE(s_calculated.empty());
    calculation.abort();
}

TEST_F(TestStandardPotential, availableStorageSelectsOnePassBeforeCalculating)
{
    ASSERT_NO_FATAL_FAILURE(request_storage(CalcMode::SOLID_GUESS));
    StandardFractal &calculation{*m_calculation};
    open_storage();

    calculation.complete_potential_storage(true);

    EXPECT_FALSE(calculation.potential_storage_required());
    EXPECT_EQ(CalcMode::ONE_PASS, g_std_calc_mode);
    EXPECT_TRUE(g_potential.store_16bit);
    EXPECT_TRUE(s_calculated.empty());
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestStandardPotential, unavailableStorageRetainsRequestedTraversal)
{
    ASSERT_NO_FATAL_FAILURE(request_storage(CalcMode::TWO_PASS));
    StandardFractal &calculation{*m_calculation};

    calculation.complete_potential_storage(false);

    EXPECT_FALSE(calculation.potential_storage_required());
    EXPECT_EQ(CalcMode::TWO_PASS, g_std_calc_mode);
    EXPECT_TRUE(g_potential.flag);
    EXPECT_FALSE(g_potential.store_16bit);
    calculation.iterate();
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
}

TEST_F(TestStandardPotential, resumeKeepsExistingStorageAndContinuesAtSavedPixel)
{
    ASSERT_NO_FATAL_FAILURE(request_storage());
    StandardFractal &calculation{*m_calculation};
    open_storage();
    write_disk_pixel(3, 11, 123);
    calculation.complete_potential_storage(true);
    ASSERT_TRUE(calculation.select_pixel());
    ASSERT_TRUE(calculation.calculate_pixel());
    calculation.suspend();
    ASSERT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    g_resuming = true;
    StandardFractal resumed;
    resumed.resume();

    resumed.iterate();

    EXPECT_FALSE(resumed.potential_storage_required());
    EXPECT_TRUE(resumed.done());
    EXPECT_EQ(64U, s_calculated.size());
    ASSERT_GE(s_calculated.size(), 2U);
    EXPECT_EQ((Point2i{0, 0}), s_calculated[0]);
    EXPECT_EQ((Point2i{1, 0}), s_calculated[1]);
    EXPECT_EQ(123, read_disk_pixel(3, 11));
    EXPECT_TRUE(g_disk_16_bit);
}

TEST_F(TestStandardPotential, threePassFollowupRequestsFreshStorage)
{
    ASSERT_NO_FATAL_FAILURE(request_storage(CalcMode::THREE_PASS));
    StandardFractal &calculation{*m_calculation};
    open_storage();
    calculation.complete_potential_storage(true);
    calculation.iterate();
    calculation.iterate();

    ASSERT_TRUE(calculation.potential_storage_required());
    EXPECT_EQ(64U, s_calculated.size());
    calculation.complete_potential_storage(true);
    calculation.iterate();

    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(128U, s_calculated.size());
}

struct PotentialStorageGuardInput
{
    const char *name;
    bool consumed;
    bool completion;
};

void PrintTo(const PotentialStorageGuardInput &value, std::ostream *out)
{
    *out << value.name;
}

class TestPotentialStorageGuard : public TestStandardPotential, public WithParamInterface<PotentialStorageGuardInput>
{
protected:
    void SetUp() override;
};

void TestPotentialStorageGuard::SetUp()
{
    TestStandardPotential::SetUp();
    if (GetParam().consumed)
    {
        ASSERT_NO_FATAL_FAILURE(request_storage());
        m_calculation->complete_potential_storage(false);
    }
    else
    {
        m_calculation.emplace();
    }
}

TEST_P(TestPotentialStorageGuard, storageCompletionRequiresAnOutstandingRequest)
{
    EXPECT_THROW(m_calculation->complete_potential_storage(GetParam().completion), std::logic_error);
    if (GetParam().consumed)
    {
        m_calculation->abort();
    }
}

INSTANTIATE_TEST_SUITE_P(RequestStates, TestPotentialStorageGuard,
    Values(PotentialStorageGuardInput{"Absent", false, false}, PotentialStorageGuardInput{"Consumed", true, true}),
    [](const TestParamInfo<PotentialStorageGuardInput> &info) { return info.param.name; });

TEST_F(TestStandardFractal, inactivePotentialDoesNotRequestStorage)
{
    g_potential.store_16bit = true;
    StandardFractal calculation;
    calculation.resume();

    calculation.iterate();

    EXPECT_FALSE(calculation.potential_storage_required());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(64U, s_calculated.size());
}

} // namespace id::test
