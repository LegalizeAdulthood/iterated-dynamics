/****************************************************************************

    PROGRAM: Select.c

    PURPOSE: Contains library routines for selecting a region

    FUNCTIONS:

        StartSelection(HWND, POINT, LPRECT, int) - begin selection area
        UpdateSelection(HWND, POINT, LPRECT, int) - update selection area
        EndSelection(POINT, LPRECT) - end selection area
        ClearSelection(HWND, LPRECT, int) - clear selection area

*******************************************************************************/

#include "windows.h"
#include "select.h"

extern BOOL bTrack, bMove, bMoving;

/****************************************************************************

    FUNCTION: StartSelection(HWND, POINT, LPRECT, int)

    PURPOSE: Begin selection of region

****************************************************************************/


POINT Center, DragPoint, ZoomDim;

void FAR PASCAL StartSelection(hWnd, ptCurrent, lpSelectRect, fFlags)
HWND hWnd;
POINT ptCurrent;
LPRECT lpSelectRect;
int fFlags;
{
    if (lpSelectRect->left != lpSelectRect->right ||
            lpSelectRect->top != lpSelectRect->bottom)
        ClearSelection(hWnd, lpSelectRect, fFlags);

    lpSelectRect->right = ptCurrent.x;
    lpSelectRect->bottom = ptCurrent.y;

    /* If you are extending the box, then invert the current rectangle */

    if ((fFlags & SL_SPECIAL) == SL_EXTEND)
        ClearSelection(hWnd, lpSelectRect, fFlags);

    /* Otherwise, set origin to current location */

    else {
        lpSelectRect->left = ptCurrent.x;
        lpSelectRect->top = ptCurrent.y;
        Center = ptCurrent;
    }
    SetCapture(hWnd);
}

extern int xdots, ydots;

/****************************************************************************

    FUNCTION: UpdateSelection(HWND, POINT, LPRECT, int) - update selection area

    PURPOSE: Update selection

****************************************************************************/

