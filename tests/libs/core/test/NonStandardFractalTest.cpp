// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <NonStandardFractalTest.h>

#include <test_data.h>

#include <engine/pixel_grid.h>
#include <engine/resume.h>
#include <engine/trig_fns.h>
#include <fractals/population.h>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace id::test::non_standard
{

int g_orbit_calls{};

namespace
{

int stable_population()
{
    ++g_orbit_calls;
    g_population = 0.5;
    return 0;
}

int unexpected_renderer()
{
    throw std::logic_error("Calculation entered a renderer callback");
}

bool unexpected_setup()
{
    throw std::logic_error("Calculation entered legacy per-image setup");
}

} // namespace

void PrintTo(const BifurcationCase &value, std::ostream *out)
{
    *out << value.name;
}

void TestNonStandardFractal::SetUp()
{
    g_orbit_calls = 0;
    select_type(FractalType::BIFURCATION);
    set_pixel_functions(driver_write_pixel, driver_read_pixel);
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                m_pixels.at(y * WIDTH + x) = color;
                m_columns.push_back(x);
            }));
}

void TestNonStandardFractal::TearDown()
{
    end_resume();
}

void TestNonStandardFractal::select_type(FractalType type)
{
    g_fractal_type = type;
    m_specific = *get_fractal_specific(type);
    m_specific.calc_type = unexpected_renderer;
    m_specific.per_image = unexpected_setup;
    m_specific.orbit_calc = stable_population;
    g_dispatch = FractalDispatch{m_specific};
}

void TestNonStandardFractal::complete(NonStandardFractal &calculation)
{
    for (int chunks = 0; chunks < WIDTH + 1 && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.finish(false);
}

void TestSelectedIFS::SetUp()
{
    TestNonStandardFractal::SetUp();
    g_max_iterations = 1;
    g_params[0] = 1;
    select_dimension(IFSDimension::TWO);
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels.at(y * WIDTH + x); }));
    std::string name{UnitTest::GetInstance()->current_test_info()->name()};
    std::replace(name.begin(), name.end(), '/', '_');
    m_path = std::filesystem::path{data::ID_TEST_DATA_DIR} / ("selected-ifs-" + name + ".ifs");
    g_ifs_filename = m_path;
    g_ifs_name = "entry";
}

void TestSelectedIFS::TearDown()
{
    std::error_code ignored;
    std::filesystem::remove(m_path, ignored);
    TestNonStandardFractal::TearDown();
}

void TestSelectedIFS::select_dimension(IFSDimension dimension)
{
    select_type(FractalType::IFS);
    g_ifs_dim = dimension;
    g_num_affine_transforms = 1;
    g_ifs_definition = dimension == IFSDimension::TWO ? std::vector<float>{0, 0, 0, 0, 3, 0.5f, 1}
                                                      : std::vector<float>{0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0.5f, 0, 1};
    g_calc_status = CalcStatus::PARAMS_CHANGED;
}

void TestSelectedIFS::complete_view(NonStandardFractal &calculation)
{
    for (int point = 0; point < 1025; ++point)
    {
        calculation.iterate();
    }
    EXPECT_EQ(1025L, g_color_iter);
}

void TestStandaloneBifurcation::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(GetParam().type);
}

void TestStandalonePopcorn::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(GetParam());
    g_params[0] = 0.05;
    g_params[1] = 0;
    g_params[2] = 3;
    g_params[3] = 0;
    g_image_region = ImageRegion{{-0.5, -0.5}, {0.5, 0.5}, {-0.5, -0.5}};
    g_delta_x = 1.0 / (WIDTH - 1);
    g_delta_y = 1.0 / (HEIGHT - 1);
    alloc_pixel_grid();
    fill_pixel_grid();
    save_trig_functions();
    set_trig_array(0, "sin");
    set_trig_array(1, "tan");
    set_trig_array(2, "sin");
    set_trig_array(3, "tan");
}

