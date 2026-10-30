// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "TemporaryDirectory.h"

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <system_error>

namespace id::test
{

TemporaryDirectory::~TemporaryDirectory()
{
    if (m_owned)
    {
        std::error_code ignored;
        std::filesystem::remove_all(m_path, ignored);
    }
}

void TemporaryDirectory::create(const std::string_view prefix)
{
    if (m_owned || std::filesystem::path{prefix}.has_parent_path())
    {
        throw std::logic_error("Invalid temporary directory creation");
    }
    const std::filesystem::path parent{std::filesystem::absolute(std::filesystem::temp_directory_path())};
    const std::string stem{
        std::string{prefix} + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())};
    for (std::uint64_t suffix{};; ++suffix)
    {
        std::filesystem::path candidate{parent / (stem + "-" + std::to_string(suffix))};
        if (std::filesystem::create_directory(candidate))
        {
            m_path.swap(candidate);
            m_owned = true;
            return;
        }
    }
}

const std::filesystem::path &TemporaryDirectory::path() const
{
    return m_path;
}

void TemporaryDirectory::remove()
{
    if (m_owned)
    {
        std::filesystem::remove_all(m_path);
        m_owned = false;
    }
}

} // namespace id::test
