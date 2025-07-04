// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <vector>

namespace id::fractals
{

// used for periodicity checking
struct BifurcationPeriod
{
    void init();
    bool periodic(long time);

    double close_enough{};
    double saved_pop{};
    int saved_inc{};
    long saved_and{};
};

class Bifurcation
{
public:
    Bifurcation();
    ~Bifurcation() = default;

    void resume();
    void suspend();
    bool iterate();

private:
    void verhulst();

    std::vector<int> m_verhulst;
    unsigned long m_filter_cycles{};
    bool m_half_time_check{};
    bool m_mono{};
    int m_outside_x{};
    int m_x{};
    BifurcationPeriod s_period;
};

} // namespace id::fractals

int bifurc_add_trig_pi_orbit();
int bifurc_lambda_trig_orbit();
int bifurc_set_trig_pi_orbit();
int bifurc_stewart_trig_orbit();
int bifurc_verhulst_trig_orbit();
