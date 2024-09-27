// SPDX-License-Identifier: GPL-3.0-only
//
#include <comments.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstring>

using namespace testing;

TEST(TestComments, expandCpu)
{
    init_comments();
    std::strcpy(g_par_comment[0], "; rendered on $cpu$ CPU");
    StrictMock<MockFunction<std::string()>> get_cpu_id;
    g_get_cpu_id = get_cpu_id.AsStdFunction();
    EXPECT_CALL(get_cpu_id, Call()).WillOnce(Return("Intel Core i9"));

    const std::string result = expand_command_comment(0);

    EXPECT_EQ("; rendered on Intel Core i9 CPU", result);
}
