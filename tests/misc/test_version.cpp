// SPDX-License-Identifier: GPL-3.0-only
//
#include <misc/version.h>

#include <config/port_config.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <iostream>

TEST(TestVersion, release)
{
    EXPECT_EQ(ID_VERSION_MAJOR * 100 + ID_VERSION_MINOR, g_release);
}

TEST(TestVersion, patchLevel)
{
    EXPECT_EQ(ID_VERSION_PATCH, g_patch_level);
}

// In GIF extension blocks, we store the version components as std::uint8_t
TEST(TestVersion, versionComponentsFitInUInt8)
{
    EXPECT_GT(256, ID_VERSION_MAJOR);
    EXPECT_GT(256, ID_VERSION_MINOR);
    EXPECT_GT(256, ID_VERSION_PATCH);
    EXPECT_GT(256, ID_VERSION_TWEAK);
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

TEST(TestVersion, parseLegacyVersionMajor)
{
    Version result{parse_legacy_version(100)};

    EXPECT_TRUE(result.legacy);
    EXPECT_EQ(1, result.major);
    EXPECT_EQ(0, result.minor);
    EXPECT_EQ(0, result.patch);
    EXPECT_EQ(0, result.tweak);
}

TEST(TestVersion, parseLegacyVersionMajorMinor)
{
    Version result{parse_legacy_version(1401)};

    EXPECT_TRUE(result.legacy);
    EXPECT_EQ(14, result.major);
    EXPECT_EQ(1, result.minor);
    EXPECT_EQ(0, result.patch);
    EXPECT_EQ(0, result.tweak);
}

inline std::ostream &operator<<(std::ostream &str, const Version &value)
{
    if (value.legacy)
    {
        return str << "legacy{" << value.major << '.' << value.minor << '}';
    }
    return str << "id{" << value.major << '.' << value.minor //
               << '.' << value.patch << '.' << value.tweak << '}';
}

TEST(TestVersion, legacyVersionLessThanIdVersion)
{
    const Version lhs{parse_legacy_version(1401)};
    const Version rhs{1, 0, 0, 0, false};

    EXPECT_LT(lhs, rhs);
}

TEST(TestVersion, idVersionGreaterThanLegacyVersion)
{
    const Version lhs{1, 0, 0, 0, false};
    const Version rhs{parse_legacy_version(1401)};

    EXPECT_GT(lhs, rhs);
}

TEST(TestVersion, legacyLesserMajorVersion)
{
    const Version lhs{parse_legacy_version(1301)};
    const Version rhs{parse_legacy_version(1401)};

    EXPECT_LT(lhs, rhs);
}

TEST(TestVersion, legacyLesserMinorVersion)
{
    const Version lhs{parse_legacy_version(1301)};
    const Version rhs{parse_legacy_version(1306)};

    EXPECT_LT(lhs, rhs);
}

TEST(TestVersion, lesserPatchVersion)
{
    const Version lhs{1, 1, 0, 0, false};
    const Version rhs{1, 1, 1, 0, false};

    EXPECT_LT(lhs, rhs);
}

TEST(TestVersion, lesserTweakVersion)
{
    const Version lhs{1, 1, 1, 0, false};
    const Version rhs{1, 1, 1, 1, false};

    EXPECT_LT(lhs, rhs);
}

TEST(TestVersion, legacyVersionNotEqualIdVersion)
{
    const Version lhs{1, 0, 0, 0, true};
    const Version rhs{1, 0, 0, 0, false};

    EXPECT_NE(lhs, rhs);
}

TEST(TestVersion, legacyVersionToDisplayString)
{
    const Version versino{parse_legacy_version(2004)};

    const std::string result{to_display_string(versino)};

    EXPECT_EQ("FRACTINT v20.04", result);
}

TEST(TestVersion, idVersionToDisplayString)
{
    const Version version{Version{1, 2, 3, 4, false}};

    const std::string result{to_display_string(version)};

    EXPECT_EQ("Id v1.2.3.4", result);
}

TEST(TestVersion, idVersionMajorMinorToDisplayString)
{
    const Version version{Version{1, 0, 0, 0, false}};

    const std::string result{to_display_string(version)};

    EXPECT_EQ("Id v1.0", result);
}

TEST(TestVersion, idVersionMajorPatchToDisplayString)
{
    const Version version{Version{1, 0, 1, 0, false}};

    const std::string result{to_display_string(version)};

    EXPECT_EQ("Id v1.0.1", result);
}

TEST(TestVersion, idVersionMajorTweakToDisplayString)
{
    const Version version{Version{1, 0, 0, 1, false}};

    const std::string result{to_display_string(version)};

    EXPECT_EQ("Id v1.0.0.1", result);
}
