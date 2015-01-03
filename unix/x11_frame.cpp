#include <cassert>

#include "x11_frame.h"

void x11_frame_window::initialize(Display *dpy,
    int screen_num,
    char const *geometry)
{
    dpy_ = dpy;

    XSetWindowAttributes attrs;
    XGCValues gc_vals;
    int x = 0;
    int y = 0;

    if (geometry)
        XGeometry(dpy, screen_num, geometry, "800x600+0+0",
            0, 1, 1, 0, 0, &x, &y, &width_, &height_);
    Screen *screen = ScreenOfDisplay(dpy, screen_num);
    attrs.background_pixel = BlackPixelOfScreen(screen);
    attrs.bit_gravity = StaticGravity;
    attrs.backing_store = DoesBackingStore(screen) != 0 ? Always : NotUseful;
    window_ = XCreateWindow(dpy, RootWindow(dpy, screen_num),
        x, y, width_, height_, 0, DefaultDepth(dpy, screen_num),
        InputOutput, CopyFromParent,
        CWBackPixel | CWBitGravity | CWBackingStore, &attrs);
    assert(window_ != 0);
    XStoreName(dpy, window_, "xfractint");
    unsigned long event_mask = KeyPressMask | KeyReleaseMask | ExposureMask
        | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
    XSelectInput(dpy, window_, event_mask);
    attrs.background_pixel = BlackPixelOfScreen(screen);
    XChangeWindowAttributes(dpy, window_, CWBackPixel, &attrs);
    XMapWindow(dpy, window_);
}

void x11_frame_window::resize(int width, int height)
{
    assert(window_ != 0);
}

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
