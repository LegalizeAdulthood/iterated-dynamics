// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <CommandTestFile.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace id::test
{

CommandTestFile::CommandTestFile(std::filesystem::path path) :
    m_path{std::move(path)}
{
}

CommandTestFile::~CommandTestFile()
{
    std::error_code ignored;
    std::filesystem::remove(m_path, ignored);
}

std::FILE *CommandTestFile::open_input(const std::string_view text)
{
    std::unique_ptr<std::FILE, decltype(&std::fclose)> file{std::fopen(m_path.string().c_str(), "w+b"), std::fclose};
    if (!file || std::fwrite(text.data(), 1, text.size(), file.get()) != text.size())
    {
        throw std::runtime_error("Cannot write parameter test file: " + m_path.string());
    }
    std::rewind(file.get());
    return file.release();
}

void CommandTestFile::write(const std::string_view text)
{
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> file{open_input(text), std::fclose};
}

const std::filesystem::path &CommandTestFile::path() const
{
    return m_path;
}

bool CommandTestFile::remove()
{
    return std::filesystem::remove(m_path);
}

} // namespace id::test
