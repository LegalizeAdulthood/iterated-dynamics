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

    std::string documents_dir() const override;

    std::string exeuctable_dir() const override;
};

} // namespace id
