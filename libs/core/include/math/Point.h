// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::math
{

template <typename T>
struct Point2
{
    T x;
    T y;
};

template <typename T>
bool operator==(const Point2<T> &lhs, const Point2<T> &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator!=(const Point2<T> &lhs, const Point2<T> &rhs)
{
    return !(lhs == rhs);
}

using Point2i = Point2<int>;

} // namespace id::math
