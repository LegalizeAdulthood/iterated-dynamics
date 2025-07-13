// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <gif_lib.h>
#include <fmt/core.h>

#include <iostream>

inline std::ostream &operator<<(std::ostream &str, const GifColorType &value)
{
    return str << fmt::format("#{0:02xd}{1:02xd}{2:02xd} {0:3d} {1:3d} {2:3d}", //
               static_cast<int>(value.Red), static_cast<int>(value.Green), static_cast<int>(value.Blue));
}

inline std::ostream &operator<<(std::ostream &str, const ColorMapObject &value)
{
    str << "Count: " << value.ColorCount << ", bits: " << value.BitsPerPixel << ", sort: " << value.SortFlag
        << ", colors:\n";
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
    return str << '\n';
}

std::ostream &operator<<(std::ostream &str, const GifImageDesc &value)
{
    str << fmt::format("Left: {:d}, Top: {:d}, Width: {:d}, Height: {:d}, Interlace: {}, ColorMap:", //
        value.Left, value.Top, value.Width, value.Height, value.Interlace);
    if (value.ColorMap != nullptr)
    {
        str << '\n' << *value.ColorMap;
    }
    else
    {
        str << "(none)\n";
    }
    return str << "\n";
}
