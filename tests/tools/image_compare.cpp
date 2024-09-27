// SPDX-License-Identifier: GPL-3.0-only
//
#include <gif_lib.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

class GIFInputFile
{
public:
    GIFInputFile(const std::string &path) :
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

    operator GifFileType *() const
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
    return str << "[ " << value.Red << ", " << value.Green << ", " << value.Red << " ]";
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
    return str << R"({ "Left": )" << value.Left           //
               << R"(, "Top": )" << value.Top             //
               << R"(, "Width": )" << value.Width         //
               << R"(, "Height": )" << value.Height       //
               << R"(, "Interlace": )" << value.Interlace //
               << R"(, "ColorMap": [ )" << *value.ColorMap << "    ]\n}";
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: " << argv[0] << ": file1 file2\n";
        return 1;
    }
    const std::string file1{argv[1]};
    const std::string file2{argv[2]};
    try
    {
        if (!std::filesystem::exists(file1))
        {
            std::cout << file1 << " does not exist.\n";
            return 1;
        }
        if (!std::filesystem::exists(file2))
        {
            std::cout << file2 << " does not exist.\n";
            return 1;
        }
    
        GIFInputFile gif1{file1};
        gif1.slurp();
        GIFInputFile gif2{file2};
        gif2.slurp();

        if (gif1.screen_width() != gif2.screen_width() || gif1.screen_height() != gif2.screen_height())
        {
            std::cout << "Image dimensions don't match: "                                        //
                      << file1 << ": " << gif1.screen_width() << "x" << gif1.screen_height() //
                      << " != "                                                                  //
                      << file2 << gif1.screen_width() << "x" << gif1.screen_height()         //
                      << '\n';
            return 1;
        }

        if (gif1.color_map() != gif2.color_map())
        {
            std::cout << "Color table doesn't match\n"
                      << file1 << ":\n"
                      << gif1.color_map() << '\n'
                      << file2 << ";\n"
                      << gif2.color_map() << '\n';
            return 1;
        }

        if (gif1.num_images() != gif2.num_images())
        {
            std::cout << "Image count doesn't match\n"
                      << file1 << ": " << gif1.num_images() << '\n'
                      << file2 << ": " << gif2.num_images() << '\n';
            return 1;
        }

        for (int i = 0; i < gif1.num_images(); ++i)
        {
            const SavedImage &image1{gif1.get_image(i)};
            const SavedImage &image2{gif2.get_image(i)};
            if (image1.ImageDesc != image2.ImageDesc)
            {
                std::cout << "Image " << i << " description doesn't match\n"
                          << file1 << ": " << image1.ImageDesc << '\n'
                          << file2 << ": " << image2.ImageDesc << '\n';
                return 1;
            }
            int pixel_count{};
            const int num_pixels{image1.ImageDesc.Width * image1.ImageDesc.Height};
            for (int j = 0; j < num_pixels; ++j)
            {
                if (image1.RasterBits[j] != image2.RasterBits[j])
                {
                    ++pixel_count;
                }
            }
            const float percentage{100.0f * static_cast<float>(pixel_count) / static_cast<float>(num_pixels)};
            if (percentage > 3.0f)
            {
                std::cout << "Images differ by " << percentage << "%\n" << file1 << "\n" << file2 << "\n";
                return 1;
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

    std::cout << file1 << " compares equal to " << file2 << '\n';
    return 0;
}
