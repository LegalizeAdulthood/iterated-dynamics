#pragma once

#include "port.h"

#include <vector>

extern bool                  g_editpal_cursor;
extern std::vector<BYTE>     g_line_buff;
extern bool                  g_using_jiim;

void EditPalette();
void put_row(int x, int y, int width, char const *buff);
void get_row(int x, int y, int width, char *buff);

void Cursor_StartMouseTracking();
void Cursor_EndMouseTracking();
int Cursor_WaitKey();
void Cursor_CheckBlink();
void Cursor_Construct();
void Cursor_SetPos(int x, int y);
int Cursor_GetX();
int Cursor_GetY();
void Cursor_Hide();
void Cursor_Show();
