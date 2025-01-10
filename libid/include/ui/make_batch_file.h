// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum
{
    MAX_MAX_LINE_LENGTH = 128, // upper limit for g_max_line_length for PARs
    MIN_MAX_LINE_LENGTH = 40   // lower limit for g_max_line_length for PARs
};

extern bool g_make_parameter_file;
extern bool g_make_parameter_file_map;
extern int g_max_line_length;

void make_batch_file();
