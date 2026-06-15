// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/double_to_string.h"

#include <string>

#include <fmt/format.h>

namespace id::ui
{

std::string double_to_string(const double val)
{
    constexpr std::size_t length = 20;
    // cellular needs 16
    for (int i = 16; i > 0; --i)
    {
        std::string result{fmt::format("{:.{}g}", val, i)};
        if (result.length() <= length)
        {
            return result;
        }
    }
    return fmt::format("{:.1g}", val);
}

} // namespace id::ui
