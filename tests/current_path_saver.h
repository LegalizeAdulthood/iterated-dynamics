#pragma once

#include <filesystem>

class current_path_saver
{
public:
    current_path_saver(const std::filesystem::path &new_path) :
        m_old_path(std::filesystem::current_path())
    {
        current_path(new_path);
    }
    ~current_path_saver()
    {
        current_path(m_old_path);
    }

private:
    std::filesystem::path m_old_path;
};
