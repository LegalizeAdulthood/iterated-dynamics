// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <gif_lib.h>

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
    if ((lhs.ColorMap == nullptr && rhs.ColorMap != nullptr) ||
        (lhs.ColorMap != nullptr && rhs.ColorMap == nullptr))
    {
        return false;
    }
    return (lhs.ColorMap == nullptr && rhs.ColorMap == nullptr) || *lhs.ColorMap == *rhs.ColorMap;
}

inline bool operator!=(const GifImageDesc &lhs, const GifImageDesc &rhs)
{
    return !(lhs == rhs);
}
