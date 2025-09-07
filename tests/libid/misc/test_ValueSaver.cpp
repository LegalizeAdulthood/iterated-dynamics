// SPDX-License-Identifier: GPL-3.0-only
//
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

using namespace id::misc;

TEST(TestValueSaver, setsNewValue)
{
    bool data{true};

    ValueSaver saved_data(data, false);

    EXPECT_FALSE(data);
}

TEST(TestValueSaver, restoresOriginalValue)
{
    bool data{true};
    {
        ValueSaver saved_data(data, false);
    }

    EXPECT_TRUE(data);
}
