// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <config/port.h>

#include <filesystem>
#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

struct GifFileType;

namespace id::io
{

class GifWriter;

class ImageSave
{
public:
    explicit ImageSave(const std::filesystem::path &filename);
    // The caller keeps this stream alive until the save is destroyed.
    ImageSave(std::ostream &output, const std::filesystem::path &filename);
    ~ImageSave();
    ImageSave(const ImageSave &) = delete;
    ImageSave(ImageSave &&) = delete;
    ImageSave &operator=(const ImageSave &) = delete;
    ImageSave &operator=(ImageSave &&) = delete;

    void iterate();

    bool done() const
    {
        return m_done;
    }

    bool succeeded() const
    {
        return m_succeeded;
    }

    int rows_written() const
    {
        return m_row;
    }

    int error() const
    {
        return m_error;
    }

    const std::filesystem::path &filename() const
    {
        return m_filename;
    }

    const std::string &diagnostic() const
    {
        return m_diagnostic;
    }

private:
    ImageSave(const std::filesystem::path &filename, bool file);
    static void free_extensions(GifFileType *extensions);
    void finish(bool saved);
    bool replace_file();
    void discard_partial_file() noexcept;

    using GifFilePtr = std::unique_ptr<GifFileType, void (*)(GifFileType *)>;
    std::filesystem::path m_filename;
    std::filesystem::path m_work_path;
    std::unique_ptr<GifWriter> m_writer;
    GifFilePtr m_extensions{nullptr, free_extensions};
    std::vector<Byte> m_pixels;
    int m_width;
    int m_height;
    int m_x_offset;
    int m_y_offset;
    int m_row{};
    int m_error{};
    bool m_potential;
    bool m_file;
    bool m_replace{};
    bool m_owns_work_file{};
    bool m_done{};
    bool m_succeeded{};
    std::string m_diagnostic;
};

} // namespace id::io
