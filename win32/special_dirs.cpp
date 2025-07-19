// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/special_dirs.h"

#include "win_defines.h"
#include <ShlObj.h>
#include <Windows.h>

#include <stdexcept>
#include <string>

std::filesystem::path get_executable_dir()
{
    char buffer[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return std::filesystem::path{buffer}.parent_path();
}

std::filesystem::path get_documents_dir()
{
    char buffer[MAX_PATH]{};
    const HRESULT status = SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, buffer);
    if (FAILED(status))
    {
        throw std::runtime_error("Couldn't get documents folder path: " + std::to_string(status));
    }
    return buffer;
}
