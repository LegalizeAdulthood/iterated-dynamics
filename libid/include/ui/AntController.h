// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include "misc/ValueSaver.h"
#include "ui/help.h"

#include <optional>

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class AntController
{
public:
    explicit AntController(engine::NonStandardFractal &calculation);

    void start();
    void iterate();
    bool done() const;
    // Ant's legacy result is zero on completion or command interruption.
    int result() const;
    void suspend();

private:
    enum class Phase
    {
        READY,
        ACTIVE,
        COMPLETE
    };

    void wait_for_delay();
    void change_wait();
    bool process_keyboard();
    void close(int result);

    engine::NonStandardFractal &m_calculation;
    std::optional<misc::ValueSaver<help::HelpLabels>> m_help_scope;
    long m_wait{};
    bool m_step{};
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
