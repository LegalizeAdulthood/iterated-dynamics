#if !defined(X11_PLOT_H)
#define X11_PLOT_H

#include <X11/X.h>

class x11_plot_window
{
public:
    int width() const { return 0; }
    int height() const { return 0; }
    Window window() const { return 0; }
};

#endif
