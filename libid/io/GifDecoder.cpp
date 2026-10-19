// SPDX-License-Identifier: GPL-3.0-only
//
// An LZW decoder for GIF
// Copyright (C) 1987, by Steven A. Bennett
//
// Permission is given by the author to freely redistribute and include
// this code in any program as long as this credit is given where due.
//
// In accordance with the above, I want to credit Steve Wilhite who wrote
// the code which this is heavily inspired by...
//
#include "io/GifDecoder.h"

#include "engine/pixel_limits.h"
#include "io/decoder.h"

#include <stdexcept>

using namespace id::engine;

namespace id::io
{

namespace
{

constexpr int BAD_CODE_SIZE{-20};
constexpr int WRITE_ERROR{-2};
constexpr short CODE_MASK[]{
    0, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF};

} // namespace

GifDecoder::GifDecoder(std::FILE *input, const int line_width, const int skip_x, const int skip_y) :
    m_input{input},
    m_line_width{line_width},
    m_skip_x{skip_x},
    m_skip_y{skip_y}
{
    if (input == nullptr || skip_x < 0 || skip_y < 0)
    {
        throw std::logic_error("Invalid GIF decoder input");
    }
    m_result = initialize();
    m_done = m_result < 0;
}

int GifDecoder::initialize()
{
    if (m_line_width < 1 || m_line_width > GIF_MAX_PIXELS)
    {
        return WRITE_ERROR;
    }
    m_size = static_cast<short>(std::getc(m_input));
    if (m_size < 0)
    {
        return m_size;
    }
    if (m_size < 2 || m_size > 9)
    {
        return BAD_CODE_SIZE;
    }
    m_current_size = static_cast<short>(m_size + 1);
    m_top_slot = static_cast<short>(1 << m_current_size);
    m_clear = static_cast<short>(1 << m_size);
    m_ending = static_cast<short>(m_clear + 1);
    m_new_codes = static_cast<short>(m_ending + 1);
    m_slot = m_new_codes;
    m_line.resize(m_line_width);
    m_buffer_count = m_line_width;
    return 0;
}

// TODO: removed later
int GifDecoder::decode()
{
    if (m_done)
    {
        return m_result;
    }

    // Expand prefix strings into the line buffer or the reverse-output stack.
    while ((m_current_code = get_next_code()) != m_ending)
    {
        if (m_current_code < 0)
        {
            // Preserve the legacy result for a mid-stream read error.
            m_done = true;
            return 0;
        }
        if (m_current_code == m_clear)
        {
            m_current_size = static_cast<short>(m_size + 1);
            m_slot = m_new_codes;
            m_string_size[m_slot] = 0;
            m_top_slot = static_cast<short>(1 << m_current_size);
            do
            {
                m_current_code = get_next_code();
            } while (m_current_code == m_clear);
            if (m_current_code == m_ending)
            {
                break;
            }
            if (m_current_code >= m_slot)
            {
                m_current_code = 0;
            }
            m_old_code = m_current_code;
            m_out_value = static_cast<Byte>(m_old_code);
            m_stack[m_stack_size++] = static_cast<Byte>(m_current_code);
        }
        else
        {
            m_code = m_current_code;
            if (m_code >= m_slot)
            {
                if (m_code > m_slot)
                {
                    ++m_bad_code_count;
                    m_current_code = m_slot;
                }
                m_code = m_old_code;
                m_stack[m_stack_size++] = m_out_value;
            }
            m_fast_loop = false;
            while (m_code >= m_new_codes)
            {
                int size = m_string_size[m_code];
                short remaining = static_cast<short>(size);
                if (size > 0 && m_buffer_count - size > 0 && m_skip_x == 0)
                {
                    m_fast_loop = true;
                    do
                    {
                        m_line[m_line_offset + remaining] = m_suffix[m_code];
                        m_code = m_prefix[m_code];
                    } while (--remaining > 0);
                    m_line[m_line_offset] = static_cast<Byte>(m_code);
                    m_line_offset += ++size;
                    m_buffer_count -= size;
                    if (m_buffer_count == 0)
                    {
                        if (--m_y_skip < 0)
                        {
                            const int result{g_out_line(m_line.data(), m_line_offset)};
                            if (result < 0)
                            {
                                m_result = result;
                                m_done = true;
                                return result;
                            }
                            m_y_skip = m_skip_y;
                        }
                        m_line_offset = 0;
                        m_buffer_count = m_line_width;
                        m_x_skip = 0;
                    }
                }
                else
                {
                    m_stack[m_stack_size++] = m_suffix[m_code];
                    m_code = m_prefix[m_code];
                }
            }
            if (!m_fast_loop)
            {
                m_stack[m_stack_size++] = static_cast<Byte>(m_code);
            }
            if (m_slot < m_top_slot)
            {
                m_string_size[m_slot] = static_cast<short>(m_string_size[m_old_code] + 1);
                m_out_value = static_cast<Byte>(m_code);
                m_suffix[m_slot] = m_out_value;
                m_prefix[m_slot++] = m_old_code;
                m_old_code = m_current_code;
            }
            if (m_slot >= m_top_slot && m_current_size < 12)
            {
                m_top_slot <<= 1;
                ++m_current_size;
            }
        }
        while (m_stack_size > 0)
        {
            --m_stack_size;
            if (--m_x_skip < 0)
            {
                m_x_skip = m_skip_x;
                m_line[m_line_offset++] = m_stack[m_stack_size];
            }
            if (--m_buffer_count == 0)
            {
                if (--m_y_skip < 0)
                {
                    const int result{g_out_line(m_line.data(), m_line_offset)};
                    if (result < 0)
                    {
                        m_result = result;
                        m_done = true;
                        return result;
                    }
                    m_y_skip = m_skip_y;
                }
                m_line_offset = 0;
                m_buffer_count = m_line_width;
                m_x_skip = 0;
            }
        }
    }
    m_done = true;
    return 0;
}

short GifDecoder::get_next_code()
{
    if (m_bits_left == 0)
    {
        if (m_available_bytes <= 0)
        {
            m_byte_offset = 0;
            m_available_bytes = static_cast<short>(std::getc(m_input));
            if (m_available_bytes < 0)
            {
                return m_available_bytes;
            }
            if (m_available_bytes)
            {
                std::fread(m_byte_buffer.data(), 1, m_available_bytes, m_input);
            }
        }
        m_current_byte = m_byte_buffer[m_byte_offset++];
        m_bits_left = 8;
        --m_available_bytes;
    }
    m_return_code = static_cast<short>(m_current_byte >> (8 - m_bits_left));
    while (m_current_size > m_bits_left)
    {
        if (m_available_bytes <= 0)
        {
            m_byte_offset = 0;
            m_available_bytes = static_cast<short>(std::getc(m_input));
            if (m_available_bytes < 0)
            {
                return m_available_bytes;
            }
            if (m_available_bytes)
            {
                std::fread(m_byte_buffer.data(), 1, m_available_bytes, m_input);
            }
        }
        m_current_byte = m_byte_buffer[m_byte_offset++];
        m_return_code |= m_current_byte << m_bits_left;
        m_bits_left += 8;
        --m_available_bytes;
    }
    m_bits_left -= m_current_size;
    return static_cast<short>(m_return_code & CODE_MASK[m_current_size]);
}

} // namespace id::io
