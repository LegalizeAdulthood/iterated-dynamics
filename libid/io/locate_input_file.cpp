// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/locate_input_file.h"

#include "engine/id_data.h"
#include "io/special_dirs.h"

#include <filesystem>
#include <initializer_list>
#include <string>

using namespace id::engine;

namespace id::io
{

std::string locate_input_file(const std::string &name)
{
    std::initializer_list dirs{
        std::filesystem::current_path(), g_save_dir, g_fractal_search_dir1, g_fractal_search_dir2};
    for (std::filesystem::path path : dirs)
    {
        path /= name;
        if (std::filesystem::exists(path))
        {
            return path.string();
        }
    }

    return {};
}

} // namespace id::io
