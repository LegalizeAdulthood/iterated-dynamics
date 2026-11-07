// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/save_image.h>

#include <engine/calcfrac.h>
#include <engine/LogicalScreen.h>
#include <misc/ValueSaver.h>
#include <ui/big_while_loop.h>
#include <ui/evolve.h>

#include <io/encoder.h>
#include <ColorMapSaver.h>
#include <FailingOutputBuffer.h>
#include <FindFractalInfoState.h>
#include <FindFractalInfoTestSupport.h>
#include <GifReadSupport.h>
#include <MockDriver.h>
#include <TemporaryDirectory.h>
#include <engine/calc_frac_init.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/pixel.h>
#include <engine/pixel_limits.h>
#include <engine/resume.h>
#include <engine/spindac.h>
#include <engine/StickyOrbits.h>
#include <engine/UserData.h>
#include <engine/VideoInfo.h>
#include <engine/Viewport.h>
#include <fractals/fractype.h>
#include <fractals/formula.h>
#include <fractals/ifs.h>
#include <fractals/lorenz.h>
#include <fractals/lsystem.h>
#include <fractals/parser.h>
#include <io/check_write_file.h>
#include <io/gif_extensions.h>
#include <io/gif_file.h>
#include <io/GifWriter.h>
#include <io/ImageSave.h>
#include <io/library.h>
#include <io/loadfile.h>
#include <io/save_timer.h>
#include <io/special_dirs.h>
#include <math/biginit.h>
#include <misc/debug_flags.h>
#include <misc/Driver.h>
#include <misc/memory.h>
#include <misc/version.h>
#include <io/disk_storage.h>
#include <ui/diskvid.h>
#include <ui/make_batch_file.h>
#include <ui/slideshw.h>
#include <ui/temp_msg.h>
#include <ui/video.h>
#include <ImageSaveTest.h>
#include <gif_lib.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;
using namespace id::ui;
using namespace testing;

using namespace id::fractals;
using namespace id::math;
using namespace id::misc::test;
using namespace id::test::image_save_test;

namespace id::test
{

class TestEvolutionSaveSnapshot : public Test
{
protected:
    void SetUp() override;

