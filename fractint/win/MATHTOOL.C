/* MathTools.c was written by:

      Mark C. Peterson
      The Yankee Programmer
      405-C Queen Street, Suite #181
      Southington, CT 06489
      (203) 276-9721

   If you make any changes to this file, please comment out the older
   code with your name, date, and a short description of the change.

   Thanks!

                                 -Mark

*/

#include "port.h"
#include "prototyp.h"

#include <windows.h>
#include "winfract.h"
#include "mathtool.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "profile.h"

static char MTClassName[] =     "FFWMathTools";
static char MTWindowTitle[] =   "Math Tools";
HWND hFractalWnd, hMathToolsWnd, hCoordBox, hZoomDlg, hZoomBar;
HANDLE hThisInst, hZoomBrush, hZoomPen;
long FAR PASCAL MTWndProc(HWND, UINT, WPARAM, LPARAM);
int MTWindowOpen = 0, CoordBoxOpen = 0, KillCoordBox = 0;
int ZoomBarOpen = 0, KillZoomBar = 0, Zooming = 0, TrackingZoom = 0;
POINT ZoomClick, ZoomCenter;
int ZoomBarMax = 100, ZoomBarMin = -100;
int Sizing = 1, ReSizing = 0;

static double Pi =  3.14159265359;
FARPROC DefZoomProc;

RECT ZoomRect;

WORD CoordFormat = IDM_RECT;
WORD AngleFormat = IDM_DEGREES;

extern int ytop, ybottom, xleft, xright, ZoomMode;
extern BOOL zoomflag;
extern int time_to_restart, time_to_cycle, time_to_reinit, calc_status;

void XorZoomBox(void);

extern int win_xoffset, win_yoffset;   /* BDT 11/6/91 */


/* Global Maintenance */

void CheckMathTools(void) {
   if(KillCoordBox) {
      KillCoordBox = 0;
      SaveParamSwitch(CoordBoxStr, FALSE);
      CheckMenuItem(GetMenu(hFractalWnd), IDM_COORD, MF_UNCHECKED);
      DestroyWindow(hCoordBox);
   }
   if(KillZoomBar) {
      XorZoomBox();
      Zooming = 0;
      KillZoomBar = 0;
      SaveParamSwitch(ZoomBoxStr, FALSE);
      CheckMenuItem(GetMenu(hFractalWnd), IDM_ZOOM, MF_UNCHECKED);
      DestroyWindow(hZoomDlg);
      CheckMenuItem(GetMenu(hFractalWnd), ZoomMode, MF_CHECKED);
   }
}

BOOL RegisterMathWindows(HANDLE hInstance) {
    WNDCLASS  wc;

    SetToolsPath();
    hThisInst = hInstance;

    wc.style = CS_OWNDC;
    wc.lpfnWndProc = MTWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "MathToolIcon");
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName =  "MathToolsMenu";
    wc.lpszClassName = MTClassName;

    return (RegisterClass(&wc));
}


/* Window Sizing */

void SizeWindow(HWND hWnd) {
   if(Sizing) {
      POINT w;

      w.x = xdots + (GetSystemMetrics(SM_CXFRAME) * 2);
      w.y = ydots + (GetSystemMetrics(SM_CYFRAME) * 2) +
                     GetSystemMetrics(SM_CYCAPTION) +
                     GetSystemMetrics(SM_CYMENU);
      ReSizing = 1;
      ShowScrollBar(hWnd, SB_BOTH, FALSE);
      SetWindowPos(hWnd, GetNextWindow(hWnd, GW_HWNDPREV), 0, 0,
                   w.x, w.y, SWP_NOMOVE);
      ReSizing = 0;
   }
}

void ReSizeWindow(HWND hWnd) {
   if(Sizing) {
      SizeWindow(hWnd);
      ShowWindow(hWnd, SW_SHOWNA);
   }
}

void WindowSizing(HWND hWnd) {
   if(Sizing) {
      CheckMenuItem(GetMenu(hWnd), IDM_SIZING, MF_UNCHECKED);
      Sizing = 0;
      ShowScrollBar(hWnd, SB_BOTH, TRUE);
   }
   else {
      CheckMenuItem(GetMenu(hWnd), IDM_SIZING, MF_CHECKED);
      Sizing = 1;
      ReSizeWindow(hWnd);
   }
   ProgStr = Winfract;
   SaveParamSwitch(WindowSizingStr, Sizing);
}



/* Zoom Box */

