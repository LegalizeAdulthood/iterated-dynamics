// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <cstddef>
#include <vector>

namespace id::io
{

class RestoredImageOutput
{
public:
    explicit RestoredImageOutput(int width, int row = 0, int column = 0);

    void set_line(const Byte *pixels, int line_len);

    bool done() const
    {
        return m_offset >= m_pixels.size();
    }

    void iterate();

    int row() const
    {
        return m_row;
    }

    int column() const
    {
        return m_column;
    }

private:
    int m_width;
    int m_row;
    int m_column;
    std::vector<Byte> m_pixels;
    std::size_t m_offset{};
};

} // namespace id::io