    ValueSaver<EvolutionModeFlags> m_saved_mode{
        g_evolving, EvolutionModeFlags::FIELD_MAP | EvolutionModeFlags::NO_GROUT};
    ValueSaver<CalcStatus> m_saved_status{g_calc_status, CalcStatus::RESUMABLE};
    ValueSaver<bool> m_saved_have_info{g_have_evolve_info, false};
    ValueSaver<EvolutionInfo> m_saved_info{g_evolve_info, EvolutionInfo{}};
    ValueSaver<LogicalScreen> m_saved_screen{g_logical_screen, LogicalScreen{8, 6, 2, 1, 7, 5}};
    ValueSaver<int> m_saved_grid{g_evolve_image_grid_size, 3};
    ValueSaver<unsigned int> m_saved_seed{g_evolve_this_generation_random_seed, 0x1ABCDU};
    ValueSaver<double> m_saved_mutation{g_evolve_max_random_mutation, 0.25};
    ValueSaver<double> m_saved_x_range{g_evolve_x_parameter_range, 4.0};
    ValueSaver<double> m_saved_y_range{g_evolve_y_parameter_range, 3.0};
    ValueSaver<double> m_saved_x_offset{g_evolve_x_parameter_offset, -2.0};
    ValueSaver<double> m_saved_y_offset{g_evolve_y_parameter_offset, -1.5};
    ValueSaver<char> m_saved_discrete_x{g_evolve_discrete_x_parameter_offset, static_cast<char>(-7)};
    ValueSaver<char> m_saved_discrete_y{g_evolve_discrete_y_parameter_offset, static_cast<char>(9)};
    ValueSaver<int> m_saved_px{g_evolve_param_grid_x, 1};
    ValueSaver<int> m_saved_py{g_evolve_param_grid_y, 2};
    ValueSaver<Variations> m_saved_gene{g_gene_bank[2].mutate, Variations::Y};
    ValueSaver<Variations> m_saved_last_gene{g_gene_bank[NUM_GENES - 1].mutate, Variations::WEIGHTED_RANDOM};
};

void TestEvolutionSaveSnapshot::SetUp()
{
    g_evolve_info.evolving = 1;
    g_evolve_info.image_grid_size = 5;
    g_evolve_info.this_generation_random_seed = 0x9876;
    g_evolve_info.max_random_mutation = 0.5;
    g_evolve_info.x_parameter_range = 8;
    g_evolve_info.y_parameter_range = 6;
    g_evolve_info.x_parameter_offset = -4;
    g_evolve_info.y_parameter_offset = -3;
    g_evolve_info.discrete_x_parameter_offset = -11;
    g_evolve_info.discrete_y_parameter_offset = 13;
    g_evolve_info.px = 3;
    g_evolve_info.py = 4;
    g_evolve_info.screen_x_offset = 5;
    g_evolve_info.screen_y_offset = 6;
    g_evolve_info.x_dots = 0xFFFF;
    g_evolve_info.y_dots = 0x8000;
    g_evolve_info.count = 17;
    std::fill(std::begin(g_evolve_info.mutate), std::end(g_evolve_info.mutate), -1);
    g_evolve_info.future[0] = -123;
    g_evolve_info.future[std::size(g_evolve_info.future) - 1] = 456;
}

class TestDisabledEvolutionSnapshot : public TestEvolutionSaveSnapshot, public WithParamInterface<EvolutionModeFlags>
{
};

TEST_P(TestDisabledEvolutionSnapshot, omitsTheRecordWithoutFieldMap)
{
    g_evolving = GetParam();
    g_have_evolve_info = true;

    EXPECT_FALSE(capture_evolution_info());
}

INSTANTIATE_TEST_SUITE_P(Modes, TestDisabledEvolutionSnapshot,
    Values(EvolutionModeFlags::NONE, EvolutionModeFlags::RAND_WALK, EvolutionModeFlags::RAND_PARAM,
        EvolutionModeFlags::NO_GROUT, EvolutionModeFlags::PARAM_BOX));

class TestLiveEvolutionSnapshot : public TestEvolutionSaveSnapshot,
                                  public WithParamInterface<std::pair<bool, CalcStatus>>
{
};

TEST_P(TestLiveEvolutionSnapshot, capturesLiveControlsAndCurrentGeneMutations)
{
    g_have_evolve_info = GetParam().first;
    g_calc_status = GetParam().second;

    const std::optional<EvolutionInfo> info{capture_evolution_info()};

    ASSERT_TRUE(info);
    EXPECT_EQ(9, info->evolving);
    EXPECT_EQ(3, info->image_grid_size);
    EXPECT_EQ(0xABCD, info->this_generation_random_seed);
    EXPECT_EQ(9, info->count);
    EXPECT_EQ(0.25, info->max_random_mutation);
    EXPECT_EQ(4.0, info->x_parameter_range);
    EXPECT_EQ(3.0, info->y_parameter_range);
    EXPECT_EQ(-2.0, info->x_parameter_offset);
    EXPECT_EQ(-1.5, info->y_parameter_offset);
    EXPECT_EQ(-7, info->discrete_x_parameter_offset);
    EXPECT_EQ(9, info->discrete_y_parameter_offset);
    EXPECT_EQ(1, info->px);
    EXPECT_EQ(2, info->py);
    EXPECT_EQ(2, info->screen_x_offset);
    EXPECT_EQ(1, info->screen_y_offset);
    EXPECT_EQ(8, info->x_dots);
    EXPECT_EQ(6, info->y_dots);
    for (int i = 0; i < NUM_GENES; ++i)
    {
        EXPECT_EQ(static_cast<std::int16_t>(g_gene_bank[i].mutate), info->mutate[i]);
    }
    EXPECT_TRUE(
        std::all_of(std::begin(info->future), std::end(info->future), [](const auto value) { return value == 0; }));
}

INSTANTIATE_TEST_SUITE_P(States, TestLiveEvolutionSnapshot,
    Values(std::make_pair(false, CalcStatus::RESUMABLE), std::make_pair(false, CalcStatus::IN_PROGRESS),
        std::make_pair(false, CalcStatus::COMPLETED), std::make_pair(true, CalcStatus::COMPLETED)));

class TestCachedEvolutionSnapshot : public TestEvolutionSaveSnapshot, public WithParamInterface<CalcStatus>
{
};

TEST_P(TestCachedEvolutionSnapshot, retainsCachedProgressAndRefreshesEveryGeneMutation)
{
    g_have_evolve_info = true;
    g_calc_status = GetParam();
    EvolutionInfo expected{g_evolve_info};
    for (int i = 0; i < NUM_GENES; ++i)
    {
        expected.mutate[i] = static_cast<std::int16_t>(g_gene_bank[i].mutate);
    }

    const std::optional<EvolutionInfo> info{capture_evolution_info()};

    ASSERT_TRUE(info);
    EXPECT_EQ(expected, *info);
    EXPECT_EQ(17, info->count);
    EXPECT_EQ(-123, info->future[0]);
    EXPECT_EQ(456, info->future[std::size(info->future) - 1]);
}

INSTANTIATE_TEST_SUITE_P(States, TestCachedEvolutionSnapshot, Values(CalcStatus::RESUMABLE, CalcStatus::IN_PROGRESS));

namespace
{

constexpr std::array<Byte, 3> EGA_PALETTE[]{
    {0, 0, 0},
    {0, 0, 42},
    {0, 42, 0},
    {0, 42, 42},
    {42, 0, 0},
    {42, 0, 42},
    {42, 21, 0},
    {42, 42, 42},
    {21, 21, 21},
    {21, 21, 63},
    {21, 63, 21},
    {21, 63, 63},
    {63, 21, 21},
    {63, 21, 63},
    {63, 63, 21},
    {63, 63, 63},
};

} // namespace

class TestGifWriterRaster : public ImageSaveTest
{
protected:
    void SetUp() override;
    void TearDown() override;
    int save();

