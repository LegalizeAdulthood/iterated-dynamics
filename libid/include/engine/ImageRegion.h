// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::engine
{

struct ImageRegion
{
    // for historical reasons (before rotation):
    //    top    left  corner of screen is (min.x,max.y)
    //    bottom left  corner of screen is (3rd.x,3rd.y)
    //    bottom right corner of screen is (max.x,min.y)
    math::DComplex m_min;
    math::DComplex m_max;
    math::DComplex m_3rd;
};

inline bool operator==(const ImageRegion &lhs, const ImageRegion &rhs)
{
    return lhs.m_min == rhs.m_min && lhs.m_max == rhs.m_max && lhs.m_3rd == rhs.m_3rd;
}

inline bool operator!=(const ImageRegion &lhs, const ImageRegion &rhs)
{
    return !(lhs == rhs);
}

extern ImageRegion g_image_region;

} // namespace id::engine
