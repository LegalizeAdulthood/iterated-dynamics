#pragma once

#include <cstdio>

extern int                   g_num_affine_transforms;

char *get_ifs_token(char *buf, std::FILE *ifsfile);
char *get_next_ifs_token(char *buf, std::FILE *ifsfile);

int ifsload();
