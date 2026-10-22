// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "ui/KeyboardHandler.h"

#include <optional>

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class SoundSession;

class OrbitController
{
public:
    OrbitController(engine::NonStandardFractal &calculation, SoundSession &sound);

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

    engine::NonStandardFractal &m_calculation;
    SoundSession &m_sound;
    std::optional<ScopedKeyboardHandler> m_keyboard_scope;
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
