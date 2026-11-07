// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <engine/lowerize_parameter.h>

#include <gtest/gtest.h>

#include <ostream>
#include <string>

using namespace id::engine;
using namespace testing;

namespace id::test
{

struct ParameterNormalizationInput
{
    const char *name;
    const char *input;
    const char *expected;
};

void PrintTo(const ParameterNormalizationInput &value, std::ostream *str)
{
    *str << value.name;
}

class TestLowerizeParameter : public TestWithParam<ParameterNormalizationInput>
{
protected:
    void SetUp() override;

    std::string m_line;
};

void TestLowerizeParameter::SetUp()
{
    Test::SetUp();
    m_line = GetParam().input;
}

TEST_P(TestLowerizeParameter, normalizesParameter)
{
    lowerize_parameter(m_line);

    EXPECT_EQ(GetParam().expected, m_line);
}

INSTANTIATE_TEST_SUITE_P(ParameterInputs, TestLowerizeParameter,
    Values(ParameterNormalizationInput{"OrdinaryValue", "FOO=SOME_ARG", "foo=some_arg"},
        ParameterNormalizationInput{"AutokeyName", "AUTOKEYNAME=SOME_ARG", "autokeyname=SOME_ARG"},
        ParameterNormalizationInput{"Colors", "COLORS=SOME_ARG", "colors=SOME_ARG"},
        ParameterNormalizationInput{"Comment", "COMMENT=SOME_ARG", "comment=SOME_ARG"},
        ParameterNormalizationInput{"Filename", "FILENAME=SOME_ARG", "filename=SOME_ARG"},
        ParameterNormalizationInput{"FormulaFile", "FORMULAFILE=SOME_ARG", "formulafile=SOME_ARG"},
        ParameterNormalizationInput{"FormulaName", "FORMULANAME=SOME_ARG", "formulaname=SOME_ARG"},
        ParameterNormalizationInput{"Ifs", "IFS=SOME_ARG", "ifs=SOME_ARG"},
        ParameterNormalizationInput{"Ifs3d", "IFS3D=SOME_ARG", "ifs3d=SOME_ARG"},
        ParameterNormalizationInput{"IfsFile", "IFSFILE=SOME_ARG", "ifsfile=SOME_ARG"},
        ParameterNormalizationInput{"LFile", "LFILE=SOME_ARG", "lfile=SOME_ARG"},
        ParameterNormalizationInput{"LightName", "LIGHTNAME=SOME_ARG", "lightname=SOME_ARG"},
        ParameterNormalizationInput{"LName", "LNAME=SOME_ARG", "lname=SOME_ARG"},
        ParameterNormalizationInput{"MakeDoc", "MAKEDOC=SOME_ARG", "makedoc=SOME_ARG"},
        ParameterNormalizationInput{"Map", "MAP=SOME_ARG", "map=SOME_ARG"},
        ParameterNormalizationInput{"OrbitSaveName", "ORBITSAVENAME=SOME_ARG", "orbitsavename=SOME_ARG"},
        ParameterNormalizationInput{"ParmFile", "PARMFILE=SOME_ARG", "parmfile=SOME_ARG"},
        ParameterNormalizationInput{"RdsTexture", "RDS-TEXTURE=SOME_ARG", "rds-texture=SOME_ARG"},
        ParameterNormalizationInput{"SaveName", "SAVENAME=SOME_ARG", "savename=SOME_ARG"},
        ParameterNormalizationInput{
            "RdsTexturePath", "RDS-TEXTURE=C:\\Tmp\\Texture.GIF", "rds-texture=C:\\Tmp\\Texture.GIF"}),
    [](const TestParamInfo<ParameterNormalizationInput> &info) { return info.param.name; });

} // namespace id::test
