#pragma once

#include <cstdio>

char *get_ifs_token(char *buf, std::FILE *ifsfile);
char *get_next_ifs_token(char *buf, std::FILE *ifsfile);

int ifsload();
