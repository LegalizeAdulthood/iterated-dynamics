// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#pragma once

#include <io/special_dirs.h>

#include <filesystem>

namespace id::io
{

class X11SpecialDirectories : public SpecialDirectories
{
public:
    ~X11SpecialDirectories() override = default;

    std::filesystem::path program_dir() const override;
    std::filesystem::path documents_dir() const override;
};

std::filesystem::path home_dir();

} // namespace id::io
