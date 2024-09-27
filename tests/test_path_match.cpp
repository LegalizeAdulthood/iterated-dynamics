// SPDX-License-Identifier: GPL-3.0-only
//
#include <path_match.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

TEST(TestPatternMatch, all)
{
    MatchFn all = match_fn("*.*");

    EXPECT_TRUE(all("foo.txt"));
    EXPECT_TRUE(all("foo.foo"));
    EXPECT_TRUE(all("id.id"));
}

TEST(TestPatternMatch, anyExtension)
{
    MatchFn any_ext = match_fn("foo.*");

    EXPECT_TRUE(any_ext("foo.txt"));
    EXPECT_TRUE(any_ext("foo.foo"));
    EXPECT_FALSE(any_ext("id.id"));
}

TEST(TestPatternMatch, anyStem)
{
    MatchFn any_stem = match_fn("*.txt");

    EXPECT_TRUE(any_stem("foo.txt"));
    EXPECT_FALSE(any_stem("foo.foo"));
    EXPECT_FALSE(any_stem("id.id"));
}

TEST(TestPatternMatch, anyStemWildExtension)
{
    MatchFn any_stem = match_fn("*.t?t");

    EXPECT_TRUE(any_stem("foo.txt"));
    EXPECT_FALSE(any_stem("foo.foo"));
    EXPECT_TRUE(any_stem("id.tst"));
}

TEST(TestPatternMatch, wildExtension)
{
    MatchFn wild_ext = match_fn("foo.t?t");

    EXPECT_TRUE(wild_ext("foo.txt"));
    EXPECT_FALSE(wild_ext("foo.foo"));
    EXPECT_FALSE(wild_ext("id.tst"));
}

TEST(TestPatternMatch, wildStem)
{
    MatchFn wild_stem = match_fn("fo?.txt");

    EXPECT_TRUE(wild_stem("foo.txt"));
    EXPECT_TRUE(wild_stem("for.txt"));
    EXPECT_FALSE(wild_stem("id.tst"));
}

TEST(TestPatternMatch, wildFilename)
{
    MatchFn wild_filename = match_fn("fo?.t?t");

    EXPECT_TRUE(wild_filename("foo.txt"));
    EXPECT_TRUE(wild_filename("for.tst"));
    EXPECT_FALSE(wild_filename("fro.txt"));
    EXPECT_FALSE(wild_filename("foo.sst"));
    EXPECT_FALSE(wild_filename("id.tst"));
}

TEST(TestPatternMatch, filenameSubstring)
{
    MatchFn arbitrary = match_fn("*frob*.*");

    EXPECT_FALSE(arbitrary("fab.bof"));
    EXPECT_TRUE(arbitrary("goinkfrob.bof"));
    EXPECT_TRUE(arbitrary("goinkfrobgoink.bof"));
    EXPECT_TRUE(arbitrary("frobgoink.goinkbof"));
}

TEST(TestPatternMatch, extensionSubstring)
{
    MatchFn arbitrary = match_fn("*.*frob*");

    EXPECT_FALSE(arbitrary("fab.bof"));
    EXPECT_TRUE(arbitrary("bof.goinkfrob"));
    EXPECT_TRUE(arbitrary("bof.goinkfrobgoink"));
    EXPECT_TRUE(arbitrary("bof.frobgoink"));
}

TEST(TestPatternMatch, arbitrary)
{
    MatchFn arbitrary = match_fn("*f?b*.*b?f*");

    EXPECT_TRUE(arbitrary("fab.bof"));
    EXPECT_TRUE(arbitrary("goinkf0b.bof"));
    EXPECT_TRUE(arbitrary("goinkf0bgoink.bof"));
    EXPECT_TRUE(arbitrary("goinkf0bgoink.goinkbof"));
    EXPECT_TRUE(arbitrary("goinkf0bgoink.goinkbofgoink"));
}

TEST(TestPatternMatch, everything)
{
    MatchFn everything = match_fn("*");

    EXPECT_TRUE(everything("fab.bof"));
    EXPECT_TRUE(everything("goinkf0b.bof"));
    EXPECT_TRUE(everything("goinkf0bgoink.bof"));
    EXPECT_TRUE(everything("goinkf0bgoink.goinkbof"));
    EXPECT_TRUE(everything("goinkf0bgoink.goinkbofgoink"));
}
