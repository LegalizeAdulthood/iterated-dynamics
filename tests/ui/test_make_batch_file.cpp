// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/make_batch_file.h>

#include "ColorMapSaver.h"

#include <ui/rotate.h>

#include <gtest/gtest.h>

TEST(TestPutEncodedColors, increasingSixBitRamp)
{
    ColorMapSaver saved_dac_box;
    for (int i = 0; i < 256; ++i)
    {
        g_dac_box[i][0] = static_cast<Byte>(i % 64);
        g_dac_box[i][1] = static_cast<Byte>(i % 64);
        g_dac_box[i][2] = static_cast<Byte>(i % 64);
    }
    WriteBatchData data{};

    put_encoded_colors(data, 256);

    EXPECT_EQ(64, data.len);
    EXPECT_STREQ("000<56>tttuuuvvv<3>zzz000<57>uuu<4>zzz000<57>uuu<4>zzz000<62>zzz", data.buf);
}
