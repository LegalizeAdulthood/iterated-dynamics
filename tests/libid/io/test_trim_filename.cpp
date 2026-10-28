// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/trim_filename.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <optional>
#include <ostream>
#include <string>

using namespace id::io;

namespace id::test
{

struct TrimFilenameInput
{
    const char *name;
    const char *filename;
    int limit;
    const char *expected;
    std::optional<std::size_t> exact_size;
};

void PrintTo(const TrimFilenameInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestTrimFilename : public testing::TestWithParam<TrimFilenameInput>
{
};

TEST_P(TestTrimFilename, fitsFilenameWithinRequestedSize)
{
    const TrimFilenameInput &input{GetParam()};

    const std::string result{trim_filename(std::string{input.filename}, input.limit)};

    EXPECT_EQ(input.expected, result);
    EXPECT_LE(result.size(), static_cast<std::size_t>(input.limit));
    if (input.exact_size)
    {
        EXPECT_EQ(*input.exact_size, result.size());
    }
}

#if defined(WIN32)
constexpr TrimFilenameInput DIRECTORY_INPUT{"IntermediateDirectoriesWindows",
    R"(C:\code\iterated-dynamics\build-default\install\pars\foo.par)", 44, R"(C:\code\...\install\pars\foo.par)", {}};
#else
constexpr TrimFilenameInput DIRECTORY_INPUT{"IntermediateDirectoriesUnix",
    R"(/home/users/l/legalize/iterated-dynamics/build-default/install/pars/foo.par)", 40,
    R"(/home/.../install/pars/foo.par)", {}};
#endif

INSTANTIATE_TEST_SUITE_P(FilenameInputs, TestTrimFilename,
    testing::Values(
        TrimFilenameInput{"AlreadyFits", R"(C:\iterated-dynamics\foo.par)", 80, R"(C:\iterated-dynamics\foo.par)", {}},
        TrimFilenameInput{"LongFilename", "abcdefghijklmnopqrstuvwxyz.gif", 16, "abcdef...xyz.gif", 16},
        DIRECTORY_INPUT),
    [](const testing::TestParamInfo<TrimFilenameInput> &info) { return info.param.name; });

} // namespace id::test
