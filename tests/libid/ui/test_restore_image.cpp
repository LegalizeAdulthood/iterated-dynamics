// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/restore_image.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <engine/Viewport.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>
#include <ui/evolve.h>
#include <ui/save_image.h>

#include <io/loadfile.h>
#include <ColorMapSaver.h>
#include <FindFractalInfoState.h>
#include <FindFractalInfoTestSupport.h>
#include <GifReadSupport.h>
#include <MockDriver.h>
#include <test_data.h>
#include <test_migrate_gif.h>
#include <engine/calc_frac_init.h>
#include <engine/cmdfiles.h>
#include <engine/color_state.h>
#include <engine/Inversion.h>
#include <engine/Potential.h>
#include <engine/random_seed.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <fractals/fractalp.h>
#include <fractals/fractype.h>
#include <fractals/lorenz.h>
#include <io/gif_extensions.h>
#include <io/gif_file.h>
#include <io/GifRestore.h>
#include <math/biginit.h>
#include <ui/get_video_mode.h>
#include <ui/video_mode.h>
#include <ui/get_3d_params.h>
#include <ui/history.h>
#include <ui/id_keys.h>
#include <ui/make_batch_file.h>
#include <io/decoder.h>
#include <engine/OrbitPlot.h>
#include <engine/pixel_limits.h>
#include <engine/sound.h>
#include <io/gifview.h>
#include <misc/Driver.h>
#include <ui/slideshw.h>
#include <ui/video.h>
#include <gif_lib.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <tuple>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>
#include <ostream>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

using namespace id::fractals;
using namespace id::misc::test;
using namespace id::test::data;
using namespace id::test::migrate_gif;
using namespace id::math;

namespace id::test
{

class TestEvolutionRestore : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    EvolutionInfo m_info{};
    GeneBase m_genes[NUM_GENES]{};
    ValueSaver<EvolutionModeFlags> m_saved_mode{g_evolving, EvolutionModeFlags::FIELD_MAP};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::RESUMABLE};
    ValueSaver<bool> m_saved_have_info{g_have_evolve_info, true};
    ValueSaver<EvolutionInfo> m_saved_info{g_evolve_info, EvolutionInfo{}};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{1, 1, 0, 0, 0, 0}};
    ValueSaver<Viewport> m_saved_viewport{g_viewport, Viewport{}};
    ValueSaver<int> m_saved_grid{g_evolve_image_grid_size};
    ValueSaver<unsigned int> m_saved_seed{g_evolve_this_generation_random_seed};
    ValueSaver<double> m_saved_mutation{g_evolve_max_random_mutation};
    ValueSaver<double> m_saved_x_range{g_evolve_x_parameter_range};
    ValueSaver<double> m_saved_y_range{g_evolve_y_parameter_range};
    ValueSaver<double> m_saved_x_offset{g_evolve_x_parameter_offset};
    ValueSaver<double> m_saved_y_offset{g_evolve_y_parameter_offset};
    ValueSaver<double> m_saved_new_x_offset{g_evolve_new_x_parameter_offset};
    ValueSaver<double> m_saved_new_y_offset{g_evolve_new_y_parameter_offset};
    ValueSaver<char> m_saved_discrete_x{g_evolve_discrete_x_parameter_offset};
    ValueSaver<char> m_saved_discrete_y{g_evolve_discrete_y_parameter_offset};
    ValueSaver<char> m_saved_new_discrete_x{g_evolve_new_discrete_x_parameter_offset};
    ValueSaver<char> m_saved_new_discrete_y{g_evolve_new_discrete_y_parameter_offset};
    ValueSaver<int> m_saved_px{g_evolve_param_grid_x};
    ValueSaver<int> m_saved_py{g_evolve_param_grid_y};
    ValueSaver<double> m_saved_dist_x{g_evolve_dist_per_x};
    ValueSaver<double> m_saved_dist_y{g_evolve_dist_per_y};
};

void TestEvolutionRestore::SetUp()
{
    copy_genes_from_bank(m_genes);
    m_info.evolving = 9;
    m_info.image_grid_size = 3;
    m_info.this_generation_random_seed = 0xABCD;
    m_info.max_random_mutation = 0.25;
    m_info.x_parameter_range = 4;
    m_info.y_parameter_range = 3;
    m_info.x_parameter_offset = -2;
    m_info.y_parameter_offset = -1.5;
    m_info.discrete_x_parameter_offset = -7;
    m_info.discrete_y_parameter_offset = 9;
    m_info.px = 1;
    m_info.py = 2;
    m_info.screen_x_offset = 2;
    m_info.screen_y_offset = 1;
    m_info.x_dots = 8;
    m_info.y_dots = 6;
    m_info.count = 4;
    for (int i = 0; i < NUM_GENES; ++i)
    {
        m_info.mutate[i] = static_cast<std::int16_t>(i % static_cast<int>(Variations::NUM));
    }
}

void TestEvolutionRestore::TearDown()
{
    copy_genes_to_bank(m_genes);
}

TEST_F(TestEvolutionRestore, absentRecordDisablesEvolutionAndInvalidatesCachedProgress)
{
    g_viewport.enabled = true;

    apply_evolution_info(std::nullopt);

    EXPECT_EQ(EvolutionModeFlags::NONE, g_evolving);
    EXPECT_FALSE(g_have_evolve_info);
    EXPECT_EQ(CalcStatus::RESUMABLE, g_calc_status);
    EXPECT_EQ(1, g_logical_screen.x_dots);
    EXPECT_TRUE(g_viewport.enabled);
    EXPECT_FALSE(capture_evolution_info());
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EXPECT_EQ(m_genes[i].mutate, g_gene_bank[i].mutate) << i;
    }
}

