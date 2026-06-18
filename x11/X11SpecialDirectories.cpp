// SPDX-License-Identifier: GPL-3.0-only
//
#include <X11Driver/X11SpecialDirectories.h>

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace id::io
{

namespace fs = std::filesystem;

fs::path home_dir()
{
    const char *home = std::getenv("HOME");
    return home != nullptr && home[0] != '\0' ? fs::path{home} : fs::path{};
}

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<X11SpecialDirectories>();
}

} // namespace id::io
