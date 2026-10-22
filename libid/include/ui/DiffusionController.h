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

class DiffusionController
{
public:
    explicit DiffusionController(engine::NonStandardFractal &calculation);

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

    void close(int result);

    engine::NonStandardFractal &m_calculation;
    bool m_interrupted{};
    std::optional<ScopedKeyboardHandler> m_keyboard_scope;
    Phase m_phase{Phase::READY};
    int m_result{1};
};

} // namespace id::ui
