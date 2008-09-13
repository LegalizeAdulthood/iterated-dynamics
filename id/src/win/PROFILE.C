#define STRICT

#include "port.h"
#include "prototyp.h"

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "winfract.h"
#include "mathtool.h"
#include "profile.h"

extern int xdots, ydots, Sizing;
extern int ZoomBarOpen, CoordBoxOpen;
extern BOOL winfract_menustyle;
extern int win_fastupdate;
static char None[]      = "None";
static char SSTools[80];
static char True[]      = "True";
static char False[]     = "False";
static char Yes[]       = "Yes";
static char One[]       = "1";
char FractintMenusStr[] = "FractintMenus";
char FractintPixelsStr[] = "PixelByPixel";

static BOOL NoSave = FALSE;

char Winfract[]  = "Winfract";
char *ProgStr = Winfract;

void SetToolsPath(void) {
   static char FileName[] = "sstools.ini";

   findpath(FileName, SSTools);
   if(!*SSTools)
      strcpy(SSTools, FileName);
}

BOOL GetParamSwitch(char *Key) {
   char Text[80];

   GetPrivateProfileString(ProgStr, Key, None, Text, sizeof(Text), SSTools);
   if(!stricmp(Text, True) ||
      !stricmp(Text, Yes) ||
      !stricmp(Text, One)
   ) return(TRUE);
   return(FALSE);
}

double GetFloatParam(char *Key, double Def) {
   char Text[80];

   GetPrivateProfileString(ProgStr, Key, None, Text, sizeof(Text), SSTools);
   if(!stricmp(None, Text))
      return(Def);
   return(atof(Text));
}

int GetIntParam(char *Key, int Def) {
   char Text[80];

   GetPrivateProfileString(ProgStr, Key, None, Text, sizeof(Text), SSTools);
   if(!stricmp(None, Text))
      return(Def);
   return(atoi(Text));
}

void SaveParamStr(char *Key, char *Str) {
   if(!NoSave)
      WritePrivateProfileString(ProgStr, Key, Str, SSTools);
}

void SaveFloatParam(char *Key, double Num) {
   char Str[80];

   sprintf(Str, "%g", Num);
   SaveParamStr(Key, Str);
}

void SaveIntParam(char *Key, int Num) {
   char Str[80];

   sprintf(Str, "%d", Num);
   SaveParamStr(Key, Str);
}

void SaveParamSwitch(char *Key, BOOL Flag) {
   char *Str;

   if(Flag)
      Str = True;
   else
      Str = False;
   SaveParamStr(Key, Str);
}

void PositionWindow(HWND hWnd, char *Key) {
   char Text[80];
   POINT Pos;

   GetPrivateProfileString(Winfract, Key, None, Text, sizeof(Text), SSTools);
   if(stricmp(Text, None)) {
      sscanf(Text, "%d, %d", &Pos.x, &Pos.y);
      NoSave = TRUE;
      SetWindowPos(hWnd, GetNextWindow(hWnd, GW_HWNDPREV), Pos.x, Pos.y,
                   0, 0, SWP_NOSIZE);
      NoSave = FALSE;
   }
}

extern int ZoomMode;

void SaveWindowPosition(HWND hWnd, char *Key) {
   char Text[80];
   RECT Rect;

   GetWindowRect(hWnd, &Rect);
   sprintf(Text, "%d, %d", Rect.left, Rect.top);
   SaveParamStr(Key, Text);
}

char WindowSizingStr[]     = "WindowSizing";
char ImageWidthStr[]       = "ImageWidth";
char ImageHeightStr[]      = "ImageHeight";
char ZoomBoxStr[]          = "ZoomBoxOpen";
char CoordBoxStr[]         = "CoordinateBoxOpen";
char WinfractPosStr[]      = "WinfractPosition";
char ZoomBoxPosStr[]       = "ZoomBoxPosition";
char CoordBoxPosStr[]      = "CoordBoxPosition";
char ZoomOutStr[]           = "ZoomOut";

void InitializeParameters(HWND hWnd) {
   NoSave = TRUE;

   xdots = GetIntParam(ImageWidthStr, 200);
   ydots = GetIntParam(ImageHeightStr, 150);

   winfract_menustyle = GetParamSwitch(FractintMenusStr);
   win_fastupdate = 0;
   if(GetParamSwitch(FractintPixelsStr))
       win_fastupdate = 1;

   PositionWindow(hWnd, WinfractPosStr);
   if(GetParamSwitch(WindowSizingStr) != Sizing)
      WindowSizing(hWnd);

   if(GetParamSwitch(ZoomBoxStr) != ZoomBarOpen)
      ZoomBar(hWnd);
   else if(GetParamSwitch(ZoomOutStr))
   {
      CheckMenuItem(GetMenu(hWnd), IDM_ZOOMOUT, MF_CHECKED);
      ZoomMode = IDM_ZOOMOUT;
   }
   else
   {
      CheckMenuItem(GetMenu(hWnd), IDM_ZOOMIN, MF_CHECKED);
      ZoomMode = IDM_ZOOMIN;
   }
   if(GetParamSwitch(CoordBoxStr) != CoordBoxOpen)
      CoordinateBox(hWnd);
   NoSave = FALSE;
}

void SaveParameters(HWND hWnd) {
   BOOL Status;

   Status = (ZoomMode == IDM_ZOOMOUT);
   SaveParamSwitch(ZoomOutStr, Status);
}
