// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/jiim.h>

#include <MockDriver.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/pixel.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/lorenz.h>
#include <io/disk_storage.h>
#include <misc/memory.h>
#include <misc/ValueSaver.h>
#include <ui/video.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <optional>
#include <ostream>
#include <stdexcept>
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

int start_orbit()
{
    g_old_z = g_init;
    g_new_z = g_init;
    return 0;
}

int step_orbit()
{
    g_new_z = {g_old_z.x + 1, g_old_z.y};
    return 0;
}

} // namespace

class TestInverseJulia : public Test
{
protected:
    using Plot = void (*)(int, int, int);

    void SetUp() override
    {
        free_queue();
        init_memory();
        set_normal_dot();
        g_dispatch.set_per_pixel(start_orbit);
        g_dispatch.set_orbit_calc(step_orbit);
        EXPECT_CALL(m_driver, write_pixel(_, _, _))
            .WillRepeatedly(Invoke([this](int x, int y, int color) { m_pixels.push_back({x, y, color}); }));
        EXPECT_CALL(m_driver, read_pixel(_, _)).WillRepeatedly(Return(0));
    }

    void TearDown() override
    {
        free_queue();
        EXPECT_TRUE(exit_check().empty());
    }

    void prepare_queue(unsigned long capacity);

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{100, 100, 0, 0, 99, 99}};
    ValueSaver<int> m_saved_width{g_screen_x_dots, 100};
    ValueSaver<int> m_saved_height{g_screen_y_dots, 100};
    ValueSaver<int> m_saved_colors{g_colors, 256};
    ValueSaver<int> m_saved_mask{g_and_color, 255};
    ValueSaver<int> m_saved_col{g_col};
    ValueSaver<int> m_saved_row{g_row};
    ValueSaver<bool> m_saved_inverse{g_has_inverse, true};
    ValueSaver<long> m_saved_maxiter{g_max_iterations, 150};
    ValueSaver<DComplex> m_saved_old{g_old_z};
    ValueSaver<DComplex> m_saved_new{g_new_z};
    ValueSaver<DComplex> m_saved_init{g_init};
    ValueSaver<DComplex> m_saved_selection{g_save_c};
    ValueSaver<FractalDispatch> m_saved_dispatch{g_dispatch};
    ValueSaver<Plot> m_saved_plot{g_put_color, put_color_a};
    InverseJuliaPlot m_plot{0, 0, 100, 100, 10, -10, 50, 50, 100, false, 1.0F, 1.0, false, false, 1};
    std::vector<std::array<int, 3>> m_pixels;
};

void TestInverseJulia::prepare_queue(const unsigned long capacity)
{
    std::uint64_t blocks{};
    ASSERT_TRUE(disk_video_memory_blocks(static_cast<std::int64_t>(capacity) * 8, 1, 256, 0, blocks));
    init_queue(capacity, memory_alloc(DISK_STORAGE_BLOCK_SIZE, blocks, MemoryLocation::MEMORY));
}

TEST_F(TestInverseJulia, incrementUsesSelectedPointAndContinues)
{
    InverseJulia calculation{JIIMType::JIIM};
    calculation.reset({-1, 0}, false);

    EXPECT_TRUE(calculation.iterate(m_plot));
    EXPECT_DOUBLE_EQ(1, g_new_z.x);
    EXPECT_DOUBLE_EQ(0, g_new_z.y);
    EXPECT_TRUE(calculation.iterate(m_plot));
    EXPECT_DOUBLE_EQ(std::sqrt(2.0), g_new_z.x);
    EXPECT_TRUE(calculation.active());
    EXPECT_TRUE(m_pixels.empty());
}