void XorZoomBox(void) {
   HDC hDC;
   HANDLE hOldBrush, hOldPen;
   int OldMode;

   if(Zooming) {
      hDC = GetDC(hFractalWnd);

      hOldBrush = SelectObject(hDC, hZoomBrush);
      hOldPen = SelectObject(hDC, hZoomPen);
      OldMode = SetROP2(hDC, R2_XORPEN);

      Rectangle(hDC, ZoomRect.left, ZoomRect.top, ZoomRect.right,
                ZoomRect.bottom);

      SelectObject(hDC, hOldBrush);
      SelectObject(hDC, hOldPen);
      SetROP2(hDC, OldMode);

      ReleaseDC(hFractalWnd, hDC);
  }
}

void PositionZoomBar(void) {
   char Temp[15];

   SetScrollPos(hZoomBar, SB_CTL, Zooming, TRUE);
   sprintf(Temp, "%d%%", Zooming);
   SetDlgItemText(hZoomDlg, ID_PERCENT, Temp);
}

void CalcZoomRect(void) {
   unsigned Midx, Midy;
   double z;

   if(Zooming) {
      Midx = xdots >> 1;
      Midy = ydots >> 1;
      z = 1.0 - (fabs((double)Zooming / 100) * .99);
      ZoomRect.left = ZoomCenter.x - (unsigned)(z * Midx) - 1;
      ZoomRect.right = ZoomCenter.x + (unsigned)(z * Midx) + 1;
      ZoomRect.top = ZoomCenter.y - (unsigned)(z * Midy) - 1;
      ZoomRect.bottom = ZoomCenter.y + (unsigned)(z * Midy) + 1;
   }
}

void CenterZoom(void) {
   ZoomCenter.x = xdots >> 1;
   ZoomCenter.y = ydots >> 1;
}

void StartZoomTracking(DWORD lp) {
   HRGN hRgn;

   ZoomClick = MAKEPOINT(lp);
   if(hRgn = CreateRectRgn(ZoomClick.x-1, ZoomClick.y-1, ZoomClick.x+1,
               ZoomClick.y+1)) {
      if(RectInRegion(hRgn, &ZoomRect))
         TrackingZoom = TRUE;
      DeleteObject(hRgn);
   }
}

void TrackZoom(DWORD lp) {
   POINT NewPoint;

   XorZoomBox();

   NewPoint = MAKEPOINT(lp);
   ZoomCenter.x += NewPoint.x - ZoomClick.x;
   ZoomCenter.y += NewPoint.y - ZoomClick.y;
   ZoomClick = NewPoint;

   CalcZoomRect();
   XorZoomBox();
}

void EndZoom(DWORD lp) {
   TrackZoom(lp);
   TrackingZoom = FALSE;
}

void PaintMathTools(void) {
   XorZoomBox();
}

void CancelZoom(void) {
   XorZoomBox();
   CenterZoom();
   TrackingZoom = Zooming = FALSE;
   PositionZoomBar();
}

void ExecuteZoom(void) {
   double xd, yd, z;

   xd = xxmin + ((double)delxx * (ZoomCenter.x + win_xoffset));  /* BDT 11/6/91 */
   yd = yymax - ((double)delyy * (ZoomCenter.y + win_yoffset));  /* BDT 11/6/91 */

   z = 1.0 - fabs((double)Zooming / 100 * .99);
   if(Zooming > 0)
      z = 1.0 / z;

   xxmin = xd - ((double)delxx * z * (xdots / 2));
   xxmax = xd + ((double)delxx * z * (xdots / 2));
   yymin = yd - ((double)delyy * z * (ydots / 2));
   yymax = yd + ((double)delyy * z * (ydots / 2));

   time_to_reinit = 1;
   time_to_cycle = 0;
   calc_status = 0;
}

BOOL FAR PASCAL ZoomBarProc(HWND hWnd, UINT Message, WPARAM wp, LPARAM dp) {
   switch(Message) {
      case WM_KEYDOWN:
         switch(wp) {
            case VK_RETURN:
               if(TrackingZoom)
                  ExecuteZoom();
               break;
            case VK_ESCAPE:
               CancelZoom();
               break;
            default:
               break;
         }
         break;
   }
   return((BOOL)CallWindowProc(DefZoomProc, hWnd, Message, wp, dp));
}

