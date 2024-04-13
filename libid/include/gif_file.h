#pragma once

#include "evolve.h"
#include "loadfile.h"

#include <vector>

struct GifFileType;

#define INFO_ID         "Fractal"

enum
{
    GIF_EXTENSION1_FRACTAL_INFO_LENGTH = 504,
    GIF_EXTENSION3_ITEM_NAME_INFO_LENGTH = 66,
    GIF_EXTENSION6_EVOLVER_INFO_LENGTH = 200,
    GIF_EXTENSION7_ORBIT_INFO_LENGTH = 200,
};

FRACTAL_INFO get_fractal_info(GifFileType *gif);
void put_fractal_info(GifFileType *gif, const FRACTAL_INFO &info);

formula_info get_formula_info(GifFileType *gif);
void put_formula_info(GifFileType *gif, const formula_info &info);

std::vector<int> get_ranges_info(GifFileType *gif);
void put_ranges_info(GifFileType *gif, const std::vector<int> &info);

EVOLUTION_INFO get_evolution_info(GifFileType *gif);
void put_evolution_info(GifFileType *gif, const EVOLUTION_INFO &info);

ORBITS_INFO get_orbits_info(GifFileType *gif);
void put_orbits_info(GifFileType *gif, const ORBITS_INFO &info);

std::vector<char> get_extended_param_info(GifFileType *gif);
void put_extended_param_info(GifFileType *gif, const std::vector<char> &params);
