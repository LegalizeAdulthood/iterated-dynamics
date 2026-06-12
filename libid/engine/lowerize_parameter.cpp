// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/lowerize_parameter.h"

#include <algos/string_algorithms.h>

#include <algorithm>
#include <string>
#include <string_view>

namespace id::engine
{

// don't convert these parameter values
static constexpr std::string_view UNCHANGED_PARAMS[] = {"autokeyname", "colors", "comment", "filename", "formulafile",
    "formulaname", "ifs", "ifs3d", "ifsfile", "lfile", "librarydirs", "lightname", "lname", "makedoc", "makepar", "map",
    "orbitsavename", "orgfrmdir", "parmfile", "rds-texture", "savename", "savedir", "tempdir", "workdir"};

void lowerize_parameter(std::string &cur_arg)
{
    const std::size_t equals{cur_arg.find('=')};
    if (equals == std::string::npos)
    {
        cur_arg = id::algos::ascii_to_lower_copy(cur_arg);
        return;
    }

    const std::string variable{id::algos::ascii_to_lower_copy(std::string_view{cur_arg}.substr(0, equals))};
    if (std::find(std::begin(UNCHANGED_PARAMS), std::end(UNCHANGED_PARAMS), std::string_view{variable}) !=
        std::end(UNCHANGED_PARAMS))
    {
        cur_arg.replace(0, equals, variable);
        return;
    }

    cur_arg = id::algos::ascii_to_lower_copy(cur_arg);
}

} // namespace id::engine
