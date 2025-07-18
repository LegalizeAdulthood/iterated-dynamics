// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/update_save_name.h>

#include <gtest/gtest.h>

#include <string>

TEST(TestUpdateSaveName, basic)
{
    std::string filename("fract0001.gif");

    update_save_name(filename);

    EXPECT_EQ("fract0002.gif", filename);
}

TEST(TestUpdateSaveName, noNumbersAtEnd)
{
    std::string filename("myfract.gif");

    update_save_name(filename);

    EXPECT_EQ("myfract2.gif", filename);
}

TEST(TestUpdateSaveName, singleNumberAtEnd)
{
    std::string filename("myfract1.gif");

    update_save_name(filename);

    EXPECT_EQ("myfract2.gif", filename);
}

TEST(TestUpdateSaveName, nineAtEnd)
{
    std::string filename("myfract9.gif");

    update_save_name(filename);

    EXPECT_EQ("myfract10.gif", filename);
}

TEST(TestNextSaveName, basic)
{
    EXPECT_EQ("fract0002.gif", next_save_name("fract0001.gif"));
}
