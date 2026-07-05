// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "fractals/lorenz.h"

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

    struct OrbitState
    {
        fractals::Affine screen_convert{};
        long count{};
        int color{1};
        bool active{};
        bool seed_color_selected{};
    };

    void advance_after_seed();
    void advance_line();
    bool advance_orbit();
    bool done() const;
    bool draw_function();
    bool draw_line();
    bool draw_rectangle();
    int encoded_orbit_color() const;
    void initialize();
    void initialize_function();
    void initialize_line();
    void initialize_rectangle();
    bool line_done() const;
    void save_current_work();
    void select_orbit_color();
    void start_orbit();

    bool m_initialized{};
    bool m_interrupted{};
    Traversal m_traversal{Traversal::START};
    int m_row{};
    int m_col{};
    LineState m_line{};
    FunctionState m_function{};
    OrbitState m_orbit{};
};

} // namespace id::engine
