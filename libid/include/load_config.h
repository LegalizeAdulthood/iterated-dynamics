#pragma once

#include <string>

extern int                   g_cfg_line_nums[];

void load_config();
void load_config(const std::string &cfg_file);
