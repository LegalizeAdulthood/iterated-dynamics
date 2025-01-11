// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

#include <vector>

extern bool                  g_log_map_auto_calculate;
extern long                  g_log_map_flag;
extern int                   g_log_map_fly_calculate;
extern bool                  g_log_map_calculate;
extern std::vector<Byte>     g_log_map_table;
extern long                  g_log_map_table_max_size;

void setup_log_table();
long log_table_calc(long color_iter);
