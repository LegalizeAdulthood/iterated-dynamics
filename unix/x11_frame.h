#if !defined(X11_FRAME_H)
#define X11_FRAME_H

#include <X11/Xlib.h>

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
        keypress_buffer_{},
        width_{},
        height_{}
    {
    }

    void initialize(Display *dpy, int screen, char const *geometry);

    int width() const { return 0; }
    int height() const { return 0; }
    Window window() const { return window_; }
    int get_key_press(int option);
    int pump_messages(bool wait_flag);
    void resize(int width, int height);

private:
    Display *dpy_;
    Window window_;
    bool timed_out_;
    int keypress_count_;
    int keypress_head_;
    int keypress_tail_;
    int keypress_buffer_[KEYBUFMAX];
    int width_;
    int height_;
};

#endif