    ValueSaver<bool> m_saved_busy{g_busy, false};
    ValueSaver<EvolutionModeFlags> m_saved_evolving{g_evolving, EvolutionModeFlags::NONE};
};

void TestGifWriterRaster::SetUp()
{
    ImageSaveTest::SetUp();
    EXPECT_CALL(m_driver, is_disk()).WillRepeatedly(Return(false));
    EXPECT_CALL(m_driver, flush()).Times(AnyNumber());
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly([this](const int x, const int y, const int color)
            { m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x) = static_cast<Byte>(color); });
    configure_image(8, 6, 256);
}

void TestGifWriterRaster::TearDown()
{
    if (m_memory_initialized)
    {
        end_disk();
        exit_check();
    }
    set_null_video();
    free_temp_msg();
    clear_save_library();
    m_saved_palette.reset();
    m_directory.remove();
}

int TestGifWriterRaster::save()
{
    std::filesystem::remove(m_path);
    std::filesystem::path filename{m_path};
    return save_image(filename);
}

class TestGifWriterPalette : public TestGifWriterRaster, public WithParamInterface<std::pair<int, bool>>
{
};

TEST_P(TestGifWriterPalette, savesRasterAndPaletteThroughProductionEntry)
{
    const int colors{GetParam().first};
    const bool real_dac{GetParam().second};
    configure_image(8, 6, colors);
    g_got_real_dac = real_dac;
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, 8, 6);
    std::ifstream input{m_path, std::ios::binary};
    std::array<char, 6> signature{};
    input.read(signature.data(), signature.size());
    EXPECT_EQ("GIF89a", (std::string{signature.data(), signature.size()}));
    EXPECT_EQ(0, gif->SBackGroundColor);
    EXPECT_EQ(8, gif->SColorResolution);
    EXPECT_EQ(49, gif->AspectByte);
    ASSERT_NE(nullptr, gif->SColorMap);
    ASSERT_EQ(colors, gif->SColorMap->ColorCount);
    for (int color = 0; color < colors; ++color)
    {
        SCOPED_TRACE(color);
        std::array<Byte, 3> expected{};
        if (colors == 2)
        {
            expected = color == 0 ? std::array<Byte, 3>{0, 0, 0} : std::array<Byte, 3>{63, 63, 63};
        }
        else if (colors == 4)
        {
            constexpr std::array<Byte, 3> CGA_PALETTE[]{{0, 0, 0}, {21, 63, 63}, {63, 21, 63}, {63, 63, 63}};
            expected = CGA_PALETTE[color];
        }
        else if (real_dac)
        {
            std::copy_n(g_dac_box[color], 3, expected.begin());
        }
        else
        {
            expected = EGA_PALETTE[color % 16];
        }
        const GifColorType &actual{gif->SColorMap->Colors[color]};
        EXPECT_EQ(expected, (std::array<Byte, 3>{actual.Red, actual.Green, actual.Blue}));
    }
    EXPECT_EQ(pixels, image_pixels());
    EXPECT_FALSE(g_busy);
}

