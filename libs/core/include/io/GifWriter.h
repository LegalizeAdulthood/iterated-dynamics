// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#pragma once

#include <config/port.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <vector>

struct GifFileType;

namespace id::io
{

class GifWriter
{
public:
    struct Screen
    {
        int width;
        int height;
        int color_resolution{8};
        int background{};
        Byte aspect{};
    };

    using Palette = std::vector<std::array<Byte, 3>>;

    GifWriter(const std::filesystem::path &path, const Screen &screen, const Palette &palette);
    // The caller owns this stream and keeps it alive until the writer is destroyed.
    GifWriter(std::ostream &output, const Screen &screen, const Palette &palette);
    ~GifWriter();
    GifWriter(const GifWriter &) = delete;
    GifWriter(GifWriter &&) = delete;
    GifWriter &operator=(const GifWriter &) = delete;
    GifWriter &operator=(GifWriter &&) = delete;

    // Accept a scanline or part of one; a write cannot cross a row boundary.
    bool write_pixels(const std::vector<Byte> &pixels);
    bool write_extensions(const GifFileType &extensions);
    // Finalize the GIF, flush output, and close an owned file. Call to check errors.
    bool finish();

    int error() const
    {
        return m_error;
    }

private:
    static void validate(const Screen &screen, const Palette &palette);
    static int write_bytes(GifFileType *gif, const Byte *bytes, int count) noexcept;
    void initialize(const Screen &screen, const Palette &palette);
    void set_error(int error);
    bool check(int result);
    void close() noexcept;

    std::ofstream m_file;
    std::ostream &m_output;
    GifFileType *m_gif{};
    int m_width;
    int m_height;
    int m_colors;
    int m_row{};
    int m_col{};
    int m_error{};
    bool m_finished{};
    std::vector<Byte> m_line;
};

} // namespace id::io
