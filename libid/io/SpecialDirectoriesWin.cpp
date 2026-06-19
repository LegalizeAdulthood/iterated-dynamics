// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "io/special_dirs.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <ShlObj.h>
#include <Windows.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace id::io
{

namespace fs = std::filesystem;

namespace
{

class WinSpecialDirectories : public SpecialDirectories
{
public:
    ~WinSpecialDirectories() override = default;

    fs::path program_dir() const override;
    fs::path documents_dir() const override;
};

fs::path WinSpecialDirectories::program_dir() const
{
    char buffer[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return fs::path{buffer}.parent_path();
}

fs::path WinSpecialDirectories::documents_dir() const
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
    return std::make_shared<WinSpecialDirectories>();
}

} // namespace id::io
