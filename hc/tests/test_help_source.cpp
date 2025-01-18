// SPDX-License-Identifier: GPL-3.0-only
//
#include <HelpSource.h>

#include <gtest/gtest.h>

using namespace hc;

TEST(TestLabels, indexLabelLeast)
{
    const Label idx{INDEX_LABEL, 0, 0, 0};
    const Label other{"HELP_OTHER", 0, 0, 0};

    EXPECT_TRUE(idx < other);
    EXPECT_FALSE(other < idx);
}

TEST(TestLabels, labelsSortByName)
{
    const Label smaller{"aardvark", 0, 0, 0};
    const Label bigger{"zebra", 0, 0, 0};

    EXPECT_TRUE(smaller < bigger);
    EXPECT_FALSE(bigger < smaller);
}
