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
        : dpy_{},
        timed_out_{},
        keypress_count_{},
        keypress_head_{},
        keypress_tail_{},
        keypress_buffer_{},
        width_{800U},
        height_{600U},
        mapped_{}
    {
    }

    void initialize(Display *dpy, int screen, char const *geometry);

    unsigned width() const { return width_; }
    unsigned height() const { return height_; }
    Window window() const { return window_; }
    int get_key_press(int option);
    int pump_messages(bool wait_flag);
    void window(unsigned width, unsigned height);

private:
    Display *dpy_;
    Window window_;
    bool timed_out_;
    int keypress_count_;
    int keypress_head_;
    int keypress_tail_;
    int keypress_buffer_[KEYBUFMAX];
    unsigned width_;
    unsigned height_;
    bool mapped_;
};

#endif