class TestEvolutionRestoreProgress : public TestEvolutionRestore, public WithParamInterface<std::tuple<CalcStatus, int>>
{
};

TEST_P(TestEvolutionRestoreProgress, restoresControlsAndPreservesProgressWhenSaved)
{
    g_calc_status = std::get<0>(GetParam());
    m_info.count = static_cast<std::int16_t>(std::get<1>(GetParam()));
    const bool resumable{g_calc_status != CalcStatus::COMPLETED && m_info.count != 9};

    apply_evolution_info(m_info);

    EXPECT_EQ(resumable ? CalcStatus::RESUMABLE : CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(resumable, g_have_evolve_info);
    if (resumable)
    {
        EXPECT_EQ(m_info, g_evolve_info);
    }
    EXPECT_EQ(EvolutionModeFlags::FIELD_MAP | EvolutionModeFlags::NO_GROUT, g_evolving);
    EXPECT_TRUE(g_viewport.enabled);
    EXPECT_EQ(8, g_logical_screen.x_dots);
    EXPECT_EQ(6, g_logical_screen.y_dots);
    EXPECT_EQ(2, g_logical_screen.x_offset);
    EXPECT_EQ(1, g_logical_screen.y_offset);
    EXPECT_EQ(3, g_evolve_image_grid_size);
    EXPECT_EQ(0xABCDU, g_evolve_this_generation_random_seed);
    EXPECT_EQ(0.25, g_evolve_max_random_mutation);
    EXPECT_EQ(4, g_evolve_x_parameter_range);
    EXPECT_EQ(3, g_evolve_y_parameter_range);
    EXPECT_EQ(-2, g_evolve_x_parameter_offset);
    EXPECT_EQ(-1.5, g_evolve_y_parameter_offset);
    EXPECT_EQ(-2, g_evolve_new_x_parameter_offset);
    EXPECT_EQ(-1.5, g_evolve_new_y_parameter_offset);
    EXPECT_EQ(-7, g_evolve_discrete_x_parameter_offset);
    EXPECT_EQ(9, g_evolve_discrete_y_parameter_offset);
    EXPECT_EQ(-7, g_evolve_new_discrete_x_parameter_offset);
    EXPECT_EQ(9, g_evolve_new_discrete_y_parameter_offset);
    EXPECT_EQ(1, g_evolve_param_grid_x);
    EXPECT_EQ(2, g_evolve_param_grid_y);
    EXPECT_EQ(2, g_evolve_dist_per_x);
    EXPECT_EQ(1.5, g_evolve_dist_per_y);
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EXPECT_EQ(m_info.mutate[i], static_cast<int>(g_gene_bank[i].mutate)) << i;
        EXPECT_EQ(m_genes[i].addr, g_gene_bank[i].addr) << i;
        EXPECT_EQ(m_genes[i].vary_fn, g_gene_bank[i].vary_fn) << i;
    }
    EvolutionInfo expected{m_info};
    if (!resumable)
    {
        expected.count = 9;
    }
    const std::optional<EvolutionInfo> saved{capture_evolution_info()};
    ASSERT_TRUE(saved);
    EXPECT_EQ(expected, *saved);
}

INSTANTIATE_TEST_SUITE_P(States, TestEvolutionRestoreProgress,
    Combine(Values(CalcStatus::PARAMS_CHANGED, CalcStatus::IN_PROGRESS, CalcStatus::RESUMABLE,
                CalcStatus::NON_RESUMABLE, CalcStatus::COMPLETED),
        Values(4, 9)));

TEST_F(TestEvolutionRestore, presentDisabledModeDisablesTheViewport)
{
    g_viewport.enabled = true;
    m_info.evolving = 0;

    apply_evolution_info(m_info);

    EXPECT_EQ(EvolutionModeFlags::NONE, g_evolving);
    EXPECT_FALSE(g_viewport.enabled);
}

struct FileFractalType
{
    const char *filename;
    FractalType type;
};

std::ostream &operator<<(std::ostream &str, const FileFractalType &value)
{
    return str << std::filesystem::path(value.filename).filename().string() << ", " << +value.type;
}

void PrintTo(const FileFractalType &value, std::ostream *str)
{
    *str << value;
}

class TestGetVideoMode : public Test
{
protected:
    void SetUp() override;
    ImageMetadata metadata() const;

    ValueSaver<int> saved_init_mode{g_init_mode};
    ValueSaver<BatchMode> saved_init_batch{g_init_batch, BatchMode::NORMAL};
    ValueSaver<bool> saved_ask_video{g_ask_video, false};
    ValueSaver<bool> saved_make_parameter_file{g_make_parameter_file, false};
    ValueSaver<bool> saved_fast_restore{g_fast_restore, false};
    ValueSaver<CalcStatus> saved_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<Viewport> saved_viewport{g_viewport, Viewport{}};
    ValueSaver<VideoInfo> saved_video_entry{g_video_entry};
    ValueSaver<int> saved_video_table_len{g_video_table_len, 1};
    ValueSaver<VideoInfo> saved_video_mode{g_video_table[0]};
    ValueSaver<int> saved_video_key{g_video_mode_keys[0]};
    ValueSaver<int> saved_file_x_dots{g_file_x_dots, 65535};
    ValueSaver<int> saved_file_y_dots{g_file_y_dots, 65535};
    ValueSaver<int> saved_file_colors{g_file_colors, 256};
    ValueSaver<float> saved_file_aspect_ratio{g_file_aspect_ratio, 0.0f};
    ValueSaver<float> saved_screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver<int> saved_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver<int> saved_skip_y_dots{g_skip_y_dots, 0};
    FractalInfo m_info{};
    ExtBlock3 m_block_3_info{};
};

