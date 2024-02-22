#pragma once

#define KEYBUFMAX 80
#define WINTEXT_MAX_COL 80
#define WINTEXT_MAX_ROW 25

struct WinText
{
    int textmode;
    bool AltF4hit;
    int showing_cursor;

    // Local copy of the "screen" characters and attributes
    char chars[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
    unsigned char attrs[WINTEXT_MAX_ROW][WINTEXT_MAX_COL];
    bool buffer_init;     // false if 'screen' is uninitialized

    // font information

    HFONT hFont;
    int char_font;
    int char_width;
    int char_height;
    int char_xchars;
    int char_ychars;
    int max_width;
    int max_height;

    // "cursor" variables (AKA the "caret" in Window-Speak)
    int cursor_x;
    int cursor_y;
    int cursor_type;
    bool cursor_owned;
    HBITMAP bitmap[3];
    short cursor_pattern[3][40];

    char title_text[128];           // title-bar text

    // a few Windows variables we need to remember globally

    HWND hWndCopy;                  // a Global copy of hWnd
    HWND hWndParent;                // a Global copy of hWnd's Parent
    HINSTANCE hInstance;            // a global copy of hInstance
};

extern void         wintext_clear(WinText *);
extern void         wintext_cursor(WinText *, int, int, int);
extern void         wintext_destroy(WinText *);
extern bool         wintext_initialize(WinText *, HINSTANCE, HWND, LPCSTR);
extern void         wintext_paintscreen(WinText *, int, int, int, int);
extern void         wintext_putstring(WinText *, int, int, int, char const *, int *, int *);
extern void         wintext_scroll_up(WinText *, int top, int bot);
extern void         wintext_set_attr(WinText *, int row, int col, int attr, int count);
extern int          wintext_textoff(WinText *);
extern int          wintext_texton(WinText *);
extern BYTE *       wintext_screen_get(WinText *);
extern void         wintext_screen_set(WinText *, const BYTE *copy);
extern void         wintext_hide_cursor(WinText *);
extern void         wintext_schedule_alarm(WinText *, int secs);
extern int          wintext_get_char_attr(WinText *, int row, int col);
extern void         wintext_put_char_attr(WinText *, int row, int col, int char_attr);
extern void         wintext_set_focus();
extern void         wintext_kill_focus();
extern void         wintext_resume(WinText *me);
