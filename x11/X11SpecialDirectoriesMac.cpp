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

#include <mach-o/dyld.h>

namespace id::io
{

namespace fs = std::filesystem;

namespace
{

static constexpr std::uint32_t INITIAL_EXECUTABLE_PATH_SIZE{1024};

fs::path canonical_path(const fs::path &path)
{
    if (path.empty())
    {
        return {};
    }

    std::error_code err;
    const fs::path canonical{fs::weakly_canonical(path, err)};
    return err || canonical.empty() ? path : canonical;
}

fs::path current_executable_path()
{
    std::uint32_t size{INITIAL_EXECUTABLE_PATH_SIZE};
    std::string path(size, '\0');
    if (_NSGetExecutablePath(path.data(), &size) != 0)
    {
        if (size == 0)
        {
            return {};
        }
        path.assign(size, '\0');
        if (_NSGetExecutablePath(path.data(), &size) != 0)
        {
            return {};
        }
    }
    return canonical_path(path.c_str());
}

} // namespace

std::filesystem::path X11SpecialDirectories::program_dir() const
{
    const fs::path executable{current_executable_path()};
    return executable.empty() ? fs::current_path() : executable.parent_path();
}

std::filesystem::path X11SpecialDirectories::documents_dir() const
{
    const fs::path home{home_dir()};
    return home.empty() ? fs::current_path() : home / "Documents";
}

} // namespace id::io
