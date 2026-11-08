// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <io/special_dirs.h>

#include <filesystem>
#include <memory>

namespace id::io
{

namespace
{

class TestSpecialDirectories : public SpecialDirectories
{
public:
    TestSpecialDirectories();

    std::filesystem::path program_dir() const override;
    std::filesystem::path documents_dir() const override;
    std::filesystem::path resource_dir() const override;

private:
    const std::filesystem::path m_root;
};

TestSpecialDirectories::TestSpecialDirectories() :
    m_root(std::filesystem::current_path())
{
}

std::filesystem::path TestSpecialDirectories::program_dir() const
{
    return m_root;
}

std::filesystem::path TestSpecialDirectories::documents_dir() const
{
    return m_root / "documents";
}

std::filesystem::path TestSpecialDirectories::resource_dir() const
{
    return m_root;
}

} // namespace

// Link this factory into the integration test executable in place of the
// platform factory, as the unit tests do with MockSpecialDirectories.
std::shared_ptr<SpecialDirectories> create_special_directories()
{
    return std::make_shared<TestSpecialDirectories>();
}

} // namespace id::io
