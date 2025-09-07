// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::math
{

template <typename T>
int sign(T x)
{
    return (T{} < x) - (x < T{});
}

} // namespace id::math
