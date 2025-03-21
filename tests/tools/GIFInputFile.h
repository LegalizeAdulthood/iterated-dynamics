// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <gif_lib.h>

#include <string>

namespace id
{

class GIFInputFile
{
public:
    explicit GIFInputFile(const std::string &path);

    ~GIFInputFile();

    // clang-format off
    int screen_width() const                    { return m_gif->SWidth; }
    int screen_height() const                   { return m_gif->SHeight; }
    int color_resolution() const                { return m_gif->SColorResolution; }
    const ColorMapObject &color_map() const     { return *m_gif->SColorMap; }
    int num_images() const                      { return m_gif->ImageCount; }
    const SavedImage &get_image(int i) const    { return m_gif->SavedImages[i]; }
    // clang-format on

    void slurp();

    int close(int *error);

    explicit operator GifFileType *() const
    {
        return m_gif;
    }

private:
    std::string m_path;
    GifFileType *m_gif;
    int m_gif_error{};
};

} // namespace id
