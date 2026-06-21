// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/KeyboardHandler.h"

#include "ui/KeyboardInput.h"

#include <cassert>
#include <vector>

namespace id::ui
{

namespace
{

std::vector<KeyboardHandlerPtr> s_keyboard_handlers;
bool s_calc_interrupted{};

void dispatch_key(const int key)
{
    for (auto it = s_keyboard_handlers.rbegin(); it != s_keyboard_handlers.rend(); ++it)
    {
        if ((*it)->handle_key(key))
        {
            break;
        }
    }
}

} // namespace

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
    while (g_kb_input->pending_key() != 0)
    {
        dispatch_key(g_kb_input->read_key());
    }
    return s_calc_interrupted;
}

} // namespace id::ui
