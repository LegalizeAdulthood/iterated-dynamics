// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/inverse_julia.h>
#include <ui/InverseJuliaFractalController.h>

#include "MockDriver.h"

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
#include <engine/NonStandardFractal.h>
#include <engine/pixel.h>
#include <engine/pixel_grid.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/lorenz.h>
#include <io/disk_storage.h>
#include <io/library.h>
#include <io/special_dirs.h>
#include <misc/debug_flags.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <ui/editpal.h>
#include <ui/big_while_loop.h>
#include <ui/help.h>
#include <ui/id_keys.h>
#include <ui/InverseJuliaFractalController.h>
#include <ui/mouse.h>
#include <ui/main_menu_switch.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::help;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class TestInverseJuliaController : public Test
{
protected:
    using Plot = void (*)(int, int, int);

    void SetUp() override
    {
        set_fractal_type(FractalType::MANDEL);
        set_normal_dot();
        alloc_pixel_grid();
        EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
        EXPECT_CALL(m_driver, read_pixel(_, _))
            .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels[y * 100 + x]; }));
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int x, int y, int color) { m_pixels[y * 100 + x] = color; }));
    }

    void TearDown() override
    {
        free_pixel_grid();
    }

    void expect_exit_key(int key)
    {
        EXPECT_CALL(m_driver, key_pressed())
            .WillRepeatedly(Invoke(
                [this, key]()
                {
                    EXPECT_TRUE(g_using_jiim);
                    EXPECT_EQ(MouseLook::POSITION, g_look_at_mouse);
                    EXPECT_TRUE(g_cursor_mouse_tracking);
                    return m_key_read ? 0 : key;
                }));
        EXPECT_CALL(m_driver, get_key())
            .WillOnce(Invoke(
                [this, key]()
                {
                    m_key_read = true;
                    return key;
                }));
        EXPECT_CALL(m_driver, unget_key(key))
            .WillOnce(Invoke(
                [](int)
                {
                    EXPECT_FALSE(g_using_jiim);
                    EXPECT_EQ(MouseLook::IGNORE_MOUSE, g_look_at_mouse);
                    EXPECT_FALSE(g_cursor_mouse_tracking);
                }));
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{100, 100, 0, 0, 99, 99}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 100};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 100};
    ValueSaver<int> m_saved_vesa_width{g_vesa_x_res};
    ValueSaver<int> m_saved_vesa_height{g_vesa_y_res};
    ValueSaver<int> m_saved_video_x{g_video_start_x, 0};
    ValueSaver<int> m_saved_video_y{g_video_start_y, 0};
    ValueSaver<int> m_saved_colors{g_colors, 16};
    ValueSaver<int> m_saved_mask{g_and_color, 15};
    ValueSaver<Plot> m_saved_put_color{g_put_color, put_color_a};
    ValueSaver<bool> m_saved_inverse{g_has_inverse, true};
    ValueSaver<bool> m_saved_dac{g_got_real_dac, false};
    ValueSaver<bool> m_saved_using_jiim{g_using_jiim, false};
    ValueSaver<MouseLook> m_saved_mouse{g_look_at_mouse, MouseLook::IGNORE_MOUSE};
    ValueSaver<bool> m_saved_tracking{g_cursor_mouse_tracking, false};
    ValueSaver<DebugFlags> m_saved_debug{g_debug_flag, DebugFlags::PREVENT_MIIM};
    ValueSaver<HelpLabels> m_saved_help{g_help_mode, HelpLabels::NONE};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<const FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{-2.5, -1.5}, {1.5, 1.5}, {-2.5, -1.5}}};
    ValueSaver<LDouble> m_saved_dx{g_delta_x, 4.0 / 99};
    ValueSaver<LDouble> m_saved_dy{g_delta_y, 3.0 / 99};
    ValueSaver<LDouble> m_saved_dx2{g_delta_x2, 0};
    ValueSaver<LDouble> m_saved_dy2{g_delta_y2, 0};
    ValueSaver<DComplex> m_saved_selection{g_save_c, DComplex{-0.5, 0}};
    ValueSaver<DComplex> m_saved_julia_c{g_julia_c};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<std::vector<Byte>> m_saved_line{g_line_buff};
    std::vector<int> m_pixels = std::vector<int>(100 * 100, 3);
    bool m_key_read{};
};

