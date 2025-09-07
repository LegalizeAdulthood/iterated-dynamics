// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

namespace id::io
{

std::shared_ptr<SpecialDirectories> g_special_dirs{create_special_directories()};
std::filesystem::path g_save_dir;

} // namespace id::io
