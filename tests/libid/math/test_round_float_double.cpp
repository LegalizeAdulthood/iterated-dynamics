// SPDX-License-Identifier: GPL-3.0-only
//
#include <math/round_float_double.h>

#include <gtest/gtest.h>

namespace id::math
{

TEST(TestRoundFloatDouble, trimsFloatConversionNoise)
{
    double value{static_cast<double>(static_cast<float>(1.0 / 3.0))};

    round_float_double(&value);

    EXPECT_DOUBLE_EQ(0.3333333, value);
}

TEST(TestRoundFloatDouble, keepsSevenSignificantDigits)
{
    double value{12345.678901234};

    round_float_double(&value);

    EXPECT_DOUBLE_EQ(12345.68, value);
}

} // namespace id::math
