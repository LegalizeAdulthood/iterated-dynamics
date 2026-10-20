// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "fractals/formula.h"
#include "misc/ValueSaver.h"
#include "test_data.h"

#include <engine/cmdfiles.h>
#include <engine/calcfrac.h>
#include <engine/ImageRegion.h>
#include <fractals/fractalp.h>
#include <fractals/parser.h>
#include <io/library.h>
#include <misc/Driver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <fstream>
#include <stdexcept>

namespace fs = std::filesystem;

using namespace id::engine;
using namespace id::fractals;
using namespace id::io;
using namespace id::misc;
using namespace id::test::data;
using namespace testing;

// clang-format off
const FormulaEntry s_moe{"moe", "",
R"frm( ; Mutation of 'Zexpe'.
   ; Original formula by Lee Skinner
   ; Modified for if..else logic 3/19/97 by Sylvie Gallet
   ; For 'Zexpe', set fn1 & fn2 = ident and p1 = default
   ; real(p1) = Bailout (default 100)
   s = exp(1.,0.), z = pixel, c = fn1(pixel)
   ; The next line sets test=100 if real(p1)<=0, else test=real(p1)
   if (real(p1) <= 0)
      test = 100
   else
      test = real(p1)
   endif
   :
   z = fn2(z)^s + c
   |z| <= test
)frm"};

static const char *const s_moe_state{
R"(=== Formula: moe ===
Symmetry: 0
Total Operations: 40
Init Operations: 10000
Variables: 25
Uses Jumps: yes
Uses p1: true p2: false p3: false p4: false p5: false
Uses ismand: false

=== Variables ===
  0: pixel           = (0.000000, 0.000000)
  1: p1              = (0.000000, 0.000000)
  2: p2              = (0.000000, 0.000000)
  3: z               = (0.000000, 0.000000)
  4: LastSqr         = (0.000000, 0.000000)
  5: pi              = (3.141593, 0.000000)
  6: e               = (2.718282, 0.000000)
  7: rand            = (0.000000, 0.000000)
  8: p3              = (0.000000, 0.000000)
  9: whitesq         = (0.000000, 0.000000)
 10: scrnpix         = (0.000000, 0.000000)
 11: scrnmax         = (0.000000, 0.000000)
 12: maxit           = (0.000000, 0.000000)
 13: ismand          = (1.000000, 0.000000)
 14: center          = (-0.500000, 0.000000)
 15: magxmag         = (0.666667, 1.333333)
 16: rotskew         = (0.000000, 0.000000)
 17: p4              = (0.000000, 0.000000)
 18: p5              = (0.000000, 0.000000)
 19: s               = (0.000000, 0.000000)
 20: 1.,0.           = (1.000000, 0.000000)
 21: c               = (0.000000, 0.000000)
 22: 0               = (0.000000, 0.000000)
 23: test            = (0.000000, 0.000000)
 24: 100             = (100.000000, 0.000000)

=== Operations ===
  0: LOAD
  1: EXP
  2: STORE
  3: CLEAR
  4: LOAD
  5: STORE
  6: CLEAR
  7: LOAD
  8: FN1
  9: STORE
 10: CLEAR
 11: LOAD
 12: REAL
 13: LOAD
 14: LTE
 15: JUMP_IF_FALSE
 16: CLEAR
 17: LOAD
 18: STORE
 19: CLEAR
 20: JUMP
 21: CLEAR
 22: LOAD
 23: REAL
 24: STORE
 25: CLEAR
 26: JUMP_LABEL
 27: END_INIT
 28: LOAD
 29: FN2
 30: LOAD
 31: PWR
 32: LOAD
 33: ADD
 34: STORE
 35: CLEAR
 36: LOAD
 37: MOD
 38: LOAD
 39: LTE

=== Load Table ===
  0: 1.,0.
  1: pixel
  2: pixel
  3: p1
  4: 0
  5: 100
  6: p1
  7: z
  8: s
  9: c
 10: z
 11: test

=== Store Table ===
  0: s
  1: z
  2: c
  3: test
  4: test
  5: z

=== Jump Control Table ===
  0: IF       -> op: 20 load:  6 store:  4 dest:  2
  1: ELSE     -> op: 26 load:  7 store:  5 dest:  3
  2: END_IF   -> op:  0 load:  0 store:  0 dest:  0

)"};
// clang-format on

namespace
{

class ParserTest : public ::testing::Test
{
protected:
    void TearDown() override;
    void expect_failure(const FormulaEntry &entry, const char *message);
    void expect_no_program();

