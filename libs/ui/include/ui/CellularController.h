// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <string>

namespace id::engine
{
class NonStandardFractal;
}

namespace id::ui
{

class CellularController
{
public:
    explicit CellularController(engine::NonStandardFractal &calculation);
    ~CellularController();
    CellularController(const CellularController &) = delete;
    CellularController(CellularController &&) = delete;
    CellularController &operator=(const CellularController &) = delete;
    CellularController &operator=(CellularController &&) = delete;

    void start();
    void iterate();
    bool done() const;
    // Cellular returns one on completion, minus one on interruption or error.
    int result() const;
    void suspend();

private:
    enum class Phase
    {
        READY,
        ACTIVE,
        COMPLETE
    };

    void update_thinking();
    void clear_thinking();
    void fail(const std::string &message);
    void close(int result);

    engine::NonStandardFractal &m_calculation;
    bool m_thinking_active{};
    Phase m_phase{Phase::READY};
    int m_result{-1};
};

} // namespace id::ui
