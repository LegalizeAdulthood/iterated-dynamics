// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/calcfrac.h>

#include <PassTestFrame.h>
#include <WorkListSnapshot.h>

#include <engine/bailout_formula.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/Inversion.h>
#include <engine/pixel.h>
#include <engine/Potential.h>
#include <engine/work_list.h>
#include <fractals/fractype.h>
#include <math/big_init.h>
#include <math/BigFloat.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <array>
#include <ostream>
#include <string>
#include <tuple>

namespace id::engine
{
void set_symmetry(SymmetryType sym, bool use_list);
}

using namespace id::engine;
using namespace id::fractals;
using namespace id::math;
using namespace id::misc;
using namespace testing;

namespace id::test
{

class TestBigFloatSymmetry : public Test
{
protected:
    void initialize_coordinates(int precision);
    void TearDown() override;

    PassTestFrame m_frame{8, 8};
    WorkListSnapshot m_work_list;
    ValueSaver<BFMathType> m_saved_math{g_bf_math, BFMathType::NONE};
    ValueSaver<int> m_saved_digits{g_bf_digits, 0};
    ValueSaver<FractalType> m_saved_type{g_fractal_type, FractalType::MANDEL_Z_POWER};
    ValueSaver<ImageRegion> m_saved_region{g_image_region, ImageRegion{{-4, -2}, {4, 2}, {-4, -2}}};
    ValueSaver<CalcMode> m_saved_mode{g_std_calc_mode, CalcMode::ONE_PASS};
    ValueSaver<SymmetryType> m_saved_force{g_force_symmetry, SymmetryType::NOT_FORCED};
    ValueSaver<SymmetryType> m_saved_symmetry{g_symmetry};
    ValueSaver<decltype(g_plot)> m_saved_plot{g_plot, put_color_a};
    ValueSaver<int> m_saved_work_symmetry{g_work_symmetry, 0};
    ValueSaver<int> m_saved_pass{g_work_pass, 0};
    ValueSaver<int> m_saved_pi{g_pi_in_pixels, -123};
    ValueSaver<int> m_saved_basin{g_basin, 0};
    ValueSaver<Potential> m_saved_potential{g_potential, Potential{}};
    ValueSaver<Inversion> m_saved_inversion{g_inversion, Inversion{}};
    ValueSaver<std::array<int, 2>> m_saved_decomp{g_decomp, std::array<int, 2>{0, 0}};
    ValueSaver<ColorMethod> m_saved_outside{g_outside_method, ColorMethod::COLOR};
    ValueSaver<ColorMethod> m_saved_inside{g_inside_method, ColorMethod::COLOR};
    ValueSaver<Bailout> m_saved_bailout{g_bailout_test, Bailout::MOD};
    ValueSaver<DComplex> m_saved_param1{g_param_z1, DComplex{}};
    ValueSaver<DComplex> m_saved_param2{g_param_z2, DComplex{}};
    ValueSaver<InitOrbitMode> m_saved_orbit{g_use_init_orbit, InitOrbitMode::NORMAL};
};

void TestBigFloatSymmetry::initialize_coordinates(const int precision)
{
    init_bf_length(precision);
    g_bf_x_min = g_image_region.m_min.x;
    g_bf_x_max = g_image_region.m_max.x;
    g_bf_y_min = g_image_region.m_min.y;
    g_bf_y_max = g_image_region.m_max.y;
    g_bf_x_3rd = g_image_region.m_3rd.x;
    g_bf_y_3rd = g_image_region.m_3rd.y;
}

void TestBigFloatSymmetry::TearDown()
{
    free_bf_vars();
}

TEST_F(TestBigFloatSymmetry, doublePrecisionNeedsNoArena)
{
    set_symmetry(SymmetryType::XY_AXIS, true);

    EXPECT_EQ(BFMathType::NONE, g_bf_math);
    EXPECT_EQ(sym_plot4, g_plot);
    EXPECT_EQ((Point2i{3, 3}), g_i_stop_pt);
    EXPECT_EQ(0x33, g_work_symmetry);
}

struct SymmetryScratchInput
{
    const char *name;
    ImageRegion region;
    SymmetryType requested;
    bool use_list;
    bool inversion;
    bool parameter;
    decltype(g_plot) plot;
    Point2i stop;
    int work_symmetry;
    int pi_pixels;
    SymmetryType symmetry;
};

void PrintTo(const SymmetryScratchInput &value, std::ostream *out)
{
    *out << value.name;
}

class TestBigFloatSymmetryScratch : public TestBigFloatSymmetry,
                                    public WithParamInterface<std::tuple<int, SymmetryScratchInput>>
{
};

TEST_P(TestBigFloatSymmetryScratch, preservesPlotDecisionsInputsAndTemporaryLifetime)
{
    const auto &input = std::get<1>(GetParam());
    g_image_region = input.region;
    initialize_coordinates(std::get<0>(GetParam()));
    g_inversion.invert = input.inversion ? 1 : 0;
    g_param_z1.x = input.parameter ? 1 : 0;
    const auto bytes = pack_bf_parameters();

    set_symmetry(input.requested, input.use_list);

    EXPECT_EQ(input.plot, g_plot);
    EXPECT_EQ(input.stop, g_i_stop_pt);
    EXPECT_EQ(input.work_symmetry, g_work_symmetry);
    EXPECT_EQ(input.pi_pixels, g_pi_in_pixels);
    EXPECT_EQ(input.symmetry, g_symmetry);
    EXPECT_EQ(0, g_num_work_list);
    EXPECT_EQ((Point2i{7, 7}), g_stop_pt);
    EXPECT_EQ(bytes, pack_bf_parameters());
    g_plot = put_color_a;
    g_i_stop_pt = {7, 7};
    g_work_symmetry = 0;
    g_pi_in_pixels = -123;

    set_symmetry(input.requested, input.use_list);

    EXPECT_EQ(input.plot, g_plot);
    EXPECT_EQ(input.stop, g_i_stop_pt);
    EXPECT_EQ(input.work_symmetry, g_work_symmetry);
    EXPECT_EQ(input.pi_pixels, g_pi_in_pixels);
    EXPECT_EQ(bytes, pack_bf_parameters());
}

INSTANTIATE_TEST_SUITE_P(Paths, TestBigFloatSymmetryScratch,
    Combine(Values(4, 708),
        Values(SymmetryScratchInput{"XOnly", {{1, -2}, {3, 2}, {1, -2}}, SymmetryType::X_AXIS, true, false, false,
                   sym_plot2, {7, 3}, 0x11, -123, SymmetryType::NONE},
            SymmetryScratchInput{"YOnly", {{-4, 1}, {4, 3}, {-4, 1}}, SymmetryType::Y_AXIS, true, false, false,
                sym_plot2y, {3, 7}, 0x22, -123, SymmetryType::NONE},
            SymmetryScratchInput{"BothAxes", {{-4, -2}, {4, 2}, {-4, -2}}, SymmetryType::XY_AXIS, true, false, false,
                sym_plot4, {3, 3}, 0x33, -123, SymmetryType::NONE},
            SymmetryScratchInput{"NoAxes", {{1, 1}, {3, 3}, {1, 1}}, SymmetryType::XY_AXIS, true, false, false,
                put_color_a, {7, 7}, 0x30, -123, SymmetryType::X_AXIS},
            SymmetryScratchInput{"Pi", {{-4, -2}, {4, 2}, {-4, -2}}, SymmetryType::PI_SYM, true, false, false,
                sym_pi_plot4j, {2, 3}, 0x33, 3, SymmetryType::NONE},
            SymmetryScratchInput{"PiReversedWidth", {{4, -2}, {-4, 2}, {4, -2}}, SymmetryType::PI_SYM, true, false,
                false, sym_pi_plot4j, {2, 3}, 0x33, 3, SymmetryType::NONE},
            SymmetryScratchInput{"PiTooNarrow", {{-0.25, -2}, {0.25, 2}, {-0.25, -2}}, SymmetryType::PI_SYM, true,
                false, false, put_color_a, {7, 7}, 0, -123, SymmetryType::X_AXIS},
            SymmetryScratchInput{"PiInversion", {{-4, -2}, {4, 2}, {-4, -2}}, SymmetryType::PI_SYM, true, true, false,
                sym_plot2j, {7, 3}, 0x33, -123, SymmetryType::NONE},
            SymmetryScratchInput{"PiParameters", {{-4, -2}, {4, 2}, {-4, -2}}, SymmetryType::PI_SYM_NO_PARAM, true,
                false, true, put_color_a, {7, 7}, 0, -123, SymmetryType::X_AXIS},
            SymmetryScratchInput{"OffCenterNoSplit", {{-4, -1}, {4, 3}, {-4, -1}}, SymmetryType::X_AXIS, false, false,
                false, put_color_a, {7, 7}, 0x10, -123, SymmetryType::X_AXIS},
            SymmetryScratchInput{"CenteredNoSplit", {{-4, -2}, {4, 2}, {-4, -2}}, SymmetryType::XY_AXIS, false, false,
                false, sym_plot4, {3, 3}, 0x33, -123, SymmetryType::NONE})),
    [](const TestParamInfo<std::tuple<int, SymmetryScratchInput>> &info)
    { return std::string{std::get<1>(info.param).name} + (std::get<0>(info.param) == 4 ? "Minimum" : "Maximum"); });

enum class SymmetryExit
{
    SYNCHRONOUS,
    ORBIT,
    NO_PLOT,
    BF_ROTATED_X,
    BF_ROTATED_Y,
    POTENTIAL,
    INVERSION_Y,
    DECOMPOSITION,
    DOUBLE_ROTATED,
    INVERSION_X,
    OUTSIDE,
    BAILOUT,
    INSIDE
};

struct SymmetryExitInput
{
    const char *name;
    SymmetryExit reason;
    bool initialized;
};

void PrintTo(const SymmetryExitInput &value, std::ostream *out)
{
    *out << value.name;
}

class TestBigFloatSymmetryExit : public TestBigFloatSymmetry, public WithParamInterface<SymmetryExitInput>
{
};

TEST_P(TestBigFloatSymmetryExit, returnsBeforeAllocatingScratch)
{
    if (GetParam().initialized)
    {
        initialize_coordinates(32);
    }
    SymmetryType requested = SymmetryType::XY_AXIS;
    switch (GetParam().reason)
    {
    case SymmetryExit::SYNCHRONOUS:
        g_std_calc_mode = CalcMode::SYNCHRONOUS_ORBIT;
        break;
    case SymmetryExit::ORBIT:
        g_std_calc_mode = CalcMode::ORBIT;
        break;
    case SymmetryExit::NO_PLOT:
        requested = SymmetryType::NO_PLOT;
        break;
    case SymmetryExit::BF_ROTATED_X:
        g_bf_x_3rd = -3;
        break;
    case SymmetryExit::BF_ROTATED_Y:
        g_bf_y_3rd = -1;
        break;
    case SymmetryExit::POTENTIAL:
        g_potential.flag = g_potential.store_16bit = true;
        break;
    case SymmetryExit::INVERSION_Y:
        g_inversion.invert = 1;
        g_inversion.params[2] = 1;
        break;
    case SymmetryExit::DECOMPOSITION:
        g_decomp[0] = 2;
        break;
    case SymmetryExit::DOUBLE_ROTATED:
        g_image_region.m_3rd.x = -3;
        break;
    case SymmetryExit::INVERSION_X:
        g_inversion.params[1] = 1;
        break;
    case SymmetryExit::OUTSIDE:
        g_outside_method = ColorMethod::REAL;
        break;
    case SymmetryExit::BAILOUT:
        g_bailout_test = Bailout::MANR;
        break;
    case SymmetryExit::INSIDE:
        g_inside_method = ColorMethod::FMODI;
        break;
    }

    set_symmetry(requested, true);

    EXPECT_EQ(requested == SymmetryType::NO_PLOT ? no_plot : put_color_a, g_plot);
    EXPECT_EQ(SymmetryType::X_AXIS, g_symmetry);
    EXPECT_EQ((Point2i{7, 7}), g_i_stop_pt);
    EXPECT_EQ(0, g_work_symmetry);
    EXPECT_EQ(0, g_num_work_list);
    if (GetParam().initialized)
    {
    }
    else
    {
        EXPECT_EQ(BFMathType::NONE, g_bf_math);
    }
}

INSTANTIATE_TEST_SUITE_P(Reasons, TestBigFloatSymmetryExit,
    Values(SymmetryExitInput{"SynchronousWithoutArena", SymmetryExit::SYNCHRONOUS, false},
        SymmetryExitInput{"OrbitWithoutArena", SymmetryExit::ORBIT, false},
        SymmetryExitInput{"NoPlotWithoutArena", SymmetryExit::NO_PLOT, false},
        SymmetryExitInput{"RotatedBigFloatX", SymmetryExit::BF_ROTATED_X, true},
        SymmetryExitInput{"RotatedBigFloatY", SymmetryExit::BF_ROTATED_Y, true},
        SymmetryExitInput{"Potential16Bit", SymmetryExit::POTENTIAL, true},
        SymmetryExitInput{"OffAxisInversion", SymmetryExit::INVERSION_Y, true},
        SymmetryExitInput{"Decomposition", SymmetryExit::DECOMPOSITION, true},
        SymmetryExitInput{"RotatedDouble", SymmetryExit::DOUBLE_ROTATED, true},
        SymmetryExitInput{"OffCenterInversion", SymmetryExit::INVERSION_X, true},
        SymmetryExitInput{"OutsideColoring", SymmetryExit::OUTSIDE, true},
        SymmetryExitInput{"ManhattanBailout", SymmetryExit::BAILOUT, true},
        SymmetryExitInput{"InsideColoring", SymmetryExit::INSIDE, true}),
    [](const TestParamInfo<SymmetryExitInput> &info) { return info.param.name; });

} // namespace id::test
