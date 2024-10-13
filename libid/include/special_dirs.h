// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>
#include <string>

class SpecialDirectories
{
public:
    virtual ~SpecialDirectories() = default;

    virtual std::string exeuctable_dir() const = 0;
    virtual std::string documents_dir() const = 0;
};

extern std::shared_ptr<SpecialDirectories> g_special_dirs;
extern std::string g_save_dir;
