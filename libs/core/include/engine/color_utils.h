// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <config/port.h>

namespace id::engine
{

/// Expands a 6-bit DAC color channel to the full 8-bit range.
inline Byte expand_6bit_color(const Byte color)
{
    const Byte expanded = color << 2;
    return expanded + (expanded >> 6);
}

/// Expands an 8-bit color channel whose low two bits are zero.
inline Byte expand_8bit_color(const Byte value)
{
    return expand_6bit_color(value >> 2U);
}

} // namespace id::engine
