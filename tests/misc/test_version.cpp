// SPDX-License-Identifier: GPL-3.0-only
//
#include <misc/version.h>

#include <config/port_config.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

TEST(TestVersion, release)
{
    EXPECT_EQ(ID_VERSION_MAJOR * 100 + ID_VERSION_MINOR, g_release);
}

TEST(TestVersion, patchLevel)
{
    EXPECT_EQ(ID_VERSION_PATCH, g_patch_level);
}

TEST(TestVersion, parStringMajor)
{
    ValueSaver saved_version{g_version, Version{5, 0, 0, 0, false}};

    EXPECT_EQ("5/0", to_par_string(g_version));
}

TEST(TestVersion, parStringMajorMinor)
{
    ValueSaver saved_version{g_version, Version{5, 6, 0, 0, false}};

    EXPECT_EQ("5/6", to_par_string(g_version));
}

TEST(TestVersion, parStringMajorMinorPatch)
{
    ValueSaver saved_version{g_version, Version{5, 6, 7, 0, false}};

    EXPECT_EQ("5/6/7", to_par_string(g_version));
}

TEST(TestVersion, parStringMajorMinorTweak)
{
    ValueSaver saved_version{g_version, Version{5, 6, 0, 7, false}};

    EXPECT_EQ("5/6/0/7", to_par_string(g_version));
}

TEST(TestVersion, parStringMajorMinorPatchTweak)
{
    ValueSaver saved_version{g_version, Version{5, 6, 7, 8, false}};

    EXPECT_EQ("5/6/7/8", to_par_string(g_version));
}
