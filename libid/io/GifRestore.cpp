// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/GifRestore.h"

#include "io/loadfile.h"

#include <cstring>
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
    m_input.reset(std::fopen(path.string().c_str(), "rb"));
    if (!m_input)
    {
        finish(-1);
        return;
    }
    initialize();
}

void GifRestore::initialize()
{
    std::array<Byte, 13> header{};
    if (!read_bytes(header.data(), header.size()))
    {
        return;
    }
    if (std::strncmp(reinterpret_cast<const char *>(header.data()), "GIF", 3) != 0 || header[3] < '0' ||
        header[3] > '9' || header[4] < '0' || header[4] > '9' || header[5] < 'A' || header[5] > 'z' ||
        (header[10] & 0x80) == 0)
    {
        finish(-1);
        return;
    }
    m_width = read_gif_uint16(&header[6]);
    m_height = read_gif_uint16(&header[8]);
    const int colors{1 << ((header[10] & 0x0F) + 1)};
    if (colors > 256)
    {
        finish(-1);
        return;
    }
    m_palette.resize(colors);
    for (std::array<Byte, 3> &color : m_palette)
    {
        if (!read_bytes(color.data(), color.size()))
        {
            return;
        }
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
    switch (std::getc(m_input.get()))
    {
    case ';':
        finish(0);
        break;
    case '!':
        if (std::getc(m_input.get()) < 0)
        {
            finish(-1);
        }
        else
        {
            m_state = State::EXTENSION;
        }
        break;
    case ',':
        read_image();
        break;
    default:
        finish(-1);
        break;
    }
}

void GifRestore::read_extension()
{
    const int size{std::getc(m_input.get())};
    if (size < 0)
    {
        finish(-1);
    }
    else if (size == 0)
    {
        m_state = State::BLOCK;
    }
    else
    {
        std::array<Byte, 255> buffer{};
        read_bytes(buffer.data(), size);
    }
}

void GifRestore::read_image()
{
    std::array<Byte, 9> descriptor{};
    if (!read_bytes(descriptor.data(), descriptor.size()))
    {
        return;
    }
    m_image = {read_gif_uint16(&descriptor[0]), read_gif_uint16(&descriptor[2]), read_gif_uint16(&descriptor[4]),
        read_gif_uint16(&descriptor[6])};
    // Preserve the existing non-interlaced, global-palette restore path.
    if ((descriptor[8] & 0x80) != 0)
    {
        const int colors{1 << ((descriptor[8] & 0x0F) + 1)};
        if (colors > 256)
        {
            finish(-1);
            return;
        }
        std::array<Byte, 3> color{};
        for (int i = 0; i < colors; ++i)
        {
            if (!read_bytes(color.data(), color.size()))
            {
                return;
            }
        }
    }
    const int width{m_potential_16bit ? m_image.width >> 1 : m_image.width};
    m_decoder = std::make_unique<GifDecoder>(m_input.get(), width, m_skip_x, m_skip_y);
    if (m_decoder->result() < 0)
    {
        finish(m_decoder->result());
        return;
    }
    m_state = State::IMAGE;
}

void GifRestore::decode_line()
{
    const bool decoded{m_decoder->next_line()};
    if (std::ferror(m_input.get()) || std::feof(m_input.get()))
    {
        finish(-1);
    }
    else if (decoded)
    {
        m_line = m_decoder->line();
        m_state = State::LINE;
    }
    else if (m_decoder->result() < 0)
    {
        finish(m_decoder->result());
    }
    else if (std::getc(m_input.get()) != 0)
    {
        finish(-1);
    }
    else
    {
        m_decoder.reset();
        m_state = State::IMAGE_END;
    }
}

bool GifRestore::read_bytes(Byte *buffer, const std::size_t size)
{
    if (std::fread(buffer, 1, size, m_input.get()) == size)
    {
        return true;
    }
    finish(-1);
    return false;
}

void GifRestore::finish(const int result)
{
    m_result = result;
    m_decoder.reset();
    m_input.reset();
    m_state = State::DONE;
}

} // namespace id::io
