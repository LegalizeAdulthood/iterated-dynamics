// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/id_data.h"

#include "misc/id.h"

#include "helpdefs.h"

#include <filesystem>

using namespace id::help;
using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

HelpLabels g_help_mode{};                         //
int g_adapter{};                                  // Video Adapter chosen from list in ...h
std::filesystem::path g_fractal_search_dir1;      //
std::filesystem::path g_fractal_search_dir2;      //
int g_screen_x_dots{}, g_screen_y_dots{};         // # of dots on the physical screen
int g_colors{256};                                // maximum colors available
long g_max_iterations{};                          // try this many iterations
double g_params[MAX_PARAMS]{};                    // parameters
bool g_has_inverse{};                             //
                                                  // variables defined by the command line/files processor
bool g_compare_gif{};                             // compare two gif files flag
int g_save_system{};                              // from and for save files
CalcStatus g_calc_status{CalcStatus::NO_FRACTAL}; //
long g_calc_time{};                               //
int g_scale_map[12]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // array for mapping notes to a (user defined) scale

} // namespace id::engine
