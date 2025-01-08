// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>

enum
{
    NUM_IFS_PARAMS = 7,
    NUM_IFS_3D_PARAMS = 13
};

extern int                   g_num_affine_transforms;

char *get_ifs_token(char *buf, std::FILE *ifs_file);
char *get_next_ifs_token(char *buf, std::FILE *ifs_file);

int ifs_load();
