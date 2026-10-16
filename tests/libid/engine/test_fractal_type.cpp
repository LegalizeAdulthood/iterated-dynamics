// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2026 Richard Thomson.
//
#include <engine/fractal_type.h>

#include <engine/cmdfiles.h>
#include <fractals/fractalp.h>
#include <io/gif_extensions.h>
#include <io/loadfile.h>
#include <misc/Driver.h>
#include <misc/ValueSaver.h>
#include <misc/version.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <ostream>
#include <tuple>

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace testing;

namespace id::test
{
namespace
{

struct SelectionVersion
{
    Version value;
};

void PrintTo(const SelectionVersion &value, std::ostream *str)
{
    *str << to_par_string(value.value);
}

enum class SelectionVersionSource
{
    PAR_RESET,
    GIF_METADATA
};

void PrintTo(const SelectionVersionSource value, std::ostream *str)
{
    *str << (value == SelectionVersionSource::PAR_RESET ? "PAR reset" : "GIF metadata");
}

class TestFractalTypeSelection
    : public Test,
      public WithParamInterface<std::tuple<SelectionVersion, FractalType, FractalType, SelectionVersionSource>>
{
protected:
    void SetUp() override;

private:
    ValueSaver<Version> m_saved_version{g_version};
    ValueSaver<Version> m_saved_file_version{g_file_version};
    ValueSaver<FractalType> m_saved_type{g_fractal_type};
    ValueSaver<FractalSpecific *> m_saved_specific{g_cur_fractal_specific};
    ValueSaver<Driver *> m_saved_driver{g_driver, nullptr};
};

void TestFractalTypeSelection::SetUp()
{
    const auto &[version, previous, candidate, source] = GetParam();
    if (source == SelectionVersionSource::PAR_RESET)
    {
        ASSERT_EQ(CmdArgFlags::FRACTAL_PARAM | CmdArgFlags::RESET,
            cmd_arg("reset=" + to_par_string(version.value), CmdFile::AT_AFTER_STARTUP));
    }
    else
    {
        FractalInfo info{};
        info.info_version = FRACTAL_INFO_VERSION;
        if (version.value.legacy)
        {
            info.release = static_cast<std::int16_t>(100 * version.value.major + version.value.minor);
        }
        else
        {
            info.version_major = version.value.major;
            info.version_minor = version.value.minor;
            info.version_patch = version.value.patch;
            info.version_tweak = version.value.tweak;
        }
        g_file_version = fractal_info_version(info);
        g_version = g_file_version;
    }
    set_fractal_type(candidate);
}

TEST_P(TestFractalTypeSelection, acceptanceKeepsCandidateAndResetsBothVersions)
{
    const auto &[version, previous, candidate, source] = GetParam();
    FractalSpecific *const candidate_specific = g_cur_fractal_specific;

    complete_fractal_type_selection(previous, true);

    EXPECT_EQ(candidate, g_fractal_type);
    EXPECT_EQ(candidate_specific, g_cur_fractal_specific);
    EXPECT_EQ(current_id_version(), g_version);
    EXPECT_EQ(current_id_version(), g_file_version);
}

TEST_P(TestFractalTypeSelection, cancellationRestoresPreviousTypeAndPreservesBothVersions)
{
    const auto &[version, previous, candidate, source] = GetParam();

    complete_fractal_type_selection(previous, false);

    EXPECT_EQ(previous, g_fractal_type);
    EXPECT_EQ(get_fractal_specific(previous), g_cur_fractal_specific);
    EXPECT_EQ(version.value, g_version);
    EXPECT_EQ(version.value, g_file_version);
}

INSTANTIATE_TEST_SUITE_P(LoadedVersions, TestFractalTypeSelection,
    Combine(Values(SelectionVersion{parse_legacy_version(1730)}, SelectionVersion{parse_legacy_version(2004)},
                SelectionVersion{id_version(1, 0)}, SelectionVersion{current_id_version()}),
        Values(FractalType::MANDEL, FractalType::LYAPUNOV, FractalType::POPCORN),
        Values(FractalType::MANDEL, FractalType::LYAPUNOV, FractalType::POPCORN),
        Values(SelectionVersionSource::PAR_RESET, SelectionVersionSource::GIF_METADATA)));

} // namespace
} // namespace id::test
