#pragma once

#include <io/special_dirs.h>

#include <gmock/gmock.h>

class MockSpecialDirectories : public SpecialDirectories
{
public:
    MockSpecialDirectories() = default;
    ~MockSpecialDirectories() override = default;

    MOCK_METHOD(std::filesystem::path, program_dir, (), (const, override));
    MOCK_METHOD(std::filesystem::path, documents_dir, (), (const, override));
};

extern std::shared_ptr<MockSpecialDirectories> g_mock_special_dirs;
