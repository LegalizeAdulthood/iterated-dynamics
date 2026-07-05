// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

enum class OrbitDrawMode
{
    NONE = 0,
    RECTANGLE = 'r',
    LINE = 'l',
    FUNCTION = 'f',
};

extern OrbitDrawMode g_draw_mode;

class StickyOrbits
{
public:
    bool iterate();
    int scan();
    void suspend();

private:
    enum class Traversal
    {
        START,
        RECTANGLE,
        LINE,
        FUNCTION,
        DONE,
    };

    struct LineState
    {
        int final{};
        int accumulator{};
        int inc1{};
        int inc2{};
        bool positive_slope{};
        bool shallow{};
    };

    struct FunctionState
    {
        int angle{};
        double rotation{};
        double x_center{};
        double y_center{};
        double x_mag_factor{};
        double x_factor{};
        double y_factor{};
    };

    void advance_line();
    bool done() const;
    bool draw_function();
    bool draw_line();
    bool draw_rectangle();
    void initialize();
    void initialize_function();
    void initialize_line();
    void initialize_rectangle();
    bool line_done() const;
    void save_current_work();

    bool m_initialized{};
    bool m_interrupted{};
    Traversal m_traversal{Traversal::START};
    int m_row{};
    int m_col{};
    LineState m_line{};
    FunctionState m_function{};
};

} // namespace id::engine
