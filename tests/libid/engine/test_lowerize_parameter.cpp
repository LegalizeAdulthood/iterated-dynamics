// SPDX-License-Identifier: GPL-3.0-only
//
#include <engine/lowerize_parameter.h>

#include <algos/string_algorithms.h>

#include <gtest/gtest.h>

using namespace id::algos;
using namespace id::engine;
using namespace testing;

namespace id::test
{

class TestLowerizeParameter : public Test, public WithParamInterface<const char *>
{
protected:
    std::string m_line;
};

TEST_F(TestLowerizeParameter, argumentLowered)
{
    m_line = "FOO=SOME_ARG";

    lowerize_parameter(m_line);

    EXPECT_EQ("foo=some_arg", m_line);
}

TEST_P(TestLowerizeParameter, valueUnchanged)
{
    const std::string param{GetParam()};
    m_line = ascii_to_upper_copy(param) + "=SOME_ARG";

    lowerize_parameter(m_line);

    EXPECT_EQ(ascii_to_lower_copy(param) + "=SOME_ARG", m_line);
}

TEST_F(TestLowerizeParameter, rdsTextureValueUnchanged)
{
    m_line = "RDS-TEXTURE=C:\\Tmp\\Texture.GIF";

    lowerize_parameter(m_line);

    EXPECT_EQ("rds-texture=C:\\Tmp\\Texture.GIF", m_line);
}

INSTANTIATE_TEST_SUITE_P(TestCaseSensitiveParameters, TestLowerizeParameter,
    Values("autokeyname", "colors", "comment", "filename", "formulafile", "formulaname", "ifs", "ifs3d", "ifsfile",
        "lfile", "lightname", "lname", "makedoc", "map", "orbitsavename", "parmfile", "rds-texture", "savename"));

} // namespace id::test
