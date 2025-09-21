// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

class Point
{
public:
    Point() = default;
    Point(const int x, const int y) :
        m_x(x),
        m_y(y)
    {
    }
    Point(const int x, const int y, const int iteration) :
        m_x(x),
        m_y(y),
        m_iteration(iteration)
    {
    }
    Point(const Point &rhs) = default;
    Point(Point &&rhs) = default;
    ~Point() = default;
    Point &operator=(const Point &rhs) = default;
    Point &operator=(Point &&rhs) = default;

    int get_x() const
    {
        return m_x;
    }
    int get_y() const
    {
        return m_y;
    }
    int get_iteration() const
    {
        return m_iteration;
    }

private:
    int m_x{};
    int m_y{};
    int m_iteration{-1};
};

} // namespace id::engine
