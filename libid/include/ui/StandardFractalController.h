// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "ui/KeyboardHandler.h"
#include "ui/ShowDotController.h"

#include <optional>

namespace id::engine
{
class StandardFractal;
}

namespace id::ui
{

class SoundSession;

class StandardFractalController
{
public:
    StandardFractalController(engine::StandardFractal &calculation, SoundSession &sound);

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

    void finish();
    void abort();
    void close(int result);

    engine::StandardFractal &m_calculation;
    SoundSession &m_sound;
    ShowDotController m_marker;
    std::optional<ScopedKeyboardHandler> m_main_loop_scope;
    std::optional<ScopedKeyboardHandler> m_orbit_toggle_scope;
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
