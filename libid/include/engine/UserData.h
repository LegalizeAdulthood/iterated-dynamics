// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "calcfrac.h"

namespace id::engine
{

enum class PerturbationMode
{
    AUTO = 0,
    YES = 1,
    NO = 2
};

constexpr double DEFAULT_PERTURBATION_TOLERANCE{1e-6};

// user_xxx is what the user wants,
// vs what we may be forced to do
struct UserData
{
    int biomorph_value{};            //
    long distance_estimator_value{}; //
    PerturbationMode perturbation{}; //
    double perturbation_tolerance{DEFAULT_PERTURBATION_TOLERANCE};
    int periodicity_value{};         //
    CalcMode std_calc_mode{};        //
    long bailout_value{};            // user input bailout value
};

extern UserData g_user;

} // namespace id::engine
