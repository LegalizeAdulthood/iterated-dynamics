// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "math/cmplx.h"

namespace id::engine
{

struct ImageRegion
{
    math::DComplex size() const
    {
        return m_max - m_min;
    }

    math::DComplex size3() const
    {
        return m_3rd - m_min;
    }

    double width() const
    {
        return m_max.x - m_min.x;
    }

    double width3() const
    {
        return m_3rd.x - m_min.x;
    }

    double height() const
    {
        return m_max.y - m_min.y;
    }

    double height3() const
    {
        return m_3rd.y - m_min.y;
    }

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
