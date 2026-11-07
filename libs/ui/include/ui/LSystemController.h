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

class LSystemController
{
public:
    explicit LSystemController(engine::NonStandardFractal &calculation);
    ~LSystemController();
    LSystemController(const LSystemController &) = delete;
    LSystemController(LSystemController &&) = delete;
    LSystemController &operator=(const LSystemController &) = delete;
    LSystemController &operator=(LSystemController &&) = delete;

    void start();
    void iterate();
    bool done() const;
    // L-system returns zero; calculation status distinguishes interruption.
    int result() const;
    void suspend();

private:
    enum class Phase
    {
        READY,
        ACTIVE,
        COMPLETE
    };

    bool update_thinking(bool sizing);
    void clear_thinking();
    void close(int result);

    engine::NonStandardFractal &m_calculation;
    unsigned char m_poll_counter{};
    bool m_thinking_active{};
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
