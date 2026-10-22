// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/ImageRenderContext.h"

#include "engine/cmdfiles.h"
#include "misc/Driver.h"
#include "math/biginit.h"
#include "ui/calculate_image.h"
#include "ui/SoundSession.h"
#include "ui/standard_orbit_plot.h"
#include "ui/stop_msg.h"

#include <memory>
#include <new>
#include <stdexcept>
#include <string>

using namespace id::engine;
using namespace id::misc;

namespace id::ui
{
namespace
{

class NonStandardRenderKeyboardHandler : public KeyboardHandler
{
public:
    explicit NonStandardRenderKeyboardHandler(bool &interrupted);
    bool handle_key(int key) override;

private:
    bool &m_interrupted;
};

NonStandardRenderKeyboardHandler::NonStandardRenderKeyboardHandler(bool &interrupted) :
    m_interrupted{interrupted}
{
}

bool NonStandardRenderKeyboardHandler::handle_key(const int key)
{
    if (g_show_orbit)
    {
        scrub_orbit_display();
    }
    if (key == 'o' || key == 'O')
    {
        if (!driver_is_disk())
        {
            g_show_orbit = !g_show_orbit;
        }
        return true;
    }
    driver_unget_key(key);
    m_interrupted = true;
    return true;
}

} // namespace

ImageRenderContext::ImageRenderContext(SoundSession &sound) :
    m_sound{sound}
{
}

ImageRenderContext::~ImageRenderContext()
{
    if (m_phase == Phase::ACTIVE)
    {
        try
        {
            abort();
        }
        catch (...)
        {
        }
    }
}

void ImageRenderContext::start()
{
    if (m_phase != Phase::READY)
    {
        throw std::logic_error("The image render context has already started");
    }
    m_phase = Phase::ACTIVE;
    try
    {
        switch (m_operation.select())
        {
        case ImageRenderOperation::Renderer::STANDARD:
            m_presentation.emplace<STANDARD>(m_operation.standard(), m_sound).start();
            break;
        case ImageRenderOperation::Renderer::LYAPUNOV:
            m_presentation.emplace<LYAPUNOV>(m_operation.lyapunov()).start();
            break;
        case ImageRenderOperation::Renderer::NON_STANDARD:
            start_non_standard();
            break;
        }
        if (presentation_done())
        {
            finish();
        }
    }
    catch (...)
    {
        fail(std::current_exception());
    }
}

void ImageRenderContext::start_non_standard()
{
    m_non_standard = true;
    m_outer_keyboard_scope.emplace(std::make_shared<NonStandardRenderKeyboardHandler>(m_non_standard_interrupted));
    NonStandardFractal &calculation{m_operation.non_standard()};
    complete_image_setup(calculation, calculation.start());
    switch (calculation.select())
    {
    case NonStandardFractal::Operation::BIFURCATION:
        m_presentation.emplace<BIFURCATION>(calculation).start();
        break;
    case NonStandardFractal::Operation::POPCORN:
        m_presentation.emplace<POPCORN>(calculation).start();
        break;
    case NonStandardFractal::Operation::DIFFUSION:
        m_presentation.emplace<DIFFUSION>(calculation).start();
        break;
    case NonStandardFractal::Operation::TEST_POINT:
        m_presentation.emplace<TEST_POINT>(calculation).start();
        break;
    case NonStandardFractal::Operation::ANT:
        m_presentation.emplace<ANT>(calculation).start();
        break;
    case NonStandardFractal::Operation::CELLULAR:
        m_presentation.emplace<CELLULAR>(calculation).start();
        break;
    case NonStandardFractal::Operation::PLASMA:
        m_presentation.emplace<PLASMA>(calculation).start();
        break;
    case NonStandardFractal::Operation::L_SYSTEM:
        m_presentation.emplace<L_SYSTEM>(calculation).start();
        break;
    case NonStandardFractal::Operation::FROTHY_BASIN:
        m_presentation.emplace<FROTHY_BASIN>(calculation, m_sound).start();
        break;
    case NonStandardFractal::Operation::IFS_2D:
        m_presentation.emplace<IFS_2D>(calculation).start();
        break;
    case NonStandardFractal::Operation::IFS_3D:
    case NonStandardFractal::Operation::ORBIT_3D:
        m_presentation.emplace<STEREO>(calculation, m_sound).start();
        break;
    case NonStandardFractal::Operation::ORBIT_2D:
    case NonStandardFractal::Operation::DYNAMIC_2D:
        m_presentation.emplace<ORBIT>(calculation, m_sound).start();
        break;
    case NonStandardFractal::Operation::INVERSE_JULIA:
        m_presentation.emplace<INVERSE_JULIA>(calculation).start();
        break;
    case NonStandardFractal::Operation::STANDARD_4D:
        m_presentation.emplace<STANDARD_4D>(calculation).start();
        break;
    }
}

void ImageRenderContext::iterate()
{
    if (m_phase == Phase::READY)
    {
        throw std::logic_error("The image render context has not started");
    }
    if (done())
    {
        return;
    }
    try
    {
        switch (m_presentation.index())
        {
        case STANDARD:
            std::get<STANDARD>(m_presentation).iterate();
            break;
        case LYAPUNOV:
            std::get<LYAPUNOV>(m_presentation).iterate();
            break;
        case BIFURCATION:
            std::get<BIFURCATION>(m_presentation).iterate();
            break;
        case POPCORN:
            std::get<POPCORN>(m_presentation).iterate();
            break;
        case DIFFUSION:
            std::get<DIFFUSION>(m_presentation).iterate();
            break;
        case TEST_POINT:
            std::get<TEST_POINT>(m_presentation).iterate();
            break;
        case ANT:
            std::get<ANT>(m_presentation).iterate();
            break;
        case CELLULAR:
            std::get<CELLULAR>(m_presentation).iterate();
            break;
        case PLASMA:
            std::get<PLASMA>(m_presentation).iterate();
            break;
        case L_SYSTEM:
            std::get<L_SYSTEM>(m_presentation).iterate();
            break;
        case FROTHY_BASIN:
            std::get<FROTHY_BASIN>(m_presentation).iterate();
            break;
        case IFS_2D:
            std::get<IFS_2D>(m_presentation).iterate();
            break;
        case STEREO:
            std::get<STEREO>(m_presentation).iterate();
            break;
        case ORBIT:
            std::get<ORBIT>(m_presentation).iterate();
            break;
        case INVERSE_JULIA:
            std::get<INVERSE_JULIA>(m_presentation).iterate();
            break;
        case STANDARD_4D:
            std::get<STANDARD_4D>(m_presentation).iterate();
            break;
        default:
            throw std::logic_error("No render presentation is selected");
        }
        if (presentation_done())
        {
            finish();
        }
    }
    catch (...)
    {
        fail(std::current_exception());
    }
}

bool ImageRenderContext::presentation_done() const
{
    switch (m_presentation.index())
    {
    case STANDARD:
        return std::get<STANDARD>(m_presentation).done();
    case LYAPUNOV:
        return std::get<LYAPUNOV>(m_presentation).done();
    case BIFURCATION:
        return std::get<BIFURCATION>(m_presentation).done();
    case POPCORN:
        return std::get<POPCORN>(m_presentation).done();
    case DIFFUSION:
        return std::get<DIFFUSION>(m_presentation).done();
    case TEST_POINT:
        return std::get<TEST_POINT>(m_presentation).done();
    case ANT:
        return std::get<ANT>(m_presentation).done();
    case CELLULAR:
        return std::get<CELLULAR>(m_presentation).done();
    case PLASMA:
        return std::get<PLASMA>(m_presentation).done();
    case L_SYSTEM:
        return std::get<L_SYSTEM>(m_presentation).done();
    case FROTHY_BASIN:
        return std::get<FROTHY_BASIN>(m_presentation).done();
    case IFS_2D:
        return std::get<IFS_2D>(m_presentation).done();
    case STEREO:
        return std::get<STEREO>(m_presentation).done();
    case ORBIT:
        return std::get<ORBIT>(m_presentation).done();
    case INVERSE_JULIA:
        return std::get<INVERSE_JULIA>(m_presentation).done();
    case STANDARD_4D:
        return std::get<STANDARD_4D>(m_presentation).done();
    default:
        throw std::logic_error("No render presentation is selected");
    }
}

bool ImageRenderContext::done() const
{
    return m_phase == Phase::COMPLETE;
}

int ImageRenderContext::result() const
{
    if (!done())
    {
        throw std::logic_error("The image render context has not completed");
    }
    return m_result;
}

void ImageRenderContext::suspend()
{
    if (m_phase != Phase::ACTIVE)
    {
        throw std::logic_error("Suspension requires an active image render context");
    }
    try
    {
        switch (m_presentation.index())
        {
        case STANDARD:
            std::get<STANDARD>(m_presentation).suspend();
            break;
        case LYAPUNOV:
            std::get<LYAPUNOV>(m_presentation).suspend();
            break;
        case BIFURCATION:
            std::get<BIFURCATION>(m_presentation).suspend();
            break;
        case POPCORN:
            std::get<POPCORN>(m_presentation).suspend();
            break;
        case DIFFUSION:
            std::get<DIFFUSION>(m_presentation).suspend();
            break;
        case TEST_POINT:
            std::get<TEST_POINT>(m_presentation).suspend();
            break;
        case ANT:
            std::get<ANT>(m_presentation).suspend();
            break;
        case CELLULAR:
            std::get<CELLULAR>(m_presentation).suspend();
            break;
        case PLASMA:
            std::get<PLASMA>(m_presentation).suspend();
            break;
        case L_SYSTEM:
            std::get<L_SYSTEM>(m_presentation).suspend();
            break;
        case FROTHY_BASIN:
            std::get<FROTHY_BASIN>(m_presentation).suspend();
            break;
        case IFS_2D:
            std::get<IFS_2D>(m_presentation).suspend();
            break;
        case STEREO:
            std::get<STEREO>(m_presentation).suspend();
            break;
        case ORBIT:
            std::get<ORBIT>(m_presentation).suspend();
            break;
        case INVERSE_JULIA:
            std::get<INVERSE_JULIA>(m_presentation).suspend();
            break;
        case STANDARD_4D:
            std::get<STANDARD_4D>(m_presentation).suspend();
            break;
        default:
            throw std::logic_error("No render presentation is selected");
        }
        finish();
    }
    catch (...)
    {
        fail(std::current_exception());
    }
}

void ImageRenderContext::finish()
{
    if (m_non_standard && driver_key_pressed() != 0)
    {
        dispatch_keyboard_key(driver_get_key());
    }
    m_operation.finish(m_non_standard_interrupted);
    m_presentation.emplace<NO_PRESENTATION>();
    m_outer_keyboard_scope.reset();
    m_result = g_calc_status == CalcStatus::COMPLETED ? 0 : -1;
    m_phase = Phase::COMPLETE;
}

void ImageRenderContext::abort()
{
    m_phase = Phase::COMPLETE;
    m_result = -1;
    m_presentation.emplace<NO_PRESENTATION>();
    m_outer_keyboard_scope.reset();
    m_operation.abort();
    m_sound.cancel();
}

void ImageRenderContext::fail(const std::exception_ptr failure)
{
    abort();
    std::string diagnostic;
    try
    {
        std::rethrow_exception(failure);
    }
    catch (const math::PrecisionError &)
    {
        throw;
    }
    catch (const std::bad_alloc &)
    {
        diagnostic = "Insufficient free memory for calculation";
    }
    catch (const std::runtime_error &error)
    {
        diagnostic = error.what();
    }
    stop_msg(diagnostic);
    if (g_init_batch != BatchMode::NONE)
    {
        g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;
    }
}

} // namespace id::ui
