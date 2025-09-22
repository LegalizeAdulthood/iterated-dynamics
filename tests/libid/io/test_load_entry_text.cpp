// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/load_entry_text.h>

#include "test_data.h"

#include <boost/algorithm/string/split.hpp>
#include <gtest/gtest.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string_view>

using namespace id::io;
using namespace id::test::data;

namespace id::test
{

static void position_to_line_starting_with(std::FILE*entry_file, const std::string_view text)
{
    long pos;
    constexpr int LINE_LENGTH{132};
    char line[LINE_LENGTH]{};
    do
    {
        pos = std::ftell(entry_file);
        ASSERT_NE(nullptr, std::fgets(line, sizeof(line), entry_file));
    } while (std::strncmp(line, text.data(), text.length()) != 0);
    ASSERT_EQ(0, std::fseek(entry_file, pos, SEEK_SET));
}

constexpr const char *FRACTINT_FORMULA{
R"frm(Fractint { ; Sylvie Gallet, 1996
           ; Modified for if..else logic 3/21/97 by Sylvie Gallet
           ; requires 'periodicity=0'
   ; It uses Newton's formula applied to the equation z^6-1 = 0 and, in the
   ; foreground, spells out the word 'FRACTINT'.
   z = pixel-0.025 , x = real(z) , y = imag(z) , text = 0
   if (y > -0.225 && y < 0.225)
      x1 = x*1.8 , x3 = 3*x
      ty2 = y < 0.025 && y > -0.025 || y > 0.175
      if ( x < -1.2 || ty2 && x > -1.25 && x < -1 )
         text = 1
      elseif ( x < -0.9 || ty2 && x > -0.95 && x < -0.8                  \
               || (cabs(sqrt(|z+(0.8,-0.1)|)-0.1) < 0.025 && x > -0.8)   \
               || (y < -x1-1.44 && y > -x1-1.53 && y < 0.025) )
         text = 1
      elseif ( y > x3+1.5 || y > -x3-1.2 || (y > -0.125 && y < -0.075)   \
               && y < x3+1.65 && y < -x3-1.05 )
         text = 1
      elseif ( cabs(sqrt(|z+0.05|)-0.2) < 0.025 && x < 0.05 )
         text = 1
      elseif ( (x > 0.225 && x < 0.275 || y > 0.175) && x > 0.1 && x < 0.4 )
         text = 1
      elseif ( x > 0.45 && x < 0.5 )
         text = 1
      elseif ( x < 0.6 || x > 0.8 || ((y > -x1+1.215) && (y < -x1+1.305))  \
               && x > 0.55 && x < 0.85 )
         text = 1
      elseif ( x > 1.025 && x < 1.075 || y > 0.175 && x > 0.9 && x < 1.2 )
         text = 1
      endif
   endif
   z = 1 + (0.0,-0.65) / (pixel+(0.0,.75))
   :
   if (text == 0)
      z2 = z*z , z4 = z2*z2 , n = z4*z2-1 , z = z-n/(6*z4*z)
      if (|n| >= 0.0001)
         continue = 1
      else
         continue = 0
      endif
   endif
   continue
}
)frm"
};

static std::vector<std::string> split_lines(std::string_view text)
{
    std::vector<std::string> result;
    boost::algorithm::split(result, text, [](const char c) { return c == '\n'; });
    return result;
}

TEST(TestLoadEntryText, columnZeroOneLine)
{
    char result[2048]{};
    constexpr int max_lines{1};
    constexpr int start_row{};
    constexpr int start_col{};
    std::filesystem::path frm{ID_TEST_FRM_DIR};
    frm /= ID_TEST_FRM_FILE;
    std::FILE *entry_file{std::fopen(frm.string().c_str(), "r")};
    ASSERT_NE(nullptr, entry_file) << "Couldn't open '" << frm.string() << "', errno=" << errno;
    ASSERT_NO_FATAL_FAILURE(position_to_line_starting_with(entry_file, "Fractint"));

    load_entry_text(entry_file, result, max_lines, start_row, start_col);

    const std::vector expected{split_lines(FRACTINT_FORMULA)};
    const std::vector actual{split_lines(result)};
    EXPECT_EQ(1U, actual.size());
    EXPECT_EQ(expected[0], actual[0]);
}

TEST(TestLoadEntryText, columnTwoOneLine)
{
    char result[2048]{};
    constexpr int max_lines{1};
    constexpr int start_row{};
    constexpr int start_col{2};
    std::filesystem::path frm{ID_TEST_FRM_DIR};
    frm /= ID_TEST_FRM_FILE;
    std::FILE *entry_file{std::fopen(frm.string().c_str(), "r")};
    ASSERT_NE(nullptr, entry_file) << "Couldn't open '" << frm.string() << "', errno=" << errno;
    ASSERT_NO_FATAL_FAILURE(position_to_line_starting_with(entry_file, "Fractint"));

    load_entry_text(entry_file, result, max_lines, start_row, start_col);

    const std::vector expected{split_lines(FRACTINT_FORMULA)};
    const std::vector actual{split_lines(result)};
    EXPECT_EQ(1U, actual.size());
    EXPECT_EQ(expected[0].substr(start_col), actual[0]);
}

} // namespace id::test
