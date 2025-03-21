// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <gif_lib.h>

#include <iostream>

inline std::ostream &operator<<(std::ostream &str, const GifColorType &value)
{
    return str << "[ " << static_cast<int>(value.Red)   //
               << ", " << static_cast<int>(value.Green) //
               << ", " << static_cast<int>(value.Red) << " ]";
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

inline std::ostream &operator<<(std::ostream &str, const GifImageDesc &value)
{
    return str << R"({ "Left": )" << value.Left           //
               << R"(, "Top": )" << value.Top             //
               << R"(, "Width": )" << value.Width         //
               << R"(, "Height": )" << value.Height       //
               << R"(, "Interlace": )" << value.Interlace //
               << R"(, "ColorMap": [ )" << *value.ColorMap << "    ]\n}";
}
