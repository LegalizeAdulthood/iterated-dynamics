// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/slideshw.h>

#include <ui/id_keys.h>

#include <gtest/gtest.h>

using namespace id::ui;

namespace id::test
{

TEST(TestSlideShow, controlKeyCodeUsesCaretSyntax)
{
    EXPECT_EQ(ID_KEY_CTL_S, get_slide_show_key_code("^S"));
    EXPECT_EQ(ID_KEY_CTL_S, get_slide_show_key_code("^s"));
    EXPECT_EQ(ID_KEY_CTL_BACKSLASH, get_slide_show_key_code("^\\"));
}

TEST(TestSlideShow, invalidCaretSyntaxIsNotAKeyCode)
{
    EXPECT_EQ(-1, get_slide_show_key_code("^"));
    EXPECT_EQ(-1, get_slide_show_key_code("^SS"));
    EXPECT_EQ(-1, get_slide_show_key_code("^1"));
}

TEST(TestSlideShow, namedKeyCodeStillWorks)
{
    EXPECT_EQ(ID_KEY_ENTER, get_slide_show_key_code("ENTER"));
}

} // namespace id::test
