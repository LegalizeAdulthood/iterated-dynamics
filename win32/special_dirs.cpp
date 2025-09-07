// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include "win_defines.h"
#include <ShlObj.h>
#include <Windows.h>

#include <stdexcept>
#include <string>

namespace id::io
{

namespace
{

class Win32SpecialDirectories : public SpecialDirectories
{
public:
    Win32SpecialDirectories() = default;
    ~Win32SpecialDirectories() override = default;

    std::filesystem::path program_dir() const override;
    std::filesystem::path documents_dir() const override;
};

std::filesystem::path Win32SpecialDirectories::program_dir() const
{
    char buffer[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path Win32SpecialDirectories::documents_dir() const
{
    char buffer[MAX_PATH]{};
    const HRESULT status = SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, buffer);
    if (FAILED(status))
    {
        throw std::runtime_error("Couldn't get documents folder path: " + std::to_string(status));
    }
    return buffer;
}

} // namespace

std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<Win32SpecialDirectories>();
}

} // namespace id::io
