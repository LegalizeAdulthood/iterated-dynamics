#include <cassert>

#include "x11_frame.h"

int x11_frame_window::get_key_press(int wait_for_key)
{
    pump_messages(wait_for_key != 0);
    if (wait_for_key && timed_out_)
    {
        return 0;
    }

    if (keypress_count_ == 0)
    {
        assert(wait_for_key == 0);
        return 0;
    }

    int i = keypress_buffer_[keypress_tail_];
    if (++keypress_tail_ >= KEYBUFMAX)
    {
        keypress_tail_ = 0;
    }
    --keypress_count_;

    return i;
}

int x11_frame_window::pump_messages(bool wait_flag)
{
    return 0;
}
