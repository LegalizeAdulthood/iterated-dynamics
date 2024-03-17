#pragma once

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

void frame_init(HINSTANCE instance, LPCSTR title);
void frame_window(int width, int height);
int frame_key_pressed();
int frame_get_key_press(int option);
int frame_pump_messages(bool waitflag);
void frame_schedule_alarm(int secs);
void frame_resize(int width, int height);
void frame_set_keyboard_timeout(int ms);