BOOL FAR PASCAL ZoomBarDlg(HWND hDlg, WORD Message, WORD wp, DWORD dp) {
   FARPROC lpFnct;

   switch(Message) {
      case WM_INITDIALOG:
         ZoomBarOpen = TRUE;
         ProgStr = Winfract;
         SaveParamSwitch(ZoomBoxStr, TRUE);
         CenterZoom();
         CheckMenuItem(GetMenu(hFractalWnd), IDM_ZOOM, MF_CHECKED);
         PositionWindow(hDlg, ZoomBoxPosStr);
         hZoomDlg = hDlg;
         hZoomBrush = GetStockObject(BLACK_BRUSH);
         hZoomPen = GetStockObject(WHITE_PEN);
         if(!(lpFnct = MakeProcInstance(ZoomBarProc, hThisInst)))
            return(FALSE);
         if(!(hZoomBar = CreateWindow("scrollbar", 0, WS_CHILD |
                           WS_TABSTOP | SBS_VERT,
                           38, 28, 18, 248, hDlg, 0, hThisInst, 0)))
            return(FALSE);

         SetScrollRange(hZoomBar, SB_CTL, ZoomBarMin, ZoomBarMax, FALSE);
         SetScrollPos(hZoomBar, SB_CTL, 0, FALSE);
         ShowScrollBar(hZoomBar, SB_CTL, TRUE);
         Zooming = 0;

         /* Create a Window Subclass */
         DefZoomProc = (FARPROC)GetWindowLong(hZoomBar, GWL_WNDPROC);
         SetWindowLong(hZoomBar, GWL_WNDPROC, (LONG)lpFnct);
         return(TRUE);
      case WM_MOVE:
         SaveWindowPosition(hDlg, ZoomBoxPosStr);
         break;
      case WM_CLOSE:
         KillZoomBar = 1;
         ProgStr = Winfract;
         break;
      case WM_DESTROY:
         ZoomBarOpen = 0;
         break;
      case WM_VSCROLL:
         if(Zooming)
            XorZoomBox();
         switch(wp) {
            case SB_PAGEDOWN:
               Zooming += 20;
            case SB_LINEDOWN:
               Zooming = min(ZoomBarMax, Zooming + 1);
               break;
            case SB_PAGEUP:
               Zooming -= 20;
            case SB_LINEUP:
               Zooming = max(ZoomBarMin, Zooming - 1);
               break;
            case SB_TOP:
               Zooming = ZoomBarMin;
               break;
            case SB_BOTTOM:
               Zooming = ZoomBarMax;
               break;
            case SB_THUMBPOSITION:
            case SB_THUMBTRACK:
               Zooming = LOWORD(dp);
               break;
         }
         PositionZoomBar();
         CalcZoomRect();
         XorZoomBox();
         break;
      default:
         break;
   }
   return(FALSE);
}

void ZoomBar(HWND hWnd) {
   FARPROC lpFnct;

   hFractalWnd = hWnd;
   if(ZoomBarOpen)
      KillZoomBar = TRUE;
   else {
      if(lpFnct = MakeProcInstance(ZoomBarDlg, hThisInst)) {
         if(CreateDialog(hThisInst, "ZoomBar", hWnd, lpFnct))
         {
            SetFocus(hWnd);
            return;
         }
      }
      MessageBox(hWnd, "Error Opening Zoom Bar",
                 NULL, MB_ICONEXCLAMATION | MB_OK);
   }
}


/* Coordinate Box */

BOOL FAR PASCAL CoordBoxDlg(HWND hDlg, WORD Message, WORD wp, DWORD dp) {
   HMENU hDlgMenu;

   hDlgMenu = GetMenu(hDlg);
   switch(Message) {
      case WM_INITDIALOG:
         CoordBoxOpen = 1;
         ProgStr = Winfract;
         SaveParamSwitch(CoordBoxStr, TRUE);
         CheckMenuItem(GetMenu(hFractalWnd), IDM_COORD, MF_CHECKED);
         hCoordBox = hDlg;
         PositionWindow(hDlg, CoordBoxPosStr);
         return(TRUE);
      case WM_MOVE:
         SaveWindowPosition(hDlg, CoordBoxPosStr);
         break;
      case WM_CLOSE:
         KillCoordBox = 1;
         ProgStr = Winfract;
         break;
      case WM_DESTROY:
         CoordBoxOpen = 0;
         break;
      case WM_COMMAND:
         CheckMenuItem(hDlgMenu, AngleFormat, MF_UNCHECKED);
         CheckMenuItem(hDlgMenu, CoordFormat, MF_UNCHECKED);
         switch(wp) {
            case IDM_RADIANS:
            case IDM_GRAD:
            case IDM_DEGREES:
               AngleFormat = wp;
               break;
            case IDM_POLAR:
            case IDM_RECT:
            case IDM_PIXEL:
               CoordFormat = wp;
               break;
         }
         CheckMenuItem(hDlgMenu, AngleFormat, MF_CHECKED);
         CheckMenuItem(hDlgMenu, CoordFormat, MF_CHECKED);
         if(CoordFormat == IDM_POLAR) {
            SetDlgItemText(hDlg, ID_X_NAME, "|z|");
            SetDlgItemText(hDlg, ID_Y_NAME, "\xD8");
            EnableMenuItem(hDlgMenu, IDM_DEGREES, MF_ENABLED);
            EnableMenuItem(hDlgMenu, IDM_RADIANS, MF_ENABLED);
            EnableMenuItem(hDlgMenu, IDM_GRAD, MF_ENABLED);
         }
         else {
            SetDlgItemText(hDlg, ID_X_NAME, "x");
            SetDlgItemText(hDlg, ID_Y_NAME, "y");
            EnableMenuItem(hDlgMenu, IDM_DEGREES, MF_DISABLED | MF_GRAYED);
            EnableMenuItem(hDlgMenu, IDM_RADIANS, MF_DISABLED | MF_GRAYED);
            EnableMenuItem(hDlgMenu, IDM_GRAD, MF_DISABLED | MF_GRAYED);
         }
   }
   return(FALSE);
}