void FAR PASCAL UpdateSelection(hWnd, ptCurrent, lpSelectRect, fFlags)
HWND hWnd;
POINT ptCurrent;
LPRECT lpSelectRect;
int fFlags;
{
    HDC hDC;
    short OldROP;
    POINT NewDim, NewPoint;
    unsigned ScaledY;

    hDC = GetDC(hWnd);

    switch (fFlags & SL_TYPE) {

        case SL_BOX:
             OldROP = SetROP2(hDC, R2_NOTXORPEN);
            MoveTo(hDC, lpSelectRect->left, lpSelectRect->top);
            LineTo(hDC, lpSelectRect->right, lpSelectRect->top);
            LineTo(hDC, lpSelectRect->right, lpSelectRect->bottom);
            LineTo(hDC, lpSelectRect->left, lpSelectRect->bottom);
            LineTo(hDC, lpSelectRect->left, lpSelectRect->top);

            LineTo(hDC, ptCurrent.x, lpSelectRect->top);
            LineTo(hDC, ptCurrent.x, ptCurrent.y);
            LineTo(hDC, lpSelectRect->left, ptCurrent.y);
            LineTo(hDC, lpSelectRect->left, lpSelectRect->top);
            SetROP2(hDC, OldROP);
            break;

        case SL_BLOCK:
            PatBlt(hDC,
                lpSelectRect->left,
                lpSelectRect->bottom,
                lpSelectRect->right - lpSelectRect->left,
                ptCurrent.y - lpSelectRect->bottom,
                DSTINVERT);
            PatBlt(hDC,
                lpSelectRect->right,
                lpSelectRect->top,
                ptCurrent.x - lpSelectRect->right,
                ptCurrent.y - lpSelectRect->top,
                DSTINVERT);
            break;

        /* MCP 6-3-92 */
        case (SL_ZOOM):
           OldROP = SetROP2(hDC, R2_NOTXORPEN);
           MoveTo(hDC, lpSelectRect->left, lpSelectRect->top);
           LineTo(hDC, lpSelectRect->right, lpSelectRect->top);
           LineTo(hDC, lpSelectRect->right, lpSelectRect->bottom);
           LineTo(hDC, lpSelectRect->left, lpSelectRect->bottom);
           LineTo(hDC, lpSelectRect->left, lpSelectRect->top);

           if(bTrack)
           {
              NewPoint = ptCurrent;

              NewDim.x = Center.x - NewPoint.x;
              if(NewDim.x < 0)
                 NewDim.x = -NewDim.x;

              NewDim.y = Center.y - NewPoint.y;
              if(NewDim.y < 0)
                 NewDim.y = -NewDim.y;

              ScaledY = (unsigned)(((long)NewDim.y) * xdots / ydots);
              if(NewDim.x < (long)ScaledY)
                 NewDim.x = ScaledY;
              else
                 NewDim.y = (unsigned)(((long)NewDim.x) * ydots / xdots);

              if(NewDim.x < 2)
                 NewDim.x = 2;
              if(NewDim.y < 2)
                 NewDim.y = 2;

              ZoomDim = NewDim;
           }
           else if(bMoving)
           {
              Center.x += ptCurrent.x - DragPoint.x;
              Center.y += ptCurrent.y - DragPoint.y;
              DragPoint = ptCurrent;
           }

           if (bTrack || bMoving) {
              lpSelectRect->left   = Center.x - ZoomDim.x;
              lpSelectRect->right  = Center.x + ZoomDim.x;
              lpSelectRect->bottom = Center.y + ZoomDim.y;
              lpSelectRect->top    = Center.y - ZoomDim.y;

                   MoveTo(hDC, lpSelectRect->left, lpSelectRect->top);
              LineTo(hDC, lpSelectRect->right, lpSelectRect->top);
              LineTo(hDC, lpSelectRect->right, lpSelectRect->bottom);
              LineTo(hDC, lpSelectRect->left, lpSelectRect->bottom);
              LineTo(hDC, lpSelectRect->left, lpSelectRect->top);
              }

           SetROP2(hDC, OldROP);
           ReleaseDC(hWnd, hDC);
           return;
    }
    lpSelectRect->right = ptCurrent.x;
    lpSelectRect->bottom = ptCurrent.y;
    ReleaseDC(hWnd, hDC);
}

/****************************************************************************

    FUNCTION: EndSelection(POINT, LPRECT)

    PURPOSE: End selection of region, release capture of mouse movement

****************************************************************************/

void FAR PASCAL EndSelection(ptCurrent, lpSelectRect)
POINT ptCurrent;
LPRECT lpSelectRect;
{
    if(!bMove)
    {
       lpSelectRect->right = ptCurrent.x;
       lpSelectRect->bottom = ptCurrent.y;
    }
    ReleaseCapture();
}

/****************************************************************************

    FUNCTION: ClearSelection(HWND, LPRECT, int) - clear selection area

    PURPOSE: Clear the current selection

****************************************************************************/

void FAR PASCAL ClearSelection(hWnd, lpSelectRect, fFlags)
HWND hWnd;
LPRECT lpSelectRect;
int fFlags;
{
    HDC hDC;
    short OldROP;

    hDC = GetDC(hWnd);
    switch (fFlags & SL_TYPE) {

        /* MCP 6-3-92 */
        case (SL_ZOOM):
        case SL_BOX:
            OldROP = SetROP2(hDC, R2_NOTXORPEN);
            MoveTo(hDC, lpSelectRect->left, lpSelectRect->top);
            LineTo(hDC, lpSelectRect->right, lpSelectRect->top);
            LineTo(hDC, lpSelectRect->right, lpSelectRect->bottom);
            LineTo(hDC, lpSelectRect->left, lpSelectRect->bottom);
            LineTo(hDC, lpSelectRect->left, lpSelectRect->top);
            SetROP2(hDC, OldROP);
            break;

        case SL_BLOCK:
            PatBlt(hDC,
                lpSelectRect->left,
                lpSelectRect->top,
                lpSelectRect->right - lpSelectRect->left,
                lpSelectRect->bottom - lpSelectRect->top,
                DSTINVERT);
            break;

    }
    ReleaseDC(hWnd, hDC);
}
