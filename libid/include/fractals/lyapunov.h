// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <array>
#include <memory>

namespace id::engine
{

class StandardFractal;

}

namespace id::fractals
{

struct LyapunovSequence
{
    int length{};
    std::array<int, 34> rxy{};
};

class Lyapunov
{
public:
    Lyapunov();
    ~Lyapunov();

    Lyapunov(const Lyapunov &) = delete;
    Lyapunov(Lyapunov &&) = delete;
    Lyapunov &operator=(const Lyapunov &) = delete;
    Lyapunov &operator=(Lyapunov &&) = delete;

    void resume();
    void suspend();
    bool done() const;
    void iterate();

private:
    std::unique_ptr<engine::StandardFractal> m_standard_fractal;
};

LyapunovSequence build_lyapunov_sequence(long order);
bool lyapunov_per_image();
int lyapunov_type();
int lyapunov_orbit();

} // namespace id::fractals
