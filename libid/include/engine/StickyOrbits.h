// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <fractals/lorenz.h>

namespace id::engine
{

class PertEngine;

enum class OrbitDrawMode
{
    NONE = 0,
    RECTANGLE = 'r',
    LINE = 'l',
    FUNCTION = 'f',
};

extern OrbitDrawMode g_draw_mode;

long sticky_orbit_chunk_limit(long count, long max_iterations);

class StickyOrbits
{
public:
    bool iterate(PertEngine *perturbation = nullptr);
    void suspend();
    void save_resume() const;
    void restore_resume(int version = 3);

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
        bool resuming{};
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
        long resume_count{};
        math::DComplex point{};
        int col{};
        int row{};
        int color{1};
        bool active{};
        bool seed_color_selected{};
        bool point_pending{};
        bool escaped{};
    };

    void advance_after_seed();
    void advance_line();
    bool advance_orbit(PertEngine *perturbation);
    bool done() const;
    bool draw_function(PertEngine *perturbation);
    bool draw_line(PertEngine *perturbation);
    bool draw_rectangle(PertEngine *perturbation);
    int encoded_orbit_color() const;
    void initialize();
    void initialize_function();
    void initialize_line();
    void initialize_rectangle();
    bool line_done() const;
    void save_current_work();
    void select_orbit_color();
    void start_orbit(PertEngine *perturbation);

    bool m_initialized{};
    Traversal m_traversal{Traversal::START};
    int m_row{};
    int m_col{};
    LineState m_line{};
    FunctionState m_function{};
    OrbitState m_orbit{};
};

} // namespace id::engine
