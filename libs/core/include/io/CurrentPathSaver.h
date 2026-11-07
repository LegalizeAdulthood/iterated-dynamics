// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <filesystem>

namespace id::io
{

class CurrentPathSaver
{
public:
    explicit CurrentPathSaver(const std::filesystem::path &new_path) :
        m_old_path(std::filesystem::current_path())
    {
        current_path(new_path);
    }

    explicit CurrentPathSaver() :
        m_old_path(std::filesystem::current_path())
    {
    }

    ~CurrentPathSaver()
    {
        current_path(m_old_path);
    }

    CurrentPathSaver(const CurrentPathSaver &rhs) = delete;
    CurrentPathSaver(CurrentPathSaver &&rhs) = delete;
    CurrentPathSaver &operator=(const CurrentPathSaver &rhs) = delete;
    CurrentPathSaver &operator=(CurrentPathSaver &&rhs) = delete;

private:
    std::filesystem::path m_old_path;
};

} // namespace id::io