TEST_F(TestInverseJulia, unrelatedGlobalsDoNotChangeContinuation)
{
    InverseJulia calculation{JIIMType::JIIM};
    calculation.reset({-1, 0}, false);
    ASSERT_TRUE(calculation.iterate(m_plot));
    g_old_z = {123, 456};
    g_new_z = {-99, 72};
    g_save_c = {500, -500};
    g_col = 99;
    g_row = 73;

    EXPECT_TRUE(calculation.iterate(m_plot));

    EXPECT_DOUBLE_EQ(std::sqrt(2.0), g_new_z.x);
    EXPECT_DOUBLE_EQ(0, g_new_z.y);
}

TEST_F(TestInverseJulia, separateCalculationsKeepTheirOwnOrbitState)
{
    InverseJulia first{JIIMType::JIIM};
    InverseJulia second{JIIMType::JIIM};
    first.reset({-1, 0}, false);
    second.reset({-0.25, 0}, false);

    ASSERT_TRUE(first.iterate(m_plot));
    EXPECT_DOUBLE_EQ(1, g_new_z.x);
    ASSERT_TRUE(second.iterate(m_plot));
    EXPECT_DOUBLE_EQ(0.5, g_new_z.x);
    ASSERT_TRUE(first.iterate(m_plot));
    EXPECT_DOUBLE_EQ(std::sqrt(2.0), g_new_z.x);
    ASSERT_TRUE(second.iterate(m_plot));
    EXPECT_DOUBLE_EQ(std::sqrt(0.75), g_new_z.x);
}

TEST_F(TestInverseJulia, resetSelectsANewPoint)
{
    InverseJulia calculation{JIIMType::JIIM};
    calculation.reset({-1, 0}, false);
    ASSERT_TRUE(calculation.iterate(m_plot));

    calculation.reset({-2, 0}, false);

    EXPECT_TRUE(calculation.iterate(m_plot));
    EXPECT_DOUBLE_EQ(std::sqrt(2.0), g_new_z.x);
}

TEST_F(TestInverseJulia, plotChangeDoesNotRestartOrbit)
{
    InverseJulia calculation{JIIMType::JIIM};
    calculation.reset({-1, 0}, false);
    for (int i = 0; i < 12; ++i)
    {
        ASSERT_TRUE(calculation.iterate(m_plot));
    }
    const double previous{g_new_z.x};
    m_plot.zoom = 2;

    ASSERT_TRUE(calculation.iterate(m_plot));

    EXPECT_DOUBLE_EQ(std::sqrt(previous + 1), g_new_z.x);
    ASSERT_FALSE(m_pixels.empty());
    EXPECT_EQ(static_cast<int>(g_new_z.x * 20 + 50), m_pixels.back()[0]);
    EXPECT_EQ(50, m_pixels.back()[1]);
}

TEST_F(TestInverseJulia, clippingDoesNotAlterOrbit)
{
    InverseJulia calculation{JIIMType::JIIM};
    calculation.reset({-1, 0}, false);
    m_plot.bottom = 50;
    for (int i = 0; i < 12; ++i)
    {
        ASSERT_TRUE(calculation.iterate(m_plot));
    }

    EXPECT_TRUE(m_pixels.empty());
    EXPECT_GT(g_new_z.x, 1.6);
}

TEST_F(TestInverseJulia, orbitCompletionDoesNotWaitForInput)
{
    g_max_iterations = 3;
    InverseJulia calculation{JIIMType::ORBIT};
    calculation.reset({0.25, 0.5}, false);

    ASSERT_TRUE(calculation.iterate(m_plot));
    EXPECT_TRUE(calculation.active());
    EXPECT_DOUBLE_EQ(1.25, g_new_z.x);
    ASSERT_TRUE(calculation.iterate(m_plot));
    EXPECT_TRUE(calculation.active());
    EXPECT_DOUBLE_EQ(2.25, g_new_z.x);
    ASSERT_TRUE(calculation.iterate(m_plot));
    EXPECT_FALSE(calculation.active());
    EXPECT_EQ(2U, m_pixels.size());
}

TEST_F(TestInverseJulia, miimRequiresAnInitializedQueue)
{
    InverseJulia calculation{JIIMType::JIIM};

    EXPECT_THROW(calculation.reset(DComplex{-1, 0}, true), std::logic_error);
}

