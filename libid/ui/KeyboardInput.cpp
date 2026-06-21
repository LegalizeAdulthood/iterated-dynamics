// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/KeyboardInput.h"

#include "misc/Driver.h"

using namespace id::misc;

namespace id::ui
{

class DriverKeyboardInput : public KeyboardInput
{
public:
    int pending_key() override
    {
        return driver_key_pressed();
    }

    int read_key() override
    {
        return driver_get_key();
    }

    int wait_for_key(const bool timeout) override
    {
        return driver_wait_key_pressed(timeout);
    }

    void push_key(const int key) override
    {
        driver_unget_key(key);
    }
};

static DriverKeyboardInput s_driver_keyboard_input;

KeyboardInput *g_kb_input{&s_driver_keyboard_input};

} // namespace id::ui
