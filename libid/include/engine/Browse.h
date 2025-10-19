// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace id::engine
{

using FilenameStack = std::vector<std::string>;

struct Browse
{
    bool auto_browse{};          //
    bool browsing{};             // browse mode flag
    bool check_fractal_params{}; //
    bool check_fractal_type{};   //
    bool sub_images{true};       //
    bool confirm_delete{};       //
    int smallest_box{};          //
    double smallest_window{};    //
    std::filesystem::path mask;  //
    std::string name;            // name for browse file
    FilenameStack stack;         // array of file names used while browsing
};

extern Browse g_browse;

} // namespace id::engine
