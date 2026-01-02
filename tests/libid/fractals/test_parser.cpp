#include "fractals/formula.h"
#include "misc/ValueSaver.h"

#include <fractals/parser.h>

#include <gtest/gtest.h>

using namespace id::fractals;
using namespace id::misc;

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
 14: center          = (0.000000, 0.000000)
 15: magxmag         = (inf, -nan)
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
};

void ParserTest::TearDown()
{
    parser_reset();
}

} // namespace

TEST_F(ParserTest, moe)
{
    ValueSaver saved_formula_filename{g_formula_filename, "test.frm"};
    ValueSaver saved_formula_name{g_formula_name, s_moe.name};
    ValueSaver saved_formula_text{g_formula.formula};

    ASSERT_TRUE(parse_formula(s_moe, true)) << "Should have parsed formula";

    EXPECT_EQ("s=exp(1.,0.),z=pixel,c=fn1(pixel),if(real(p1)<=0),"
              "test=100,else,test=real(p1),endif:z=fn2(z)^s+c,|z|<=test",
        g_formula.formula);
    EXPECT_EQ(s_moe_state, get_parser_state());
}
