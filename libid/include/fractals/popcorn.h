// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>

namespace id::engine
{

class StandardFractal;

}

namespace id::fractals
{

class Popcorn
{
public:
    Popcorn();
    ~Popcorn();

    Popcorn(const Popcorn &) = delete;
    Popcorn(Popcorn &&) = delete;
    Popcorn &operator=(const Popcorn &) = delete;
    Popcorn &operator=(Popcorn &&) = delete;

    void resume();
    void suspend();
    bool done() const;
    void iterate();

private:
    void complete();

    std::unique_ptr<engine::StandardFractal> m_standard_fractal;
    int m_row{};
    int m_col{};
    bool m_done{};
};

int popcorn_fractal_old();
int popcorn_fractal();
int popcorn_orbit();

} // namespace id::fractals