void TestGetVideoMode::SetUp()
{
    g_video_mode_keys[0] = ID_KEY_F2;
    g_video_table[0].x_dots = 800;
    g_video_table[0].y_dots = 600;
    g_video_table[0].colors = 256;
    g_video_table[0].driver = nullptr;
    m_info.x_dots = 800;
    m_info.y_dots = 600;
    m_info.colors = 256;
}

ImageMetadata TestGetVideoMode::metadata() const
{
    ImageMetadata data;
    data.fractal = m_info;
    data.fractal.info_version = 4;
    data.fractal.calc_status = static_cast<short>(g_calc_status);
    data.width = g_file_x_dots;
    data.height = g_file_y_dots;
    data.colors = g_file_colors;
    data.aspect_ratio = g_file_aspect_ratio;
    return data;
}

struct PendingViewportCase
{
    const char *name;
    int file_width;
    int file_height;
    float file_aspect;
    CalcStatus initial_status;
    int view_width;
    int view_height;
    bool disable_pending_view;
    int expected_width;
    int expected_height;
    int expected_skip_x;
    int expected_skip_y;
    CalcStatus expected_status;
    std::optional<bool> expected_view_enabled;
    std::optional<float> expected_reduction;
};

void PrintTo(const PendingViewportCase &value, std::ostream *str)
{
    *str << value.name;
}

class TestPendingViewport : public TestGetVideoMode, public WithParamInterface<PendingViewportCase>
{
protected:
    void SetUp() override;

    Viewport m_pending_viewport{};
};

void TestPendingViewport::SetUp()
{
    TestGetVideoMode::SetUp();
    const PendingViewportCase &input{GetParam()};
    g_file_x_dots = input.file_width;
    g_file_y_dots = input.file_height;
    g_file_aspect_ratio = input.file_aspect;
    g_calc_status = input.initial_status;
    g_viewport.enabled = input.view_width != 0;
    g_viewport.x_dots = input.view_width;
    g_viewport.y_dots = input.view_height;
    m_pending_viewport = g_viewport;
    if (input.disable_pending_view)
    {
        m_pending_viewport.enabled = false;
    }
}

TEST_P(TestPendingViewport, computesPendingModeWithoutChangingActiveImage)
{
    const PendingViewportCase &expected{GetParam()};

    const std::optional<ImageVideoMode> setup{get_video_mode(metadata(), m_pending_viewport)};

    ASSERT_TRUE(setup);
    EXPECT_EQ(expected.expected_width, setup->width);
    EXPECT_EQ(expected.expected_height, setup->height);
    EXPECT_EQ(expected.expected_skip_x, setup->skip_x);
    EXPECT_EQ(expected.expected_skip_y, setup->skip_y);
    EXPECT_EQ(expected.expected_status, setup->calc_status);
    if (expected.expected_view_enabled)
    {
        EXPECT_EQ(*expected.expected_view_enabled, setup->viewport.enabled);
    }
    if (expected.expected_reduction)
    {
        EXPECT_FLOAT_EQ(*expected.expected_reduction, setup->viewport.reduction);
        EXPECT_EQ(0, setup->viewport.x_dots);
        EXPECT_EQ(0, setup->viewport.y_dots);
    }
    EXPECT_EQ(expected.file_width, g_file_x_dots);
    EXPECT_EQ(expected.file_height, g_file_y_dots);
    EXPECT_EQ(expected.initial_status, g_calc_status);
    EXPECT_EQ(expected.view_width != 0, g_viewport.enabled);
    EXPECT_EQ(expected.view_width, g_viewport.x_dots);
    EXPECT_EQ(expected.view_height, g_viewport.y_dots);
    EXPECT_EQ(0, g_skip_x_dots);
}

INSTANTIATE_TEST_SUITE_P(ViewportInputs, TestPendingViewport,
    Values(PendingViewportCase{"LargeGif", 65535, 65535, 0.0f, CalcStatus::COMPLETED, 0, 0, false, 596, 596, 109, 109,
               CalcStatus::COMPLETED, std::nullopt, std::nullopt},
        PendingViewportCase{"UnequalSkipFactors", 1600, 1800, 0.75f, CalcStatus::RESUMABLE, 0, 0, false, 800, 600, 1, 2,
            CalcStatus::PARAMS_CHANGED, std::nullopt, std::nullopt},
        PendingViewportCase{"ExplicitView", 800, 600, 0.0f, CalcStatus::RESUMABLE, 200, 0, false, 800, 600, 3, 3,
            CalcStatus::PARAMS_CHANGED, true, 4.0f},
        PendingViewportCase{"DisabledPendingView", 800, 600, 0.0f, CalcStatus::RESUMABLE, 200, 150, true, 800, 600, 0,
            0, CalcStatus::RESUMABLE, false, std::nullopt}),
    [](const TestParamInfo<PendingViewportCase> &info) { return info.param.name; });

class LoadFileTest : public Test
{
protected:
    void SetUp() override;
    void TearDown() override;

