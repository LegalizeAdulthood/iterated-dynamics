// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>
#include <utility>

extern bool                  g_cursor_mouse_tracking;
extern int                   g_look_at_mouse;

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

class MouseNotification
{
public:
    virtual ~MouseNotification() = default;

    virtual void left_down(bool double_click, int x, int y, int key_flags) = 0;
    virtual void right_down(int x, int y, int key_flags) = 0;
    virtual void move(int x, int y, int key_flags) = 0;
};

class NullMouseNotification : public MouseNotification
{
public:
    ~NullMouseNotification() override = default;
    void left_down(bool double_click, int x, int y, int key_flags) override
    {
    }
    void right_down(int x, int y, int key_flags) override
    {
    }
    void move(int x, int y, int key_flags) override
    {
    }
};

int mouse_subscribe(std::shared_ptr<MouseNotification> subscriber);
void mouse_unsubscribe(int id);
void mouse_notify_left_down(bool double_click, int x, int y, int key_flags);
void mouse_notify_right_down(int x, int y, int key_flags);
void mouse_notify_move(int x, int y, int key_flags);

class MouseSubscription
{
public:
    MouseSubscription(std::shared_ptr<MouseNotification> subscriber) :
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
