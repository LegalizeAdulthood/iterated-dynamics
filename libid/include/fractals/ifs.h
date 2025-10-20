// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

namespace id::fractals
{

enum
{
    NUM_IFS_2D_PARAMS = 7,
    NUM_IFS_3D_PARAMS = 13
};


enum class IFSDimension
{
    TWO = 2,
    THREE = 3,
};

extern int                   g_num_affine_transforms;
extern std::vector<float>    g_ifs_definition;
extern std::filesystem::path g_ifs_filename;
extern std::string           g_ifs_name;
extern IFSDimension          g_ifs_dim;

char *get_ifs_token(char *buf, std::FILE *ifs_file);
char *get_next_ifs_token(char *buf, std::FILE *ifs_file);

int ifs_load();

} // namespace id::fractals
