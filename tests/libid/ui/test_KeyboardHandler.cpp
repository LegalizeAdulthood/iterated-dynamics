// SPDX-License-Identifier: GPL-3.0-only
//
#include <ui/KeyboardHandler.h>
#include <ui/KeyboardInput.h>

#include <misc/ValueSaver.h>

#include <deque>
#include <memory>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace id::misc;
using namespace id::ui;
using namespace testing;

namespace id::test
{

class FakeKeyboardInput : public KeyboardInput
{
public:
    void add_key(const int key)
    {
        m_keys.push_back(key);
    }

    bool empty() const
    {
        return m_keys.empty();
    }

    int pending_key() override
    {
        return m_keys.empty() ? 0 : m_keys.front();
    }

    int read_key() override
    {
        const int key{m_keys.front()};
        m_keys.pop_front();
        return key;
    }

    int wait_for_key(bool /*timeout*/) override
    {
        return pending_key();
    }

    void push_key(const int key) override
    {
        m_keys.push_front(key);
    }

private:
    std::deque<int> m_keys;
};

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

    std::shared_ptr<FakeKeyboardInput> m_input{std::make_shared<FakeKeyboardInput>()};
    ValueSaver<KeyboardInputPtr> m_saved_input{g_kb_input, m_input};
};

TEST_F(TestKeyboardHandler, calcInterruptedOffersKeyFromTopToBottom)
{
    m_input->add_key(17);
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
    m_input->add_key(23);
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
    m_input->add_key(29);
    KeyboardEvents events;
    auto handler{std::make_shared<RecordingKeyboardHandler>(1, false, events)};
    ScopedKeyboardHandler scope{handler};

    EXPECT_FALSE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(1, 29)));
    EXPECT_TRUE(m_input->empty());
}

TEST_F(TestKeyboardHandler, handlerRequestsCalculationInterruption)
{
    m_input->add_key(31);
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
    m_input->add_key(37);

    EXPECT_FALSE(calc_interrupted());

    EXPECT_THAT(events, ElementsAre(Pair(1, 37)));
}

} // namespace id::test