INSTANTIATE_TEST_SUITE_P(Palettes, TestGifWriterPalette,
    Values(std::pair{2, false}, std::pair{2, true}, std::pair{4, false}, std::pair{4, true}, std::pair{16, false},
        std::pair{16, true}, std::pair{256, false}, std::pair{256, true}));

TEST_F(TestGifWriterRaster, savesLogicalPixelsAtPhysicalScreenOffset)
{
    configure_image(9, 5, 256, 3, 2);
    const std::vector<Byte> pixels{image_pixels()};
    const std::vector<Byte> frame{m_frame};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, 9, 5);
    EXPECT_EQ(frame, m_frame);
}

struct SavedGifAspect
{
    const char *name;
    float aspect;
    int byte;
};

void PrintTo(const SavedGifAspect &value, std::ostream *str)
{
    *str << value.name;
}

class TestGifWriterAspect : public TestGifWriterRaster, public WithParamInterface<SavedGifAspect>
{
protected:
    void SetUp() override;
};

void TestGifWriterAspect::SetUp()
{
    TestGifWriterRaster::SetUp();
    g_viewport.enabled = true;
    g_viewport.x_dots = 8;
    g_viewport.y_dots = 6;
    g_viewport.final_aspect_ratio = GetParam().aspect;
}

TEST_P(TestGifWriterAspect, savesAspectByteForExplicitView)
{
    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    EXPECT_EQ(GetParam().byte, gif->AspectByte);
}

INSTANTIATE_TEST_SUITE_P(AspectInputs, TestGifWriterAspect,
    Values(SavedGifAspect{"NormalAspect", 0.75f, 49}, SavedGifAspect{"WideAspect", 0.5f, 81},
        SavedGifAspect{"ClampLowByte", 1000.0f, 1}, SavedGifAspect{"ClampHighByte", 0.01f, 255}),
    [](const TestParamInfo<SavedGifAspect> &info) { return info.param.name; });

TEST_F(TestGifWriterRaster, savesAutomaticViewAspectFromPhysicalScreen)
{
    configure_image(8, 3, 256);
    g_viewport.enabled = true;
    g_screen_x_dots = 800;
    g_screen_y_dots = 600;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    EXPECT_EQ(49, gif->AspectByte);
    EXPECT_EQ(8, gif->SWidth);
    EXPECT_EQ(3, gif->SHeight);
}

class TestGifWriterDimensions : public TestGifWriterRaster, public WithParamInterface<std::pair<int, int>>
{
};

TEST_P(TestGifWriterDimensions, savesUnsignedDimensionsThroughProductionEntry)
{
    const int width{GetParam().first};
    const int height{GetParam().second};
    configure_image(width, height, 2);
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, width, height);
}

