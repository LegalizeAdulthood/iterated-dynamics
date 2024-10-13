// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "io/special_dirs.h"

namespace id
{

class WxSpecialDirectories : public SpecialDirectories
{
public:
    ~WxSpecialDirectories() override = default;

    std::filesystem::path documents_dir() const override;

    std::filesystem::path exeuctable_dir() const override;
};

} // namespace id
