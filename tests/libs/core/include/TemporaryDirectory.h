// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#pragma once

#include <filesystem>
#include <string_view>
#include <system_error>

namespace id::test
{

class TemporaryDirectory
{
public:
    TemporaryDirectory() = default;
    ~TemporaryDirectory();
    TemporaryDirectory(const TemporaryDirectory &) = delete;
    TemporaryDirectory(TemporaryDirectory &&) = delete;
    TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;
    TemporaryDirectory &operator=(TemporaryDirectory &&) = delete;

    void create(std::string_view prefix);
    const std::filesystem::path &path() const;
    void remove();
    void remove(std::error_code &error);

private:
    std::filesystem::path m_path;
    bool m_owned{};
};

} // namespace id::test
