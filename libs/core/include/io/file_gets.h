// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <cstdio>

namespace id::io
{

int file_gets(char *buf, int max_len, std::FILE *infile);

} // namespace id::io
