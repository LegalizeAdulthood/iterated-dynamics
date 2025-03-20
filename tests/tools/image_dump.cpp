// SPDX-License-Identifier: GPL-3.0-only
//
#include <gif_lib.h>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

class GIFInputFile
{
public:
    explicit GIFInputFile(const std::string &path) :
        m_path(path),
        m_gif(DGifOpenFileName(m_path.c_str(), &m_gif_error))
    {
        if (m_gif_error != D_GIF_SUCCEEDED)
        {
            throw std::runtime_error(
                "Unexpected error opening " + std::string{path} + " for reading: " + std::to_string(m_gif_error));
        }
    }
    ~GIFInputFile()
    {
        close(nullptr);
    }

    // clang-format off
    int screen_width() const                    { return m_gif->SWidth; }
    int screen_height() const                   { return m_gif->SHeight; }
    int color_resolution() const                { return m_gif->SColorResolution; }
    const ColorMapObject &color_map() const     { return *m_gif->SColorMap; }
    int num_images() const                      { return m_gif->ImageCount; }
    const SavedImage &get_image(int i) const    { return m_gif->SavedImages[i]; }
    // clang-format on

    void slurp()
    {
        const int result = DGifSlurp(m_gif);
        if (result != GIF_OK)
        {
            throw std::runtime_error("Unexpected error slurping image");
        }
    }

    int close(int *error)
    {
        int result{GIF_OK};
        if (m_gif != nullptr)
        {
            result = DGifCloseFile(m_gif, error);
            m_gif = nullptr;
        }
        return result;
    }

    explicit operator GifFileType *() const
    {
        return m_gif;
    }

private:
    std::string m_path;
    GifFileType *m_gif;
    int m_gif_error{};
};

inline bool operator==(const GifColorType &lhs, const GifColorType &rhs)
{
    return lhs.Red == rhs.Red     //
        && lhs.Green == rhs.Green //
        && lhs.Blue == rhs.Blue;
}
inline bool operator!=(const GifColorType &lhs, const GifColorType &rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const ColorMapObject &lhs, const ColorMapObject &rhs)
{
    const bool result = lhs.ColorCount == rhs.ColorCount //
        && lhs.BitsPerPixel == rhs.BitsPerPixel          //
        && lhs.SortFlag == rhs.SortFlag;
    if (result)
    {
        for (int i = 0; i < lhs.ColorCount; ++i)
        {
            if (lhs.Colors[i] != rhs.Colors[i])
            {
                return false;
            }
        }
    }
    return result;
}
inline bool operator!=(const ColorMapObject &lhs, const ColorMapObject &rhs)
{
    return !(lhs == rhs);
}

inline std::ostream &operator<<(std::ostream &str, const GifColorType &value)
{
    return str << fmt::format("#{0:02x}{1:02x}{2:02x} {0:3} {1:3} {2:3}", static_cast<int>(value.Red), static_cast<int>(value.Green),
               static_cast<int>(value.Blue));
}

inline std::ostream &operator<<(std::ostream &str, const ColorMapObject &value)
{
    str << R"({ "count": )" << value.ColorCount << R"(, "bits": )" << value.BitsPerPixel << R"(, "sort": )"
        << value.SortFlag << R"(, "colors": [)" << '\n';
    bool first{true};
    for (int i = 0; i < value.ColorCount; ++i)
    {
        if (!first)
        {
            str << ",\n";
        }
        str << "    " << value.Colors[i];
        first = false;
    }
    return str << "\n    ]\n}";
}

inline bool operator==(const GifImageDesc &lhs, const GifImageDesc &rhs)
{
    const bool result = lhs.Left == rhs.Left //
        && lhs.Top == rhs.Top                //
        && lhs.Width == rhs.Width            //
        && lhs.Height == rhs.Height          //
        && lhs.Interlace == rhs.Interlace;
    if (!result)
    {
        return false;
    }
    if ((lhs.ColorMap == nullptr && rhs.ColorMap != nullptr) || (lhs.ColorMap != nullptr && rhs.ColorMap == nullptr))
    {
        return false;
    }
    return (lhs.ColorMap == nullptr && rhs.ColorMap == nullptr) || *lhs.ColorMap == *rhs.ColorMap;
}
inline bool operator!=(const GifImageDesc &lhs, const GifImageDesc &rhs)
{
    return !(lhs == rhs);
}

std::ostream &operator<<(std::ostream &str, const GifImageDesc &value)
{
    str << R"({ "Left": )" << value.Left           //
        << R"(, "Top": )" << value.Top             //
        << R"(, "Width": )" << value.Width         //
        << R"(, "Height": )" << value.Height       //
        << R"(, "Interlace": )" << value.Interlace //
        << R"(, "ColorMap": [ )";
    if (value.ColorMap != nullptr)
    {
        str << *value.ColorMap;
    }
    else
    {
        str << "(none)";
    }
    return str << "    ]\n}";
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 2)
    {
        std::cout << "Usage: " << argv[0] << ": file\n";
        return 1;
    }
    const std::string file{argv[1]};
    if (!std::filesystem::exists(file))
    {
        std::cout << file << " does not exist.\n";
        return 1;
    }

    try
    {
        GIFInputFile gif{file};
        gif.slurp();

        std::cout << file << ":\n";
        std::cout << "    Screen width: " << gif.screen_width() << '\n';
        std::cout << "    Screen height: " << gif.screen_height() << '\n';
        std::cout << "    Color resolution: " << gif.color_resolution() << '\n';
        std::cout << "    Color map: " << gif.color_map() << '\n';
        std::cout << "    Number of images: " << gif.num_images() << '\n';
        for (int i = 0; i < gif.num_images(); ++i)
        {
            const SavedImage &image{gif.get_image(i)};
            std::cout << "    Image " << i << ": " << image.ImageDesc << '\n';
            for (int y = 0; y < image.ImageDesc.Height; ++y)
            {
                std::cout << fmt::format("        {:4}: ", y);
                for (int x = 0; x < image.ImageDesc.Width; ++x)
                {
                    const GifByteType pixel{image.RasterBits[y * image.ImageDesc.Width + x]};
                    if (x > 0)
                    {
                        if (x % 16 == 0)
                        {
                            std::cout << "\n              ";
                        }
                        else
                        {
                            std::cout << ' ';
                        }
                    }
                    std::cout << fmt::format("{:3}", pixel);
                }
                std::cout << '\n';
            }
        }
    }
    catch (const std::exception &bang)
    {
        std::cout << "Unexpected exception: " << bang.what() << '\n';
        return 100;
    }
    catch (...)
    {
        std::cout << "Unexpected exception\n";
        return 200;
    }

    return 0;
}
