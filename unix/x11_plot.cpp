#include "x11_plot.h"

#include <cassert>

#include <X11/Xlib.h>

void x11_plot_window::initialize(Display *dpy, int screen_num, Window parent)
{
    dpy_ = dpy;
    int x = 0;
    int y = 0;
    width_ = 800;
    height_ = 600;
    XSetWindowAttributes attrs = { 0 };
    Screen *screen = ScreenOfDisplay(dpy, screen_num);
    attrs.background_pixel = BlackPixelOfScreen(screen);
    attrs.bit_gravity = StaticGravity;
    attrs.backing_store = DoesBackingStore(screen) ? Always : NotUseful;
    window_ = XCreateWindow(dpy, parent, x, y, width_, height_, 0,
            DefaultDepth(dpy, screen_num), InputOutput, CopyFromParent,
            CWBackPixel | CWBitGravity | CWBackingStore, &attrs);
    assert(window_ != 0);
}

void x11_plot_window::set_position(int x, int y)
{
    assert(dpy_ != nullptr && window_ != 0);
    XMoveWindow(dpy_, window_, x, y);
}

void x11_plot_window::clear()
{

}
