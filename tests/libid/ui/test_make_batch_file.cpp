// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/make_batch_file.h>

#include "ColorMapSaver.h"

#include <ui/rotate.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <numeric>

namespace
{

class TestPutEncodedColors : public testing::Test
{
protected:
    void fill_dac_from_values(int count);
    void up_down_values(int count);
    void up_down_values(int count, int up, int down);
    void iota4(int count);
    ColorMapSaver m_saved_dac_box;
    std::array<Byte, 256> values{};
    WriteBatchData m_data{};
};

void TestPutEncodedColors::fill_dac_from_values(int count)
{
    for (int i =0; i < count; ++i)
    {
        g_dac_box[i][0] = values[i];
        g_dac_box[i][1] = values[i];
        g_dac_box[i][2] = values[i];
    }
}

void TestPutEncodedColors::up_down_values(int count)
{
    int up{0};
    int down{static_cast<Byte>(count * 2 - 1)};
    up_down_values(count, up, down);
}

void TestPutEncodedColors::up_down_values(int count, int up, int down)
{
    int offset{};
    for (int i = 0; i < count; ++i)
    {
        values[offset] = static_cast<Byte>(up * 4);
        ++up;
        ++offset;
        values[offset] = static_cast<Byte>(down * 4);
        --down;
        ++offset;
    }
}

void TestPutEncodedColors::iota4(int count)
{
    std::generate_n(values.begin(), count,
        []
        {
            static int value{};
            const int result{value};
            value += 4;
            return result;
        });
}

} // namespace

TEST_F(TestPutEncodedColors, increasingSixBitRamp)
{
    for (int i = 0; i < 256; ++i)
    {
        const Byte val{static_cast<Byte>((i % 64) * 4)};
        g_dac_box[i][0] = val;
        g_dac_box[i][1] = val;
        g_dac_box[i][2] = val;
    }

    put_encoded_colors(m_data, 256);

    EXPECT_EQ(64, m_data.len);
    EXPECT_STREQ("000<56>tttuuuvvv<3>zzz000<57>uuu<4>zzz000<57>uuu<4>zzz000<62>zzz", m_data.buf);
}

TEST_F(TestPutEncodedColors, values0Through9EncodedAsDigits)
{
    up_down_values(5);
    fill_dac_from_values(10);

    put_encoded_colors(m_data, 10);

    std::string_view expected{"000999111888222777333666444555"};
    std::string_view actual{m_data.buf, static_cast<size_t>(m_data.len)};
    EXPECT_EQ(expected, actual);
}

TEST_F(TestPutEncodedColors, values10Through35EncodedAsUpperCaseLetters)
{
    up_down_values(13, 10, 35);
    fill_dac_from_values(26);

    put_encoded_colors(m_data, 26);

    std::string_view expected{"AAAZZZBBBYYYCCCXXXDDDWWWEEEVVVFFFUUUGGGTTTHHHSSSIIIRRRJJJQQQKKKPPPLLLOOOMMMNNN"};
    std::string_view actual{m_data.buf, static_cast<size_t>(m_data.len)};
    EXPECT_EQ(expected, actual);
}

TEST_F(TestPutEncodedColors, values36Through63EncodedAsLowerCaseLetters)
{
    up_down_values(14, 36, 63);
    fill_dac_from_values(28);

    put_encoded_colors(m_data, 28);

    std::string_view expected{"___zzz"
                              "```yyy"
                              "aaaxxx"
                              "bbbwww"
                              "cccvvv"
                              "ddduuu"
                              "eeettt"
                              "fffsss"
                              "gggrrr"
                              "hhhqqq"
                              "iiippp"
                              "jjjooo"
                              "kkknnn"
                              "lllmmm"};
    std::string_view actual{m_data.buf, static_cast<size_t>(m_data.len)};
    EXPECT_EQ(expected, actual);
}

TEST_F(TestPutEncodedColors, smoothFourColors)
{
    static constexpr int NUM_COLORS{4};
    iota4(NUM_COLORS);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    std::string_view expected{"000<2>333"};
    std::string_view actual{m_data.buf, static_cast<size_t>(m_data.len)};
    EXPECT_EQ(expected, actual);
}

TEST_F(TestPutEncodedColors, smoothFiveColors)
{
    static constexpr int NUM_COLORS{5};
    iota4(NUM_COLORS);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    std::string_view expected{"000<3>444"};
    std::string_view actual{m_data.buf, static_cast<size_t>(m_data.len)};
    EXPECT_EQ(expected, actual);
}

TEST_F(TestPutEncodedColors, twoLeastLSBSetEncodedAsHex)
{
    static constexpr int NUM_COLORS{1};
    std::iota(values.begin(), values.begin() + NUM_COLORS, 64U + 3U);
    fill_dac_from_values(NUM_COLORS);

    put_encoded_colors(m_data, NUM_COLORS);

    std::string_view expected{"#434343"};
    std::string_view actual{m_data.buf, static_cast<size_t>(m_data.len)};
    EXPECT_EQ(expected, actual);
}
