// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "io/gif_extensions.h"
#include "ui/evolve.h"

#include <vector>

struct GifFileType;

#define INFO_ID         "Fractal"

namespace id::io
{

enum
{
    GIF_EXTENSION1_FRACTAL_INFO_LENGTH = 504,
    GIF_EXTENSION3_ITEM_NAME_INFO_LENGTH = 66,
    GIF_EXTENSION6_EVOLVER_INFO_LENGTH = 200,
    GIF_EXTENSION7_ORBIT_INFO_LENGTH = 200,
};

FractalInfo get_fractal_info(GifFileType *gif);
void put_fractal_info(GifFileType *gif, const FractalInfo &info);

FormulaInfo get_formula_info(const GifFileType *gif);
void put_formula_info(GifFileType *gif, const FormulaInfo &info);

std::vector<int> get_ranges_info(const GifFileType *gif);
void put_ranges_info(GifFileType *gif, const std::vector<int> &info);

ui::EvolutionInfo get_evolution_info(const GifFileType *gif);
void put_evolution_info(GifFileType *gif, const ui::EvolutionInfo &info);

OrbitsInfo get_orbits_info(const GifFileType *gif);
void put_orbits_info(GifFileType *gif, const OrbitsInfo &info);

std::vector<char> get_extended_param_info(const GifFileType *gif);
void put_extended_param_info(GifFileType *gif, const std::vector<char> &params);

} // namespace id::io
