// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace id::io
{

class GifDecoder
{
public:
    // The restore operation owns the input file.
    GifDecoder(std::FILE *input, int line_width, int skip_x = 0, int skip_y = 0);
    GifDecoder(const GifDecoder &) = delete;
    GifDecoder(GifDecoder &&) = delete;
    GifDecoder &operator=(const GifDecoder &) = delete;
    GifDecoder &operator=(GifDecoder &&) = delete;

    // TODO: removed later
    int decode();

private:
    static constexpr int MAX_CODES{4095};

    int initialize();
    short get_next_code();

    std::FILE *m_input;
    int m_line_width;
    int m_skip_x;
    int m_skip_y;
    std::array<std::uint16_t, MAX_CODES + 1> m_prefix{};
    std::array<Byte, 10000> m_suffix{};
    std::array<short, MAX_CODES + 1> m_string_size{};
    std::array<Byte, 4096> m_stack{};
    std::array<Byte, 257> m_byte_buffer{};
    std::vector<Byte> m_line;
    short m_size{};
    short m_current_size{};
    short m_top_slot{};
    short m_clear{};
    short m_ending{};
    short m_new_codes{};
    short m_slot{};
    short m_old_code{};
    short m_current_code{};
    short m_code{};
    short m_available_bytes{};
    short m_bits_left{};
    Byte m_current_byte{};
    unsigned short m_return_code{};
    Byte m_out_value{};
    int m_byte_offset{};
    int m_stack_size{};
    int m_line_offset{};
    int m_buffer_count{};
    int m_x_skip{};
    int m_y_skip{};
    int m_bad_code_count{};
    int m_result{};
    bool m_fast_loop{};
    bool m_done{};
};

} // namespace id::io
