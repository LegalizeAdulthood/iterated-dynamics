// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

// This file contains prototypes for unix/linux specific functions.

int stricmp(char const *, char const *);
int strnicmp(char const *, char const *, int);

void load_dac();
