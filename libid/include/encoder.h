#pragma once

#include "port.h"

#include <string>

extern BYTE                  g_block[];

int save_image(std::string &filename);
bool encoder();
int new_to_old(int new_fractype);
