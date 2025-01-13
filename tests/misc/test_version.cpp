// SPDX-License-Identifier: GPL-3.0-only
//
#include <misc/version.h>

#include <config/port_config.h>

#include <gtest/gtest.h>

TEST(TestVersion, release)
{
    EXPECT_EQ(ID_VERSION_MAJOR * 100 + ID_VERSION_MINOR, g_release);
}

TEST(TestVersion, patchLevel)
{
    EXPECT_EQ(ID_VERSION_PATCH, g_patch_level);
}
