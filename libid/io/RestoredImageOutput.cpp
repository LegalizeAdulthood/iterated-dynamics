// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/RestoredImageOutput.h"

#include "engine/calcfrac.h"

#include <stdexcept>

using namespace id::engine;

namespace id::io
{

RestoredImageOutput::RestoredImageOutput(const int width, const int row, const int column) :
    m_width{width},
    m_row{row},
    m_column{column}
{
    if (width <= 0 || row < 0 || column < 0 || column >= width)
    {
        throw std::logic_error("Invalid restored-image output position");
    }
}

void RestoredImageOutput::set_line(const Byte *pixels, const int line_len)
{
    if (!done())
    {
        throw std::logic_error("Restored-image output still has pending pixels");
    }
    if (line_len < 0 || (line_len > 0 && pixels == nullptr))
    {
        throw std::logic_error("Invalid restored-image output line");
    }
    if (line_len == 0)
    {
        m_pixels.clear();
    }
    else
    {
        m_pixels.assign(pixels, pixels + line_len);
    }
    m_offset = 0;
}

void RestoredImageOutput::iterate()
{
    if (done())
    {
        return;
    }
    g_put_color(m_column, m_row, m_pixels[m_offset]);
    ++m_offset;
    if (++m_column == m_width)
    {
        m_column = 0;
        ++m_row;
    }
}

// TODO: removed later
Byte RestoredImageOutput::pending_color() const
{
    if (done())
    {
        throw std::logic_error("Restored-image output has no pending pixel");
    }
    return m_pixels[m_offset];
}

} // namespace id::io
