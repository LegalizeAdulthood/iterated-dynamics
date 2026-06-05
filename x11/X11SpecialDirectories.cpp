// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include <cstdlib>

namespace id::io
{

namespace
{

class X11SpecialDirectories : public SpecialDirectories
{
public:
    ~X11SpecialDirectories() override = default;

    std::filesystem::path program_dir() const override;
    std::filesystem::path documents_dir() const override;
};

std::filesystem::path X11SpecialDirectories::program_dir() const
{
    return std::filesystem::current_path();
}

std::filesystem::path X11SpecialDirectories::documents_dir() const
{
    const char *home = std::getenv("HOME");
    if (home != nullptr && home[0] != '\0')
    {
        return home;
    }
    return std::filesystem::current_path();
}

} // namespace

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<X11SpecialDirectories>();
}

} // namespace id::io
