// SPDX-License-Identifier: GPL-3.0-only
//
#include <ValueSaver.h>

#include <gtest/gtest.h>

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