TEST_F(TestInverseJuliaController, selectionRestoresDisplayAndRequeuesKey)
{
    expect_exit_key(ID_KEY_SPACE);

    jiim(JIIMType::JIIM);

    EXPECT_DOUBLE_EQ(-0.5, g_julia_c.x);
    EXPECT_DOUBLE_EQ(0, g_julia_c.y);
    EXPECT_THAT(m_pixels, Each(3));
    EXPECT_TRUE(g_line_buff.empty());
    EXPECT_EQ(DebugFlags::PREVENT_MIIM, g_debug_flag);
    EXPECT_EQ(HelpLabels::NONE, g_help_mode);
    EXPECT_FALSE(dispatch_keyboard_key(ID_KEY_LEFT_ARROW));
}

TEST_F(TestInverseJuliaController, menuExitRestoresControllerState)
{
    expect_exit_key('x');

    jiim(JIIMType::JIIM);

    EXPECT_DOUBLE_EQ(JULIA_C_NOT_SET, g_julia_c.x);
    EXPECT_DOUBLE_EQ(JULIA_C_NOT_SET, g_julia_c.y);
    EXPECT_THAT(m_pixels, Each(3));
    EXPECT_TRUE(g_line_buff.empty());
    EXPECT_FALSE(dispatch_keyboard_key(ID_KEY_LEFT_ARROW));
}

TEST_F(TestInverseJuliaController, frothyBasinOrbitWindowUsesSetupWithoutARenderer)
{
    ValueSaver saved_mapping{g_params[0], 1.0};
    ValueSaver saved_shading{g_params[1], 0.0};
    ValueSaver saved_a{g_params[2], 1.02871376822};
    set_fractal_type(FractalType::FROTHY_BASIN);
    ASSERT_EQ(nullptr, g_cur_fractal_specific->calc_type);
    expect_exit_key('x');
    MainContext context{};
    context.key = 'o';

    EXPECT_EQ(CommandResult::PRESENTATION_CHANGED, main_menu_switch(context));

    EXPECT_TRUE(context.orbits_window_active);
    EXPECT_TRUE(context.orbits_window_key_pending);
    EXPECT_TRUE(m_key_read);
    EXPECT_THAT(m_pixels, Each(3));
    EXPECT_TRUE(g_line_buff.empty());
    EXPECT_FALSE(dispatch_keyboard_key(ID_KEY_LEFT_ARROW));
}

TEST_F(TestInverseJuliaController, mouseScopeTracksCursorAndUnsubscribes)
{
    CrossHairCursor cursor;
    {
        InverseJuliaMouseScope scope{cursor};

        mouse_notify_move(23, 31, 0);

        EXPECT_EQ(23, cursor.get_x());
        EXPECT_EQ(31, cursor.get_y());
        EXPECT_EQ(MouseLook::POSITION, g_look_at_mouse);
        EXPECT_TRUE(g_cursor_mouse_tracking);
    }

    mouse_notify_move(70, 80, 0);

    EXPECT_EQ(23, cursor.get_x());
    EXPECT_EQ(31, cursor.get_y());
    EXPECT_EQ(MouseLook::IGNORE_MOUSE, g_look_at_mouse);
    EXPECT_FALSE(g_cursor_mouse_tracking);
}

class TestInverseJuliaQueue : public TestInverseJuliaController
{
protected:
    void SetUp() override;
    void TearDown() override;
    void expect_dialog();

    ValueSaver<bool> m_saved_first_init{g_first_init, false};
    ValueSaver<BatchMode> m_saved_batch{g_init_batch, BatchMode::NONE};
    ValueSaver<std::filesystem::path> m_saved_temp{g_temp_dir};
    ValueSaver<Major> m_saved_major{g_major_method, Major::BREADTH_FIRST};
    ValueSaver<bool> m_saved_resuming{g_resuming, false};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::PARAMS_CHANGED};
    ValueSaver<bool> m_saved_show_orbit{g_show_orbit, false};
    ValueSaver<double> m_saved_param0{g_params[0], -1.0};
    ValueSaver<double> m_saved_param1{g_params[1], 0.0};
    ValueSaver<double> m_saved_param2{g_params[2], 4.0};
    ValueSaver<double> m_saved_param3{g_params[3], 1024.0};
    ValueSaver<DComplex> m_saved_old_z{g_old_z};
    ValueSaver<DComplex> m_saved_new_z{g_new_z};
    std::filesystem::path m_directory;
};