TEST_F(TestInverseJulia, miimSeedsAndAdvancesTheCalculationQueue)
{
    prepare_queue(8192);
    InverseJulia calculation{JIIMType::JIIM};

    calculation.reset({-1, 0}, true);
    const DComplex seed{dequeue_float()};
    EXPECT_NEAR((1 + std::sqrt(5.0)) / 2, seed.x, 1e-6);
    EXPECT_DOUBLE_EQ(0, seed.y);
    ASSERT_TRUE(calculation.iterate(m_plot));

    EXPECT_FALSE(queue_empty());
    EXPECT_TRUE(calculation.active());
    EXPECT_EQ(1U, m_pixels.size());
}

class TestPreparedInverseJuliaQueue : public TestInverseJulia
{
protected:
    void SetUp() override;
    void wrap_queue();

    ValueSaver<Driver *> m_without_driver{g_driver, nullptr};
};

void TestPreparedInverseJuliaQueue::SetUp()
{
    TestInverseJulia::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_queue(4));
}

void TestPreparedInverseJuliaQueue::wrap_queue()
{
    for (int i = 0; i < 8; ++i)
    {
        EXPECT_EQ(1, enqueue_float(static_cast<float>(i), 0.0F));
        EXPECT_EQ((DComplex{static_cast<double>(i), 0.0}), dequeue_float());
    }
}

TEST_F(TestPreparedInverseJuliaQueue, backingProvidesAnEmptyQueueWithoutUi)
{
    EXPECT_EQ(4UL, queue_capacity());
    EXPECT_TRUE(queue_empty());
}

class TestFilledInverseJuliaQueue : public TestPreparedInverseJuliaQueue
{
protected:
    void SetUp() override;
};

void TestFilledInverseJuliaQueue::SetUp()
{
    TestPreparedInverseJuliaQueue::SetUp();
    ASSERT_EQ(1, enqueue_float(1.0F, 2.0F));
    ASSERT_EQ(1, enqueue_float(3.0F, 4.0F));
    ASSERT_TRUE(queue_full_almost());
    ASSERT_EQ(1, push_float(5.0F, 6.0F));
}

class TestDrainedInverseJuliaQueue : public TestFilledInverseJuliaQueue
{
protected:
    void SetUp() override;
};

void TestDrainedInverseJuliaQueue::SetUp()
{
    TestFilledInverseJuliaQueue::SetUp();
    ASSERT_EQ(0, push_float(7.0F, 8.0F));
    ASSERT_EQ((DComplex{1.0, 2.0}), dequeue_float());
    ASSERT_EQ((DComplex{5.0, 6.0}), pop_float());
    ASSERT_EQ((DComplex{3.0, 4.0}), dequeue_float());
    ASSERT_TRUE(queue_empty());
}

class TestWrappedInverseJuliaQueue : public TestDrainedInverseJuliaQueue
{
protected:
    void SetUp() override;
};

void TestWrappedInverseJuliaQueue::SetUp()
{
    TestDrainedInverseJuliaQueue::SetUp();
    ASSERT_NO_FATAL_FAILURE(wrap_queue());
}

TEST_F(TestWrappedInverseJuliaQueue, freeingQueueReleasesItsBacking)
{
    free_queue();

    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
}

class TestInvalidInverseJuliaCapacity : public TestInverseJulia
{
protected:
    void SetUp() override;
    void TearDown() override;

    MemoryHandle m_backing{};
};

void TestInvalidInverseJuliaCapacity::SetUp()
{
    TestInverseJulia::SetUp();
    m_backing = memory_alloc(DISK_STORAGE_BLOCK_SIZE, 1, MemoryLocation::MEMORY);
}

void TestInvalidInverseJuliaCapacity::TearDown()
{
    memory_release(m_backing);
    TestInverseJulia::TearDown();
}

