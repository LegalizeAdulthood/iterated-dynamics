// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/locate_input_file.h"

#include "engine/id_data.h"
#include "io/special_dirs.h"
#include "ui/cmdfiles.h"

#include <filesystem>
#include <initializer_list>
#include <string>

std::string locate_input_file(const std::string &name)
{
    std::vector dirs{g_save_dir, g_fractal_search_dir1, g_fractal_search_dir2};
    if (g_check_cur_dir)
    {
        dirs.insert(dirs.begin(), std::filesystem::current_path());
    }
    else
    {
        dirs.push_back(std::filesystem::current_path());
    }
    for (std::filesystem::path path : dirs)
    {
        path /= name;
        if (exists(path))
        {
            return path.string();
        }
    }

    return {};
}
