// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/KeyboardHandler.h"

#include "misc/Driver.h"

#include <cassert>
#include <vector>

using namespace id::misc;

namespace id::ui
{

namespace
{

std::vector<KeyboardHandlerPtr> s_keyboard_handlers;
bool s_calc_interrupted{};

} // namespace

bool MainLoopKeyboardHandler::handle_key(const int key)
{
    driver_unget_key(key);
    set_calc_interrupted();
    return true;
}

ScopedKeyboardHandler::ScopedKeyboardHandler(KeyboardHandlerPtr handler) :
    m_handler{std::move(handler)}
{
    assert(m_handler != nullptr);
    push_keyboard_handler(m_handler);
}

ScopedKeyboardHandler::~ScopedKeyboardHandler()
{
    pop_keyboard_handler(m_handler);
}

void push_keyboard_handler(KeyboardHandlerPtr handler)
{
    assert(handler != nullptr);
    s_keyboard_handlers.push_back(std::move(handler));
}

void pop_keyboard_handler(const KeyboardHandlerPtr &handler)
{
    assert(handler != nullptr);
    assert(!s_keyboard_handlers.empty());
    assert(s_keyboard_handlers.back() == handler);
    if (!s_keyboard_handlers.empty() && s_keyboard_handlers.back() == handler)
    {
        s_keyboard_handlers.pop_back();
    }
}

bool dispatch_keyboard_key(const int key)
{
    for (auto it = s_keyboard_handlers.rbegin(); it != s_keyboard_handlers.rend(); ++it)
    {
        if ((*it)->handle_key(key))
        {
            return true;
        }
    }
    return false;
}

void set_calc_interrupted()
{
    s_calc_interrupted = true;
}

void reset_calc_interrupted()
{
    s_calc_interrupted = false;
}

bool calc_interrupted()
{
    while (driver_key_pressed() != 0)
    {
        dispatch_keyboard_key(driver_get_key());
        if (s_calc_interrupted)
        {
            break;
        }
    }
    return s_calc_interrupted;
}

} // namespace id::ui
