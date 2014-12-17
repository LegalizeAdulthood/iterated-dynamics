#if !defined(FRAME_H)
#define FRAME_H

#define KEYBUFMAX 80

struct Frame
{
    HINSTANCE instance;
    HWND window;
    char title[80];
    int width;
    int height;
    int nc_width;
    int nc_height;
    HWND child;
    bool has_focus;
    bool timed_out;

    // the keypress buffer
    unsigned int  keypress_count;
    unsigned int  keypress_head;
    unsigned int  keypress_tail;
    unsigned int  keypress_buffer[KEYBUFMAX];
};

extern Frame g_frame;

extern void frame_init(HINSTANCE instance, LPCSTR title);
extern void frame_window(int width, int height);
extern int frame_key_pressed();
extern int frame_get_key_press(int option);
extern int frame_pump_messages(bool waitflag);
extern void frame_schedule_alarm(int soon);
extern void frame_resize(int width, int height);
extern void frame_set_keyboard_timeout(int ms);

#endif
