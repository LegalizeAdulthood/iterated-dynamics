// SPDX-License-Identifier: GPL-3.0-only
//
#include "GIFInputFile.h"

#include <stdexcept>

namespace id
{

GIFInputFile::GIFInputFile(const std::string &path) :
    m_path(path),
    m_gif(DGifOpenFileName(m_path.c_str(), &m_gif_error))
{
    if (m_gif_error != D_GIF_SUCCEEDED)
    {
        throw std::runtime_error(
            "Unexpected error opening " + std::string{path} + " for reading: " + std::to_string(m_gif_error));
    }
}

GIFInputFile::~GIFInputFile()
{
    close(nullptr);
}

void GIFInputFile::slurp()
{
    const int result = DGifSlurp(m_gif);
    if (result != GIF_OK)
    {
        throw std::runtime_error("Unexpected error slurping image");
    }
}

int GIFInputFile::close(int *error)
{
    int result{GIF_OK};
    if (m_gif != nullptr)
    {
        result = DGifCloseFile(m_gif, error);
        m_gif = nullptr;
    }
    return result;
}

} // namespace id