    MockDriver m_driver;
    ValueSaver<ShowFile> saved_show_file{g_show_file};
    ValueSaver<int> saved_init_mode{g_init_mode};
    ValueSaver<bool> saved_loaded_3d{g_loaded_3d};
    ValueSaver<bool> saved_fast_restore{g_fast_restore};
    ValueSaver<bool> saved_view_window{g_viewport.enabled};
    ValueSaver<FractalType> saved_fractal_type{g_fractal_type, FractalType::JULIA};
    ValueSaver<const FractalSpecific *> saved_fractal_specific{
        g_cur_fractal_specific, get_fractal_specific(FractalType::JULIA)};
    ValueSaver<int> saved_invert{g_inversion.invert};
    ValueSaver<bool> saved_make_parameter_file{g_make_parameter_file};
    ValueSaver<int> saved_colors{g_colors};
    ValueSaver<bool> saved_potential_flag{g_potential.flag};
    ValueSaver<int> saved_user_biomorph_value{g_user.biomorph_value};
    ValueSaver<long> saved_user_distance_estimator_value{g_user.distance_estimator_value};
    ValueSaver<bool> saved_legacy_float{g_user.legacy_float};
    ValueSaver<long> saved_calc_time{g_calc_time};
    ValueSaver<CalcStatus> saved_calc_status{g_calc_status};
    ValueSaver<bool> saved_potential_16bit{g_potential.store_16bit};
    ValueSaver<int> saved_file_x_dots{g_file_x_dots};
    ValueSaver<int> saved_file_y_dots{g_file_y_dots};
    ValueSaver<float> saved_file_aspect_ratio{g_file_aspect_ratio};
    ValueSaver<bool> saved_new_bifurcation_functions_loaded{g_new_bifurcation_functions_loaded};
    ValueSaver<double> saved_math_tol0{g_math_tol[0]};
    ValueSaver<double> saved_math_tol1{g_math_tol[1]};
    ValueSaver<Version> saved_version{g_version};
    ValueSaver<Version> saved_file_version{g_file_version};
    ValueSaver<bool> saved_ask_video{g_ask_video, false};
    ValueSaver<int> saved_video_table_len{g_video_table_len, 1};
    ValueSaver<int> saved_video_colors{g_video_table[0].colors, 256};
    ValueSaver<int> saved_video_width{g_video_table[0].x_dots, 800};
    ValueSaver<int> saved_video_height{g_video_table[0].y_dots, 600};
    ValueSaver<Driver *> saved_driver{g_driver, &m_driver};
};

void LoadFileTest::SetUp()
{
    EXPECT_CALL(m_driver, check_memory);
    EXPECT_CALL(m_driver, write_palette);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    history_init();
    save_history_info();
}

void LoadFileTest::TearDown()
{
    restore_history_info(g_history_ptr);
    Test::TearDown();
}

class TestLoadFile : public LoadFileTest, public WithParamInterface<FileFractalType>
{
};

TEST_P(TestLoadFile, integerTypeMigrated)
{
    ValueSaver saved_read_filename{g_read_filename, GetParam().filename};

    const int result{prepare_image_restore(g_viewport)};

    EXPECT_EQ(0, result);
    EXPECT_EQ(GetParam().type, g_fractal_type);
    EXPECT_EQ(g_file_version, g_version);
}

TEST_F(TestLoadFile, randomSeedRestored)
{
    FindFractalInfoResult metadata;
    ASSERT_FALSE(metadata.read(ID_TEST_GIF_MANDEL_INT));
    ValueSaver saved_read_filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver saved_random_seed{g_random_seed, -999};
    ValueSaver saved_random_seed_flag{g_random_seed_flag, true};

    const int result{prepare_image_restore(g_viewport)};

    EXPECT_EQ(0, result);
    EXPECT_EQ(metadata.info.random_seed, g_random_seed);
    EXPECT_EQ(metadata.info.random_seed_flag != 0, g_random_seed_flag);
}

TEST_F(TestLoadFile, legacyFloatRestored)
{
    ValueSaver saved_read_filename{g_read_filename, ID_TEST_GIF_MANDELFN_INT};
    g_user.legacy_float = true;

    const int result{prepare_image_restore(g_viewport)};

    EXPECT_EQ(0, result);
    EXPECT_FALSE(g_user.legacy_float);
}

class TestMatchingGifViewport : public LoadFileTest, public WithParamInterface<bool>
{
protected:
    void SetUp() override;

    ValueSaver<std::filesystem::path> filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver<Viewport> viewport{g_viewport, Viewport{}};
    ValueSaver<int> adapter{g_adapter, 0};
    ValueSaver<int> key{g_video_mode_keys[0], ID_KEY_F2};
    ValueSaver<Driver *> mode_driver{g_video_table[0].driver, nullptr};
    ValueSaver<float> screen_aspect{g_screen_aspect, 0.75f};
    ValueSaver<int> skip_x{g_skip_x_dots, 0};
    ValueSaver<int> skip_y{g_skip_y_dots, 0};
};

void TestMatchingGifViewport::SetUp()
{
    LoadFileTest::SetUp();
    g_fast_restore = GetParam();
    g_viewport.enabled = true;
    g_viewport.x_dots = 200;
    g_viewport.y_dots = 150;
}