void TestInverseJuliaQueue::SetUp()
{
    TestInverseJuliaController::SetUp();
    free_queue();
    init_memory();
    reset_calc_interrupted();
    g_debug_flag = DebugFlags::NONE;
    m_directory = std::filesystem::temp_directory_path() /
        ("id-inverse-queue-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(m_directory);
    g_temp_dir = m_directory / "missing";
    set_save_library(m_directory);
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
}

void TestInverseJuliaQueue::TearDown()
{
    free_queue();
    EXPECT_TRUE(exit_check().empty());
    reset_calc_interrupted();
    clear_save_library();
    std::error_code ignored;
    std::filesystem::remove_all(m_directory, ignored);
    TestInverseJuliaController::TearDown();
}

void TestInverseJuliaQueue::expect_dialog()
{
    EXPECT_CALL(m_driver, stack_screen()).Times(AnyNumber());
    EXPECT_CALL(m_driver, unstack_screen()).Times(AnyNumber());
    EXPECT_CALL(m_driver, move_cursor(_, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
}

TEST_F(TestInverseJuliaQueue, successfulPreparationLeavesAnEmptyUsableQueue)
{
    ASSERT_TRUE(prepare_inverse_julia_queue(8192));

    EXPECT_EQ(8192UL, queue_capacity());
    EXPECT_TRUE(queue_empty());
    EXPECT_EQ(1, enqueue_float(1.0F, 2.0F));
    EXPECT_EQ((DComplex{1.0, 2.0}), dequeue_float());
}

TEST_F(TestInverseJuliaQueue, retryUsesASmallerQueue)
{
    g_debug_flag = DebugFlags::FORCE_MEMORY_FROM_DISK;
    expect_dialog();
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, get_key())
        .WillOnce(Invoke(
            [this]
            {
                EXPECT_EQ(0UL, queue_capacity());
                EXPECT_FALSE(g_disk_flag);
                EXPECT_TRUE(exit_check().empty());
                g_temp_dir = m_directory;
                return ID_KEY_ENTER;
            }));

    ASSERT_TRUE(prepare_inverse_julia_queue(8192));

    EXPECT_EQ(4096UL, queue_capacity());
    EXPECT_EQ(MemoryLocation::DISK, disk_video_memory_type());
    EXPECT_EQ(1, enqueue_float(3.0F, 4.0F));
    EXPECT_EQ((DComplex{3.0, 4.0}), dequeue_float());
}

TEST_F(TestInverseJuliaQueue, cancellationDoesNotRetryAnotherQueueSize)
{
    g_debug_flag = DebugFlags::FORCE_MEMORY_FROM_DISK;
    expect_dialog();
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));

    EXPECT_FALSE(prepare_inverse_julia_queue(8192));

    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestInverseJuliaQueue, retriesStopAtTheMinimumQueueSize)
{
    g_debug_flag = DebugFlags::FORCE_MEMORY_FROM_DISK;
    expect_dialog();
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_FALSE(prepare_inverse_julia_queue(2048));

    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
}

TEST_F(TestInverseJuliaQueue, diskVideoRejectionPreservesTheDisplayStorage)
{
    start_disk_storage(16, 16, 256, memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY));
    write_disk_pixel(3, 4, 123);
    EXPECT_CALL(m_driver, is_disk()).WillOnce(Return(true));
    expect_dialog();
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("Don't try this in disk video mode, kids...\n")));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_FALSE(prepare_inverse_julia_queue(8192));

    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_TRUE(g_disk_flag);
    EXPECT_EQ(123, read_disk_pixel(3, 4));
}

