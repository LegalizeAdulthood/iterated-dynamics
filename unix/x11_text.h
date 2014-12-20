#if !defined(X11_TEXT_H)
#define X11_TEXT_H

#include <string>

#include <X11/X.h>

#define KEYBUFMAX 80
#define X11_TEXT_MAX_COL 80
#define X11_TEXT_MAX_ROW 25

class x11_text_window
{
public:
    x11_text_window();
    ~x11_text_window();

    int max_width() const { return max_width_; }
    int max_height() const { return max_height_; }
    Window window() const { return 0; }

    int text_on();
    int text_off();

    void put_string(int xpos, int ypos, int attrib, std::string const& text);
    void paint_screen(int x_min, int x_max, int y_min, int y_max);
    void cursor(int x, int y, int cursor_type);
    unsigned get_key_press(int option);
    int look_for_activity(int option);
    void add_key_press(unsigned key);

private:
    Window parent_;
    int max_width_;
    int max_height_;
    int text_mode_;
    int cursor_x_;
    int cursor_y_;
    int cursor_type_;
    bool cursor_owned_;
    bool showing_cursor_;
    bool alt_f4_hit_;
};

#endif
