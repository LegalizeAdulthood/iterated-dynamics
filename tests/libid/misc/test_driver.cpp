// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include <misc/Driver.h>

#include <MockDriver.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <vector>

using namespace id::misc;
using namespace id::misc::test;
using namespace testing;

namespace id::test
{

class TestDriver : public Test
{
protected:
    void TearDown() override;

    std::vector<std::string> m_args{"id"};
    MockDriver m_first;
    MockDriver m_second;
};

void TestDriver::TearDown()
{
    close_drivers();
    EXPECT_EQ(nullptr, g_driver);
}

TEST_F(TestDriver, loadCanRemoveConsumedArguments)
{
    m_args = {"id", "--driver-option", "type=mandel"};
    EXPECT_CALL(m_first, init(Ref(m_args)))
        .WillOnce(Invoke(
            [](std::vector<std::string> &init_args)
            {
                init_args.erase(init_args.begin() + 1);
                return true;
            }));
    EXPECT_CALL(m_first, terminate());

    load_driver(&m_first, m_args);

    EXPECT_THAT(m_args, ElementsAre("id", "type=mandel"));
}

TEST_F(TestDriver, initializationUsesProvidedOrderAndSelectsFirstDriver)
{
    {
        const InSequence sequence;
        EXPECT_CALL(m_first, init(Ref(m_args))).WillOnce(Return(true));
        EXPECT_CALL(m_second, init(Ref(m_args))).WillOnce(Return(true));
    }
    EXPECT_CALL(m_first, terminate());
    EXPECT_CALL(m_second, terminate());

    EXPECT_EQ(2, init_drivers(m_args, {&m_first, &m_second}));
    EXPECT_EQ(&m_first, g_driver);
}

TEST_F(TestDriver, terminationReversesInitializationOrder)
{
    EXPECT_CALL(m_first, init(Ref(m_args))).WillOnce(Return(true));
    EXPECT_CALL(m_second, init(Ref(m_args))).WillOnce(Return(true));
    ASSERT_EQ(2, init_drivers(m_args, {&m_first, &m_second}));

    const InSequence sequence;
    EXPECT_CALL(m_second, terminate());
    EXPECT_CALL(m_first, terminate());

    close_drivers();

    EXPECT_EQ(nullptr, g_driver);
}

TEST_F(TestDriver, initSkipsUnavailableDrivers)
{
    {
        const InSequence sequence;
        EXPECT_CALL(m_first, init(Ref(m_args))).WillOnce(Return(false));
        EXPECT_CALL(m_second, init(Ref(m_args))).WillOnce(Return(true));
    }
    EXPECT_CALL(m_second, terminate());

    EXPECT_EQ(1, init_drivers(m_args, {nullptr, &m_first, &m_second}));
    EXPECT_EQ(&m_second, g_driver);
}

} // namespace id::test
