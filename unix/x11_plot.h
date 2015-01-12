#if !defined(X11_PLOT_H)
#define X11_PLOT_H

#include <X11/Xlib.h>

// cppcheck-suppress noConstructor
class x11_plot_window
{
public:
    x11_plot_window()
            : dpy_{},
            x_{},
            y_{},
            width_{},
            height_{},
            window_{}
    {
    }

    void set_position(int x, int y);
    void initialize(Display *dpy, int screen_num, Window parent);
    int width() const { return 0; }
    int height() const { return 0; }
    Window window() const { return window_; }

private:
    Display *dpy_;
    int x_;
    int y_;
    unsigned width_;
    unsigned height_;
    Window window_;
};

#endif
