// SPDX-License-Identifier: GPL-3.0-only
//
#include <Compiler.h>

#include <gtest/gtest.h>

TEST(TestAsciiDoc, createCompiler)
{
    hc::Options options{};
    options.mode = hc::modes::ASCII_DOC;

    std::shared_ptr compiler{create_compiler(options)};

    EXPECT_TRUE(compiler);
}
