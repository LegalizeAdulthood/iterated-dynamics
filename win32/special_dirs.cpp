#include "special_dirs.h"

#include "win_defines.h"
#include <ShlObj.h>
#include <Windows.h>

#include <stdexcept>

std::string get_executable_dir()
{
    char buffer[MAX_PATH]{};
    GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    return buffer;
}

std::string get_documents_dir()
{
    char buffer[MAX_PATH]{};
    const HRESULT status = SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, buffer);
    if (FAILED(status))
    {
        throw std::runtime_error("Couldn't get documents folder path: " + std::to_string(status));
    }
    return buffer;
}
