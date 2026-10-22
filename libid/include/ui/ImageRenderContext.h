// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "engine/ImageRenderOperation.h"
#include "ui/AntController.h"
#include "ui/BifurcationController.h"
#include "ui/CellularController.h"
#include "ui/DiffusionController.h"
#include "ui/FrothyBasinController.h"
#include "ui/IFS2DController.h"
#include "ui/InverseJuliaFractalController.h"
#include "ui/LSystemController.h"
#include "ui/LyapunovController.h"
#include "ui/OrbitController.h"
#include "ui/PlasmaController.h"
#include "ui/PopcornController.h"
#include "ui/Standard4DController.h"
#include "ui/StandardFractalController.h"
#include "ui/StereoController.h"
#include "ui/TestPointController.h"

#include <exception>
#include <optional>
#include <variant>

namespace id::ui
{

class SoundSession;

class ImageRenderContext
{
public:
    explicit ImageRenderContext(SoundSession &sound);
    ~ImageRenderContext();

    void start();
    void iterate();
    bool done() const;
    int result() const;
    void suspend();

private:
    enum class Phase
    {
        READY,
        ACTIVE,
        COMPLETE
    };

    enum PresentationIndex
    {
        NO_PRESENTATION,
        STANDARD,
        LYAPUNOV,
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
        STEREO,
        ORBIT,
        INVERSE_JULIA,
        STANDARD_4D
    };

    void start_non_standard();
    bool presentation_done() const;
    void finish();
    void abort();
    void fail(std::exception_ptr failure);

    engine::ImageRenderOperation m_operation;
    SoundSession &m_sound;
    bool m_non_standard{};
    bool m_non_standard_interrupted{};
    std::optional<ScopedKeyboardHandler> m_outer_keyboard_scope;
    std::variant<std::monostate, StandardFractalController, LyapunovController, BifurcationController,
        PopcornController, DiffusionController, TestPointController, AntController, CellularController,
        PlasmaController, LSystemController, FrothyBasinController, IFS2DController, StereoController, OrbitController,
        InverseJuliaFractalController, Standard4DController>
        m_presentation;
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
