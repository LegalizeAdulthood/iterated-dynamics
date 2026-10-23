// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "engine/reset_version.h"

#include "engine/Command.h"
#include "io/loadfile.h"

using namespace id::misc;

namespace id::engine
{

std::optional<Version> parse_reset_version(const Command &cmd)
{
    if (cmd.status != CmdArgFlags::NONE || cmd.num_val < 0)
    {
        return std::nullopt;
    }

    Version version{};
    // PAR release unknown unless specified.
    if (cmd.num_int_params == 0)
    {
        version = parse_legacy_version(1730);
    }
    else if (cmd.num_int_params == 1)
    {
        if (cmd.int_vals[0] == 0)
        {
            version = parse_legacy_version(1730);
        }
        // Id version: reset=100, reset=101; legacy version: reset=1960
        else if (cmd.int_vals[0] >= 100)
        {
            const int value{cmd.int_vals[0]};
            if (value == 100 || value == 101)
            {
                version = Version{value / 100, value % 100, 0, 0, false};
            }
            else
            {
                version = parse_legacy_version(value);
            }
        }
        // Id version: reset=<major>
        else
        {
            version.major = cmd.int_vals[0];
        }
    }
    // Id version: reset=<major>/<minor>[/<patch>[/<tweak>]]
    else if (cmd.num_int_params > 1)
    {
        version.major = cmd.int_vals[0];
        version.minor = cmd.int_vals[1];
        if (cmd.num_int_params > 2)
        {
            version.patch = cmd.int_vals[2];
        }
        if (cmd.num_int_params > 3)
        {
            version.tweak = cmd.int_vals[3];
        }
    }
    return version;
}

void apply_reset_version(const Version &version)
{
    g_version = version;
    io::g_file_version = version;
}

} // namespace id::engine
