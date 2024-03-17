#pragma once

#include <string>

extern BYTE                  g_block[];

int savetodisk(char *filename);
int savetodisk(std::string &filename);
bool encoder();
int new_to_old(int new_fractype);
