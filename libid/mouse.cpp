// SPDX-License-Identifier: GPL-3.0-only
//
#include "mouse.h"

#include <cassert>
#include <map>
#include <utility>

// g_look_at_mouse:
//     0  ignore the mouse entirely
//    <0  only test for left button click; if it occurs return fake key number of opposite sign
//     1  return <Enter> key for left button, arrow keys for mouse movement,
//        mouse sensitivity is suitable for graphics cursor
//     2  same as 1 but sensitivity is suitable for text cursor
//     3  specials for zoom box, left/right double-clicks generate fake
//        keys, mouse movement generates a variety of fake keys
//        depending on state of buttons
//
int g_look_at_mouse{};          //
bool g_cursor_mouse_tracking{}; //

static int s_subscriber_id{};
static std::map<int, std::shared_ptr<MouseNotification>> s_subscribers;

int mouse_subscribe(std::shared_ptr<MouseNotification> subscriber)
{
    const int result{s_subscriber_id++};
    s_subscribers[result] = std::move(subscriber);
    return result;
}

void mouse_unsubscribe(int id)
{
    auto it = s_subscribers.find(id);
    assert(it != s_subscribers.end());
    s_subscribers.erase(id);
}

void mouse_notify_left_down(bool double_click, int x, int y, int key_flags)
{
    for (const auto &entry : s_subscribers)
    {
        entry.second->left_down(double_click, x, y, key_flags);
    }
}

void mouse_notify_right_down(int x, int y, int key_flags)
{
    for (const auto &entry : s_subscribers)
    {
        entry.second->right_down(x, y, key_flags);
    }
}

void mouse_notify_move(int x, int y, int key_flags)
{
    for (const auto &entry : s_subscribers)
    {
        entry.second->move(x, y, key_flags);
    }
}
