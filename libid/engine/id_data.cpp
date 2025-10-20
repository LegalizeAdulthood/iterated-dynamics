// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/id_data.h"

#include "misc/id.h"

#include "helpdefs.h"

#include <filesystem>

using namespace id::help;
using namespace id::misc;

namespace id::engine
{

double g_params[MAX_PARAMS]{};                    // parameters
                                                  // variables defined by the command line/files processor
int g_save_system{};                              // from and for save files
int g_scale_map[12]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // array for mapping notes to a (user defined) scale

} // namespace id::engine
