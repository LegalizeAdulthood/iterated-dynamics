// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <ui/id_keys.h>

#include <gtest/gtest.h>

#include <array>

using namespace id::ui;

namespace id::test
{

TEST(TestIdKeys, lowerCaseConvertsUppercaseCommands)
{
    for (int letter = 0; letter < 26; ++letter)
    {
        EXPECT_EQ('a' + letter, key_to_lower('A' + letter));
    }
}

TEST(TestIdKeys, lowerCasePreservesOtherKeys)
{
    constexpr std::array<int, 14> keys{'a', 'z', '0', '@', ID_KEY_ESC, 0, -1, 128, 255, 256, ID_KEY_F6, ID_KEY_INSERT,
        ID_KEY_CTL_INSERT, ID_KEY_FAKE_AUTOSAVE};
    for (const int key : keys)
    {
        EXPECT_EQ(key, key_to_lower(key)) << key;
    }
}

} // namespace id::test
