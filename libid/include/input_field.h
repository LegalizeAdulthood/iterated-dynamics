// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum InputFieldFlags
{
    INPUTFIELD_NUMERIC  = 1,
    INPUTFIELD_INTEGER  = 2,
    INPUTFIELD_DOUBLE   = 4
};

int input_field(int options, int attr, char *fld, int len, int row, int col,
    int (*checkkey)(int curkey));
