// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "math/cmplx.h"
#include "misc/memory.h"

namespace id::engine
{

enum class JIIMType
{
    JIIM = 0,
    ORBIT
};

constexpr double JULIA_C_NOT_SET{100000.0}; //

extern bool g_has_inverse;                  //
extern math::DComplex g_julia_c;            //
extern math::DComplex g_save_c;             //

struct InverseJuliaPlot
{
    int corner_x;
    int corner_y;
    int width;
    int height;
    int x_factor;
    int y_factor;
    int x_offset;
    int y_offset;
    int bottom;
    bool protect_image;
    float zoom;
    double aspect;
    bool circles;
    bool lines;
    int secret_mode;
};

class InverseJulia
{
public:
    explicit InverseJulia(JIIMType which);

    void reset(math::DComplex point, bool use_miim);
    // Returns false when no inverse point is available for this increment.
    bool iterate(const InverseJuliaPlot &plot);
    bool active() const;

private:
    bool iterate_jiim(const InverseJuliaPlot &plot);
    void iterate_orbit(const InverseJuliaPlot &plot);

    JIIMType m_which;
    math::DComplex m_c{};
    math::DComplex m_init{};
    math::DComplex m_old_z{};
    math::DComplex m_new_z{};
    int m_count{};
    double m_r{};
    int m_x{};
    int m_y{};
    long m_iter{1};
    int m_color{};
    int m_old_x{-1};
    int m_old_y{-1};
    int m_max_hits{1};
    int m_ran_dir{};
    int m_ran_cnt{};
    bool m_use_miim{};
    bool m_actively_computing{true};
};

math::DComplex pop_float();
math::DComplex dequeue_float();
// Takes ownership of backing when storage initialization begins.
void init_queue(unsigned long capacity, misc::MemoryHandle backing);
unsigned long queue_capacity();
void free_queue();
void   clear_queue();
int    queue_empty();
int    queue_full();
int    queue_full_almost();
int    push_float(float x, float y);
int    enqueue_float(float x, float y);

} // namespace id::engine
