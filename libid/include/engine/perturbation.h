// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

bool perturbation();

extern bool is_perturbation();
extern int get_number_references();

enum class PerturbationMode
{
    AUTO = 0, // the default
    YES = 1,
    NO = 2
};
extern PerturbationMode g_perturbation;
extern double g_perturbation_tolerance;
extern bool g_use_perturbation; // select perturbation code
