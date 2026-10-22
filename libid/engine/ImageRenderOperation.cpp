// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/ImageRenderOperation.h"

#include "engine/OrbitPlot.h"
#include "fractals/fractalp.h"

#include <stdexcept>

using namespace id::fractals;

namespace id::engine
{

ImageRenderOperation::Renderer ImageRenderOperation::select()
{
    if (m_state.index() != NO_OPERATION)
    {
        throw std::logic_error("An image calculation is already selected");
    }
    const CalcType calc_type{g_cur_fractal_specific->calc_type};
    if (calc_type == lyapunov_type)
    {
        m_state.emplace<LYAPUNOV>();
        return Renderer::LYAPUNOV;
    }
    if (calc_type == standard_fractal_type || calc_type == calc_mandelbrot_type)
    {
        m_state.emplace<STANDARD>();
        return Renderer::STANDARD;
    }
    m_state.emplace<NON_STANDARD>();
    return Renderer::NON_STANDARD;
}

StandardFractal &ImageRenderOperation::standard()
{
    if (m_state.index() != STANDARD)
    {
        throw std::logic_error("No standard image calculation is selected");
    }
    return std::get<STANDARD>(m_state);
}

Lyapunov &ImageRenderOperation::lyapunov()
{
    if (m_state.index() != LYAPUNOV)
    {
        throw std::logic_error("No Lyapunov image calculation is selected");
    }
    return std::get<LYAPUNOV>(m_state);
}

NonStandardFractal &ImageRenderOperation::non_standard()
{
    if (m_state.index() != NON_STANDARD)
    {
        throw std::logic_error("No non-standard image calculation is selected");
    }
    return std::get<NON_STANDARD>(m_state);
}

void ImageRenderOperation::finish(const bool interrupted)
{
    switch (m_state.index())
    {
    case STANDARD:
        standard().finish();
        break;
    case LYAPUNOV:
        lyapunov().finish();
        break;
    case NON_STANDARD:
        non_standard().finish(interrupted);
        break;
    default:
        throw std::logic_error("No image calculation is selected");
    }
}

void ImageRenderOperation::abort()
{
    g_calc_status = CalcStatus::NON_RESUMABLE;
    scrub_orbit();
    switch (m_state.index())
    {
    case STANDARD:
        standard().abort();
        break;
    case LYAPUNOV:
        lyapunov().calculation().abort();
        break;
    case NON_STANDARD:
        non_standard().finish(true);
        break;
    default:
        break;
    }
}

} // namespace id::engine
