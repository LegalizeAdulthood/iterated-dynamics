// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

int field_prompt(const char *hdg, const char *instr, char *fld, int len, int (*check_key)(int key));
