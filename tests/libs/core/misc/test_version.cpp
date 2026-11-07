// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <misc/version.h>

#include <config/port_config.h>
#include <io/loadfile.h>
#include <misc/ValueSaver.h>

#include <gtest/gtest.h>

#include <iostream>

using namespace id::config;
using namespace id::io;
using namespace id::misc;

namespace id::test
{

// In GIF extension blocks, we store the version components as std::uint8_t
TEST(TestVersion, versionComponentsFitInUInt8)
{
    EXPECT_GT(256, ID_VERSION_MAJOR);
    EXPECT_GT(256, ID_VERSION_MINOR);
    EXPECT_GT(256, ID_VERSION_PATCH);
    EXPECT_GT(256, ID_VERSION_TWEAK);
}

TEST(TestVersion, currentIdVersion)
{
    const Version version{current_id_version()};

    EXPECT_EQ(ID_VERSION_MAJOR, version.major);
    EXPECT_EQ(ID_VERSION_MINOR, version.minor);
    EXPECT_EQ(ID_VERSION_PATCH, version.patch);
    EXPECT_EQ(ID_VERSION_TWEAK, version.tweak);
    EXPECT_FALSE(version.legacy);
}

TEST(TestVersion, resetVersionToCurrent)
{
    ValueSaver saved_version{g_version};
    ValueSaver saved_file_version{g_file_version};
    g_version = parse_legacy_version(1730);
    g_file_version = parse_legacy_version(1730);

    reset_version_to_current();

    EXPECT_EQ(current_id_version(), g_version);
    EXPECT_EQ(current_id_version(), g_file_version);
    EXPECT_FALSE(g_version.legacy);
    EXPECT_FALSE(g_file_version.legacy);
}

struct VersionTextInput
{
    const char *name;
    Version version;
    const char *expected;
};

void PrintTo(const VersionTextInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestVersionStringFormat : public testing::TestWithParam<VersionTextInput>
{
};

TEST_P(TestVersionStringFormat, formatsVersion)
{
    EXPECT_EQ(GetParam().expected, to_string(GetParam().version));
}

INSTANTIATE_TEST_SUITE_P(Components, TestVersionStringFormat,
    testing::Values(VersionTextInput{"legacyVersionToString", {20, 4, 0, 0, true}, "20.04"},
        VersionTextInput{"toStringMajor", {5, 0, 0, 0, false}, "5.0"},
        VersionTextInput{"toStringMajorMinor", {5, 6, 0, 0, false}, "5.6"},
        VersionTextInput{"toStringMajorMinorPatch", {5, 6, 7, 0, false}, "5.6.7"},
        VersionTextInput{"toStringMajorPatch", {5, 0, 7, 0, false}, "5.0.7"},
        VersionTextInput{"toStringMajorTweak", {5, 0, 0, 8, false}, "5.0.0.8"},
        VersionTextInput{"toStringMajorMinorTweak", {5, 6, 0, 8, false}, "5.6.0.8"},
        VersionTextInput{"currentVersionStringMajorMinorPatchTweak", {3, 4, 5, 6, false}, "3.4.5.6"}),
    [](const testing::TestParamInfo<VersionTextInput> &info) { return info.param.name; });

class TestVersionParameterFormat : public testing::TestWithParam<VersionTextInput>
{
protected:
    ValueSaver<Version> m_saved_version{g_version, GetParam().version};
};

TEST_P(TestVersionParameterFormat, formatsParameterVersion)
{
    EXPECT_EQ(GetParam().expected, to_par_string(g_version));
}

INSTANTIATE_TEST_SUITE_P(Components, TestVersionParameterFormat,
    testing::Values(VersionTextInput{"Major", {5, 0, 0, 0, false}, "5/0"},
        VersionTextInput{"Legacy", parse_legacy_version(1730), "1730"},
        VersionTextInput{"MajorMinor", {5, 6, 0, 0, false}, "5/6"},
        VersionTextInput{"MajorMinorPatch", {5, 6, 7, 0, false}, "5/6/7"},
        VersionTextInput{"MajorMinorTweak", {5, 6, 0, 7, false}, "5/6/0/7"},
        VersionTextInput{"AllFields", {5, 6, 7, 8, false}, "5/6/7/8"}),
    [](const testing::TestParamInfo<VersionTextInput> &info) { return info.param.name; });

struct LegacyVersionParseInput
{
    const char *name;
    int encoded;
    Version expected;
};

void PrintTo(const LegacyVersionParseInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestLegacyVersionParsing : public testing::TestWithParam<LegacyVersionParseInput>
{
};

TEST_P(TestLegacyVersionParsing, decodesVersionComponents)
{
    const Version result{parse_legacy_version(GetParam().encoded)};

    EXPECT_EQ(GetParam().expected, result);
}

INSTANTIATE_TEST_SUITE_P(Encoded, TestLegacyVersionParsing,
    testing::Values(LegacyVersionParseInput{"Major", 100, {1, 0, 0, 0, true}},
        LegacyVersionParseInput{"MajorMinor", 1401, {14, 1, 0, 0, true}}),
    [](const testing::TestParamInfo<LegacyVersionParseInput> &info) { return info.param.name; });

inline std::ostream &operator<<(std::ostream &str, const Version &value)
{
    return str << (value.legacy ? "legacy{" : "id{") << to_string(value) << '}';
}

struct OrderedVersionPairInput
{
    const char *name;
    Version left;
    Version right;
};

void PrintTo(const OrderedVersionPairInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestVersionOrdering : public testing::TestWithParam<OrderedVersionPairInput>
{
};

TEST_P(TestVersionOrdering, comparesVersionOrder)
{
    EXPECT_LT(GetParam().left, GetParam().right);
}

INSTANTIATE_TEST_SUITE_P(Components, TestVersionOrdering,
    testing::Values(OrderedVersionPairInput{"LegacyBeforeId", parse_legacy_version(1401), {1, 0, 0, 0, false}},
        OrderedVersionPairInput{"LegacyMajor", parse_legacy_version(1301), parse_legacy_version(1401)},
        OrderedVersionPairInput{"LegacyMinor", parse_legacy_version(1301), parse_legacy_version(1306)},
        OrderedVersionPairInput{"Patch", {1, 1, 0, 0, false}, {1, 1, 1, 0, false}},
        OrderedVersionPairInput{"Tweak", {1, 1, 1, 0, false}, {1, 1, 1, 1, false}}),
    [](const testing::TestParamInfo<OrderedVersionPairInput> &info) { return info.param.name; });

TEST(TestVersion, idVersionGreaterThanLegacyVersion)
{
    const Version lhs{1, 0, 0, 0, false};
    const Version rhs{parse_legacy_version(1401)};

    EXPECT_GT(lhs, rhs);
}

TEST(TestVersion, versionGreaterThanLegacyInteger)
{
    const Version lhs{parse_legacy_version(2003)};

    EXPECT_GT(lhs, 2002);
}

struct EqualVersionPairInput
{
    const char *name;
    Version left;
    Version right;
    bool equal;
};

void PrintTo(const EqualVersionPairInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestVersionEquality : public testing::TestWithParam<EqualVersionPairInput>
{
};

TEST_P(TestVersionEquality, comparesVersionEquality)
{
    EXPECT_EQ(GetParam().equal, GetParam().left == GetParam().right);
    EXPECT_EQ(!GetParam().equal, GetParam().left != GetParam().right);
}

INSTANTIATE_TEST_SUITE_P(Components, TestVersionEquality,
    testing::Values(EqualVersionPairInput{"LegacyLeft", {1, 0, 0, 0, true}, {1, 0, 0, 0, false}, false},
        EqualVersionPairInput{"LegacyRight", {1, 0, 0, 0, false}, {1, 0, 0, 0, true}, false},
        EqualVersionPairInput{"Same", {1, 0, 0, 0, false}, {1, 0, 0, 0, false}, true},
        EqualVersionPairInput{"Major", {1, 0, 0, 0, false}, {2, 0, 0, 0, false}, false},
        EqualVersionPairInput{"Minor", {1, 0, 0, 0, false}, {1, 1, 0, 0, false}, false},
        EqualVersionPairInput{"Patch", {1, 1, 0, 0, false}, {1, 1, 1, 0, false}, false},
        EqualVersionPairInput{"Tweak", {1, 1, 1, 0, false}, {1, 1, 1, 1, false}, false}),
    [](const testing::TestParamInfo<EqualVersionPairInput> &info) { return info.param.name; });

class TestVersionDisplayFormat : public testing::TestWithParam<VersionTextInput>
{
};

TEST_P(TestVersionDisplayFormat, formatsDisplayVersion)
{
    EXPECT_EQ(GetParam().expected, to_display_string(GetParam().version));
}

INSTANTIATE_TEST_SUITE_P(Components, TestVersionDisplayFormat,
    testing::Values(VersionTextInput{"Legacy", parse_legacy_version(2004), "FRACTINT v20.04"},
        VersionTextInput{"AllFields", {1, 2, 3, 4, false}, "Id v1.2.3.4"}),
    [](const testing::TestParamInfo<VersionTextInput> &info) { return info.param.name; });

} // namespace id::test
