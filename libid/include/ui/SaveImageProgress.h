// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson
//
#pragma once

#include <filesystem>

namespace id::ui
{

class SaveImageProgress
{
public:
    explicit SaveImageProgress(const std::filesystem::path &filename);
    ~SaveImageProgress();
    SaveImageProgress(const SaveImageProgress &) = delete;
    SaveImageProgress(SaveImageProgress &&) = delete;
    SaveImageProgress &operator=(const SaveImageProgress &) = delete;
    SaveImageProgress &operator=(SaveImageProgress &&) = delete;

    void advance(int row);

private:
    static void next_colors(int row, int &color1, int &color2);
    void toggle_row(int row, int color1, int color2) const;

    bool m_disk;
    int m_width;
    int m_initial_color1;
    int m_initial_color2;
    int m_color1;
    int m_color2;
    int m_last_row{-1};
};

} // namespace id::ui
