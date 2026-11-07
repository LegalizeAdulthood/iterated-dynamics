// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class TestPointController
{
public:
    explicit TestPointController(engine::NonStandardFractal &calculation);

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
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
