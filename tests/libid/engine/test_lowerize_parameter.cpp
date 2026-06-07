// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/lowerize_parameter.h>

#include <boost/algorithm/string/case_conv.hpp>

#include <gtest/gtest.h>

#include <cstring>

using namespace boost::algorithm;
using namespace id::engine;
using namespace testing;

namespace id::test
{

class TestLowerizeParameter : public Test, public WithParamInterface<const char *>
{
protected:
    char m_line[100]{};
};

TEST_F(TestLowerizeParameter, argumentLowered)
{
    std::strcpy(m_line, "FOO=SOME_ARG");

    lowerize_parameter(m_line);

    EXPECT_STREQ("foo=some_arg", m_line);
}

TEST_P(TestLowerizeParameter, valueUnchanged)
{
    const std::string param{GetParam()};
    std::strcpy(m_line, (to_upper_copy(param) + "=SOME_ARG").c_str());

    lowerize_parameter(m_line);

    EXPECT_EQ(to_lower_copy(param) + "=SOME_ARG", m_line);
}

TEST_F(TestLowerizeParameter, rdsTextureValueUnchanged)
{
    std::strcpy(m_line, "RDS-TEXTURE=C:\\Tmp\\Texture.GIF");

    lowerize_parameter(m_line);

    EXPECT_STREQ("rds-texture=C:\\Tmp\\Texture.GIF", m_line);
}

INSTANTIATE_TEST_SUITE_P(TestCaseSensitiveParameters, TestLowerizeParameter,
    Values("autokeyname", "colors", "comment", "filename", "formulafile", "formulaname", "ifs", "ifs3d", "ifsfile",
        "lfile", "lightname", "lname", "makedoc", "map", "orbitsavename", "parmfile", "rds-texture", "savename"));

} // namespace id::test