void UpdateCoordBox(DWORD dw) {
   unsigned xPixel, yPixel;
   double xd, yd, Angle, Modulus;
   char xStr[40], yStr[40];

   xPixel = (unsigned)dw         + win_xoffset;  /* BDT 11/6/91 */
   yPixel = (unsigned)(dw >> 16) + win_yoffset;  /* BDT 11/6/91 */
   xd = xxmin + ((double)delxx * xPixel);
   yd = yymax - ((double)delyy * yPixel);
   switch(CoordFormat) {
      case IDM_PIXEL:
         sprintf(xStr, "%d", xPixel);
         sprintf(yStr, "%d", yPixel);
         break;
      case IDM_RECT:
         sprintf(xStr, "%+.8g", xd);
         sprintf(yStr, "%+.8g", yd);
         break;
      case IDM_POLAR:
         Modulus = (xd*xd) + (yd*yd);
         if(Modulus > 1E-20) {
            Modulus = sqrt(Modulus);
            Angle = atan2(yd, xd);
            switch(AngleFormat) {
               case IDM_DEGREES:
                  Angle = (Angle / Pi) * 180;
                  break;
               case IDM_GRAD:
                  Angle = (Angle / Pi) * 200;
               case IDM_RADIANS:
                  break;
            }
         }
         else {
            Modulus = 0.0;
            Angle = 0.0;
         }
         sprintf(xStr, "%+.8g", Modulus);
         sprintf(yStr, "%+.8g", Angle);
         break;
   }
   SetDlgItemText(hCoordBox, ID_X_COORD, xStr);
   SetDlgItemText(hCoordBox, ID_Y_COORD, yStr);
}

void CoordinateBox(HWND hWnd) {
   FARPROC lpCoordBox;

   hFractalWnd = hWnd;
   if(CoordBoxOpen)
      KillCoordBox = TRUE;
   else {
      if(lpCoordBox = MakeProcInstance(CoordBoxDlg, hThisInst)) {
         if(CreateDialog(hThisInst, "CoordBox", hWnd, lpCoordBox))
            return;
      }
      MessageBox(hWnd, "Error Opening Coordinate Box",
                 NULL, MB_ICONEXCLAMATION | MB_OK);
   }
   ProgStr = Winfract;
}



/* Math Tools Window - Not Implemented Yet */

BOOL OpenMTWnd(void) {
    hMathToolsWnd = CreateWindow(
        MTClassName,
        MTWindowTitle,
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT,
        xdots, ydots,
        hFractalWnd,
        0,
        hThisInst,
        0
    );
    if(!hMathToolsWnd)
        return(FALSE);

    ShowWindow(hMathToolsWnd, SW_SHOWNORMAL);
    UpdateWindow(hMathToolsWnd);

    return(TRUE);
}

void MathToolBox(HWND hWnd) {
    hFractalWnd = hWnd;
    if(MTWindowOpen)
       DestroyWindow(hMathToolsWnd);
    else {
      if(!OpenMTWnd())
         MessageBox(hWnd, "Error Opening Math Tools Window", NULL,
                    MB_ICONEXCLAMATION | MB_OK);
    }
}


long FAR PASCAL MTWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam) {
   switch (Message) {
      case WM_CREATE:
         CheckMenuItem(GetMenu(hFractalWnd), IDM_MATH_TOOLS, MF_CHECKED);
         MTWindowOpen = 1;
         break;
      case WM_COMMAND:
         switch(wParam) {
            case IDM_EXIT:
               DestroyWindow(hWnd);
               break;
         }
         break;
      case WM_DESTROY:
         CheckMenuItem(GetMenu(hFractalWnd), IDM_MATH_TOOLS, MF_UNCHECKED);
         MTWindowOpen = 0;
         break;
      default:
         return(DefWindowProc(hWnd, Message, wParam, lParam));
    }
    return(0L);
}
