// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

template <typename T>
int sign(T x)
{
    return (T{} < x) - (x < T{});
}
