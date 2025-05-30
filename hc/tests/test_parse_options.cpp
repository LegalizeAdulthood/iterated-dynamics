// SPDX-License-Identifier: GPL-3.0-only
//
#include <Compiler.h>

#include <Options.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

using namespace testing;

namespace std
{

std::ostream &operator<<(std::ostream &str, const std::vector<const char *> &options)
{
    str << '{';
    bool first{true};
    for (const char *opt : options)
    {
        if (!first)
        {
            str << ", ";
        }
        str << opt;
        first = false;
    }
    return str << '}';
}

void PrintTo(const std::vector<const char *> &options, std::ostream *str)
{
    *str << options;
}

} // namespace std

namespace hc
{

std::ostream &operator<<(std::ostream &str, hc::Mode value)
{
    switch (value)
    {
    case Mode::NONE:
        return str << "NONE";
    case Mode::COMPILE:
        return str << "COMPILE";
    case Mode::PRINT:
        return str << "PRINT";
    case Mode::APPEND:
        return str << "APPEND";
    case Mode::DELETE:
        return str << "DELETE";
    case Mode::ASCII_DOC:
        return str << "ASCII_DOC";
    }

    return str << "? (" << static_cast<int>(value) << ")";
}

}

class TestParseCompilerOptions : public Test
{
protected:
    void parse_options(const std::vector<const char *> &args);

    std::vector<std::string> m_args;
    std::vector<char *> m_argv;
    hc::Options m_options;
};

void TestParseCompilerOptions::parse_options(const std::vector<const char *> &args)
{
    m_args.resize(args.size() + 1);
    m_argv.resize(args.size() + 1);
    m_args[0] = "hc.exe";
    std::copy(args.begin(), args.end(), m_args.begin() + 1);
    std::transform(m_args.begin(), m_args.end(), m_argv.begin(), [](std::string &arg) { return arg.data(); });
    m_options = hc::parse_options(static_cast<int>(m_argv.size()), m_argv.data());
}

TEST_F(TestParseCompilerOptions, modeCompile)
{
    parse_options({"/c"});

    EXPECT_EQ(hc::Mode::COMPILE, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modePrint)
{
    parse_options({"/p"});

    EXPECT_EQ(hc::Mode::PRINT, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeAppend)
{
    parse_options({"/a"});

    EXPECT_EQ(hc::Mode::APPEND, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeDelete)
{
    parse_options({"/d"});

    EXPECT_EQ(hc::Mode::DELETE, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeAsciiDoc)
{
    parse_options({"/adoc"});

    EXPECT_EQ(hc::Mode::ASCII_DOC, m_options.mode);
}

TEST_F(TestParseCompilerOptions, asciiDocOutputDir)
{
    parse_options({"/adoc", "/o", "out"});

    EXPECT_EQ(hc::Mode::ASCII_DOC, m_options.mode);
    EXPECT_EQ("out", m_options.output_dir);
}

TEST_F(TestParseCompilerOptions, asciiDocSwapPath)
{
    parse_options({"/adoc", "/r", "swap"});

    EXPECT_EQ(hc::Mode::ASCII_DOC, m_options.mode);
    EXPECT_EQ("swap", m_options.swap_path);
}

TEST_F(TestParseCompilerOptions, firstFile)
{
    parse_options({"foo.src"});

    EXPECT_EQ("foo.src", m_options.fname1);
}

TEST_F(TestParseCompilerOptions, secondFile)
{
    parse_options({"foo.src", "foo.txt"});

    EXPECT_EQ("foo.src", m_options.fname1);
    EXPECT_EQ("foo.txt", m_options.fname2);
}

class withOptions : public TestParseCompilerOptions, public WithParamInterface<std::vector<const char *>>
{
};

TEST_P(withOptions, invalidCombination)
{
    EXPECT_THROW(parse_options(GetParam()), std::runtime_error);
}

static std::vector<const char *> s_invalid_options[]{
    {"/a", "/adoc"},          //
    {"/a", "/c"},             //
    {"/a", "/d"},             //
    {"/a", "/m"},             //
    {"/a", "/o", "."},        //
    {"/a", "/p"},             //
    {"/a", "/r", "."},        //
    {"/a", "/s"},             //
    {"/adoc", "/a"},          //
    {"/adoc", "/c"},          //
    {"/adoc", "/d"},          //
    {"/adoc", "/m"},          //
    {"/adoc", "/p"},          //
    {"/adoc", "/s"},          //
    {"/c", "/a"},             //
    {"/c", "/adoc"},          //
    {"/c", "/d"},             //
    {"/c", "/o", "."},        //
    {"/c", "/p"},             //
    {"/d", "/a"},             //
    {"/d", "/adoc"},          //
    {"/d", "/c"},             //
    {"/d", "/m"},             //
    {"/d", "/o", "."},        //
    {"/d", "/p"},             //
    {"/d", "/r", "."},        //
    {"/d", "/s"},             //
    {"/i"},                   //
    {"/o"},                   //
    {"/p", "/a"},             //
    {"/p", "/adoc"},          //
    {"/p", "/d"},             //
    {"/p", "/m"},             //
    {"/p", "/o", "."},        //
    {"/p", "/s"},             //
    {"/r"},                   //
    {"/unknown-option"},      //
    {"too", "many", "files"}, //
};

INSTANTIATE_TEST_SUITE_P(TestOptionCombos, withOptions, ValuesIn(s_invalid_options));
