// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

#include <string>

extern Byte                  g_block[];

int save_image(std::string &filename);
bool encoder();
