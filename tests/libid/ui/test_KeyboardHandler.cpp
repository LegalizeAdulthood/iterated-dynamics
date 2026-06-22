// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/KeyboardHandler.h>

#include <misc/ValueSaver.h>

#include "MockDriver.h"

#include <memory>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::misc;
using namespace id::misc::test;
using namespace id::ui;
using namespace testing;

namespace id::test
{

using KeyboardEvent = std::pair<int, int>;
using KeyboardEvents = std::vector<KeyboardEvent>;

class RecordingKeyboardHandler : public KeyboardHandler
{
public:
    RecordingKeyboardHandler(const int id, const bool consumed, KeyboardEvents &events) :
        m_id{id},
        m_consumed{consumed},
        m_events{events}
    {
    }

    void interrupt_calc()
    {
        m_interrupt_calc = true;
    }

    bool handle_key(const int key) override
    {
        m_events.emplace_back(m_id, key);
        if (m_interrupt_calc)
        {
            set_calc_interrupted();
        }
        return m_consumed;
    }

private:
    int m_id;
    bool m_consumed;
    bool m_interrupt_calc{};
    KeyboardEvents &m_events;
};

class TestKeyboardHandler : public Test
{
protected:
    void SetUp() override
    {
        reset_calc_interrupted();
    }

    void TearDown() override
    {
        reset_calc_interrupted();
    }

    void expect_key(int key)
    {
        InSequence sequence;
        EXPECT_CALL(m_driver, key_pressed()).WillOnce(Return(key));
        EXPECT_CALL(m_driver, get_key()).WillOnce(Return(key));
        EXPECT_CALL(m_driver, key_pressed()).WillRepeatedly(Return(0));
    }

    MockDriver m_driver;
    ValueSaver<Driver *> m_saved_driver{g_driver, &m_driver};
};

TEST_F(TestKeyboardHandler, calcInterruptedOffersKeyFromTopToBottom)
{
    expect_key(17);
    KeyboardEvents events;
    auto bottom{std::make_shared<RecordingKeyboardHandler>(1, true, events)};
    auto top{std::make_shared<RecordingKeyboardHandler>(2, false, events)};
    ScopedKeyboardHandler bottom_scope{bottom};
    ScopedKeyboardHandler top_scope{top};

    EXPECT_FALSE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(2, 17), Pair(1, 17)));
}

TEST_F(TestKeyboardHandler, consumedKeyStopsPropagation)
{
    expect_key(23);
    KeyboardEvents events;
    auto bottom{std::make_shared<RecordingKeyboardHandler>(1, true, events)};
    auto top{std::make_shared<RecordingKeyboardHandler>(2, true, events)};
    ScopedKeyboardHandler bottom_scope{bottom};
    ScopedKeyboardHandler top_scope{top};

    EXPECT_FALSE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(2, 23)));
}

TEST_F(TestKeyboardHandler, unconsumedKeyIsDiscarded)
{
    expect_key(29);
    KeyboardEvents events;
    auto handler{std::make_shared<RecordingKeyboardHandler>(1, false, events)};
    ScopedKeyboardHandler scope{handler};

    EXPECT_FALSE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(1, 29)));
}

TEST_F(TestKeyboardHandler, handlerRequestsCalculationInterruption)
{
    expect_key(31);
    KeyboardEvents events;
    auto handler{std::make_shared<RecordingKeyboardHandler>(1, true, events)};
    handler->interrupt_calc();
    ScopedKeyboardHandler scope{handler};

    EXPECT_TRUE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(1, 31)));
}

TEST_F(TestKeyboardHandler, scopedHandlerRegistrationPopsHandler)
{
    KeyboardEvents events;
    auto bottom{std::make_shared<RecordingKeyboardHandler>(1, true, events)};
    auto top{std::make_shared<RecordingKeyboardHandler>(2, true, events)};
    ScopedKeyboardHandler bottom_scope{bottom};
    {
        ScopedKeyboardHandler top_scope{top};
    }
    expect_key(37);

    EXPECT_FALSE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(1, 37)));
}

} // namespace id::test
