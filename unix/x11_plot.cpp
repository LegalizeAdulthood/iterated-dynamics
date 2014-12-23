#include "x11_plot.h"

void x11_plot_window::initialize(Display *dpy, Window parent)
{
    dpy_ = dpy;
    window_ = 0;
}
