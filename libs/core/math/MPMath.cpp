// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <math/MPMath.h>

#include <fmt/format.h>

#include <algorithm>
#include <string>

namespace id::math
{

MPPrecision::MPPrecision(const int required_digits, const int user_digits)
{
    if (user_digits < 0 || user_digits > static_cast<int>(MAX_DIGITS))
    {
        throw PrecisionError(fmt::format("Requested bfdigits value of {} is unsupported", user_digits));
    }
    const int digits = user_digits > 0 ? user_digits : required_digits;
    if (digits <= 0 || digits > static_cast<int>(MAX_DIGITS))
    {
        throw PrecisionError(fmt::format("Requested precision of {} is unsupported", digits));
    }
    m_digits = static_cast<unsigned>(user_digits > 0 ? user_digits : std::max(required_digits, 3));
}

unsigned MPPrecision::requested_digits() const
{
    return m_digits;
}

unsigned MPPrecision::working_digits() const
{
    return m_digits + GUARD_DIGITS;
}

MPReal MPPrecision::real(const std::string_view value) const
{
    return MPReal{std::string{value}, working_digits()};
}

MPComplex MPPrecision::complex(const std::string_view real_part, const std::string_view imag_part) const
{
    MPComplex result{0, 0, working_digits()};
    result.real(real(real_part));
    result.imag(real(imag_part));
    return result;
}

} // namespace id::math
