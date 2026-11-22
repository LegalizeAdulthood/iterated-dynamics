// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <math/PrecisionError.h>

#include <boost/multiprecision/mpc.hpp>
#include <boost/multiprecision/mpfr.hpp>

#include <string_view>

namespace id::math
{

// Eager values: no expression objects retained across assignments or yields.
using MPReal =
    boost::multiprecision::number<boost::multiprecision::mpfr_float_backend<0>, boost::multiprecision::et_off>;
using MPComplex =
    boost::multiprecision::number<boost::multiprecision::mpc_complex_backend<0>, boost::multiprecision::et_off>;

class MPPrecision
{
public:
    static constexpr unsigned MAX_DIGITS{2000}; // Accepted bfdigits range.
    static constexpr unsigned GUARD_DIGITS{8};

    // required_digits is the viewport/resolution estimate. A nonzero user
    // override replaces that estimate, matching the existing bfdigits policy.
    explicit MPPrecision(int required_digits, int user_digits = 0);
    unsigned requested_digits() const;
    unsigned working_digits() const;

    // Construct every value explicitly; never change Boost's default precision.
    MPReal real(std::string_view value = "0") const;
    MPComplex complex(std::string_view real = "0", std::string_view imag = "0") const;

private:
    unsigned m_digits;
};

} // namespace id::math