INSTANTIATE_TEST_SUITE_P(UnsignedBounds, TestGifWriterDimensions,
    Values(std::pair{32768, 1}, std::pair{GIF_MAX_PIXELS, 1}, std::pair{1, 32768}, std::pair{1, GIF_MAX_PIXELS}));

TEST_F(TestGifWriterRaster, savesPotentialRowsAsColorsThenLowBytes)
{
    configure_image(5, 3, 256, 2, 1);
    const std::vector<Byte> pixels{image_pixels()};
    init_memory();
    m_memory_initialized = true;
    ASSERT_TRUE(prepare_disk_storage(g_screen_x_dots, g_screen_y_dots * 2, 256));
    g_disk_16_bit = true;
    std::vector<Byte> stored;
    for (int row = 0; row < 3; ++row)
    {
        stored.insert(stored.end(), pixels.begin() + row * 5, pixels.begin() + (row + 1) * 5);
        for (int col = 0; col < 5; ++col)
        {
            const Byte low{static_cast<Byte>(201 - row * 13 - col * 7)};
            write_disk_pixel(col + 2, row + 3 + 1, low);
            stored.push_back(low);
        }
    }
    // Progress must not run until both halves of the stored row are captured.
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(
            [this](const int x, const int y, const int color)
            {
                m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x) = static_cast<Byte>(color);
                for (int col = 0; col < 5; ++col)
                {
                    write_disk_pixel(col + 2, y - g_logical_screen.y_offset + 3 + 1, 0);
                }
            });
    EXPECT_CALL(m_driver, flush()).Times(4);

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, stored, 10, 3);
    EXPECT_EQ(pixels, image_pixels());
}

struct InvalidSavedGifDimensions
{
    const char *name;
    int width;
    int height;
    bool potential;
};

void PrintTo(const InvalidSavedGifDimensions &value, std::ostream *str)
{
    *str << value.name;
}

class TestGifWriterInvalidDimensions : public TestGifWriterRaster, public WithParamInterface<InvalidSavedGifDimensions>
{
protected:
    void SetUp() override;
};

void TestGifWriterInvalidDimensions::SetUp()
{
    TestGifWriterRaster::SetUp();
    configure_image(GetParam().width, GetParam().height, 256);
    g_disk_16_bit = GetParam().potential;
}

TEST_P(TestGifWriterInvalidDimensions, rejectsUnrepresentableDimensions)
{
    EXPECT_EQ(-1, save());

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

INSTANTIATE_TEST_SUITE_P(DimensionInputs, TestGifWriterInvalidDimensions,
    Values(InvalidSavedGifDimensions{"PotentialWidth", GIF_MAX_PIXELS / 2 + 1, 1, true},
        InvalidSavedGifDimensions{"Width", GIF_MAX_PIXELS + 1, 1, false},
        InvalidSavedGifDimensions{"Height", 1, GIF_MAX_PIXELS + 1, false}),
    [](const TestParamInfo<InvalidSavedGifDimensions> &info) { return info.param.name; });

TEST_F(TestGifWriterRaster, reportsOutputOpenFailure)
{
    std::filesystem::path filename{m_directory.path() / "missing" / "image.gif"};

    EXPECT_EQ(-1, save_image(filename));

    EXPECT_FALSE(std::filesystem::exists(filename));
    EXPECT_FALSE(g_busy);
}

class TestGifWriterIncrementalSave : public TestGifWriterRaster
{
protected:
    void SetUp() override;
    void TearDown() override;

    std::vector<Byte> m_pixels;
    std::unique_ptr<ImageSave> m_image;
};

void TestGifWriterIncrementalSave::SetUp()
{
    TestGifWriterRaster::SetUp();
    configure_image(8, 3, 256);
    m_pixels = image_pixels();
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    EXPECT_CALL(m_driver, flush()).Times(0);
    EXPECT_CALL(m_driver, write_pixel(_, _, _)).Times(0);
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(
            [this](const int x, const int y)
            {
                EXPECT_FALSE(g_busy);
                return m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x);
            });
    m_image = std::make_unique<ImageSave>(m_path, std::nullopt);
}

