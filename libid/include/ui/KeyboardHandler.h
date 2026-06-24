// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include <memory>

namespace id::ui
{

class KeyboardHandler
{
public:
    virtual ~KeyboardHandler() = default;

    virtual bool handle_key(int key) = 0;
};

using KeyboardHandlerPtr = std::shared_ptr<KeyboardHandler>;

class MainLoopKeyboardHandler : public KeyboardHandler
{
public:
    bool handle_key(int key) override;
};

class ScopedKeyboardHandler
{
public:
    explicit ScopedKeyboardHandler(KeyboardHandlerPtr handler);
    ~ScopedKeyboardHandler();

    ScopedKeyboardHandler(const ScopedKeyboardHandler &) = delete;
    ScopedKeyboardHandler(ScopedKeyboardHandler &&) = delete;
    ScopedKeyboardHandler &operator=(const ScopedKeyboardHandler &) = delete;
    ScopedKeyboardHandler &operator=(ScopedKeyboardHandler &&) = delete;

private:
    KeyboardHandlerPtr m_handler;
};

void push_keyboard_handler(KeyboardHandlerPtr handler);
void pop_keyboard_handler(const KeyboardHandlerPtr &handler);
bool dispatch_keyboard_key(int key);

void set_calc_interrupted();
void reset_calc_interrupted();
bool calc_interrupted();

} // namespace id::ui
