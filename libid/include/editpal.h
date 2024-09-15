// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

#include <vector>

//
// Class:     CrossHairCursor
//
// Purpose:   Draw the blinking cross-hair cursor.
//
class CrossHairCursor
{
public:
    CrossHairCursor();
    CrossHairCursor(const CrossHairCursor &rhs) = delete;
    CrossHairCursor(CrossHairCursor &&rhs) = delete;
    ~CrossHairCursor() = default;
    CrossHairCursor &operator=(const CrossHairCursor &rhs) = default;
    CrossHairCursor &operator=(CrossHairCursor &&rhs) = default;
    
    void draw();
    void save();
    void restore();
    void set_pos(int x, int y);
    void move(int xoff, int yoff);
    int get_x() const
    {
        return m_x;
    }
    int get_y() const
    {
        return m_y;
    }
    void check_blink();
    int wait_key();
    void hide();
    void show();

private:
    enum
    {
        CURSOR_SIZE = 5, // length of one side of the x-hair cursor
    };
    int m_x;
    int m_y;
    int m_hidden; // >0 if mouse hidden
    long m_last_blink;
    bool m_blink;
    char m_top[CURSOR_SIZE]; // save line segments here
    char m_bottom[CURSOR_SIZE];
    char m_left[CURSOR_SIZE];
    char m_right[CURSOR_SIZE];
};

extern std::vector<BYTE>     g_line_buff;
extern bool                  g_using_jiim;

void EditPalette();
void put_row(int x, int y, int width, char const *buff);
void get_row(int x, int y, int width, char *buff);
