//
// Copyright 2026 Richard Thomson
//
#include <algos/string_algorithms.h>

#include <gtest/gtest.h>

#include <ostream>
#include <string>
#include <vector>

using namespace id::algos;

namespace id::test
{

TEST(TestStringAlgorithms, asciiToLowerCopy)
{
    EXPECT_EQ("abc 123 !?", ascii_to_lower_copy("AbC 123 !?"));
}

TEST(TestStringAlgorithms, asciiToUpperCopy)
{
    EXPECT_EQ("ABC 123 !?", ascii_to_upper_copy("aBc 123 !?"));
}

struct ReplaceAllInput
{
    const char *name;
    const char *text;
    const char *old_text;
    const char *new_text;
    const char *expected;
};

void PrintTo(const ReplaceAllInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestReplaceAllInputs : public testing::TestWithParam<ReplaceAllInput>
{
};

TEST_P(TestReplaceAllInputs, replacesMatchingText)
{
    std::string text{GetParam().text};

    replace_all(text, GetParam().old_text, GetParam().new_text);

    EXPECT_EQ(GetParam().expected, text);
}

INSTANTIATE_TEST_SUITE_P(Text, TestReplaceAllInputs,
    testing::Values(ReplaceAllInput{"Repeated", "one two one two", "one", "three", "three two three two"},
        ReplaceAllInput{"NoMatches", "one two", "three", "four", "one two"},
        ReplaceAllInput{"EmptyOldText", "one two", "", "x", "one two"}),
    [](const testing::TestParamInfo<ReplaceAllInput> &info) { return info.param.name; });

struct SplitInput
{
    const char *name;
    const char *text;
    char delimiter;
    std::vector<std::string> expected;
};

void PrintTo(const SplitInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSplitInputs : public testing::TestWithParam<SplitInput>
{
};

TEST_P(TestSplitInputs, splitsAtDelimiter)
{
    EXPECT_EQ(GetParam().expected, split(GetParam().text, GetParam().delimiter));
}

INSTANTIATE_TEST_SUITE_P(Text, TestSplitInputs,
    testing::Values(SplitInput{"Empty", "", ',', {""}}, SplitInput{"NoDelimiters", "abc", ',', {"abc"}},
        SplitInput{"EmptyFields", ",a,,b,", ',', {"", "a", "", "b", ""}}),
    [](const testing::TestParamInfo<SplitInput> &info) { return info.param.name; });

struct SplitAnyInput
{
    const char *name;
    const char *text;
    const char *delimiters;
    std::vector<std::string> expected;
};

void PrintTo(const SplitAnyInput &input, std::ostream *out)
{
    *out << input.name;
}

class TestSplitAnyInputs : public testing::TestWithParam<SplitAnyInput>
{
};

TEST_P(TestSplitAnyInputs, splitsAtAnyDelimiter)
{
    EXPECT_EQ(GetParam().expected, split_any(GetParam().text, GetParam().delimiters));
}

INSTANTIATE_TEST_SUITE_P(Text, TestSplitAnyInputs,
    testing::Values(SplitAnyInput{"NoDelimiters", "abc", ",;", {"abc"}},
        SplitAnyInput{"EmptyFields", ",a;:b,", ",;:", {"", "a", "", "b", ""}},
        SplitAnyInput{"EmptyDelimiterSet", "a,b", "", {"a,b"}}),
    [](const testing::TestParamInfo<SplitAnyInput> &info) { return info.param.name; });

} // namespace id::test
