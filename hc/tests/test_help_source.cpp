// SPDX-License-Identifier: GPL-3.0-only
//
#include <help_source.h>

#include <gtest/gtest.h>

using namespace hc;

TEST(TestLabels, indexLabelLeast)
{
    const LABEL idx{INDEX_LABEL};
    const LABEL other{"HELP_OTHER"};

    EXPECT_TRUE(idx < other);
    EXPECT_FALSE(other < idx);
}

TEST(TestLabels, labelsSortByName)
{
    const LABEL smaller{"aardvark"};
    const LABEL bigger{"zebra"};

    EXPECT_TRUE(smaller < bigger);
    EXPECT_FALSE(bigger < smaller);
}
