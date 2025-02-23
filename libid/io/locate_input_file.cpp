// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/locate_input_file.h"

#include "engine/id_data.h"
#include "io/special_dirs.h"

#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>

std::string locate_input_file(const std::string &name)
{
    const std::string current_dir{std::filesystem::current_path().string()};
    std::initializer_list<std::string_view> dirs{current_dir, g_save_dir, g_fractal_search_dir1, g_fractal_search_dir2};
    for (std::string_view search_dir : dirs)
    {
        std::filesystem::path file_path{search_dir};
        file_path /= name;
        if (exists(file_path))
        {
            return file_path.string();
        }
    }

    return {};
}
