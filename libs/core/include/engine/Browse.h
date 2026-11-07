// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace id::engine
{

using BrowsePathStack = std::vector<std::filesystem::path>;

struct Browse
{
    bool auto_browse{};                  //
    bool browsing{};                     // browse mode flag
    bool check_fractal_params{};         //
    bool check_fractal_type{};           //
    bool sub_images{true};               //
    bool confirm_delete{};               //
    int smallest_box{};                  //
    double smallest_window{};            //
    std::filesystem::path mask;          //
    std::filesystem::path selected_path; // selected browse file path
    std::string name;                    // name for browse file display
    BrowsePathStack stack;               // array of file paths used while browsing
};

extern Browse g_browse;

} // namespace id::engine