TEST_F(TestInverseJuliaQueue, standaloneControllerSeedsThenReleasesThePreparedQueue)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    EXPECT_CALL(m_driver, key_pressed())
        .WillOnce(Invoke(
            []
            {
                EXPECT_EQ(32768UL, queue_capacity());
                EXPECT_NEAR((1 + std::sqrt(5.0)) / 2, dequeue_float().x, 1e-6);
                return 'x';
            }))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return('x'));
    EXPECT_CALL(m_driver, unget_key('x'));

    InverseJuliaFractalController controller{calculation};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    EXPECT_EQ(-1, controller.result());

    EXPECT_EQ(Major::BREADTH_FIRST, g_major_method);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestInverseJuliaQueue, standaloneControllerOwnsTheRandomWalkFallback)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    g_debug_flag = DebugFlags::FORCE_MEMORY_FROM_DISK;
    expect_dialog();
    EXPECT_CALL(m_driver, buzzer(Buzzer::PROBLEM));
    EXPECT_CALL(m_driver, put_string(_, _, _, StrEq("Not enough memory: switching to random walk.\n")));
    int pending_key{};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([&pending_key] { return pending_key; }));
    EXPECT_CALL(m_driver, get_key())
        .WillOnce(Return(ID_KEY_ESC))
        .WillOnce(Invoke(
            [&pending_key]
            {
                pending_key = 'x';
                return ID_KEY_ENTER;
            }))
        .WillOnce(Invoke(
            [&pending_key]
            {
                EXPECT_EQ(Major::RANDOM_WALK, g_major_method);
                EXPECT_GT(g_new_z.x, 1.0);
                pending_key = 0;
                return 'x';
            }));
    EXPECT_CALL(m_driver, unget_key('x'));

    InverseJuliaFractalController controller{calculation};
    controller.start();
    while (!controller.done())
    {
        controller.iterate();
    }
    EXPECT_EQ(-1, controller.result());

    EXPECT_EQ(Major::RANDOM_WALK, g_major_method);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
}

TEST_F(TestInverseJuliaQueue, stepwiseControllerAdvancesOnePointAndLeavesCompletedQueueWithCalculation)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    g_dispatch.set_orbit_calc(
        []
        {
            g_new_z.x += 1.0;
            return g_new_z.x >= 3.0 ? -1 : 0;
        });
    g_new_z = {};
    InverseJuliaFractalController controller{calculation};
    EXPECT_THROW(controller.iterate(), std::logic_error);
    EXPECT_THROW(controller.result(), std::logic_error);
    EXPECT_THROW(controller.suspend(), std::logic_error);

    controller.start();

    EXPECT_FALSE(controller.done());
    EXPECT_EQ(32768UL, queue_capacity());
    EXPECT_DOUBLE_EQ(0.0, g_new_z.x);
    EXPECT_THROW(controller.start(), std::logic_error);
    controller.iterate();
    EXPECT_DOUBLE_EQ(1.0, g_old_z.x);
    EXPECT_FALSE(controller.done());
    controller.iterate();
    EXPECT_DOUBLE_EQ(2.0, g_old_z.x);
    EXPECT_FALSE(controller.done());
    controller.iterate();
    EXPECT_DOUBLE_EQ(3.0, g_old_z.x);
    ASSERT_TRUE(controller.done());
    EXPECT_EQ(0, controller.result());
    EXPECT_TRUE(calculation.completed());
    EXPECT_EQ(32768UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    controller.iterate();
    EXPECT_DOUBLE_EQ(3.0, g_new_z.x);
    EXPECT_THROW(controller.suspend(), std::logic_error);

    calculation.finish(false);

    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_TRUE(exit_check().empty());
}

