#if !defined(X11_FRAME_H)
#define X11_FRAME_H

#include <X11/X.h>

enum
{
    KEYBUFMAX = 80
};

// cppcheck-suppress noConstructor
class x11_frame_window
{
public:
    x11_frame_window()
        : timed_out_{},
        keypress_count_{},
        keypress_head_{},
        keypress_tail_{},
        keypress_buffer_{}
    {
    }

    int width() const { return 0; }
    int height() const { return 0; }
    Window window() const { return 0; }
    int get_key_press(int option);
    int pump_messages(bool wait_flag);

private:
    bool timed_out_;
    int keypress_count_;
    int keypress_head_;
    int keypress_tail_;
    int keypress_buffer_[KEYBUFMAX];
};

#endif
