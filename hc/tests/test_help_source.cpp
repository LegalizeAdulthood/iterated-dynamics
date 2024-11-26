// SPDX-License-Identifier: GPL-3.0-only
//
#include <HelpSource.h>

#include <gtest/gtest.h>

using namespace hc;

TEST(TestLabels, indexLabelLeast)
{
    const Label idx{INDEX_LABEL};
    const Label other{"HELP_OTHER"};

    EXPECT_TRUE(idx < other);
    EXPECT_FALSE(other < idx);
}

TEST(TestLabels, labelsSortByName)
{
    const Label smaller{"aardvark"};
    const Label bigger{"zebra"};

    EXPECT_TRUE(smaller < bigger);
    EXPECT_FALSE(bigger < smaller);
}
