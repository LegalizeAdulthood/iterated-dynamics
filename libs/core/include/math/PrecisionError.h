// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <stdexcept>

namespace id::math
{

class PrecisionError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

} // namespace id::math