TEST_F(TestInvalidInverseJuliaCapacity, rejectionLeavesBackingWithTheCaller)
{
    EXPECT_THROW(init_queue(1, m_backing), std::logic_error);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
    EXPECT_EQ(MemoryLocation::MEMORY, memory_type(m_backing));
}

TEST_F(TestInverseJulia, initializationRejectsInvalidBacking)
{
    EXPECT_THROW(init_queue(4, MemoryHandle{}), std::logic_error);
}

class TestStandaloneInverseJulia : public TestInverseJulia
{
protected:
    void SetUp() override;

    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::INVERSE_JULIA};
    ValueSaver<Major> m_saved_major{g_major_method, Major::BREADTH_FIRST};
    ValueSaver<double> m_saved_param{g_params[2], 1000.0};
};

void TestStandaloneInverseJulia::SetUp()
{
    TestInverseJulia::SetUp();
    ASSERT_TRUE(orbit3d_per_image());
}

TEST_F(TestStandaloneInverseJulia, setupDoesNotAllocateOrChooseFallback)
{
    EXPECT_EQ(Major::BREADTH_FIRST, g_major_method);
    EXPECT_EQ(255.0, g_params[2]);
    EXPECT_EQ(0UL, queue_capacity());
    EXPECT_FALSE(g_disk_flag);
}

TEST_F(TestStandaloneInverseJulia, initializationRequiresAQueue)
{
    EXPECT_THROW(init_inverse_julia(), std::logic_error);
}

struct InverseJuliaSeedInput
{
    const char *name;
    Major major;
    std::optional<Minor> minor;
    bool positive_first;
};

void PrintTo(const InverseJuliaSeedInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestInverseJuliaSeedOrder : public TestInverseJulia, public WithParamInterface<InverseJuliaSeedInput>
{
protected:
    void SetUp() override;

    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::INVERSE_JULIA};
    ValueSaver<Major> m_saved_major{g_major_method, Major::BREADTH_FIRST};
    ValueSaver<Minor> m_saved_minor{g_inverse_julia_minor_method};
    ValueSaver<double> m_saved_param0{g_params[0], -1.0};
    ValueSaver<double> m_saved_param1{g_params[1], 0.0};
    ValueSaver<double> m_saved_param2{g_params[2], 4.0};
};

void TestInverseJuliaSeedOrder::SetUp()
{
    TestInverseJulia::SetUp();
    ASSERT_NO_FATAL_FAILURE(prepare_queue(16));
    ASSERT_TRUE(orbit3d_per_image());
    g_major_method = GetParam().major;
    if (GetParam().minor)
    {
        g_inverse_julia_minor_method = *GetParam().minor;
    }
}

TEST_P(TestInverseJuliaSeedOrder, seedsPreserveSelectedOrdering)
{
    const double positive{(1 + std::sqrt(5.0)) / 2};
    const double negative{(1 - std::sqrt(5.0)) / 2};

    init_inverse_julia();

    const DComplex first{GetParam().major == Major::BREADTH_FIRST ? dequeue_float() : pop_float()};
    const DComplex second{GetParam().major == Major::BREADTH_FIRST ? dequeue_float() : pop_float()};
    EXPECT_NEAR(GetParam().positive_first ? positive : negative, first.x, 1e-6);
    EXPECT_NEAR(GetParam().positive_first ? negative : positive, second.x, 1e-6);
}

INSTANTIATE_TEST_SUITE_P(Methods, TestInverseJuliaSeedOrder,
    Values(InverseJuliaSeedInput{"BreadthFirst", Major::BREADTH_FIRST, std::nullopt, true},
        InverseJuliaSeedInput{"DepthLeft", Major::DEPTH_FIRST, Minor::LEFT_FIRST, false},
        InverseJuliaSeedInput{"DepthRight", Major::DEPTH_FIRST, Minor::RIGHT_FIRST, true}),
    [](const TestParamInfo<InverseJuliaSeedInput> &info) { return info.param.name; });

} // namespace id::test
