// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <cstdio>
#include <filesystem>
#include <string_view>

namespace id::test
{

class CommandTestFile
{
public:
    explicit CommandTestFile(std::filesystem::path path);
    ~CommandTestFile();
    CommandTestFile(const CommandTestFile &) = delete;
    CommandTestFile(CommandTestFile &&) = delete;
    CommandTestFile &operator=(const CommandTestFile &) = delete;
    CommandTestFile &operator=(CommandTestFile &&) = delete;

    // The caller owns the returned file, positioned at the beginning.
    std::FILE *open_input(std::string_view text);

    void write(std::string_view text);
    const std::filesystem::path &path() const;
    bool remove();

private:
    const std::filesystem::path m_path;
};

} // namespace id::test
