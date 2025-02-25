// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>
#include <utility>

// g_look_at_mouse is set to one of these positive values for handling mouse events,
// or it is set to a negative key code for primary button click to generate a key.
enum class MouseLook
{
    IGNORE_MOUSE = 0,      // ignoring the mouse
    NAVIGATE_GRAPHICS = 1, // <Return> + arrow keys at graphics cursor grid
    NAVIGATE_TEXT = 2,     // <Return> + arrow keys at text cursor grid
    POSITION = 3,          // for positioning things like the zoom box, palette editor, etc.
};
inline int operator+(MouseLook value)
{
    return static_cast<int>(value);
}
inline MouseLook mouse_look_key(int key)
{
    return static_cast<MouseLook>(-key);
}

extern bool                  g_cursor_mouse_tracking;
extern MouseLook             g_look_at_mouse;

class MouseNotification
{
public:
    virtual ~MouseNotification() = default;

    virtual void primary_down(bool double_click, int x, int y, int key_flags) = 0;
    virtual void secondary_down(bool double_click, int x, int y, int key_flags) = 0;
    virtual void middle_down(bool double_click, int i, int y, int key_flags) = 0;
    virtual void primary_up(int x, int y, int key_flags) = 0;
    virtual void secondary_up(int x, int y, int key_flags) = 0;
    virtual void middle_up(int x, int y, int key_flags) = 0;
    virtual void move(int x, int y, int key_flags) = 0;
};

class NullMouseNotification : public MouseNotification
{
public:
    ~NullMouseNotification() override = default;

    void primary_down(bool double_click, int x, int y, int key_flags) override
    {
    }

    void secondary_down(bool double_click, int x, int y, int key_flags) override
    {
    }

    void middle_down(bool double_click, int x, int y, int key_flags) override
    {
    }

    void primary_up(int x, int y, int key_flags) override
    {
    }

    void secondary_up(int x, int y, int key_flags) override
    {
    }

    void middle_up(int x, int y, int key_flags) override
    {
    }

    void move(int x, int y, int key_flags) override
    {
    }
};

using MouseNotificationPtr = std::shared_ptr<MouseNotification>;

int mouse_subscribe(MouseNotificationPtr subscriber);
void mouse_unsubscribe(int id);
void mouse_notify_primary_down(bool double_click, int x, int y, int key_flags);
void mouse_notify_secondary_down(bool double_click, int x, int y, int key_flags);
void mouse_notify_middle_down(bool double_click, int x, int y, int key_flags);
void mouse_notify_primary_up(int x, int y, int key_flags);
void mouse_notify_secondary_up(int x, int y, int key_flags);
void mouse_notify_middle_up(int x, int y, int key_flags);
void mouse_notify_move(int x, int y, int key_flags);

class MouseSubscription
{
public:
    explicit MouseSubscription(MouseNotificationPtr subscriber) :
        m_id(mouse_subscribe(std::move(subscriber)))
    {
    }
    MouseSubscription(const MouseSubscription &) = delete;
    MouseSubscription(MouseSubscription &&) = delete;
    ~MouseSubscription()
    {
        mouse_unsubscribe(m_id);
    }
    MouseSubscription &operator=(const MouseSubscription &) = delete;
    MouseSubscription &operator=(MouseSubscription &&) = delete;

private:
    int m_id;
};
