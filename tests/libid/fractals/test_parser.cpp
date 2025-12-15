#include <fractals/parser.h>

#include <gtest/gtest.h>

using namespace id::fractals;

// clang-format off
const char *const s_moe{
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
R"(=== Formula: TEST_FORMULA ===
Symmetry: 0
Total Operations: 93
Init Operations: 10000
Variables: 54
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
 19: Mutation        = (0.000000, 0.000000)
 20: of              = (0.000000, 0.000000)
 21: 'Zexpe          = (0.000000, 0.000000)
 22: '.              = (0.000000, 0.000000)
 23: Original        = (0.000000, 0.000000)
 24: formula         = (0.000000, 0.000000)
 25: by              = (0.000000, 0.000000)
 26: Lee             = (0.000000, 0.000000)
 27: Skinner         = (0.000000, 0.000000)
 28: Modified        = (0.000000, 0.000000)
 29: for             = (0.000000, 0.000000)
 30: if..else        = (0.000000, 0.000000)
 31: logic           = (0.000000, 0.000000)
 32: 3               = (3.000000, 0.000000)
 33: 19              = (19.000000, 0.000000)
 34: 97              = (97.000000, 0.000000)
 35: Sylvie          = (0.000000, 0.000000)
 36: Gallet          = (0.000000, 0.000000)
 37: '               = (0.000000, 0.000000)
 38: set             = (0.000000, 0.000000)
 39: fn1             = (0.000000, 0.000000)
 40: fn2             = (0.000000, 0.000000)
 41: ident           = (0.000000, 0.000000)
 42: and             = (0.000000, 0.000000)
 43: default         = (0.000000, 0.000000)
 44: 100             = (100.000000, 0.000000)
 45: s               = (0.000000, 0.000000)
 46: 1.,0.           = (1.000000, 0.000000)
 47: c               = (0.000000, 0.000000)
 48: The             = (0.000000, 0.000000)
 49: next            = (0.000000, 0.000000)
 50: line            = (0.000000, 0.000000)
 51: sets            = (0.000000, 0.000000)
 52: test            = (0.000000, 0.000000)
 53: 0               = (0.000000, 0.000000)

=== Operations ===
  0: LOAD
  1: LOAD
  2: LOAD
  3: LOAD
  4: CLEAR
  5: LOAD
  6: LOAD
  7: LOAD
  8: LOAD
  9: LOAD
 10: CLEAR
 11: LOAD
 12: LOAD
 13: LOAD
 14: LOAD
 15: LOAD
 16: LOAD
 17: DIV
 18: LOAD
 19: LOAD
 20: LOAD
 21: LOAD
 22: DIV
 23: CLEAR
 24: LOAD
 25: LOAD
 26: LOAD
 27: CLEAR
 28: LOAD
 29: LOAD
 30: LOAD
 31: LOAD
 32: LOAD
 33: STORE
 34: STORE
 35: AND
 36: CLEAR
 37: REAL
 38: LOAD
 39: LOAD
 40: ?unknown function
 41: LOAD
 42: EXP
 43: STORE
 44: STORE
 45: CLEAR
 46: LOAD
 47: STORE
 48: CLEAR
 49: LOAD
 50: FN1
 51: STORE
 52: CLEAR
 53: LOAD
 54: LOAD
 55: LOAD
 56: LOAD
 57: LOAD
 58: LOAD
 59: REAL
 60: LOAD
 61: LTE
 62: JUMP_IF_FALSE
 63: STORE
 64: CLEAR
 65: JUMP
 66: LOAD
 67: REAL
 68: LOAD
 69: REAL
 70: LOAD
 71: LTE
 72: LOAD
 73: STORE
 74: JUMP_IF_FALSE
 75: LOAD
 76: REAL
 77: STORE
 78: JUMP
 79: JUMP_LABEL
 80: STORE
 81: END_INIT
 82: LOAD
 83: FN2
 84: LOAD
 85: PWR
 86: LOAD
 87: LOAD
 88: MOD
 89: ADD
 90: LOAD
 91: LTE
 92: STORE
 93: LOAD
 94: LOAD
 95: LOAD
 96: LOAD
 97: CLEAR
 98: LOAD
 99: LOAD
100: LOAD
101: LOAD
102: LOAD
103: CLEAR
104: LOAD
105: LOAD
106: LOAD
107: LOAD
108: LOAD
109: LOAD
110: DIV
111: LOAD
112: LOAD
113: LOAD
114: LOAD
115: DIV
116: CLEAR
117: LOAD
118: LOAD
119: LOAD
120: CLEAR
121: LOAD
122: LOAD
123: LOAD
124: LOAD
125: LOAD
126: STORE
127: STORE
128: AND
129: CLEAR
130: REAL
131: LOAD
132: LOAD
133: ?unknown function
134: LOAD
135: EXP
136: STORE
137: STORE
138: CLEAR
139: LOAD
140: STORE
141: CLEAR
142: LOAD
143: FN1
144: STORE
145: CLEAR
146: LOAD
147: LOAD
148: LOAD
149: LOAD
150: LOAD
151: LOAD
152: REAL
153: LOAD
154: LTE
155: JUMP_IF_FALSE
156: STORE
157: CLEAR
158: JUMP
159: LOAD
160: REAL
161: LOAD
162: REAL
163: LOAD
164: LTE
165: LOAD
166: STORE
167: JUMP_IF_FALSE
168: LOAD
169: REAL
170: STORE
171: JUMP
172: JUMP_LABEL
173: STORE
174: END_INIT
175: LOAD
176: FN2
177: LOAD
178: PWR
179: LOAD
180: LOAD
181: MOD
182: ADD
183: LOAD
184: LTE
185: STORE

=== Load Table ===
  0: Mutation
  1: of
  2: 'Zexpe
  3: '.
  4: Original
  5: formula
  6: by
  7: Lee
  8: Skinner
  9: Modified
 10: for
 11: if..else
 12: logic
 13: 3
 14: 19
 15: 97
 16: by
 17: Sylvie
 18: Gallet
 19: for
 20: 'Zexpe
 21: '
 22: set
 23: fn1
 24: ident
 25: and
 26: default
 27: default
 28: 100
 29: 1.,0.
 30: pixel
 31: pixel
 32: The
 33: next
 34: line
 35: sets
 36: 100
 37: p1
 38: 0
 39: p1
 40: p1
 41: 0
 42: 100
 43: p1
 44: z
 45: s
 46: c
 47: z
 48: test

=== Store Table ===
  0: fn2
  1: p1
  2: p1
  3: s
  4: z
  5: c
  6: test
  7: test
  8: test
  9: test
 10: z

=== Jump Control Table ===
  0: IF       -> op: 65 load: 39 store:  7 dest:  2
  1: ELSE     -> op:  0 load:  0 store:  0 dest:  0
  2: IF       -> op: 78 load: 44 store:  9 dest:  4
  3: ELSE     -> op: 79 load: 44 store:  9 dest:  5
  4: END_IF   -> op:  0 load:  0 store:  0 dest:  0

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

TEST_F(ParserTest, DISABLED_moe)
{
    std::string errors;

    ASSERT_TRUE(parse_formula(s_moe, errors)) << "Errors: " << errors;

    EXPECT_EQ(s_moe_state, get_parser_state());
}
