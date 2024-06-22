#pragma once

#include "modes.h"

#include <string>
#include <vector>

namespace hc
{

struct Options
{
    modes mode{modes::NONE};
    std::string fname1;
    std::string fname2;
    std::string swap_path;
    bool show_mem{};
    bool show_stats{};
    bool quiet_mode{}; // true if "/Q" option used
    std::vector<std::string> include_paths;
    std::string output_dir;
};

Options parse_options(int argc, char **argv);

} // namespace hc
