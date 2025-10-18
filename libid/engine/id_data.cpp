// SPDX-License-Identifier: GPL-3.0-only
//
#include "engine/id_data.h"

#include <config/port.h>

#include "misc/id.h"
#include "ui/video_mode.h"

#include "helpdefs.h"

#include <filesystem>

using namespace id::help;
using namespace id::misc;
using namespace id::ui;

namespace id::engine
{

VideoInfo g_video_entry{};                                    //
HelpLabels g_help_mode{};                                     //
int g_adapter{};                                              // Video Adapter chosen from list in ...h
std::filesystem::path g_fractal_search_dir1;                  //
std::filesystem::path g_fractal_search_dir2;                  //
int g_screen_x_dots{}, g_screen_y_dots{};                     // # of dots on the physical screen
int g_logical_screen_x_offset{}, g_logical_screen_y_offset{}; // physical top left of logical screen
int g_logical_screen_x_dots{}, g_logical_screen_y_dots{};     // # of dots on the logical screen
double g_logical_screen_x_size_dots{}, g_logical_screen_y_size_dots{}; // xdots-1, ydots-1
int g_colors{256};                                                     // maximum colors available
long g_max_iterations{};                                               // try this many iterations
LDouble g_delta_x{}, g_delta_y{};                                      // screen pixel increments
LDouble g_delta_x2{}, g_delta_y2{};                                    // screen pixel increments
double g_delta_min{};                                                  // same as a double
double g_params[MAX_PARAMS]{};                                         // parameters
double g_potential_params[3]{};                                        // three potential parameters
ConfigStatus g_bad_config{};                                           // 'id.cfg' ok?
bool g_has_inverse{};                                                  //
                      // user_xxx is what the user wants, vs what we may be forced to do
int g_user_periodicity_value{};         //
long g_user_distance_estimator_value{}; //
bool g_view_window{};                   // false for full screen, true for window
float g_view_reduction{};               // window auto-sizing
bool g_view_crop{};                     // true to crop default coords
float g_final_aspect_ratio{};           // for view shape and rotation
int g_view_x_dots{}, g_view_y_dots{};   // explicit view sizing
bool g_keep_aspect_ratio{};             // true to keep virtual aspect
bool g_z_scroll{};                      // screen/zoombox false fixed, true relaxed
                                        // variables defined by the command line/files processor
bool g_compare_gif{};                   // compare two gif files flag
int g_save_system{};                    // from and for save files
bool g_tab_mode{true};                  // tab display enabled
double g_save_x_min{}, g_save_x_max{};  //
double g_save_y_min{}, g_save_y_max{};  //
double g_save_x_3rd{}, g_save_y_3rd{};  // displayed screen corners
double g_plot_mx1{}, g_plot_mx2{};      //
double g_plot_my1{}, g_plot_my2{};      // real->screen multipliers
CalcStatus g_calc_status{CalcStatus::NO_FRACTAL};               //
long g_calc_time{};                                             //
SaveDAC g_save_dac{};                                           // save-the-Video DAC flag
bool g_browsing{};                                              // browse mode flag
FilenameStack g_filename_stack;                                 // array of file names used while browsing
double g_smallest_window_display_size{};                        //
int g_smallest_box_size_shown{};                                //
bool g_browse_sub_images{true};                                 //
bool g_auto_browse{};                                           //
bool g_confirm_file_deletes{};                                  //
bool g_browse_check_fractal_params{};                           //
bool g_browse_check_fractal_type{};                             //
std::filesystem::path g_browse_mask;                            //
int g_scale_map[12]{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}; // array for mapping notes to a (user defined) scale

} // namespace id::engine