void TestGifWriterIncrementalSave::TearDown()
{
    m_image.reset();
    TestGifWriterRaster::TearDown();
}

TEST_F(TestGifWriterIncrementalSave, incrementalSaveDoesNotPresentUiOrChangeBusyState)
{
    for (int row = 0; row < 3; ++row)
    {
        EXPECT_FALSE(m_image->done());
        m_image->iterate();
        EXPECT_EQ(row + 1, m_image->rows_written());
    }
    EXPECT_FALSE(m_image->done());

    m_image->iterate();

    EXPECT_TRUE(m_image->done());
    EXPECT_TRUE(m_image->succeeded());
    EXPECT_TRUE(m_image->diagnostic().empty());
    EXPECT_FALSE(g_busy);
    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, m_pixels, 8, 3);
    EXPECT_EQ(m_pixels, image_pixels());
}

class TestGifWriterCompletedSave : public TestGifWriterIncrementalSave
{
protected:
    void SetUp() override;
};

void TestGifWriterCompletedSave::SetUp()
{
    TestGifWriterIncrementalSave::SetUp();
    for (int row = 0; row < 3; ++row)
    {
        m_image->iterate();
    }
    m_image->iterate();
    ASSERT_TRUE(m_image->done());
    ASSERT_TRUE(m_image->succeeded());
}

TEST_F(TestGifWriterCompletedSave, rejectsIterationAfterCompletion)
{
    EXPECT_THROW(m_image->iterate(), std::logic_error);
}

TEST_F(TestGifWriterRaster, unfinishedSaveRemovesItsPartialFileWithoutUi)
{
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    EXPECT_CALL(m_driver, flush()).Times(0);
    {
        ImageSave image{m_path, std::nullopt};
        image.iterate();
        EXPECT_TRUE(std::filesystem::exists(m_path));
        EXPECT_FALSE(image.done());
    }

    EXPECT_FALSE(std::filesystem::exists(m_path));
    EXPECT_FALSE(g_busy);
}

class TestGifWriterExistingImage : public TestGifWriterRaster
{
protected:
    void SetUp() override;

    std::vector<Byte> m_original_pixels;
};

void TestGifWriterExistingImage::SetUp()
{
    TestGifWriterRaster::SetUp();
    m_original_pixels = image_pixels();
    ASSERT_EQ(0, save());
}

TEST_F(TestGifWriterExistingImage, existingTemporaryDestinationIsNotDeleted)
{
    const std::filesystem::path temporary{m_directory.path() / "id.tmp"};
    std::filesystem::create_directory(temporary);
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    EXPECT_CALL(m_driver, flush()).Times(0);
    {
        ImageSave image{m_path, std::nullopt};
        EXPECT_TRUE(image.done());
        EXPECT_FALSE(image.succeeded());
        EXPECT_THAT(image.diagnostic(), HasSubstr("Can't create"));
    }

    EXPECT_TRUE(std::filesystem::is_directory(temporary));
    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, m_original_pixels, 8, 6);
}

TEST_F(TestGifWriterExistingImage, replacementFailurePreservesOriginalAndExistingBackup)
{
    const std::filesystem::path backup{m_directory.path() / "id.tmp.bak"};
    {
        std::ofstream reserved{backup};
        reserved << "reserved";
    }
    m_frame[0] = 123;
    EXPECT_CALL(m_driver, is_disk()).Times(0);
    EXPECT_CALL(m_driver, flush()).Times(0);
    ImageSave image{m_path, std::nullopt};
    while (!image.done())
    {
        image.iterate();
    }

    EXPECT_FALSE(image.succeeded());
    EXPECT_THAT(image.diagnostic(), HasSubstr("Can't replace"));
    EXPECT_TRUE(std::filesystem::exists(backup));
    EXPECT_FALSE(std::filesystem::exists(m_directory.path() / "id.tmp"));
    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, m_original_pixels, 8, 6);
}

