// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/mouse.h"

#include "misc/ValueSaver.h"

#include <cassert>
#include <functional>
#include <map>
#include <utility>
#include <vector>

using namespace id::misc;

namespace id::ui
{

// g_look_at_mouse:
//  IGNORE_MOUSE
//      ignore the mouse entirely
//  <IGNORE_MOUSE
//      only test for left button click; if it occurs return fake key number of opposite sign
//  NAVIGATE_GRAPHICS
//      return <Enter> key for left button, arrow keys for mouse movement,
//      mouse sensitivity is suitable for graphics cursor
//  NAVIGATE_TEXT
//      same as NAVIGATE_GRAPHICS, but sensitivity is suitable for text cursor
//  POSITION
//      specials for zoom box, left/right double-clicks generate fake
//      keys, mouse movement generates a variety of fake keys
//      depending on state of buttons
//
MouseLook g_look_at_mouse{};    //
bool g_cursor_mouse_tracking{}; //

static int s_subscriber_id{};
static std::map<int, MouseNotificationPtr> s_subscribers;
static bool s_inside_notification{};
static std::vector<int> s_pending_unsubscribe;

int mouse_subscribe(MouseNotificationPtr subscriber)
{
    assert(!s_inside_notification);
    const int result{s_subscriber_id++};
    s_subscribers[result] = std::move(subscriber);
    return result;
}

void mouse_unsubscribe(int id)
{
    if (s_inside_notification)
    {
        s_pending_unsubscribe.push_back(id);
        return;
    }
    const auto it = s_subscribers.find(id);
    assert(it != s_subscribers.end());
    s_subscribers.erase(id);
}

static void notify(std::function<void(const MouseNotificationPtr &)> handler)
{
    {
        ValueSaver saved_inside_notification(s_inside_notification, true);
        for (const auto &entry : s_subscribers)
        {
            handler(entry.second);
        }
    }
    if (!s_inside_notification)
    {
        for (const int id : s_pending_unsubscribe)
        {
            mouse_unsubscribe(id);
        }
    }
}

void mouse_notify_primary_down(bool double_click, int x, int y, int key_flags)
{
    notify(
        [=](const MouseNotificationPtr &handler) { handler->primary_down(double_click, x, y, key_flags); });
}

void mouse_notify_secondary_down(bool double_click, int x, int y, int key_flags)
{
    notify(
        [=](const MouseNotificationPtr &handler) { handler->secondary_down(double_click, x, y, key_flags); });
}

void mouse_notify_middle_down(bool double_click, int x, int y, int key_flags)
{
    notify([=](const MouseNotificationPtr &handler) { handler->middle_down(double_click, x, y, key_flags); });
}

void mouse_notify_primary_up(int x, int y, int key_flags)
{
    notify([=](const MouseNotificationPtr &handler) { handler->primary_up(x, y, key_flags); });
}

void mouse_notify_secondary_up(int x, int y, int key_flags)
{
    notify([=](const MouseNotificationPtr &handler) { handler->secondary_up(x, y, key_flags); });
}

void mouse_notify_middle_up(int x, int y, int key_flags)
{
    notify([=](const MouseNotificationPtr &handler) { handler->middle_up(x, y, key_flags); });
}

void mouse_notify_move(int x, int y, int key_flags)
{
    notify([=](const MouseNotificationPtr &handler) { handler->move(x, y, key_flags); });
}

} // namespace id::ui
