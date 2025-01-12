// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

class CurrentPathSaver
{
public:
    explicit CurrentPathSaver(const std::filesystem::path &new_path) :
        m_old_path(std::filesystem::current_path())
    {
        current_path(new_path);
    }
    ~CurrentPathSaver()
    {
        current_path(m_old_path);
    }

private:
    std::filesystem::path m_old_path;
};
