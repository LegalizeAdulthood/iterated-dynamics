#if !defined(X11_TEXT_H)
#define X11_TEXT_H

#include <string>
#include <vector>

#include <X11/Xlib.h>

enum
{
    X11_TEXT_MAX_COL = 80,
    X11_TEXT_MAX_ROW = 25
};

class x11_text_window
{
public:
    x11_text_window();
    ~x11_text_window();

    void initialize(Display *dpy, int screen_num, Window parent);

    void set_position(int x, int y);
    unsigned max_width() const { return max_width_; }
    unsigned max_height() const { return max_height_; }
    Window window() const { return window_; }

    int text_on();
    int text_off();

    void put_string(int xpos, int ypos, int attrib, std::string const& text, int *end_row, int *end_col);
    void paint_screen(int x_min, int x_max, int y_min, int y_max);
    void cursor(int x, int y, int cursor_type);
    unsigned get_key_press(int option);
    int look_for_activity(int option);
    void add_key_press(unsigned key);

    void show()
    {
        XMapWindow(dpy_, window_);
    }
    void hide()
    {
        XUnmapWindow(dpy_, window_);
    }

    void clear();

private:
    Display *dpy_;
    int screen_num_;
    XFontStruct const *font_;
    Window parent_;
    Window window_;
    int char_width_;
    int char_height_;
    int char_xchars_;
    int char_ychars_;
    unsigned max_width_;
    unsigned max_height_;
    int text_mode_;
    int cursor_x_;
    int cursor_y_;
    int cursor_type_;
    bool cursor_owned_;
    bool showing_cursor_;
    bool alt_f4_hit_;
    int x_;
    int y_;
    std::vector<std::vector<char>> text_;
    std::vector<std::vector<unsigned char>> attributes_;

    void repaint(int xpos, int ypos, int maxcol, int maxrow);

    Colormap colormap_;
    bool buffer_init_;
};

#endif
