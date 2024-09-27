// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

int field_prompt(char const *hdg, char const *instr, char *fld, int len,
    int (*checkkey)(int curkey));
