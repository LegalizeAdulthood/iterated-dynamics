// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/encoder.h>

#include <ColorMapSaver.h>
#include <FailingOutputBuffer.h>
#include <FindFractalInfoState.h>
#include <FindFractalInfoTestSupport.h>
#include <GifReadSupport.h>
#include <MockDriver.h>
#include <ImageSaveTest.h>
#include <MemoryDiskStorage.h>
#include <TemporaryDirectory.h>

#include <engine/calc_frac_init.h>
#include <engine/calcfrac.h>
#include <engine/cmdfiles.h>
#include <engine/fractals.h>
#include <engine/ImageRegion.h>
#include <engine/LogicalScreen.h>
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
#include <misc/ValueSaver.h>
#include <misc/version.h>
#include <io/disk_storage.h>

#include <gif_lib.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::math;
using namespace id::misc;
using namespace id::misc::test;
using namespace testing;
using namespace id::test::image_save_test;

namespace id::test
{

class EncoderSaveInfoTest : public Test
{
protected:
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<bool> m_saved_legacy_float{g_user.legacy_float};
    FractalInfo m_info{};
};

struct EncoderVersionInput
{
    const char *name;
    Version version;
    int release;
    std::array<int, 4> stored;
};

void PrintTo(const EncoderVersionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestEncoderVersion : public EncoderSaveInfoTest, public WithParamInterface<EncoderVersionInput>
{
protected:
    void SetUp() override;
};

void TestEncoderVersion::SetUp()
{
    g_version = GetParam().version;
}

TEST_P(TestEncoderVersion, saveInfoPreservesActiveCompatibilityVersion)
{
    const EncoderVersionInput &expected{GetParam()};

    setup_save_info(&m_info);

    EXPECT_EQ(expected.release, m_info.release);
    EXPECT_EQ(expected.stored[0], m_info.version_major);
    EXPECT_EQ(expected.stored[1], m_info.version_minor);
    EXPECT_EQ(expected.stored[2], m_info.version_patch);
    EXPECT_EQ(expected.stored[3], m_info.version_tweak);
    EXPECT_EQ(expected.version, fractal_info_version(m_info));
}

INSTANTIATE_TEST_SUITE_P(CompatibilityVersions, TestEncoderVersion,
    Values(EncoderVersionInput{"IdVersion", {1, 3, 2, 7, false}, 2004, {1, 3, 2, 7}},
        EncoderVersionInput{"LegacyVersion", parse_legacy_version(1730), 1730, {0, 0, 0, 0}}),
    [](const TestParamInfo<EncoderVersionInput> &info) { return info.param.name; });

struct EncoderFloatInput
{
    const char *name;
    Version version;
    bool legacy_float;
    int flag;
};

void PrintTo(const EncoderFloatInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestEncoderFloatPolicy : public EncoderSaveInfoTest, public WithParamInterface<EncoderFloatInput>
{
protected:
    void SetUp() override;
};

void TestEncoderFloatPolicy::SetUp()
{
    g_version = GetParam().version;
    g_user.legacy_float = GetParam().legacy_float;
}

TEST_P(TestEncoderFloatPolicy, savedFloatFlagFollowsCompatibilityPolicy)
{
    setup_save_info(&m_info);

    EXPECT_EQ(GetParam().flag, m_info.float_flag);
}

INSTANTIATE_TEST_SUITE_P(FloatPolicies, TestEncoderFloatPolicy,
    Values(EncoderFloatInput{"LegacyInteger", parse_legacy_version(1730), false, 0},
        EncoderFloatInput{"LegacyFloat", parse_legacy_version(1730), true, 1},
        EncoderFloatInput{"ModernId", {1, 4, 0, 0, false}, false, 1}),
    [](const TestParamInfo<EncoderFloatInput> &info) { return info.param.name; });

TEST_F(ImageSaveTest, savesNativeRasterAndPaletteWithoutPresentation)
{
    configure_image(9, 5, 256, 3, 2);
    g_got_real_dac = true;
    const std::vector<Byte> pixels{image_pixels()};
    const std::vector<Byte> frame{m_frame};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, pixels, 9, 5);
    ASSERT_NE(nullptr, gif->SColorMap);
    ASSERT_EQ(256, gif->SColorMap->ColorCount);
    for (int color = 0; color < 256; ++color)
    {
        const GifColorType &actual{gif->SColorMap->Colors[color]};
        EXPECT_EQ(g_dac_box[color][0], actual.Red);
        EXPECT_EQ(g_dac_box[color][1], actual.Green);
        EXPECT_EQ(g_dac_box[color][2], actual.Blue);
    }
    EXPECT_EQ(frame, m_frame);
}

TEST_F(ImageSaveTest, savesNativePotentialRowsWithoutPresentation)
{
    configure_image(5, 3, 256, 2, 1);
    const std::vector<Byte> pixels{image_pixels()};
    init_memory_disk_storage();
    m_memory_initialized = true;
    open_memory_disk_storage(g_screen_x_dots, g_screen_y_dots * 2, 256);
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

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_gif_raster(*gif, stored, 10, 3);
    EXPECT_EQ(pixels, image_pixels());
}

TEST_F(ImageSaveTest, reportsNativeScanlineWriteFailureWithoutPresentation)
{
    configure_image(600, 12, 256);
    const std::vector<Byte> pixels{image_pixels()};
    FailingOutputBuffer buffer;
    std::ostream output{&buffer};
    ImageSave image{output, m_path, std::nullopt};
    buffer.remaining = 0;

    while (!image.done())
    {
        image.iterate();
    }

    EXPECT_FALSE(image.succeeded());
    EXPECT_EQ(E_GIF_ERR_WRITE_FAILED, image.error());
    EXPECT_LT(image.rows_written(), 12);
    EXPECT_EQ(pixels, image_pixels());
}

TEST_F(ImageSaveTest, reportsNativeCloseFailureWithoutPresentation)
{
    const std::vector<Byte> pixels{image_pixels()};
    FailingOutputBuffer buffer;
    std::ostream output{&buffer};
    ImageSave image{output, m_path, std::nullopt};
    buffer.fail_sync = true;

    while (!image.done())
    {
        image.iterate();
    }

    EXPECT_FALSE(image.succeeded());
    EXPECT_EQ(E_GIF_ERR_CLOSE_FAILED, image.error());
    EXPECT_EQ(6, image.rows_written());
    EXPECT_EQ(pixels, image_pixels());
}

class TestGifWriterExtensions : public ImageSaveTest
{
protected:
    void expect_layout(GifFileType *gif, const std::vector<std::string> &identifiers) const;
    std::vector<Byte> payload(const GifFileType *gif, const std::string &identifier) const;

    FindFractalInfoResult m_result;
    ValueSaver<std::vector<Byte>> m_saved_resume{g_resume_data, std::vector<Byte>{}};
    ValueSaver<int> m_saved_resume_len{g_resume_len, 0};
    ValueSaver<std::vector<int>> m_saved_ranges{g_iteration_ranges, std::vector<int>{}};
    ValueSaver<Display3DMode> m_saved_display_3d{g_display_3d, Display3DMode::NONE};
    ValueSaver<CompiledFormula> m_saved_formula{g_formula};
    FindFractalInfoState m_saved_metadata_state;
};

void TestGifWriterExtensions::expect_layout(GifFileType *gif, const std::vector<std::string> &identifiers) const
{
    ASSERT_EQ(1, gif->ImageCount);
    EXPECT_EQ(0, gif->SavedImages[0].ExtensionBlockCount);
    std::vector<std::string> actual;
    int encoded_length{};
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (block.Function == APPLICATION_EXT_FUNC_CODE)
        {
            ASSERT_EQ(11, block.ByteCount);
            actual.emplace_back(reinterpret_cast<const char *>(block.Bytes), block.ByteCount);
            encoded_length += 3; // Introducer, function code, and terminator.
        }
        else
        {
            EXPECT_EQ(CONTINUE_EXT_FUNC_CODE, block.Function);
            EXPECT_GT(block.ByteCount, 0);
            EXPECT_LE(block.ByteCount, 255);
        }
        encoded_length += 1 + block.ByteCount;
    }
    EXPECT_EQ(identifiers, actual);
    EXPECT_EQ(encoded_length, get_fractal_info(gif).tot_extend_len);

    // Older readers find the final main record at this fixed trailer offset.
    std::ifstream input{m_path, std::ios::binary};
    std::array<char, 14> header{};
    input.seekg(-encoded_length - 1, std::ios::end);
    input.read(header.data(), header.size());
    EXPECT_EQ((std::array<char, 3>{'!', static_cast<char>(0xFF), 11}),
        (std::array<char, 3>{header[0], header[1], header[2]}));
    ASSERT_FALSE(identifiers.empty());
    EXPECT_EQ(identifiers.front(), (std::string{header.data() + 3, 11}));
    input.seekg(-522, std::ios::end);
    input.read(header.data(), header.size());
    EXPECT_EQ("fractint001", (std::string{header.data() + 3, 11}));
    input.seekg(-1, std::ios::end);
    EXPECT_EQ(';', input.get());
}

std::vector<Byte> TestGifWriterExtensions::payload(const GifFileType *gif, const std::string &identifier) const
{
    bool found{};
    std::vector<Byte> bytes;
    for (int i = 0; i < gif->ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{gif->ExtensionBlocks[i]};
        if (block.Function == APPLICATION_EXT_FUNC_CODE)
        {
            if (found)
            {
                break;
            }
            found = std::string{reinterpret_cast<const char *>(block.Bytes),
                        static_cast<std::size_t>(block.ByteCount)} == identifier;
        }
        else if (found)
        {
            bytes.insert(bytes.end(), block.Bytes, block.Bytes + block.ByteCount);
        }
    }
    return bytes;
}

TEST_F(TestGifWriterExtensions, savesMainRecordWithoutInapplicableOptionalRecords)
{
    ValueSaver saved_iterations{g_max_iterations, 0x123456L};
    ValueSaver saved_time{g_calc_time, 0x12345678L};
    ValueSaver saved_version{g_version, Version{1, 5, 2, 3, false}};
    ValueSaver saved_region{g_image_region, ImageRegion{{-2.5, -1.5}, {1.5, 1.5}, {-2.5, -1.5}}};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint001"});
    const std::vector<Byte> bytes{payload(gif.get(), "fractint001")};
    ASSERT_EQ(504, bytes.size());
    EXPECT_EQ((std::vector<Byte>{'F', 'r', 'a', 'c', 't', 'a', 'l', 0, 0xFF, 0x7F, 4, 0}),
        (std::vector<Byte>{bytes.begin(), bytes.begin() + 12}));
    EXPECT_EQ(
        (std::vector<Byte>{0, 0, 0, 0, 0, 0, 4, 0xC0}), (std::vector<Byte>{bytes.begin() + 12, bytes.begin() + 20}));
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_STREQ("Fractal", m_result.info.info_id);
    EXPECT_EQ(FRACTAL_INFO_VERSION, m_result.info.info_version);
    EXPECT_EQ(0x123456, m_result.info.iterations);
    EXPECT_EQ(32767, m_result.info.iterations_old);
    EXPECT_EQ(0x12345678, m_result.info.calc_time);
    EXPECT_EQ((Version{1, 5, 2, 3, false}), fractal_info_version(m_result.info));
    EXPECT_EQ(8, m_result.info.x_dots);
    EXPECT_EQ(6, m_result.info.y_dots);
    EXPECT_EQ(256, m_result.info.colors);
    EXPECT_EQ(8, g_file_x_dots);
    EXPECT_EQ(6, g_file_y_dots);
    EXPECT_EQ(256, g_file_colors);
    EXPECT_FALSE(m_result.resume.got_data);
    EXPECT_FALSE(m_result.formula.got_data);
    EXPECT_FALSE(m_result.ranges.got_data);
    EXPECT_FALSE(m_result.parameters.got_data);
    EXPECT_FALSE(m_result.evolution.got_data);
    EXPECT_FALSE(m_result.orbits.got_data);
}

class TestGifWriterResume : public TestGifWriterExtensions, public WithParamInterface<int>
{
};

TEST_P(TestGifWriterResume, savesOnlyUsedResumeBytesWithLegacySubBlocks)
{
    const int size{GetParam()};
    std::vector<Byte> expected(size);
    for (int i = 0; i < size; ++i)
    {
        expected[i] = static_cast<Byte>(i * 37 + 129);
    }
    g_resume_data = expected;
    g_resume_data.push_back(0x5A); // Allocated capacity beyond the used prefix is not saved.
    g_resume_len = size;
    g_calc_status = CalcStatus::RESUMABLE;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint002", "fractint001"});
    EXPECT_TRUE(has_gif_extension(gif.get(), "fractint002"));
    EXPECT_EQ(expected, get_resume_info(gif.get()));
    const int chunks{(size + 254) / 255};
    ASSERT_EQ(chunks + 4, gif->ExtensionBlockCount);
    for (int i = 0; i < chunks; ++i)
    {
        EXPECT_EQ(CONTINUE_EXT_FUNC_CODE, gif->ExtensionBlocks[i + 1].Function);
        EXPECT_EQ(std::min(255, size - i * 255), gif->ExtensionBlocks[i + 1].ByteCount);
    }
    EXPECT_EQ(521 + 15 + size + chunks, get_fractal_info(gif.get()).tot_extend_len);
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_EQ(static_cast<int>(CalcStatus::RESUMABLE), m_result.info.calc_status);
    EXPECT_TRUE(m_result.resume.got_data);
    EXPECT_EQ(size, m_result.resume.length);
    EXPECT_EQ(expected, m_result.resume.resume_data);
}

INSTANTIATE_TEST_SUITE_P(SubBlockSizes, TestGifWriterResume, Values(0, 1, 254, 255, 256, 510, 511, 4097));

struct GifResumeOmissionInput
{
    const char *name;
    CalcStatus status;
    std::vector<Byte> data;
    int length;
};

void PrintTo(const GifResumeOmissionInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestGifWriterResumeOmission : public TestGifWriterExtensions, public WithParamInterface<GifResumeOmissionInput>
{
protected:
    void SetUp() override;
};

void TestGifWriterResumeOmission::SetUp()
{
    TestGifWriterExtensions::SetUp();
    g_calc_status = GetParam().status;
    g_resume_data = GetParam().data;
    g_resume_len = GetParam().length;
}

TEST_P(TestGifWriterResumeOmission, omitsInapplicableResumeRecord)
{
    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint001"});
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_FALSE(m_result.resume.got_data);
}

INSTANTIATE_TEST_SUITE_P(ResumeOmissionInputs, TestGifWriterResumeOmission,
    Values(GifResumeOmissionInput{"CompletedImage", CalcStatus::COMPLETED, {0x12, 0x34}, 2},
        GifResumeOmissionInput{"EmptyStorage", CalcStatus::RESUMABLE, {}, 0}),
    [](const TestParamInfo<GifResumeOmissionInput> &info) { return info.param.name; });

class TestGifWriterName : public TestGifWriterExtensions, public WithParamInterface<FractalType>
{
};

TEST_P(TestGifWriterName, savesSelectedItemNameAndOnlyFormulaUsageFlags)
{
    g_fractal_type = GetParam();
    ValueSaver saved_formula_name{g_formula_name, std::string{"SavedFormula"}};
    ValueSaver saved_l_system_name{g_l_system_name, std::string{"SavedLSystem"}};
    ValueSaver saved_ifs_name{g_ifs_name, std::string{"SavedIFS"}};
    ValueSaver saved_ismand{g_frm_is_mandelbrot, false};
    g_formula.uses_p1 = true;
    g_formula.uses_p2 = false;
    g_formula.uses_p3 = true;
    g_formula.uses_p4 = false;
    g_formula.uses_p5 = true;
    g_formula.uses_ismand = true;
    const std::string name{g_fractal_type == FractalType::FORMULA ? "SavedFormula"
            : g_fractal_type == FractalType::L_SYSTEM             ? "SavedLSystem"
                                                                  : "SavedIFS"};
    const bool formula{g_fractal_type == FractalType::FORMULA};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint003", "fractint001"});
    FormulaInfo expected{};
    std::copy(name.begin(), name.end(), expected.form_name);
    expected.uses_p1 = formula;
    expected.uses_p3 = formula;
    expected.uses_ismand = formula;
    expected.uses_p5 = formula;
    EXPECT_EQ(expected, get_formula_info(gif.get()));
    const std::vector<Byte> bytes{payload(gif.get(), "fractint003")};
    ASSERT_EQ(66, bytes.size());
    EXPECT_EQ(name, (std::string{bytes.begin(), bytes.begin() + name.size()}));
    const Byte flag{static_cast<Byte>(formula)};
    EXPECT_EQ((std::vector<Byte>{flag, 0, 0, 0, flag, 0, flag, 0, 0, 0, 0, 0, flag, 0}),
        (std::vector<Byte>{bytes.begin() + 40, bytes.begin() + 54}));
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_TRUE(m_result.formula.got_data);
    EXPECT_EQ(66, m_result.formula.length);
    EXPECT_EQ(name, m_result.formula.form_name);
    EXPECT_EQ(formula, m_result.formula.uses_p1);
    EXPECT_EQ(formula, m_result.formula.uses_p3);
    EXPECT_EQ(formula, m_result.formula.uses_ismand);
    EXPECT_EQ(0, m_result.formula.uses_p2);
    EXPECT_EQ(0, m_result.formula.uses_p4);
    EXPECT_EQ(formula, m_result.formula.uses_p5);
    EXPECT_EQ(0, m_result.formula.ismand);
}

INSTANTIATE_TEST_SUITE_P(ItemTypes, TestGifWriterName,
    Values(FractalType::FORMULA, FractalType::L_SYSTEM, FractalType::IFS, FractalType::IFS_3D));

class TestGifWriterStoredName : public TestGifWriterExtensions, public WithParamInterface<std::string>
{
protected:
    void SetUp() override;

    ValueSaver<std::string> m_saved_name{g_l_system_name};
};

void TestGifWriterStoredName::SetUp()
{
    TestGifWriterExtensions::SetUp();
    g_fractal_type = FractalType::L_SYSTEM;
    g_l_system_name = GetParam();
}

TEST_P(TestGifWriterStoredName, storesTerminatedItemName)
{
    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint003", "fractint001"});
    const FormulaInfo info{get_formula_info(gif.get())};
    EXPECT_EQ(GetParam().substr(0, 39), info.form_name);
    EXPECT_EQ(0, info.form_name[39]);
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_TRUE(m_result.formula.got_data);
    EXPECT_EQ(GetParam().substr(0, 39), m_result.formula.form_name);
}

INSTANTIATE_TEST_SUITE_P(ItemNames, TestGifWriterStoredName, Values(std::string(60, 'A'), std::string{}),
    [](const TestParamInfo<std::string> &info) { return info.param.empty() ? "EmptyName" : "TruncatedName"; });

TEST_F(TestGifWriterExtensions, savesSignedIterationRangesInLittleEndianOrder)
{
    g_iteration_ranges = {-1, 0x1234, -32768, 32767};

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint004", "fractint001"});
    EXPECT_EQ((std::vector<Byte>{0xFF, 0xFF, 0x34, 0x12, 0, 0x80, 0xFF, 0x7F}), payload(gif.get(), "fractint004"));
    EXPECT_EQ(g_iteration_ranges, get_ranges_info(gif.get()));
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_TRUE(m_result.ranges.got_data);
    EXPECT_EQ(4, m_result.ranges.length);
    EXPECT_EQ(g_iteration_ranges, m_result.ranges.range_data);
}

TEST_F(TestGifWriterExtensions, omitsIterationRangesForThreeDimensionalDisplay)
{
    g_iteration_ranges = {16, 32};
    g_display_3d = Display3DMode::YES;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint001"});
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_FALSE(m_result.ranges.got_data);
}

TEST_F(TestGifWriterExtensions, preservesOpaqueArbitraryPrecisionParameters)
{
    std::vector<Byte> parameters(22 * (16 + 2));
    for (std::size_t i = 0; i < parameters.size(); ++i)
    {
        parameters[i] = static_cast<Byte>(i * 53 + 157);
    }
    ValueSaver saved_bf_length{g_bf_length, 16};
    ValueSaver saved_bn_length{g_bn_length, 12};
    ValueSaver saved_min{g_bf_x_min, parameters.data()};
    g_bf_math = BFMathType::BIG_FLT;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint005", "fractint001"});
    const std::vector<char> expected{parameters.begin(), parameters.end()};
    EXPECT_EQ(parameters, payload(gif.get(), "fractint005"));
    EXPECT_EQ(expected, get_extended_param_info(gif.get()));
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_EQ(static_cast<int>(BFMathType::BIG_FLT), m_result.info.bf_math);
    EXPECT_EQ(12, m_result.info.bf_length);
    EXPECT_TRUE(m_result.parameters.got_data);
    EXPECT_EQ(expected, m_result.parameters.apm_data);
}

class TestGifWriterEvolution : public TestGifWriterExtensions, public WithParamInterface<std::tuple<bool, bool>>
{
};

TEST_P(TestGifWriterEvolution, encodesOnlyTheSuppliedSnapshotForFilesAndStreams)
{
    const bool stream{std::get<0>(GetParam())};
    const bool present{std::get<1>(GetParam())};
    EvolutionInfo expected{};
    expected.evolving = 1;
    expected.image_grid_size = 3;
    expected.this_generation_random_seed = 0xABCD;
    expected.max_random_mutation = 0.25;
    expected.x_parameter_range = 4;
    expected.y_parameter_range = 3;
    expected.x_parameter_offset = -2;
    expected.y_parameter_offset = -1.5;
    expected.discrete_x_parameter_offset = -7;
    expected.discrete_y_parameter_offset = 9;
    expected.px = 1;
    expected.py = 2;
    expected.screen_x_offset = 2;
    expected.screen_y_offset = 1;
    expected.x_dots = 0xFFFF;
    expected.y_dots = 0x8000;
    expected.count = 17;
    expected.future[0] = -123;
    for (int i = 0; i < NUM_GENES; ++i)
    {
        expected.mutate[i] = static_cast<std::int16_t>(i % 2 == 0 ? -i : i + 7);
    }
    std::optional<EvolutionInfo> snapshot;
    if (present)
    {
        snapshot = expected;
    }
    std::ofstream output;
    std::unique_ptr<ImageSave> image;
    if (stream)
    {
        output.open(m_path, std::ios::binary);
        ASSERT_TRUE(output);
        image = std::make_unique<ImageSave>(output, m_path, snapshot);
    }
    else
    {
        image = std::make_unique<ImageSave>(m_path, snapshot);
    }
    if (snapshot)
    {
        snapshot->count = 99;
        snapshot->mutate[0] = 99;
        snapshot.reset();
    }
    while (!image->done())
    {
        image->iterate();
    }
    ASSERT_TRUE(image->succeeded()) << image->diagnostic();
    image.reset();
    if (output.is_open())
    {
        output.close();
    }

    const GifFilePtr gif{read_gif(m_path)};
    if (!present)
    {
        expect_layout(gif.get(), {"fractint001"});
        EXPECT_FALSE(has_gif_extension(gif.get(), "fractint006"));
        return;
    }
    expect_layout(gif.get(), {"fractint006", "fractint001"});
    EXPECT_EQ(expected, get_evolution_info(gif.get()));
    const std::vector<Byte> bytes{payload(gif.get(), "fractint006")};
    ASSERT_EQ(200, bytes.size());
    EXPECT_EQ((std::vector<Byte>{1, 0, 3, 0, 0xCD, 0xAB}), (std::vector<Byte>{bytes.begin(), bytes.begin() + 6}));
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_TRUE(m_result.evolution.got_data);
    EXPECT_EQ(200, m_result.evolution.length);
    EXPECT_EQ(0xABCD, m_result.evolution.this_generation_random_seed);
    EXPECT_EQ(17, m_result.evolution.e_count);
    EXPECT_EQ(-2.0, m_result.evolution.x_parameter_offset);
    EXPECT_EQ(-2, m_result.evolution.mutate[2]);
    EXPECT_EQ(65535, m_result.evolution.x_dots);
    EXPECT_EQ(32768, m_result.evolution.y_dots);
}

INSTANTIATE_TEST_SUITE_P(OutputsAndPresence, TestGifWriterEvolution, Combine(Bool(), Bool()));

class TestGifWriterOrbitSettings : public TestGifWriterExtensions
{
protected:
    void SetUp() override;

    ValueSaver<ImageRegion> m_saved_corner{g_orbit_corner};
    ValueSaver<bool> m_saved_keep{g_keep_screen_coords};
    ValueSaver<OrbitDrawMode> m_saved_draw{g_draw_mode};
};

void TestGifWriterOrbitSettings::SetUp()
{
    TestGifWriterExtensions::SetUp();
    g_orbit_corner = ImageRegion{{-2.5, -1.5}, {1.5, 1.5}, {-2.0, -1.0}};
    g_keep_screen_coords = true;
    g_draw_mode = OrbitDrawMode::LINE;
}

TEST_F(TestGifWriterOrbitSettings, serializesOrbitSettingsForOrbitPass)
{
    g_std_calc_mode = CalcMode::ORBIT;

    ASSERT_EQ(0, save());

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(), {"fractint007", "fractint001"});
    const OrbitsInfo info{get_orbits_info(gif.get())};
    EXPECT_EQ(-2.5, info.orbit_corner_min_x);
    EXPECT_EQ(1.5, info.orbit_corner_max_x);
    EXPECT_EQ(-1.5, info.orbit_corner_min_y);
    EXPECT_EQ(1.5, info.orbit_corner_max_y);
    EXPECT_EQ(-2.0, info.orbit_corner_3rd_x);
    EXPECT_EQ(-1.0, info.orbit_corner_3rd_y);
    EXPECT_EQ(1, info.keep_screen_coords);
    EXPECT_EQ('l', info.draw_mode);
    const std::vector<Byte> bytes{payload(gif.get(), "fractint007")};
    ASSERT_EQ(200, bytes.size());
    EXPECT_EQ((std::vector<Byte>{0, 0, 0, 0, 0, 0, 4, 0xC0}), (std::vector<Byte>{bytes.begin(), bytes.begin() + 8}));
    EXPECT_EQ((std::vector<Byte>{1, 0, 'l', 0}), (std::vector<Byte>{bytes.begin() + 48, bytes.begin() + 52}));
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_TRUE(m_result.orbits.got_data);
    EXPECT_EQ(200, m_result.orbits.length);
    EXPECT_EQ(-2.5, m_result.orbits.ox_min);
    EXPECT_EQ(-1.0, m_result.orbits.oy_3rd);
    EXPECT_EQ(1, m_result.orbits.keep_screen_coords);
    EXPECT_EQ('l', m_result.orbits.draw_mode);
}

TEST_F(TestGifWriterOrbitSettings, omitsOrbitSettingsForOtherPass)
{
    g_std_calc_mode = CalcMode::ONE_PASS;
    ASSERT_EQ(0, save());
    const GifFilePtr other_pass{read_gif(m_path)};
    expect_layout(other_pass.get(), {"fractint001"});
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_FALSE(m_result.orbits.got_data);
}

TEST_F(TestGifWriterExtensions, preservesCompleteRecordOrderingAndTotalEncodedLength)
{
    g_fractal_type = FractalType::FORMULA;
    ValueSaver saved_name{g_formula_name, std::string{"AllRecords"}};
    g_resume_data = {0x12, 0x34, 0x56};
    g_resume_len = 2;
    g_calc_status = CalcStatus::RESUMABLE;
    g_iteration_ranges = {16, 32};
    std::vector<Byte> parameters(22 * (16 + 2), 0xA5);
    ValueSaver saved_bf_length{g_bf_length, 16};
    ValueSaver saved_bn_length{g_bn_length, 12};
    ValueSaver saved_min{g_bf_x_min, parameters.data()};
    g_bf_math = BFMathType::BIG_FLT;
    const std::optional<EvolutionInfo> evolution{EvolutionInfo{}};
    g_std_calc_mode = CalcMode::ORBIT;

    ImageSave image{m_path, evolution};
    while (!image.done())
    {
        image.iterate();
    }
    ASSERT_TRUE(image.succeeded()) << image.diagnostic();

    const GifFilePtr gif{read_gif(m_path)};
    expect_layout(gif.get(),
        {"fractint002", "fractint003", "fractint004", "fractint005", "fractint006", "fractint007", "fractint001"});
    ASSERT_FALSE(m_result.read(m_path));
    EXPECT_TRUE(m_result.resume.got_data);
    EXPECT_TRUE(m_result.formula.got_data);
    EXPECT_TRUE(m_result.ranges.got_data);
    EXPECT_TRUE(m_result.parameters.got_data);
    EXPECT_TRUE(m_result.evolution.got_data);
    EXPECT_TRUE(m_result.orbits.got_data);
    EXPECT_EQ((std::vector<Byte>{0x12, 0x34}), m_result.resume.resume_data);
    EXPECT_EQ("AllRecords", std::string{m_result.formula.form_name});
    EXPECT_EQ((std::vector<int>{16, 32}), m_result.ranges.range_data);
    EXPECT_EQ((std::vector<char>{parameters.begin(), parameters.end()}), m_result.parameters.apm_data);
}

} // namespace id::test
