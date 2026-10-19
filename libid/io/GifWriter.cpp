// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#include "io/GifWriter.h"

#include <gif_lib.h>

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace id::io
{

GifWriter::GifWriter(const std::filesystem::path &path, const Screen &screen, const Palette &palette) :
    m_output{m_file},
    m_width{screen.width},
    m_height{screen.height},
    m_colors{static_cast<int>(palette.size())}
{
    validate(screen, palette);
    m_file.open(path, std::ios::binary | std::ios::trunc);
    if (!m_file)
    {
        set_error(E_GIF_ERR_OPEN_FAILED);
        return;
    }
    initialize(screen, palette);
}

GifWriter::GifWriter(std::ostream &output, const Screen &screen, const Palette &palette) :
    m_output{output},
    m_width{screen.width},
    m_height{screen.height},
    m_colors{static_cast<int>(palette.size())}
{
    validate(screen, palette);
    if (!m_output)
    {
        set_error(E_GIF_ERR_OPEN_FAILED);
        return;
    }
    initialize(screen, palette);
}

GifWriter::~GifWriter()
{
    close();
}

void GifWriter::validate(const Screen &screen, const Palette &palette)
{
    constexpr int MAX_DIMENSION{std::numeric_limits<std::uint16_t>::max()};
    if (screen.width <= 0 || screen.width > MAX_DIMENSION || screen.height <= 0 || screen.height > MAX_DIMENSION ||
        palette.size() < 2 || palette.size() > 256 || (palette.size() & (palette.size() - 1)) != 0 ||
        screen.color_resolution < 1 || screen.color_resolution > 8 || screen.background < 0 ||
        static_cast<std::size_t>(screen.background) >= palette.size())
    {
        throw std::logic_error("Invalid GIF screen or palette");
    }
}

void GifWriter::initialize(const Screen &screen, const Palette &palette)
{
    m_line.reserve(screen.width);
    std::unique_ptr<ColorMapObject, decltype(&GifFreeMapObject)> colors{
        GifMakeMapObject(static_cast<int>(palette.size()), nullptr), GifFreeMapObject};
    if (!colors)
    {
        set_error(E_GIF_ERR_NOT_ENOUGH_MEM);
        return;
    }
    for (std::size_t i = 0; i < palette.size(); ++i)
    {
        colors->Colors[i] = {palette[i][0], palette[i][1], palette[i][2]};
    }
    m_gif = EGifOpen(this, write_bytes, &m_error);
    if (m_gif == nullptr)
    {
        return;
    }
    EGifSetGifVersion(m_gif, true);
    m_gif->AspectByte = screen.aspect;
    if (!check(EGifPutScreenDesc(
            m_gif, screen.width, screen.height, screen.color_resolution, screen.background, colors.get())))
    {
        return;
    }
    check(EGifPutImageDesc(m_gif, 0, 0, screen.width, screen.height, false, nullptr));
}

int GifWriter::write_bytes(GifFileType *gif, const Byte *bytes, const int count) noexcept
{
    GifWriter &writer{*static_cast<GifWriter *>(gif->UserData)};
    if (writer.m_error != 0)
    {
        return 0;
    }
    // Some giflib operations ignore callback failures, so retain them here.
    try
    {
        writer.m_output.write(reinterpret_cast<const char *>(bytes), count);
        if (writer.m_output)
        {
            return count;
        }
    }
    catch (...)
    {
    }
    writer.set_error(E_GIF_ERR_WRITE_FAILED);
    return 0;
}

void GifWriter::set_error(const int error)
{
    if (m_error == 0)
    {
        m_error = error;
    }
}

bool GifWriter::check(const int result)
{
    if (result == GIF_ERROR)
    {
        set_error(m_gif->Error != 0 ? m_gif->Error : E_GIF_ERR_WRITE_FAILED);
    }
    return m_error == 0;
}

bool GifWriter::write_pixels(const std::vector<Byte> &pixels)
{
    if (m_finished)
    {
        throw std::logic_error("GIF writer is closed");
    }
    if (m_error != 0)
    {
        return false;
    }
    if (m_row == m_height || pixels.empty() || pixels.size() > static_cast<std::size_t>(m_width - m_col) ||
        std::any_of(pixels.begin(), pixels.end(), [this](const Byte pixel) { return pixel >= m_colors; }))
    {
        throw std::logic_error("Invalid GIF pixel row");
    }
    // giflib masks its input in place. Keep the caller's pixels unchanged.
    m_line.assign(pixels.begin(), pixels.end());
    if (!check(EGifPutLine(m_gif, m_line.data(), static_cast<int>(m_line.size()))))
    {
        return false;
    }
    m_col += static_cast<int>(pixels.size());
    if (m_col == m_width)
    {
        m_col = 0;
        ++m_row;
    }
    return true;
}

bool GifWriter::write_extensions(const GifFileType &extensions)
{
    if (m_finished)
    {
        throw std::logic_error("GIF writer is closed");
    }
    if (m_error != 0)
    {
        return false;
    }
    if (m_row != m_height || extensions.ExtensionBlockCount < 0 ||
        (extensions.ExtensionBlockCount > 0 && extensions.ExtensionBlocks == nullptr))
    {
        throw std::logic_error("Invalid GIF extension sequence");
    }
    for (int i = 0; i < extensions.ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{extensions.ExtensionBlocks[i]};
        if ((i == 0 && block.Function == CONTINUE_EXT_FUNC_CODE) || block.Function < 0 || block.Function > 255 ||
            block.ByteCount <= 0 || block.ByteCount > 255 || block.Bytes == nullptr)
        {
            throw std::logic_error("Invalid GIF extension block");
        }
    }
    bool active{};
    for (int i = 0; i < extensions.ExtensionBlockCount; ++i)
    {
        const ExtensionBlock &block{extensions.ExtensionBlocks[i]};
        if (block.Function != CONTINUE_EXT_FUNC_CODE)
        {
            if (active && !check(EGifPutExtensionTrailer(m_gif)))
            {
                return false;
            }
            if (!check(EGifPutExtensionLeader(m_gif, block.Function)))
            {
                return false;
            }
            active = true;
        }
        if (!check(EGifPutExtensionBlock(m_gif, block.ByteCount, block.Bytes)))
        {
            return false;
        }
    }
    return !active || check(EGifPutExtensionTrailer(m_gif));
}

bool GifWriter::finish()
{
    if (!m_finished && m_error == 0 && m_row != m_height)
    {
        throw std::logic_error("GIF raster is incomplete");
    }
    close();
    return m_error == 0;
}

void GifWriter::close() noexcept
{
    if (m_finished)
    {
        return;
    }
    m_finished = true;
    if (m_gif != nullptr)
    {
        int error{};
        if (EGifCloseFile(std::exchange(m_gif, nullptr), &error) == GIF_ERROR)
        {
            set_error(error != 0 ? error : E_GIF_ERR_CLOSE_FAILED);
        }
    }
    try
    {
        m_output.flush();
        if (!m_output)
        {
            set_error(E_GIF_ERR_CLOSE_FAILED);
        }
    }
    catch (...)
    {
        set_error(E_GIF_ERR_CLOSE_FAILED);
    }
    if (m_file.is_open())
    {
        m_file.close();
        if (!m_file)
        {
            set_error(E_GIF_ERR_CLOSE_FAILED);
        }
    }
}

} // namespace id::io
