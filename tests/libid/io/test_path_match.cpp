// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/path_match.h>

#include <gtest/gtest.h>

#include <ostream>

using namespace id::io;

namespace id::test
{

struct PatternMatchInput
{
    const char *name;
    const char *pattern;
    const char *filename;
    bool expected;
};

void PrintTo(const PatternMatchInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestPatternMatch : public testing::TestWithParam<PatternMatchInput>
{
};

TEST_P(TestPatternMatch, matchesFilenameUsingPattern)
{
    const MatchFn match{match_fn(GetParam().pattern)};

    EXPECT_EQ(GetParam().expected, match(GetParam().filename));
}

INSTANTIATE_TEST_SUITE_P(PatternInputs, TestPatternMatch,
    testing::Values(PatternMatchInput{"AllFooText", "*.*", "foo.txt", true},
        PatternMatchInput{"AllFooFoo", "*.*", "foo.foo", true}, PatternMatchInput{"AllIdId", "*.*", "id.id", true},
        PatternMatchInput{"AnyExtensionText", "foo.*", "foo.txt", true},
        PatternMatchInput{"AnyExtensionFoo", "foo.*", "foo.foo", true},
        PatternMatchInput{"AnyExtensionOtherStem", "foo.*", "id.id", false},
        PatternMatchInput{"AnyStemText", "*.txt", "foo.txt", true},
        PatternMatchInput{"AnyStemOtherExtension", "*.txt", "foo.foo", false},
        PatternMatchInput{"AnyStemOtherName", "*.txt", "id.id", false},
        PatternMatchInput{"WildExtensionText", "*.t?t", "foo.txt", true},
        PatternMatchInput{"WildExtensionWrongSuffix", "*.t?t", "foo.foo", false},
        PatternMatchInput{"WildExtensionTst", "*.t?t", "id.tst", true},
        PatternMatchInput{"FixedStemText", "foo.t?t", "foo.txt", true},
        PatternMatchInput{"FixedStemWrongExtension", "foo.t?t", "foo.foo", false},
        PatternMatchInput{"FixedStemWrongStem", "foo.t?t", "id.tst", false},
        PatternMatchInput{"WildStemFoo", "fo?.txt", "foo.txt", true},
        PatternMatchInput{"WildStemFor", "fo?.txt", "for.txt", true},
        PatternMatchInput{"WildStemWrongName", "fo?.txt", "id.tst", false},
        PatternMatchInput{"WildFilenameFoo", "fo?.t?t", "foo.txt", true},
        PatternMatchInput{"WildFilenameFor", "fo?.t?t", "for.tst", true},
        PatternMatchInput{"WildFilenameWrongStem", "fo?.t?t", "fro.txt", false},
        PatternMatchInput{"WildFilenameWrongExtension", "fo?.t?t", "foo.sst", false},
        PatternMatchInput{"WildFilenameWrongName", "fo?.t?t", "id.tst", false},
        PatternMatchInput{"StemSubstringAbsent", "*frob*.*", "fab.bof", false},
        PatternMatchInput{"StemSubstringAtEnd", "*frob*.*", "goinkfrob.bof", true},
        PatternMatchInput{"StemSubstringEmbedded", "*frob*.*", "goinkfrobgoink.bof", true},
        PatternMatchInput{"StemSubstringAtStart", "*frob*.*", "frobgoink.goinkbof", true},
        PatternMatchInput{"ExtensionSubstringAbsent", "*.*frob*", "fab.bof", false},
        PatternMatchInput{"ExtensionSubstringAtEnd", "*.*frob*", "bof.goinkfrob", true},
        PatternMatchInput{"ExtensionSubstringEmbedded", "*.*frob*", "bof.goinkfrobgoink", true},
        PatternMatchInput{"ExtensionSubstringAtStart", "*.*frob*", "bof.frobgoink", true},
        PatternMatchInput{"ArbitraryExact", "*f?b*.*b?f*", "fab.bof", true},
        PatternMatchInput{"ArbitraryStemSuffix", "*f?b*.*b?f*", "goinkf0b.bof", true},
        PatternMatchInput{"ArbitraryStemEmbedded", "*f?b*.*b?f*", "goinkf0bgoink.bof", true},
        PatternMatchInput{"ArbitraryExtensionSuffix", "*f?b*.*b?f*", "goinkf0bgoink.goinkbof", true},
        PatternMatchInput{"ArbitraryBothEmbedded", "*f?b*.*b?f*", "goinkf0bgoink.goinkbofgoink", true},
        PatternMatchInput{"EverythingExact", "*", "fab.bof", true},
        PatternMatchInput{"EverythingStemSuffix", "*", "goinkf0b.bof", true},
        PatternMatchInput{"EverythingStemEmbedded", "*", "goinkf0bgoink.bof", true},
        PatternMatchInput{"EverythingExtensionSuffix", "*", "goinkf0bgoink.goinkbof", true},
        PatternMatchInput{"EverythingBothEmbedded", "*", "goinkf0bgoink.goinkbofgoink", true}),
    [](const testing::TestParamInfo<PatternMatchInput> &info) { return info.param.name; });

} // namespace id::test
