//
// Copyright 2026 Richard Thomson
//
#include <algos/string_algorithms.h>

#include <gtest/gtest.h>

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

TEST(TestStringAlgorithms, replaceAllRepeatedOccurrences)
{
    std::string text{"one two one two"};

    replace_all(text, "one", "three");

    EXPECT_EQ("three two three two", text);
}

TEST(TestStringAlgorithms, replaceAllNoMatches)
{
    std::string text{"one two"};

    replace_all(text, "three", "four");

    EXPECT_EQ("one two", text);
}

TEST(TestStringAlgorithms, replaceAllEmptyOldTextDoesNothing)
{
    std::string text{"one two"};

    replace_all(text, "", "x");

    EXPECT_EQ("one two", text);
}

TEST(TestStringAlgorithms, splitEmptyInput)
{
    const std::vector<std::string> expected{""};

    EXPECT_EQ(expected, split("", ','));
}

TEST(TestStringAlgorithms, splitNoDelimiters)
{
    const std::vector<std::string> expected{"abc"};

    EXPECT_EQ(expected, split("abc", ','));
}

TEST(TestStringAlgorithms, splitPreservesEmptyFields)
{
    const std::vector<std::string> expected{"", "a", "", "b", ""};

    EXPECT_EQ(expected, split(",a,,b,", ','));
}

TEST(TestStringAlgorithms, splitAnyNoDelimiters)
{
    const std::vector<std::string> expected{"abc"};

    EXPECT_EQ(expected, split_any("abc", ",;"));
}

TEST(TestStringAlgorithms, splitAnyPreservesEmptyFields)
{
    const std::vector<std::string> expected{"", "a", "", "b", ""};

    EXPECT_EQ(expected, split_any(",a;:b,", ",;:"));
}

TEST(TestStringAlgorithms, splitAnyEmptyDelimiterSet)
{
    const std::vector<std::string> expected{"a,b"};

    EXPECT_EQ(expected, split_any("a,b", ""));
}

} // namespace id::test