TEST_F(TestInverseJuliaQueue, stepwiseControllerTogglesOrbitsAndReturnsCommandsBeforeAnotherPoint)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    g_dispatch.set_orbit_calc(
        []
        {
            g_new_z.x += 1.0;
            return 0;
        });
    g_new_z = {};
    InverseJuliaFractalController controller{calculation};
    controller.start();
    int pending_key{'o'};
    EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Invoke([&pending_key] { return pending_key; }));
    EXPECT_CALL(m_driver, get_key()).WillRepeatedly(Invoke([&pending_key] { return std::exchange(pending_key, 0); }));
    EXPECT_CALL(m_driver, unget_key('x')).WillOnce(Invoke([&pending_key](int key) { pending_key = key; }));

    controller.iterate();

    EXPECT_TRUE(g_show_orbit);
    EXPECT_DOUBLE_EQ(1.0, g_new_z.x);
    EXPECT_FALSE(controller.done());
    EXPECT_FALSE(calc_interrupt_requested());
    pending_key = 'O';
    controller.iterate();
    EXPECT_FALSE(g_show_orbit);
    EXPECT_DOUBLE_EQ(2.0, g_new_z.x);
    EXPECT_FALSE(controller.done());
    pending_key = 'x';
    controller.iterate();
    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_DOUBLE_EQ(2.0, g_new_z.x);
    EXPECT_EQ('x', pending_key);
    EXPECT_TRUE(calc_interrupt_requested());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestInverseJuliaQueue, stepwiseSuspensionReleasesQueueAndKeyboardScope)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    InverseJuliaFractalController controller{calculation};
    controller.start();
    ASSERT_EQ(32768UL, queue_capacity());

    controller.suspend();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_TRUE(g_resume_data.empty());
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    EXPECT_THROW(controller.suspend(), std::logic_error);
}

TEST_F(TestInverseJuliaQueue, stepwiseCalculationFailureReleasesQueueAndKeyboardScope)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    g_dispatch.set_orbit_calc([]() -> int { throw std::runtime_error("Orbit failed"); });
    InverseJuliaFractalController controller{calculation};
    controller.start();
    ASSERT_EQ(32768UL, queue_capacity());

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    controller.iterate();
}

TEST_F(TestInverseJuliaQueue, stepwiseInputFailureClosesBeforeComputingAPoint)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    g_dispatch.set_orbit_calc([]() -> int { throw std::logic_error("Unexpected orbit calculation"); });
    InverseJuliaFractalController controller{calculation};
    controller.start();
    EXPECT_CALL(m_driver, key_pressed()).WillOnce(Throw(std::runtime_error("Input failed")));

    EXPECT_THROW(controller.iterate(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestInverseJuliaQueue, stepwisePreparationFailureTerminatesTheSelectedOperation)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    EXPECT_CALL(m_driver, is_disk()).WillOnce(Throw(std::runtime_error("Storage check failed")));
    InverseJuliaFractalController controller{calculation};

    EXPECT_THROW(controller.start(), std::runtime_error);

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_TRUE(calculation.done());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
}

TEST_F(TestInverseJuliaQueue, stepwiseControllerRejectsOldResumeWithoutPollingOrAllocating)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    g_calc_status = CalcStatus::RESUMABLE;
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    ASSERT_TRUE(calculation.done());
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    InverseJuliaFractalController controller{calculation};

    controller.start();

    EXPECT_TRUE(controller.done());
    EXPECT_EQ(-1, controller.result());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(dispatch_keyboard_key('o'));
    controller.iterate();
    calculation.finish(false);
}

TEST_F(TestInverseJuliaQueue, stepwiseControllerDestructionReleasesOnlyItsKeyboardScope)
{
    set_fractal_type(FractalType::INVERSE_JULIA);
    NonStandardFractal calculation;
    calculation.start();
    ASSERT_EQ(NonStandardFractal::Operation::INVERSE_JULIA, calculation.select());
    {
        InverseJuliaFractalController controller{calculation};
        controller.start();
        ASSERT_EQ(32768UL, queue_capacity());
    }

    EXPECT_FALSE(dispatch_keyboard_key('o'));
    EXPECT_FALSE(calculation.done());
    EXPECT_EQ(32768UL, queue_capacity());
    calculation.finish(true);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_EQ(CalcStatus::NON_RESUMABLE, g_calc_status);
}

TEST_F(TestInverseJuliaQueue, jiimControllerReleasesItsQueueAndRestoresTheImage)
{
    expect_exit_key(ID_KEY_SPACE);

    jiim(JIIMType::JIIM);

    EXPECT_THAT(m_pixels, Each(3));
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_TRUE(exit_check().empty());
}

} // namespace id::test
