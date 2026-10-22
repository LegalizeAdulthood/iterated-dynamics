// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/calcfrac.h"
#include "fractals/Ant.h"
#include "fractals/Bifurcation.h"
#include "fractals/Cellular.h"
#include "fractals/Diffusion.h"
#include "fractals/FrothyBasin.h"
#include "fractals/julibrot.h"
#include "fractals/lorenz.h"
#include "fractals/lsystem.h"
#include "fractals/Plasma.h"
#include "fractals/popcorn.h"
#include "fractals/TestPoint.h"
#include "geometry/StereoView.h"

#include <variant>

namespace id::engine
{

class NonStandardFractal : public ImageCalculation
{
public:
    enum class Operation
    {
        BIFURCATION,
        POPCORN,
        DIFFUSION,
        TEST_POINT,
        ANT,
        CELLULAR,
        PLASMA,
        L_SYSTEM,
        FROTHY_BASIN,
        IFS_2D,
        IFS_3D,
        ORBIT_3D,
        ORBIT_2D,
        DYNAMIC_2D,
        INVERSE_JULIA,
        STANDARD_4D
    };

    NonStandardFractal() = default;
    ~NonStandardFractal();

    Operation select();
    void select_ant_on_image();
    bool done() const;
    bool view_done() const;
    void next_view();
    bool preparing() const;
    bool sizing() const;
    std::string prepare_palette();
    bool consume_orbit_overlay_displayed();
    void scrub_overlay();
    bool potential_storage_required() const;
    void complete_potential_storage(bool available);
    bool inverse_julia_queue_required() const;
    void complete_inverse_julia_queue(bool available);
    void iterate();
    bool consume_batch_complete();
    void suspend();
    bool completed() const;
    void finish(bool interrupted);

private:
    enum StateIndex
    {
        NO_OPERATION,
        BIFURCATION,
        POPCORN,
        DIFFUSION,
        TEST_POINT,
        ANT,
        CELLULAR,
        PLASMA,
        L_SYSTEM,
        FROTHY_BASIN,
        IFS_2D,
        IFS_3D,
        ORBIT_3D,
        ORBIT_2D,
        DYNAMIC_2D,
        INVERSE_JULIA,
        STANDARD_4D
    };

    struct NoOperation
    {
    };

    void prepare();

    geometry::StereoView m_stereo_view;
    std::variant<NoOperation, fractals::Bifurcation, fractals::Popcorn, fractals::Diffusion, fractals::TestPoint,
        fractals::Ant, fractals::Cellular, fractals::Plasma, fractals::LSystem, fractals::FrothyBasin, fractals::IFS2D,
        fractals::IFS3D, fractals::Orbit3D, fractals::Orbit2D, fractals::Dynamic2D, fractals::InverseJuliaFractal,
        fractals::Standard4D>
        m_state{NoOperation{}};
    bool m_done{true};
};

} // namespace id::engine