TEST_P(TestMatchingGifViewport, restoresUsingRequestedViewWindowPolicy)
{
    const bool fast_restore{GetParam()};

    ASSERT_EQ(0, prepare_image_restore(g_viewport));

    EXPECT_EQ(!fast_restore, g_viewport.enabled);
    EXPECT_EQ(0, g_viewport.x_dots);
    EXPECT_EQ(0, g_viewport.y_dots);
    EXPECT_EQ(fast_restore ? 0 : 3, g_skip_x_dots);
    EXPECT_EQ(fast_restore ? 0 : 3, g_skip_y_dots);
    if (!fast_restore)
    {
        EXPECT_FLOAT_EQ(4.0f, g_viewport.reduction);
    }
    EXPECT_EQ(800, g_file_x_dots);
    EXPECT_EQ(600, g_file_y_dots);
    EXPECT_EQ(ShowFile::LOAD_IMAGE, g_show_file);
}

INSTANTIATE_TEST_SUITE_P(RestorePolicy, TestMatchingGifViewport, Bool(),
    [](const TestParamInfo<bool> &info) { return info.param ? "Fast" : "ViewWindow"; });

class TestRestoreCancellation : public LoadFileTest
{
protected:
    void SetUp() override;
    void expect_active_image_unchanged() const;

    ValueSaver<std::filesystem::path> filename{g_read_filename, ID_TEST_GIF_MANDEL_INT};
    ValueSaver<BatchMode> batch{g_init_batch, BatchMode::NONE};
    ValueSaver<CalcStatus> status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<long> iterations{g_max_iterations, 1234L};
    ValueSaver<int> width{g_file_x_dots, 987};
};

void TestRestoreCancellation::SetUp()
{
    LoadFileTest::SetUp();
    EXPECT_CALL(m_driver, set_clear()).Times(AnyNumber());
    EXPECT_CALL(m_driver, set_attr(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, put_string(_, _, _, _)).Times(AnyNumber());
    EXPECT_CALL(m_driver, hide_text_cursor()).Times(AnyNumber());
}

void TestRestoreCancellation::expect_active_image_unchanged() const
{
    EXPECT_EQ(FractalType::JULIA, g_fractal_type);
    EXPECT_EQ(1234, g_max_iterations);
    EXPECT_EQ(987, g_file_x_dots);
    EXPECT_EQ(CalcStatus::COMPLETED, g_calc_status);
    EXPECT_EQ(-1, g_init_mode);
}

class TestVideoRestoreCancellation : public TestRestoreCancellation
{
protected:
    void SetUp() override;
    void expect_viewport_unchanged() const;

    ValueSaver<bool> ask{g_ask_video, true};
    ValueSaver<Driver *> mode_driver{g_video_table[0].driver, &m_driver};
    ValueSaver<Viewport> viewport{g_viewport, Viewport{}};
    ValueSaver<int> skip_x{g_skip_x_dots};
    ValueSaver<int> skip_y{g_skip_y_dots};
    Viewport m_pending_viewport{};
    const std::string m_description{"mock"};
};

void TestVideoRestoreCancellation::SetUp()
{
    TestRestoreCancellation::SetUp();
    g_viewport.enabled = true;
    g_viewport.x_dots = 200;
    g_viewport.y_dots = 150;
    m_pending_viewport = g_viewport;
    m_pending_viewport.enabled = false;
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, get_description()).WillRepeatedly(ReturnRef(m_description));
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(1));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ESC));
}

void TestVideoRestoreCancellation::expect_viewport_unchanged() const
{
    EXPECT_TRUE(g_viewport.enabled);
    EXPECT_EQ(200, g_viewport.x_dots);
    EXPECT_EQ(150, g_viewport.y_dots);
}

TEST_F(TestVideoRestoreCancellation, retryAfterCancellationAppliesPendingImage)
{
    ASSERT_EQ(-1, prepare_image_restore(m_pending_viewport));
    expect_active_image_unchanged();
    expect_viewport_unchanged();

    EXPECT_CALL(m_driver, check_memory());
    EXPECT_CALL(m_driver, wait_key_pressed(false)).WillOnce(Return(1));
    EXPECT_CALL(m_driver, get_key()).WillOnce(Return(ID_KEY_ENTER));

    EXPECT_EQ(0, prepare_image_restore(m_pending_viewport));

    EXPECT_EQ(FractalType::MANDEL, g_fractal_type);
    EXPECT_EQ(ShowFile::LOAD_IMAGE, g_show_file);
    EXPECT_NE(1234, g_max_iterations);
    EXPECT_FALSE(g_viewport.enabled);
    EXPECT_EQ(0, g_skip_x_dots);
    EXPECT_EQ(0, g_skip_y_dots);
}

TEST_F(TestRestoreCancellation, threeDCancellationKeepsActiveImage)
{
    ValueSaver display{g_display_3d, Display3DMode::YES};
    const ThreeDSettings before;
    EXPECT_CALL(m_driver, key_cursor(_, _)).WillOnce(Return(ID_KEY_ESC));

    EXPECT_EQ(-1, prepare_image_restore(g_viewport));

    expect_active_image_unchanged();
    EXPECT_EQ(before.x_rot, id::geometry::g_x_rot);
    EXPECT_EQ(before.sphere, id::geometry::g_sphere);
}

TEST(TestThreeDSettings, editsRemainPendingUntilApplied)
{
    ValueSaver rotation{id::geometry::g_x_rot, 60};
    ValueSaver preview{id::geometry::g_preview, false};
    ValueSaver color_state{g_color_state, ColorState::DEFAULT_MAP};
    ThreeDSettings pending;

    pending.x_rot = 75;
    pending.preview = true;
    pending.color_state = ColorState::MAP_FILE;

    EXPECT_EQ(60, id::geometry::g_x_rot);
    EXPECT_FALSE(id::geometry::g_preview);
    EXPECT_EQ(ColorState::DEFAULT_MAP, g_color_state);
    pending.apply();
    EXPECT_EQ(75, id::geometry::g_x_rot);
    EXPECT_TRUE(id::geometry::g_preview);
    EXPECT_EQ(ColorState::MAP_FILE, g_color_state);
}

