#pragma once
#if !defined(EDITPAL_H)
#define EDITPAL_H

#include <vector>

extern std::vector<BYTE>     g_line_buff;
extern bool                  g_using_jiim;

extern void EditPalette();
void putrow(int x, int y, int width, char const *buff);
void getrow(int x, int y, int width, char *buff);
int Cursor_WaitKey();
void Cursor_CheckBlink();
void clip_putcolor(int x, int y, int color);
int clip_getcolor(int x, int y);
void Cursor_Construct();
void Cursor_SetPos(int x, int y);
void Cursor_Move(int xoff, int yoff);
int Cursor_GetX();
int Cursor_GetY();
void Cursor_Hide();
void Cursor_Show();
extern void displayc(int, int, int, int, int);

#endif
