// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

namespace id::ui
{

class KeyboardInput
{
public:
    virtual ~KeyboardInput() = default;

    virtual int pending_key() = 0;
    virtual int read_key() = 0;
    virtual int wait_for_key(bool timeout) = 0;
    virtual void push_key(int key) = 0;
};

extern KeyboardInput *g_kb_input;

} // namespace id::ui
