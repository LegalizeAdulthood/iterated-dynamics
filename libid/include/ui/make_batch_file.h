// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <string>

namespace id::ui
{

struct WriteBatchData // buffer for parms to break lines nicely
{
    std::string buf;
};

extern bool g_make_parameter_file;
extern bool g_make_parameter_file_map;

void make_batch_file();
void put_calculation_mode_params(WriteBatchData &wb_data);
void put_encoded_colors(WriteBatchData &wb_data, int max_color);
void put_fractal_params(WriteBatchData &wb_data);
void put_rds_params(WriteBatchData &wb_data); // RDS options for PAR output

} // namespace id::ui