void TestStandalonePopcorn::TearDown()
{
    restore_trig_functions();
    free_pixel_grid();
    TestNonStandardFractal::TearDown();
}

void TestStandalonePopcorn::complete(NonStandardFractal &calculation)
{
    for (long chunks = 0; chunks <= WIDTH * HEIGHT * g_max_iterations && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.finish(false);
}

void TestStandaloneDiffusion::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::DIFFUSION);
    g_params[0] = 10;
    g_params[1] = GetParam();
    g_params[2] = 3;
    g_logical_screen = LogicalScreen{SIDE, SIDE, 0, 0, SIDE - 1.0, SIDE - 1.0};
    g_screen_x_dots = SIDE;
    g_screen_y_dots = SIDE;
    g_put_color = put_color_a;
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](int x, int y) { return m_frame.at(y * SIDE + x); }));
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(Invoke(
            [this](int x, int y, int color)
            {
                if (x < SIDE && y < SIDE)
                {
                    m_frame.at(y * SIDE + x) = color;
                }
            }));
}

void TestStandaloneDiffusion::complete(NonStandardFractal &calculation)
{
    for (int chunks = 0; chunks < MAX_CHUNKS && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.finish(false);
}

void TestStandaloneTestPoint::SetUp()
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::TEST);
    g_std_calc_mode = GetParam();
    g_params[0] = 0;
    g_params[1] = 0;
    g_params[2] = 0;
    g_image_region = ImageRegion{{-2.0, -1.5}, {1.0, 1.5}, {-2.0, -1.5}};
    g_delta_x = 3.0 / (WIDTH - 1);
    g_delta_y = 3.0 / (HEIGHT - 1);
    g_param_z1 = {};
    g_i_stop_pt = {WIDTH - 1, HEIGHT - 1};
    g_plot = put_color_a;
    alloc_pixel_grid();
    fill_pixel_grid();
}

void TestStandaloneTestPoint::TearDown()
{
    free_pixel_grid();
    TestNonStandardFractal::TearDown();
}

void TestStandaloneTestPoint::complete(NonStandardFractal &calculation)
{
    for (int chunks = 0; chunks < MAX_CHUNKS && !calculation.done(); ++chunks)
    {
        calculation.iterate();
    }
    ASSERT_TRUE(calculation.done());
    EXPECT_TRUE(calculation.completed());
    calculation.finish(false);
}

auto SelectedAntFixture::reference_frames() -> std::vector<std::array<int, WIDTH * HEIGHT>>
{
    std::vector<std::array<int, WIDTH * HEIGHT>> result;
    Ant ant;
    while (!ant.done())
    {
        ant.iterate();
        result.push_back(m_pixels);
    }
    m_pixels.fill(0);
    m_columns.clear();
    return result;
}

void SelectedAntFixture::initialize_ant(const AntType type)
{
    TestNonStandardFractal::SetUp();
    select_type(FractalType::ANT);
    g_params[0] = 1100;
    g_param_text[0] = "1100";
    g_params[1] = 400;
    g_params[2] = 2;
    g_params[3] = static_cast<int>(type);
    g_params[4] = 1;
    g_params[5] = 12345;
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(Invoke([this](int x, int y) { return m_pixels.at(y * WIDTH + x); }));
}

void TestSelectedAnt::SetUp()
{
    initialize_ant(GetParam());
}

INSTANTIATE_TEST_SUITE_P(Bifurcations, TestStandaloneBifurcation, ValuesIn(BIFURCATION_CASES),
    [](const TestParamInfo<BifurcationCase> &info) -> std::string { return info.param.name; });

INSTANTIATE_TEST_SUITE_P(TestPointModes, TestStandaloneTestPoint, Values(CalcMode::ONE_PASS, CalcMode::TWO_PASS),
    [](const TestParamInfo<CalcMode> &info) -> std::string
    { return info.param == CalcMode::ONE_PASS ? "OnePass" : "TwoPass"; });

} // namespace id::test::non_standard
