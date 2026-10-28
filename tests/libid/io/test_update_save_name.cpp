// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/update_save_name.h>

#include <gtest/gtest.h>

#include <string>
#include <ostream>

using namespace id::io;

namespace id::test
{

struct SaveNameInput
{
    const char *name;
    const char *requested;
    const char *expected;
};

void PrintTo(const SaveNameInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestUpdateSaveName : public testing::TestWithParam<SaveNameInput>
{
};

TEST_P(TestUpdateSaveName, advancesNumericFilenameSuffix)
{
    std::string filename{GetParam().requested};

    update_save_name(filename);

    EXPECT_EQ(GetParam().expected, filename);
}

INSTANTIATE_TEST_SUITE_P(FilenameInputs, TestUpdateSaveName,
    testing::Values(SaveNameInput{"ZeroPadded", "fract0001.gif", "fract0002.gif"},
        SaveNameInput{"NoNumericSuffix", "myfract.gif", "myfract2.gif"},
        SaveNameInput{"SingleDigit", "myfract1.gif", "myfract2.gif"},
        SaveNameInput{"CarryIntoSecondDigit", "myfract9.gif", "myfract10.gif"}),
    [](const testing::TestParamInfo<SaveNameInput> &info) { return info.param.name; });

TEST(TestNextSaveName, basic)
{
    EXPECT_EQ("fract0002.gif", next_save_name("fract0001.gif"));
}

} // namespace id::test
