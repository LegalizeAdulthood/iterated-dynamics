// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <exception>

namespace id::misc
{

// Attempt every action, then rethrow the original or first cleanup failure.
template <typename... Actions>
void run_cleanup(std::exception_ptr failure, Actions &&...actions)
{
    const auto attempt = [&failure](auto &action)
    {
        try
        {
            action();
        }
        catch (...)
        {
            if (!failure)
            {
                failure = std::current_exception();
            }
        }
    };
    (attempt(actions), ...);
    if (failure)
    {
        std::rethrow_exception(failure);
    }
}

} // namespace id::misc
