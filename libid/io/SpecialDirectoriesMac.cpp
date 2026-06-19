// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "io/special_dirs.h"

#include <mach-o/dyld.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace id::io
{

namespace fs = std::filesystem;

namespace
{

constexpr std::uint32_t INITIAL_EXECUTABLE_PATH_SIZE{1024};

class MacSpecialDirectories : public SpecialDirectories
{
public:
    ~MacSpecialDirectories() override = default;

    fs::path program_dir() const override;
    fs::path documents_dir() const override;
};

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

fs::path home_dir()
{
    const char *home = std::getenv("HOME");
    return home != nullptr && home[0] != '\0' ? fs::path{home} : fs::path{};
}

fs::path MacSpecialDirectories::program_dir() const
{
    const fs::path executable{current_executable_path()};
    return executable.empty() ? fs::current_path() : executable.parent_path();
}

fs::path MacSpecialDirectories::documents_dir() const
{
    const fs::path home{home_dir()};
    return home.empty() ? fs::current_path() : home / "Documents";
}

} // namespace

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<MacSpecialDirectories>();
}

} // namespace id::io
