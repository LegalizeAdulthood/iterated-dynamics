// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "io/GifDecoder.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <vector>

namespace id::io
{

class GifRestore
{
public:
    enum class State
    {
        BLOCK,
        EXTENSION,
        IMAGE,
        LINE,
        IMAGE_END,
        DONE
    };

    struct Image
    {
        int left{};
        int top{};
        int width{};
        int height{};
    };

    explicit GifRestore(
        const std::filesystem::path &path, int skip_x = 0, int skip_y = 0, bool potential_16bit = false);
    GifRestore(const GifRestore &) = delete;
    GifRestore(GifRestore &&) = delete;
    GifRestore &operator=(const GifRestore &) = delete;
    GifRestore &operator=(GifRestore &&) = delete;

    // Advance one format block, extension sub-block, or decoded line.
    void iterate();

    State state() const
    {
        return m_state;
    }

    bool done() const
    {
        return m_state == State::DONE;
    }

    int result() const
    {
        return m_result;
    }

    int width() const
    {
        return m_width;
    }

    int height() const
    {
        return m_height;
    }

    const std::vector<std::array<Byte, 3>> &palette() const
    {
        return m_palette;
    }

    const Image &image() const
    {
        return m_image;
    }

    // LINE data remains valid until the next iterate() call.
    std::vector<Byte> &line()
    {
        return m_line;
    }

    const std::vector<Byte> &line() const
    {
        return m_line;
    }

private:
    void initialize();
    void read_block();
    void read_extension();
    void read_image();
    void decode_line();
    bool read_bytes(Byte *buffer, std::size_t size);
    void finish(int result);

    std::unique_ptr<std::FILE, decltype(&std::fclose)> m_input{nullptr, std::fclose};
    std::unique_ptr<GifDecoder> m_decoder;
    int m_skip_x;
    int m_skip_y;
    bool m_potential_16bit;
    State m_state{State::BLOCK};
    int m_result{};
    int m_width{};
    int m_height{};
    std::vector<std::array<Byte, 3>> m_palette;
    Image m_image;
    std::vector<Byte> m_line;
};

} // namespace id::io
