#include <lowerize_parameter.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <gtest/gtest.h>

using namespace testing;
using namespace boost::algorithm;

class TestLowerizeParameter : public Test, public WithParamInterface<const char *>
{
protected:
    char m_line[100]{};
};

TEST_F(TestLowerizeParameter, argumentLowered)
{
    strcpy(m_line, "FOO=SOME_ARG");

    lowerize_parameter(m_line);

    EXPECT_STREQ("foo=some_arg", m_line);
}

TEST_P(TestLowerizeParameter, valueUnchanged)
{
    const std::string param{GetParam()};
    strcpy(m_line, (to_upper_copy(param) + "=SOME_ARG").c_str());

    lowerize_parameter(m_line);

    EXPECT_EQ(to_lower_copy(param) + "=SOME_ARG", m_line);
}

INSTANTIATE_TEST_SUITE_P(TestCaseSensitiveParameters, TestLowerizeParameter,
    Values("autokeyname", "colors", "comment", "filename", "formulafile", "ifsfile", "lfile", "lightname",
        "makedoc", "map", "orbitsavename", "parmfile", "savename"));
