#include <compiler.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <iostream>
#include <vector>

using namespace testing;

namespace hc {

std::ostream &operator<<(std::ostream&str, hc::modes value)
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
    m_args.resize(args.size());
    m_argv.resize(args.size());
    std::copy(args.begin(), args.end(), m_args.begin());
    std::transform(m_args.begin(), m_args.end(), m_argv.begin(), [](std::string &arg) { return arg.data(); });
    m_options = hc::parse_compiler_options(static_cast<int>(m_argv.size()), m_argv.data());
}

TEST_F(TestParseCompilerOptions, modeCompile)
{
    parse_options({"hc.exe", "/c"});

    EXPECT_EQ(hc::modes::COMPILE, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modePrint)
{
    parse_options({"hc.exe", "/p"});

    EXPECT_EQ(hc::modes::PRINT, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeAppend)
{
    parse_options({"hc.exe", "/a"});

    EXPECT_EQ(hc::modes::APPEND, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeDelete)
{
    parse_options({"hc.exe", "/d"});

    EXPECT_EQ(hc::modes::DELETE, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeHTML)
{
    parse_options({"hc.exe", "/h"});

    EXPECT_EQ(hc::modes::HTML, m_options.mode);
}

TEST_F(TestParseCompilerOptions, modeAsciiDoc)
{
    parse_options({"hc.exe", "/adoc"});

    EXPECT_EQ(hc::modes::ASCII_DOC, m_options.mode);
}

TEST_F(TestParseCompilerOptions, firstFile)
{
    parse_options({"hc.exe", "foo.src"});

    EXPECT_EQ("foo.src", m_options.fname1);
}

TEST_F(TestParseCompilerOptions, secondFile)
{
    parse_options({"hc.exe", "foo.src", "foo.txt"});

    EXPECT_EQ("foo.src", m_options.fname1);
    EXPECT_EQ("foo.txt", m_options.fname2);
}