static FileFractalType s_int_types[]{
    {ID_TEST_GIF_MANDEL_INT, FractalType::MANDEL},                      //
    {ID_TEST_GIF_JULIA_INT, FractalType::JULIA},                        //
    {ID_TEST_GIF_LAMBDA_INT, FractalType::LAMBDA},                      //
    {ID_TEST_GIF_MANOWAR_INT, FractalType::MAN_O_WAR},                  //
    {ID_TEST_GIF_SIERPINSKI_INT, FractalType::SIERPINSKI},              //
    {ID_TEST_GIF_BARNSLEYM1_INT, FractalType::BARNSLEY_M1},             //
    {ID_TEST_GIF_BARNSLEYJ1_INT, FractalType::BARNSLEY_J1},             //
    {ID_TEST_GIF_BARNSLEYM2_INT, FractalType::BARNSLEY_M2},             //
    {ID_TEST_GIF_BARNSLEYJ2_INT, FractalType::BARNSLEY_J2},             //
    {ID_TEST_GIF_SQRTRIG_INT, FractalType::SQR_FN},                     //
    {ID_TEST_GIF_FNPLUSFN_INT, FractalType::FN_PLUS_FN},                //
    {ID_TEST_GIF_MANDELLAMBDA_INT, FractalType::MANDEL_LAMBDA},         //
    {ID_TEST_GIF_MARKSMANDEL_INT, FractalType::MARKS_MANDEL},           //
    {ID_TEST_GIF_MARKSJULIA_INT, FractalType::MARKS_JULIA},             //
    {ID_TEST_GIF_UNITY_INT, FractalType::UNITY},                        //
    {ID_TEST_GIF_MANDEL4_INT, FractalType::MANDEL4},                    //
    {ID_TEST_GIF_JULIA4_INT, FractalType::JULIA4},                      //
    {ID_TEST_GIF_BARNSLEYM3_INT, FractalType::BARNSLEY_M3},             //
    {ID_TEST_GIF_BARNSLEYJ3_INT, FractalType::BARNSLEY_J3},             //
    {ID_TEST_GIF_TRIGSQR_INT, FractalType::FN_Z_SQR},                   //
    {ID_TEST_GIF_FNTIMESFN_INT, FractalType::FN_TIMES_FN},              //
    {ID_TEST_GIF_SQR1FN_INT, FractalType::SQR_1_OVER_FN},               //
    {ID_TEST_GIF_FNMULZZ_INT, FractalType::FN_MUL_Z_PLUS_Z},            //
    {ID_TEST_GIF_KAM_INT, FractalType::KAM},                            //
    {ID_TEST_GIF_KAM3D_INT, FractalType::KAM_3D},                       //
    {ID_TEST_GIF_LAMBDAFN_INT, FractalType::LAMBDA_FN},                 //
    {ID_TEST_GIF_MANFNZSQRD_INT, FractalType::MANDEL_FN_PLUS_Z_SQRD},   //
    {ID_TEST_GIF_JULFNZSQRD_INT, FractalType::JULIA_FN_PLUS_Z_SQRD},    //
    {ID_TEST_GIF_MANDELFN_INT, FractalType::MANDEL_FN},                 //
    {ID_TEST_GIF_MANZPOWER_INT, FractalType::MANDEL_Z_POWER},           //
    {ID_TEST_GIF_JULZPOWER_INT, FractalType::JULIA_Z_POWER},            //
    {ID_TEST_GIF_MANFNPLUSEXP_INT, FractalType::MANDEL_FN_PLUS_EXP},    //
    {ID_TEST_GIF_JULFNPLUSEXP_INT, FractalType::JULIA_FN_PLUS_EXP},     //
    {ID_TEST_GIF_POPCORN_INT, FractalType::POPCORN},                    //
    {ID_TEST_GIF_LORENZ_INT, FractalType::LORENZ},                      //
    {ID_TEST_GIF_NEWTON_INT, FractalType::NEWTON},                      //
    {ID_TEST_GIF_NEWTBASIN_INT, FractalType::NEWT_BASIN},               //
    {ID_TEST_GIF_LORENZ3D_INT, FractalType::LORENZ_3D},                 //
    {ID_TEST_GIF_FORMULA_INT, FractalType::FORMULA},                    //
    {ID_TEST_GIF_JULIBROT_INT, FractalType::JULIBROT},                  //
    {ID_TEST_GIF_ROSSLER_INT, FractalType::ROSSLER},                    //
    {ID_TEST_GIF_HENON_INT, FractalType::HENON},                        //
    {ID_TEST_GIF_SPIDER_INT, FractalType::SPIDER},                      //
    {ID_TEST_GIF_BIFURCATION_INT, FractalType::BIFURCATION},            //
    {ID_TEST_GIF_BIFLAMBDA_INT, FractalType::BIF_LAMBDA},               //
    {ID_TEST_GIF_POPCORNJULIA_INT, FractalType::POPCORN_JUL},           //
    {ID_TEST_GIF_MANOWARJULIA_INT, FractalType::MAN_O_WAR_J},           //
    {ID_TEST_GIF_FNZFNPIX_INT, FractalType::FN_PLUS_FN_PIX},            //
    {ID_TEST_GIF_MARKSMANDELPOWER_INT, FractalType::MARKS_MANDEL_PWR},  //
    {ID_TEST_GIF_BIFEQSINPI_INT, FractalType::BIF_EQ_SIN_PI},           //
    {ID_TEST_GIF_BIFPLUSSINPI_INT, FractalType::BIF_PLUS_SIN_PI},       //
    {ID_TEST_GIF_BIFSTEWART_INT, FractalType::BIF_STEWART},             //
    {ID_TEST_GIF_LAMBDAFNFN_INT, FractalType::LAMBDA_FN_FN},            //
    {ID_TEST_GIF_JULIAFNFN_INT, FractalType::JUL_FN_FN},                //
    {ID_TEST_GIF_MANLAMFNFN_INT, FractalType::MAN_LAM_FN_FN},           //
    {ID_TEST_GIF_MANDELFNFN_INT, FractalType::MAN_FN_FN},               //
    {ID_TEST_GIF_BIFMAY_INT, FractalType::BIF_MAY},                     //
    {ID_TEST_GIF_HALLEYMP_INT, FractalType::HALLEY},                    //
    {ID_TEST_GIF_JULIAINVERSE_INT, FractalType::INVERSE_JULIA},         //
    {ID_TEST_GIF_PHOENIX_INT, FractalType::PHOENIX},                    //
    {ID_TEST_GIF_MANDPHOENIX_INT, FractalType::MAND_PHOENIX},           //
    {ID_TEST_GIF_FROTHYBASIN_INT, FractalType::FROTHY_BASIN},           //
    {ID_TEST_GIF_PHOENIXCMPLX_INT, FractalType::PHOENIX_CPLX},          //
    {ID_TEST_GIF_MANDPHOENIXCMPLX_INT, FractalType::MAND_PHOENIX_CPLX}, //
};

