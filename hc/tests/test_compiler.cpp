// SPDX-License-Identifier: GPL-3.0-only
//
#include <Compiler.h>

#include <Options.h>

#include <gtest/gtest.h>

namespace hc::test
{

TEST(TestAsciiDoc, createCompiler)
{
    Options options{};
    options.mode = Mode::ASCII_DOC;

    const std::shared_ptr compiler{create_compiler(options)};

    EXPECT_TRUE(compiler);
}

} // namespace id::test
