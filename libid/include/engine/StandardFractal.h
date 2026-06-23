// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::engine
{

class StandardFractal
{
public:
    void resume()
    {
        m_phase = Phase::START;
    }

    void suspend()
    {
    }

    bool done() const
    {
        return m_phase == Phase::COMPLETE;
    }

    void iterate();

private:
    enum class Phase
    {
        START,
        COMPLETE
    };

    Phase m_phase{Phase::START};
};

} // namespace id::engine