INSTANTIATE_TEST_SUITE_P(MigrateTypes, TestLoadFile, ValuesIn(s_int_types));

namespace
{

std::vector<int> s_line_lengths;
std::vector<std::array<int, 3>> s_sound_pixels;

int capture_positive_out_line(Byte *, const int line_len)
{
    s_line_lengths.push_back(line_len);
    return 17;
}

void capture_sound_pixel(const int col, const int row, const int color)
{
    s_sound_pixels.push_back({col, row, color});
}

class TestGifOutputFile
{
public:
    TestGifOutputFile(const std::filesystem::path &path, int width, int left = 0, int top = 0);

    ~TestGifOutputFile()
    {
        std::filesystem::remove(m_path);
    }

    const std::filesystem::path &path() const
    {
        return m_path;
    }

private:
    void write(int width, int left, int top) const;

    std::filesystem::path m_path;
};

TestGifOutputFile::TestGifOutputFile(
    const std::filesystem::path &path, const int width, const int left, const int top) :
    m_path(path)
{
    write(width, left, top);
}

void TestGifOutputFile::write(const int width, const int left, const int top) const
{
    int error{};
    GifFileType *gif = EGifOpenFileName(m_path.string().c_str(), false, &error);
    if (gif == nullptr)
    {
        throw std::runtime_error("Failed to create test GIF");
    }
    ColorMapObject *color_map = GifMakeMapObject(2, nullptr);
    color_map->Colors[0].Red = 255;
    color_map->Colors[0].Green = 0;
    color_map->Colors[0].Blue = 0;
    color_map->Colors[1].Red = 0;
    color_map->Colors[1].Green = 0;
    color_map->Colors[1].Blue = 0;
    std::vector<GifPixelType> pixels(width, 0);
    const bool failed = EGifPutScreenDesc(gif, width + left, 1 + top, 2, 0, color_map) == GIF_ERROR ||
        EGifPutImageDesc(gif, left, top, width, 1, false, nullptr) == GIF_ERROR ||
        EGifPutLine(gif, pixels.data(), width) == GIF_ERROR || EGifCloseFile(gif, &error) == GIF_ERROR;
    GifFreeMapObject(color_map);
    if (failed)
    {
        throw std::runtime_error("Failed to write test GIF");
    }
}

} // namespace

class TestGifViewRestore : public Test
{
protected:
    void SetUp() override;
    void create_gif(int width, int left = 0, int top = 0);
    void expect_no_input();

    MockDriver m_driver;
    std::unique_ptr<TestGifOutputFile> m_gif;

private:
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
    ValueSaver<std::filesystem::path> m_read_filename{g_read_filename};
    ValueSaver<OutLine> m_out_line{g_out_line, out_line};
    ValueSaver<bool> m_read_color{g_read_color, true};
    ValueSaver<ColorState> m_color_state{g_color_state, ColorState::DEFAULT_MAP};
    ValueSaver<Display3DMode> m_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<bool> m_map_set{g_map_set, false};
    ValueSaver<int> m_colors{g_colors, 256};
    ValueSaver<bool> m_dither_flag{g_dither_flag, false};
    ValueSaver<bool> m_potential_16bit{g_potential.store_16bit, false};
    ValueSaver<int> m_skip_x_dots{g_skip_x_dots, 0};
    ValueSaver<int> m_skip_y_dots{g_skip_y_dots, 0};
    ValueSaver<CalcStatus> m_calc_status{g_calc_status, CalcStatus::COMPLETED};
    ValueSaver<bool> m_busy{g_busy, false};
    ValueSaver<LogicalScreen> m_screen{g_logical_screen};
    ValueSaver<int> m_row_count{g_row_count};
    ValueSaver<std::vector<int>> m_line_lengths{s_line_lengths};
    ValueSaver<std::vector<std::array<int, 3>>> m_sound_pixels{s_sound_pixels};
};