TEST_F(TestGifWriterRaster, notificationFollowsProgressRestorationAndFileClosure)
{
    g_timed_save = TimedSave::NONE;
    const std::vector<Byte> pixels{image_pixels()};
    EXPECT_CALL(m_driver, buzzer(Buzzer::COMPLETE))
        .WillOnce(
            [this, &pixels](Buzzer)
            {
                EXPECT_FALSE(g_busy);
                EXPECT_EQ(pixels, image_pixels());
                const std::filesystem::path renamed{m_directory.path() / "closed.gif"};
                EXPECT_NO_THROW(std::filesystem::rename(m_path, renamed));
                EXPECT_NO_THROW(std::filesystem::rename(renamed, m_path));
            });

    EXPECT_EQ(0, save());
}

TEST_F(TestGifWriterRaster, savesWithoutConsumingPendingInput)
{
    ON_CALL(m_driver, key_pressed()).WillByDefault(Return('x'));
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, 8, 6);
    EXPECT_EQ(FRACTAL_INFO_VERSION, get_fractal_info(gif.get()).info_version);
}

TEST_F(TestGifWriterRaster, flushesProgressAfterEachStoredRowAndRestoresIt)
{
    configure_image(600, 3, 256);
    const std::vector<Byte> pixels{image_pixels()};
    int flushes{};
    int reads{};
    int writes{};
    const std::vector<int> edges{0, 599, 1, 598, 2, 597};
    EXPECT_CALL(m_driver, read_pixel(_, _))
        .WillRepeatedly(
            [this, &reads](const int x, const int y)
            {
                ++reads;
                return m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x);
            });
    EXPECT_CALL(m_driver, write_pixel(_, _, _))
        .WillRepeatedly(
            [this, &flushes, &reads, &writes, &edges](const int x, const int y, const int color)
            {
                EXPECT_TRUE(g_busy);
                if (flushes < 3)
                {
                    EXPECT_EQ(flushes, y);
                    EXPECT_EQ(edges[writes % edges.size()], x);
                    if (writes % edges.size() == 0)
                    {
                        EXPECT_EQ(601, reads);
                    }
                }
                ++writes;
                m_frame.at(static_cast<std::size_t>(y) * m_frame_width + x) = static_cast<Byte>(color);
            });
    EXPECT_CALL(m_driver, flush())
        .Times(4)
        .WillRepeatedly(
            [this, &flushes, &reads, &writes, &pixels]
            {
                EXPECT_TRUE(g_busy);
                if (flushes < 3)
                {
                    EXPECT_EQ(6 * (flushes + 1), writes);
                    EXPECT_NE(pixels, image_pixels());
                }
                else
                {
                    EXPECT_EQ(pixels, image_pixels());
                }
                reads = 0;
                ++flushes;
            });

    ASSERT_EQ(0, save());

    EXPECT_EQ(36, writes);
    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, 600, 3);
    EXPECT_FALSE(g_busy);
}

class GifWriterSaveStreamTest : public TestGifWriterRaster
{
protected:
    void TearDown() override;
    void start_save(int width, int height);

    FailingOutputBuffer m_buffer;
    std::ostream m_output{&m_buffer};
    std::vector<Byte> m_pixels;
    std::unique_ptr<ImageSave> m_image;
};

void GifWriterSaveStreamTest::start_save(const int width, const int height)
{
    configure_image(width, height, 256);
    m_pixels = image_pixels();
    m_image = std::make_unique<ImageSave>(m_output, m_path, std::nullopt);
}

void GifWriterSaveStreamTest::TearDown()
{
    m_image.reset();
    TestGifWriterRaster::TearDown();
}

class TestGifWriterRowWriteFailure : public GifWriterSaveStreamTest
{
protected:
    void SetUp() override;

    int m_flushes{};
};

void TestGifWriterRowWriteFailure::SetUp()
{
    TestGifWriterRaster::SetUp();
    start_save(600, 12);
    EXPECT_CALL(m_driver, flush())
        .WillRepeatedly(
            [this]
            {
                ++m_flushes;
                m_buffer.remaining = 0;
            });
}

