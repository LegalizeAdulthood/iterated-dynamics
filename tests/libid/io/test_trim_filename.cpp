// SPDX-License-Identifier: GPL-3.0-only
//
#include <io/trim_filename.h>

#include <gtest/gtest.h>

#include <filesystem>

using namespace id::io;

TEST(TestTrimFilename, fitsInRequestedSize)
{
    const std::string filename{R"(C:\iterated-dynamics\foo.par)"};

    const std::string result = trim_filename(filename, 80);

    EXPECT_EQ(filename, result);
}

#if defined(WIN32)
TEST(TestTrimFilename, dropsIntermediateDirectoriesWindows)
{
    const std::string filename{R"(C:\code\iterated-dynamics\build-default\install\pars\foo.par)"};

    const std::string result = trim_filename(filename, 44);

               //12345678901234567890123456789012345678901234
    EXPECT_GE(44U, result.size());
    EXPECT_EQ(R"(C:\code\...\install\pars\foo.par)", result);
}
#else
TEST(TestTrimFilename, dropsIntermediateDirectoriesUnix)
{
    const std::string filename{R"(/home/users/l/legalize/iterated-dynamics/build-default/install/pars/foo.par)"};

    const std::string result = trim_filename(filename, 40);

               //12345678901234567890123456789012345678901234
    EXPECT_GE(40U, result.size());
    EXPECT_EQ(R"(/home/.../install/pars/foo.par)", result);
}
#endif
