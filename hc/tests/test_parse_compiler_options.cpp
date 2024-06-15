#include <compiler.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

using namespace testing;

namespace hc
{

std::ostream &operator<<(std::ostream &str, hc::modes value)
{
    switch (value)
    {
    case modes::NONE:
        return str << "NONE";
    case modes::COMPILE:
        return str << "COMPILE";
    case modes::PRINT:
        return str << "PRINT";
    case modes::APPEND:
        return str << "APPEND";
    case modes::DELETE:
        return str << "DELETE";
    case modes::HTML:
        return str << "HTML";
    case modes::ASCII_DOC:
        return str << "ASCII_DOC";
    }

    return str << "? (" << static_cast<int>(value) << ")";
}

}

class TestParseCompilerOptions : public Test
{
protected:
    void parse_options(const std::initializer_list<const char *> &args);

    std::vector<std::string> m_args;
    std::vector<char *> m_argv;
    hc::compiler_options m_options;
};

void TestParseCompilerOptions::parse_options(const std::initializer_list<const char *> &args)
{
    m_args.resize(args.size() + 1);
    m_argv.resize(args.size() + 1);
    m_args[0] = "hc.exe";
    std::copy(args.begin(), args.end(), m_args.begin() + 1);
    std::transform(m_args.begin(), m_args.end(), m_argv.begin(), [](std::string &arg) { return arg.data(); });
    m_options = hc::parse_compiler_options(static_cast<int>(m_argv.size()), m_argv.data());
}

TEST_F(TestParseCompilerOptions, modeCompile)
{
    parse_options({"/c"});

    EXPECT_EQ(hc::modes::COMPILE, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modePrint)
{
    parse_options({"/p"});

    EXPECT_EQ(hc::modes::PRINT, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeAppend)
{
    parse_options({"/a"});

    EXPECT_EQ(hc::modes::APPEND, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeDelete)
{
    parse_options({"/d"});

    EXPECT_EQ(hc::modes::DELETE, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeHTML)
{
    parse_options({"/h"});

    EXPECT_EQ(hc::modes::HTML, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeAsciiDoc)
{
    parse_options({"/adoc"});

    EXPECT_EQ(hc::modes::ASCII_DOC, m_options.mode);
}

TEST_F(TestParseCompilerOptions, asciiDocOutputDir)
{
    parse_options({"/adoc", "/o", "out"});

    EXPECT_EQ(hc::modes::ASCII_DOC, m_options.mode);
    EXPECT_EQ("out", m_options.output_dir);
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

class TestOptionCombos : public TestParseCompilerOptions,
                                      public WithParamInterface<std::initializer_list<const char *>>
{
};

TEST_P(TestOptionCombos, invalidCombination)
{
    EXPECT_THROW(parse_options(GetParam()), std::runtime_error);
}

static std::initializer_list<const char *> s_invalid_options[]{
    {"/a", "/adoc"},          //
    {"/a", "/c"},             //
    {"/a", "/d"},             //
    {"/a", "/h"},             //
    {"/a", "/m"},             //
    {"/a", "/o", "."},        //
    {"/a", "/p"},             //
    {"/a", "/r", "."},        //
    {"/a", "/s"},             //
    {"/adoc", "/a"},          //
    {"/adoc", "/c"},          //
    {"/adoc", "/d"},          //
    {"/adoc", "/h"},          //
    {"/adoc", "/m"},          //
    {"/adoc", "/p"},          //
    {"/adoc", "/r", "."},     //
    {"/adoc", "/s"},          //
    {"/c", "/a"},             //
    {"/c", "/adoc"},          //
    {"/c", "/d"},             //
    {"/c", "/h"},             //
    {"/c", "/o", "."},        //
    {"/c", "/p"},             //
    {"/d", "/a"},             //
    {"/d", "/adoc"},          //
    {"/d", "/c"},             //
    {"/d", "/h"},             //
    {"/d", "/m"},             //
    {"/d", "/o", "."},        //
    {"/d", "/p"},             //
    {"/d", "/r", "."},        //
    {"/d", "/s"},             //
    {"/h", "/a"},             //
    {"/h", "/adoc"},          //
    {"/h", "/c"},             //
    {"/h", "/d"},             //
    {"/h", "/m"},             //
    {"/h", "/o"},             //
    {"/h", "/p"},             //
    {"/h", "/r", "."},        //
    {"/h", "/s"},             //
    {"/i"},                   //
    {"/o"},                   //
    {"/p", "/a"},             //
    {"/p", "/adoc"},          //
    {"/p", "/d"},             //
    {"/p", "/h"},             //
    {"/p", "/m"},             //
    {"/p", "/o", "."},        //
    {"/p", "/s"},             //
    {"/r"},                   //
    {"/unknown-option"},      //
    {"too", "many", "files"}, //
};

INSTANTIATE_TEST_SUITE_P(withOptions, TestOptionCombos, ValuesIn(s_invalid_options));