TEST_F(TestGifWriterRowWriteFailure, restoresProgressAfterScanlineWriteFailure)
{
    EXPECT_FALSE(write_image(*m_image));

    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, m_image->error());
    EXPECT_GE(m_flushes, 2);
    EXPECT_LT(m_flushes, 13);
    EXPECT_EQ(m_pixels, image_pixels());
    EXPECT_TRUE(m_image->done());
    EXPECT_FALSE(m_image->succeeded());
}

class TestGifWriterSaveCloseFailure : public GifWriterSaveStreamTest
{
protected:
    void SetUp() override;
};

void TestGifWriterSaveCloseFailure::SetUp()
{
    TestGifWriterRaster::SetUp();
    start_save(8, 6);
    m_buffer.fail_sync = true;
    EXPECT_CALL(m_driver, flush()).Times(7);
}

TEST_F(TestGifWriterSaveCloseFailure, restoresProgressAndReportsCloseFailure)
{
    EXPECT_FALSE(write_image(*m_image));

    EXPECT_EQ(E_GIF_ERR_CLOSE_FAILED, m_image->error());
    EXPECT_EQ(m_pixels, image_pixels());
    EXPECT_TRUE(m_image->done());
    EXPECT_FALSE(m_image->succeeded());
}

TEST_F(TestGifWriterExistingImage, failedReplacementPreservesOriginalAndRemovesTemporaryFile)
{
    g_logical_screen.x_dots = GIF_MAX_PIXELS + 1;
    std::filesystem::path filename{m_path};

    EXPECT_EQ(-1, save_image(filename));

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, m_original_pixels, 8, 6);
    EXPECT_FALSE(std::filesystem::exists(m_directory.path() / "id.tmp"));
    EXPECT_EQ(BatchMode::BAILOUT_ERROR_NO_SAVE, g_init_batch);
    EXPECT_FALSE(g_busy);
}

TEST_F(TestGifWriterExistingImage, overwriteReplacesOriginalOnlyAfterSuccessfulSave)
{
    m_frame[0] = 123;
    const std::vector<Byte> pixels{image_pixels()};
    std::filesystem::path filename{m_path};

    ASSERT_EQ(0, save_image(filename));

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, 8, 6);
    EXPECT_FALSE(std::filesystem::exists(m_directory.path() / "id.tmp"));
}

TEST_F(TestGifWriterExistingImage, selectsNextFilenameWithoutOverwriting)
{
    g_overwrite_file = false;
    std::filesystem::path filename{m_path};

    ASSERT_EQ(0, save_image(filename));

    EXPECT_EQ(m_directory.path() / "image3.gif", filename);
    EXPECT_TRUE(std::filesystem::exists(m_directory.path() / "image2.gif"));
    EXPECT_TRUE(std::filesystem::exists(m_path));
    EXPECT_FALSE(std::filesystem::exists(m_directory.path() / "id.tmp"));
}

TEST_F(TestGifWriterRaster, autosaveReusesFilenameAndFinalSaveEndsResaving)
{
    g_resave_flag = TimedSave::STARTED;
    std::filesystem::path filename{m_path};
    ASSERT_EQ(0, save_image(filename));
    EXPECT_EQ(m_path, filename);
    EXPECT_TRUE(g_started_resaves);
    m_frame[0] = 123;
    const std::vector<Byte> pixels{image_pixels()};

    ASSERT_EQ(0, save_image(filename));

    EXPECT_EQ(m_path, filename);
    EXPECT_TRUE(g_started_resaves);
    EXPECT_FALSE(std::filesystem::exists(m_directory.path() / "id.tmp"));
    {
        const GifFilePtr gif{read_gif(m_path)};
        expect_gif_raster(*gif, pixels, 8, 6);
    }
    g_resave_flag = TimedSave::FINAL;

    ASSERT_EQ(0, save_image(filename));

    EXPECT_EQ(TimedSave::NONE, g_resave_flag);
    EXPECT_FALSE(g_started_resaves);
    EXPECT_EQ(m_directory.path() / "image2.gif", filename);
}

} // namespace id::test
