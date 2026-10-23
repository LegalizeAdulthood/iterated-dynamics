// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <misc/Driver.h>

#include "MockDriver.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

TEST(TestDriver, loadClose)
{
    std::vector<std::string> args{"id"};
    MockDriver gdi;
    EXPECT_CALL(gdi, init(Ref(args))).WillOnce(Return(true));
    EXPECT_CALL(gdi, terminate());

    load_driver(&gdi, args);

    EXPECT_EQ(&gdi, g_driver);

    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

TEST(TestDriver, loadCanRemoveConsumedArguments)
{
    std::vector<std::string> args{"id", "--driver-option", "type=mandel"};
    MockDriver gdi;
    EXPECT_CALL(gdi, init(Ref(args)))
        .WillOnce(Invoke(
            [](std::vector<std::string> &init_args)
            {
                init_args.erase(init_args.begin() + 1);
                return true;
            }));
    EXPECT_CALL(gdi, terminate());

    load_driver(&gdi, args);

    EXPECT_THAT(args, ElementsAre("id", "type=mandel"));

    close_drivers();
}

TEST(TestDriver, initUsesProvidedDriverOrder)
{
    std::vector<std::string> args{"id"};
    MockDriver disk;
    MockDriver display;
    const InSequence sequence;
    EXPECT_CALL(disk, init(Ref(args))).WillOnce(Return(true));
    EXPECT_CALL(display, init(Ref(args))).WillOnce(Return(true));

    EXPECT_EQ(2, init_drivers(args, {&disk, &display}));
    EXPECT_EQ(&disk, g_driver);

    EXPECT_CALL(display, terminate());
    EXPECT_CALL(disk, terminate());
    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

TEST(TestDriver, initSkipsUnavailableDrivers)
{
    std::vector<std::string> args{"id"};
    MockDriver unavailable;
    MockDriver available;
    const InSequence sequence;
    EXPECT_CALL(unavailable, init(Ref(args))).WillOnce(Return(false));
    EXPECT_CALL(available, init(Ref(args))).WillOnce(Return(true));

    EXPECT_EQ(1, init_drivers(args, {nullptr, &unavailable, &available}));
    EXPECT_EQ(&available, g_driver);

    EXPECT_CALL(available, terminate());
    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

} // namespace id::test