    ValueSaver<Driver *> m_driver{g_driver, nullptr};
    ValueSaver<FractalDispatch> m_dispatch{g_dispatch};
    ValueSaver<const FractalSpecific *> m_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::FORMULA)};
    ValueSaver<std::string> m_name{g_formula_name, "entry"};
};

void ParserTest::TearDown()
{
    parser_reset();
}

void ParserTest::expect_no_program()
{
    EXPECT_TRUE(g_formula.formula.empty());
    EXPECT_TRUE(g_formula.fns.empty());
    EXPECT_TRUE(g_formula.vars.empty());
    EXPECT_TRUE(g_formula.load.empty());
    EXPECT_TRUE(g_formula.store.empty());
    EXPECT_TRUE(g_formula.ops.empty());
    EXPECT_TRUE(g_formula.jump_control.empty());
    EXPECT_EQ(bad_formula, g_dispatch.per_pixel());
    EXPECT_EQ(bad_formula, g_dispatch.orbit_calc());
}

void ParserTest::expect_failure(const FormulaEntry &entry, const char *message)
{
    try
    {
        parse_formula(entry);
        FAIL() << "Expected a parser diagnostic";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_THAT(error.what(), HasSubstr(message));
    }
    expect_no_program();
}

} // namespace

TEST_F(ParserTest, moe)
{
    ValueSaver saved_formula_filename{g_formula_filename, "test.frm"};
    ValueSaver saved_formula_name{g_formula_name, s_moe.name};
    ValueSaver saved_formula_text{g_formula.formula};
    ValueSaver saved_image_region{
        g_image_region, ImageRegion{{-2.0, -1.5}, {1.0, 1.5}, {-2.0, -1.5}}};

    ASSERT_TRUE(parse_formula(s_moe).empty());

    EXPECT_EQ("s=exp(1.,0.),z=pixel,c=fn1(pixel),if(real(p1)<=0),"
              "test=100,else,test=real(p1),endif:z=fn2(z)^s+c,|z|<=test",
        g_formula.formula);
    EXPECT_EQ(s_moe_state, get_parser_state());
}

TEST_F(ParserTest, inlineFormulaIsUsedWhenFormulaFileIsMissing)
{
    fs::path path{"missing.frm"};
    ValueSaver saved_parameter_file{g_parameter_file, fs::path{ID_TEST_PAR_DIR} / ID_TEST_PAR_FILE};
    ValueSaver saved_formula_name{g_formula_name, "Fractint"};
    ValueSaver saved_cur_fractal_specific{g_cur_fractal_specific, get_fractal_specific(FractalType::FORMULA)};

    const std::vector<std::string> warnings{parse_formula(path, g_formula_name)};

    EXPECT_TRUE(warnings.empty());
    EXPECT_EQ(g_parameter_file, path);
}

TEST_F(ParserTest, rejectsEmptyFormulaWithoutUI)
{
    expect_failure({"empty", "", ""}, "Formula has no executable instructions");
}

TEST_F(ParserTest, preservesErrorLocationExpressionAndCaret)
{
    try
    {
        parse_formula({"bad", "", "\nz = *pixel\n"});
        FAIL() << "Expected a parser diagnostic";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_THAT(error.what(), HasSubstr("at line 2:  Should be an Argument"));
        EXPECT_THAT(error.what(), HasSubstr("z=*pixel"));
        EXPECT_THAT(error.what(), HasSubstr("^"));
    }
    expect_no_program();
}

TEST_F(ParserTest, rejectsUndefinedFunction)
{
    expect_failure({"bad", "", "z = unknown(pixel)"}, "Undefined Function");
}

TEST_F(ParserTest, rejectsUnmatchedJump)
{
    expect_failure({"bad", "", "if (pixel)\nz = pixel"}, "has no matching \"endif\"");
}

TEST_F(ParserTest, rejectsOverlongToken)
{
    expect_failure({"bad", "", std::string(80, 'z') + "=pixel"}, "32 character limit");
}

TEST_F(ParserTest, rejectsOversizedName)
{
    expect_failure({std::string(100, 'a'), "", "z=pixel"}, "Formula name has too many characters");
}

TEST_F(ParserTest, returnsRecoverableSymmetryWarningWithoutUI)
{
    const std::vector<std::string> warnings{parse_formula({"warning", "INVALID", "z=pixel"})};

    ASSERT_EQ(1U, warnings.size());
    EXPECT_THAT(warnings[0], HasSubstr("Symmetry below is invalid, will use NOSYM"));
    EXPECT_THAT(warnings[0], HasSubstr("INVALID"));
    EXPECT_EQ(SymmetryType::NONE, g_symmetry);
    EXPECT_FALSE(g_formula.fns.empty());
}