void TestGifViewRestore::SetUp()
{
    Test::SetUp();
    s_line_lengths.clear();
    s_sound_pixels.clear();
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
}

void TestGifViewRestore::create_gif(const int width, const int left, const int top)
{
    const TestInfo *info{UnitTest::GetInstance()->current_test_info()};
    std::string name{std::string{info->test_suite_name()} + "-" + info->name()};
    std::replace(name.begin(), name.end(), '/', '-');
    m_gif = std::make_unique<TestGifOutputFile>(
        std::filesystem::temp_directory_path() / ("id-gifview-" + name + ".gif"), width, left, top);
    g_read_filename = m_gif->path();
}

void TestGifViewRestore::expect_no_input()
{
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    EXPECT_CALL(m_driver, key_pressed()).Times(0);
    EXPECT_CALL(m_driver, get_key()).Times(0);
}

class TestGifViewLineWidth : public TestGifViewRestore, public WithParamInterface<int>
{
protected:
    void SetUp() override;
};

void TestGifViewLineWidth::SetUp()
{
    TestGifViewRestore::SetUp();
    create_gif(GetParam());
}

TEST_P(TestGifViewLineWidth, positiveCallbackResultPreservesFullWidthAndSuccessfulRestore)
{
    g_out_line = capture_positive_out_line;
    expect_no_input();

    EXPECT_EQ(0, restore_image());

    ASSERT_EQ(1, s_line_lengths.size());
    EXPECT_EQ(GetParam(), s_line_lengths[0]);
    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(m_gif->path()));
}

TEST_P(TestGifViewLineWidth, callbackFailureClosesFileAndClearsBusyState)
{
    g_out_line = [](Byte *, int)
    {
        EXPECT_TRUE(g_busy);
        return -73;
    };

    EXPECT_EQ(-73, restore_image());

    EXPECT_FALSE(g_busy);
    EXPECT_TRUE(std::filesystem::remove(m_gif->path()));
}

INSTANTIATE_TEST_SUITE_P(Widths, TestGifViewLineWidth, Values(640, MAX_PIXELS, MAX_PIXELS + 1, GIF_MAX_PIXELS));

class TestGifViewSound : public TestGifViewRestore, public WithParamInterface<bool>
{
protected:
    void SetUp() override;

private:
    ValueSaver<int> m_sound_flag{g_sound_flag, SOUNDFLAG_X};
    ValueSaver<decltype(g_put_color)> m_put_color{g_put_color, capture_sound_pixel};
    ValueSaver<int> m_orbit_delay{g_orbit_delay, 0};
    ValueSaver<int> m_orbit_flags{g_orbit_save_flags, 0};
    ValueSaver<int> m_base_hertz{g_base_hertz, 220};
};

void TestGifViewSound::SetUp()
{
    TestGifViewRestore::SetUp();
    create_gif(8);
    g_logical_screen = LogicalScreen{4, 2, 0, 0, 3.0, 1.0};
}

TEST_P(TestGifViewSound, restoresDecodedPixelsAcrossDisplayRowsWithoutPollingInput)
{
    const bool supported{GetParam()};
    EXPECT_CALL(m_driver, sound_on(220)).Times(8).WillRepeatedly(Return(supported));
    EXPECT_CALL(m_driver, sound_off()).Times(supported ? 8 : 0);
    EXPECT_CALL(m_driver, mute()).Times(0);
    EXPECT_CALL(m_driver, delay(_)).Times(0);
    expect_no_input();

    EXPECT_EQ(0, restore_image());

    const std::vector<std::array<int, 3>> expected{
        {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {3, 1, 0}};
    EXPECT_EQ(expected, s_sound_pixels);
    EXPECT_EQ(2, g_row_count);
}

INSTANTIATE_TEST_SUITE_P(DriverSound, TestGifViewSound, Bool());

class TestGifViewMigs : public TestGifViewRestore, public WithParamInterface<bool>
{
protected:
    void SetUp() override;
    void TearDown() override;

private:
    ValueSaver<int> m_screen_x{g_screen_x_dots, 8};
    ValueSaver<int> m_screen_y{g_screen_y_dots, 8};
};

void TestGifViewMigs::SetUp()
{
    TestGifViewRestore::SetUp();
    const bool skip_x{GetParam()};
    create_gif(4, 3, 4);
    g_logical_screen = LogicalScreen{8, 8, 0, 0, 1.0, 1.0};
    g_skip_x_dots = skip_x ? 1 : 0;
    g_skip_y_dots = skip_x ? 0 : 1;
    set_normal_dot();
}

void TestGifViewMigs::TearDown()
{
    set_null_video();
    TestGifViewRestore::TearDown();
}

TEST_P(TestGifViewMigs, reducesImageOffsetsAlongTheirOwnAxisWithoutPollingInput)
{
    const bool skip_x{GetParam()};
    expect_no_input();
    const int left{skip_x ? 1 : 3};
    const int top{skip_x ? 4 : 2};
    const int width{skip_x ? 2 : 4};
    for (int col = left; col < left + width; ++col)
    {
        EXPECT_CALL(m_driver, write_pixel(col, top, 0));
    }

    EXPECT_EQ(0, restore_image());

    EXPECT_EQ(1, g_row_count);
}

INSTANTIATE_TEST_SUITE_P(SkipAxis, TestGifViewMigs, Bool());

} // namespace id::test
