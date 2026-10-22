// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "ui/KeyboardHandler.h"
#include "ui/ShowDotController.h"

#include <optional>

namespace id::fractals
{
class Lyapunov;
}

namespace id::ui
{

class LyapunovController
{
public:
    explicit LyapunovController(fractals::Lyapunov &lyapunov);

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

    fractals::Lyapunov &m_lyapunov;
    ShowDotController m_marker;
    std::optional<ScopedKeyboardHandler> m_main_loop_scope;
    std::optional<ScopedKeyboardHandler> m_orbit_toggle_scope;
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
