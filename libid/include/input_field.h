// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

enum class InputFieldFlags
{
    NONE = 0,
    NUMERIC = 1,
    INTEGER = 2,
    DOUBLE = 4
};
inline int operator+(InputFieldFlags value)
{
    return static_cast<int>(value);
}
inline InputFieldFlags operator|(InputFieldFlags lhs, InputFieldFlags rhs)
{
    return static_cast<InputFieldFlags>(+lhs | +rhs);
}
inline InputFieldFlags& operator|=(InputFieldFlags &lhs, InputFieldFlags rhs)
{
    lhs = lhs | rhs;
    return lhs;
}
inline InputFieldFlags operator&(InputFieldFlags lhs, InputFieldFlags rhs)
{
    return static_cast<InputFieldFlags>(+lhs & +rhs);
}
inline InputFieldFlags operator&=(InputFieldFlags &lhs, InputFieldFlags rhs)
{
    lhs = lhs & rhs;
    return lhs;
}
inline bool bit_set(InputFieldFlags value, InputFieldFlags flags)
{
    return (value & flags) == flags;
}

int input_field(InputFieldFlags options, int attr, char *fld, int len, //
    int row, int col,                                                  //
    int (*check_key)(int key));
