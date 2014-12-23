#if !defined(X11_PLOT_H)
#define X11_PLOT_H

#include <X11/Xlib.h>

class x11_plot_window
{
public:
    x11_plot_window()
        : dpy_(nullptr),
        window_(0)
    {
    }
    void initialize(Display *dpy, Window parent);
    int width() const { return 0; }
    int height() const { return 0; }
    Window window() const { return window_; }

private:
    Display *dpy_;
    Window window_;
};

#endif
