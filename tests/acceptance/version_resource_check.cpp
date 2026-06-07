// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
#include <string>
#include <vector>

namespace
{

std::string format_version(const VS_FIXEDFILEINFO &info)
{
    return std::to_string((info.dwFileVersionMS >> 16U) & 0xffffU) + "." +
        std::to_string(info.dwFileVersionMS & 0xffffU) + "." + std::to_string((info.dwFileVersionLS >> 16U) & 0xffffU) +
        "." + std::to_string(info.dwFileVersionLS & 0xffffU);
}

} // namespace

int main(const int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: version-resource-check <exe> <version>\n";
        return 1;
    }

    const std::string executable{argv[1]};
    const std::string expected{argv[2]};
    DWORD ignored{};
    const DWORD size{GetFileVersionInfoSizeA(executable.c_str(), &ignored)};
    if (size == 0)
    {
        std::cerr << executable << " has no version resource. GetLastError=" << GetLastError() << "\n";
        return 1;
    }

    std::vector<char> version_info(size);
    if (!GetFileVersionInfoA(executable.c_str(), 0, size, version_info.data()))
    {
        std::cerr << "Could not read version resource from " << executable << ". GetLastError=" << GetLastError()
                  << "\n";
        return 1;
    }

    void *value{};
    UINT length{};
    if (!VerQueryValueA(version_info.data(), "\\", &value, &length) || value == nullptr ||
        length < sizeof(VS_FIXEDFILEINFO))
    {
        std::cerr << executable << " has an invalid version resource.\n";
        return 1;
    }

    const auto *info{static_cast<const VS_FIXEDFILEINFO *>(value)};
    if (info->dwSignature != VS_FFI_SIGNATURE)
    {
        std::cerr << executable << " has an invalid version signature.\n";
        return 1;
    }

    const std::string actual{format_version(*info)};
    if (actual != expected)
    {
        std::cerr << executable << " version is " << actual << ", expected " << expected << ".\n";
        return 1;
    }

    return 0;
}
