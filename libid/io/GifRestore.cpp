// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/GifRestore.h"

#include <gif_lib.h>

#include <stdexcept>

namespace id::io
{

GifRestore::GifRestore(
    const std::filesystem::path &path, const int skip_x, const int skip_y, const bool potential_16bit) :
    m_skip_x{skip_x},
    m_skip_y{skip_y},
    m_potential_16bit{potential_16bit}
{
    if (skip_x < 0 || skip_y < 0)
    {
        throw std::logic_error("Invalid GIF restore skip factors");
    }
    int error{};
    m_input.reset(DGifOpenFileName(path.string().c_str(), &error));
    if (!m_input)
    {
        finish(-1);
        return;
    }
    initialize();
}

void GifRestore::initialize()
{
    const ColorMapObject *palette{m_input->SColorMap};
    if (palette == nullptr || m_input->SWidth <= 0 || m_input->SHeight <= 0)
    {
        finish(-1);
        return;
    }
    m_width = m_input->SWidth;
    m_height = m_input->SHeight;
    m_palette.reserve(palette->ColorCount);
    for (int i = 0; i < palette->ColorCount; ++i)
    {
        const GifColorType &color{palette->Colors[i]};
        m_palette.push_back({color.Red, color.Green, color.Blue});
    }
}

void GifRestore::iterate()
{
    switch (m_state)
    {
    case State::BLOCK:
    case State::IMAGE_END:
        read_block();
        break;
    case State::EXTENSION:
        read_extension();
        break;
    case State::IMAGE:
    case State::LINE:
        decode_line();
        break;
    case State::DONE:
        break;
    }
}

void GifRestore::read_block()
{
    GifRecordType record{};
    if (DGifGetRecordType(m_input.get(), &record) == GIF_ERROR)
    {
        finish(-1);
        return;
    }
    switch (record)
    {
    case TERMINATE_RECORD_TYPE:
        finish(0);
        break;
    case EXTENSION_RECORD_TYPE:
        m_state = State::EXTENSION;
        break;
    case IMAGE_DESC_RECORD_TYPE:
        read_image();
        break;
    default:
        finish(-1);
        break;
    }
}

void GifRestore::read_extension()
{
    int code{};
    GifByteType *block{};
    if (DGifGetExtension(m_input.get(), &code, &block) == GIF_ERROR)
    {
        finish(-1);
        return;
    }
    while (block != nullptr)
    {
        if (DGifGetExtensionNext(m_input.get(), &block) == GIF_ERROR)
        {
            finish(-1);
            return;
        }
    }
    m_state = State::BLOCK;
}

void GifRestore::read_image()
{
    if (DGifGetImageDesc(m_input.get()) == GIF_ERROR)
    {
        finish(-1);
        return;
    }
    const GifImageDesc &image{m_input->Image};
    m_image = {image.Left, image.Top, image.Width, image.Height};
    if (image.Width <= 0 || image.Height <= 0 || (m_potential_16bit && image.Width % 2 != 0))
    {
        finish(-1);
        return;
    }
    // Keep global palette selection and the existing non-interlaced order.
    m_decoded_line.resize(m_potential_16bit ? image.Width / 2 : image.Width);
    m_row = 0;
    m_low_byte = false;
    m_state = State::IMAGE;
}

void GifRestore::decode_line()
{
    while (m_row < m_image.height)
    {
        if (DGifGetLine(m_input.get(), m_decoded_line.data(), static_cast<int>(m_decoded_line.size())) == GIF_ERROR)
        {
            finish(-1);
            return;
        }
        const bool keep{static_cast<unsigned int>(m_row) % (static_cast<unsigned int>(m_skip_y) + 1) == 0};
        if (m_potential_16bit && !m_low_byte)
        {
            m_low_byte = true;
        }
        else
        {
            m_low_byte = false;
            ++m_row;
        }
        if (keep)
        {
            m_line.clear();
            const std::size_t stride{static_cast<std::size_t>(m_skip_x) + 1};
            for (std::size_t col = 0; col < m_decoded_line.size(); col += stride)
            {
                m_line.push_back(m_decoded_line[col]);
            }
            m_state = State::LINE;
            return;
        }
    }
    // The last pixel read also consumes the compressed-data terminator.
    m_state = State::IMAGE_END;
}

void GifRestore::finish(int result)
{
    if (m_input)
    {
        int error{};
        if (DGifCloseFile(m_input.release(), &error) == GIF_ERROR && result == 0)
        {
            result = -1;
        }
    }
    m_result = result;
    m_state = State::DONE;
}

void GifRestore::close_file(GifFileType *gif)
{
    int error{};
    DGifCloseFile(gif, &error);
}

} // namespace id::io
