// SPDX-License-Identifier: GPL-3.0-only
//
// Copyright 2026 Richard Thomson
//
#include "io/special_dirs.h"

#include "test_executable_dir.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <system_error>

namespace id::io
{

namespace fs = std::filesystem;

namespace
{

fs::path canonical_path(const fs::path &path)
{
    std::error_code err;
    const fs::path canonical{fs::weakly_canonical(path, err)};
    return err || canonical.empty() ? path : canonical;
}

TEST(TestSpecialDirectories, programDir)
{
    EXPECT_EQ(canonical_path(id::test::data::ID_TEST_PROGRAM_DIR), canonical_path(g_special_dirs->program_dir()));
}

} // namespace

} // namespace id::io