TEST_F(ParserTest, failedCompilationDiscardsPreviousProgram)
{
    ASSERT_TRUE(parse_formula(s_moe).empty());
    ASSERT_FALSE(g_formula.fns.empty());

    expect_failure({"bad", "", "z = *pixel"}, "Should be an Argument");
}

TEST_F(ParserTest, successfulCompilationAfterFailureRestoresDispatch)
{
    expect_failure({"bad", "", "z = *pixel"}, "Should be an Argument");

    EXPECT_TRUE(parse_formula(s_moe).empty());
    EXPECT_EQ(formula_per_pixel, g_dispatch.per_pixel());
    EXPECT_EQ(formula_orbit, g_dispatch.orbit_calc());
    EXPECT_FALSE(g_formula.fns.empty());
}

namespace
{

class FormulaLoadTest : public ParserTest
{
protected:
    void SetUp() override
    {
        m_path = fs::path{ID_TEST_DATA_DIR} /
            ("formula-" + std::string{UnitTest::GetInstance()->current_test_info()->name()} + ".frm");
    }

    void TearDown() override
    {
        ParserTest::TearDown();
        std::error_code ignored;
        fs::remove(m_path, ignored);
    }

    void write_definition(const std::string &text)
    {
        std::ofstream file{m_path};
        ASSERT_TRUE(file);
        file << text;
    }

    void expect_load_failure(const char *message)
    {
        try
        {
            parse_formula(m_path, g_formula_name);
            FAIL() << "Expected a lookup or parser diagnostic";
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_THAT(error.what(), HasSubstr(message));
        }
        expect_no_program();
    }

    fs::path m_path;
    ValueSaver<fs::path> m_parameter_file{g_parameter_file, fs::path{}};
    ValueSaver<bool> m_search_current{g_check_cur_dir, false};
};

} // namespace

TEST_F(FormulaLoadTest, reportsMissingFileWithoutUI)
{
    expect_load_failure("'entry' file entry item not found");
}

TEST_F(FormulaLoadTest, reportsMissingEntryWithoutUI)
{
    write_definition("other { z=pixel }\n");

    expect_load_failure("'entry' file entry item not found");
}

TEST_F(FormulaLoadTest, rejectsEmptyName)
{
    g_formula_name.clear();

    expect_load_failure("File entry item name is empty");
}

TEST_F(FormulaLoadTest, closesFileAfterHeaderFailure)
{
    write_definition("entry (INVALID) { z=pixel }\n");
    expect_load_failure("Symmetry below is invalid");

    EXPECT_TRUE(fs::remove(m_path));
    write_definition("entry { z=pixel }\n");
    EXPECT_TRUE(parse_formula(m_path, g_formula_name).empty());
}

TEST_F(FormulaLoadTest, closesFileAfterPrescanFailure)
{
    write_definition("entry { z=*pixel }\n");
    expect_load_failure("Should be an Argument");

    EXPECT_TRUE(fs::remove(m_path));
    write_definition("entry { z=pixel }\n");
    EXPECT_TRUE(parse_formula(m_path, g_formula_name).empty());
}

TEST_F(FormulaLoadTest, inspectionReportsMissingDefinitionWithoutUI)
{
    EXPECT_THROW(frm_get_param_stuff(m_path, g_formula_name.c_str()), std::runtime_error);
    EXPECT_FALSE(g_formula.uses_p1);
}

TEST_F(FormulaLoadTest, inspectionClearsParametersAfterInvalidToken)
{
    write_definition("entry { z=p1+fn2(pixel)+ismand, z=~ }\n");

    EXPECT_THROW(frm_get_param_stuff(m_path, g_formula_name.c_str()), std::runtime_error);

    EXPECT_FALSE(g_formula.uses_p1);
    EXPECT_FALSE(g_formula.uses_ismand);
    EXPECT_EQ(0, g_formula.max_function);
}

TEST_F(FormulaLoadTest, inspectionFindsParametersAndFunctions)
{
    write_definition("entry { z=p1+fn2(pixel)+ismand }\n");

    EXPECT_NO_THROW(frm_get_param_stuff(m_path, g_formula_name.c_str()));

    EXPECT_TRUE(g_formula.uses_p1);
    EXPECT_TRUE(g_formula.uses_ismand);
    EXPECT_EQ(2, g_formula.max_function);
}
