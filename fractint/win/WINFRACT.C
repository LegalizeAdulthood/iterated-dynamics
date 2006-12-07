/****************************************************************************


    PROGRAM: winfract.c

    PURPOSE: Windows-specific main-driver code for Fractint for Windows
             (look in MAINFRAC.C for the non-windows-specific code)

    Copyright (C) 1990-1993 The Stone Soup Group.  Fractint for Windows
    may be freely copied and distributed, but may not be sold.

    We are, of course, copyrighting the code we wrote to implement
    Fractint-for-Windows, and not the routines we lifted directly
    or indirectly from Microsoft's Windows 3.0 Software Development Kit.

****************************************************************************/

#define STRICT

#include "port.h"
#include "prototyp.h"

#include <windows.h>
#include <search.h>
#include <time.h>
#include <string.h>

#include "winfract.h"
#include "mathtool.h"
#include "fractype.h"
#include "select.h"
#include "profile.h"

unsigned int windows_version;                /* 0x0300 = Win 3.0, 0x030A = 3.1 */

extern LPSTR win_lpCmdLine;

HANDLE hInst;

HANDLE hAccTable;                        /* handle to accelerator table */
extern BOOL ZoomBarOpen;
int ZoomMode;

HWND hMainWnd, hwnd;                     /* handle to main window */
HWND hWndCopy;                                 /* Copy of hWnd */

char far winfract_title_text[41];        /* Winfract title-bar text */

#define PALETTESIZE 256               /* dull-normal VGA                    */
HANDLE hpal;                          /* palette handle                     */
PAINTSTRUCT ps;                       /* paint structure                    */
HDC hDC;                              /* handle to device context           */
HDC hMemoryDC;                        /* handle to memory device context    */
BITMAP Bitmap;                        /* bitmap structure                   */
extern time_t last_time;
unsigned IconWidth, IconHeight, IconSize;
HANDLE hIconBitmap;

HANDLE  hPal;          /* Handle to the application's logical palette  */
HANDLE  hLogPal;       /* Temporary Handle */
LPLOGPALETTE pLogPal;  /* pointer to the application's logical palette */
int     iNumColors;    /* Number of colors supported by device               */
int     iRasterCaps;   /* Raster capabilities                                       */
int     iPalSize;      /* Size of Physical palette                               */

BOOL        win_systempaletteused = FALSE;        /* flag system palette set */
extern int win_syscolorindex[21];
extern DWORD win_syscolorold[21];
extern DWORD win_syscolornew[21];

/* MCP 6-16-91 */
extern int pixelshift_per_byte;
extern long pixels_per_bytem1;        /* pixels / byte - 1 (for ANDing) */
extern unsigned char win_andmask[8];
extern unsigned char win_notmask[8];
extern unsigned char win_bitshift[8];

extern int CoordBoxOpen;
extern HWND hCoordBox;
extern int TrackingZoom, Zooming, ReSizing;

#define EXE_NAME_MAX_SIZE  128

BOOL       bHelp = FALSE;      /* Help mode flag; TRUE = "ON"*/
HCURSOR    hHelpCursor;        /* Cursor displayed when in help mode*/
char       szHelpFileName[EXE_NAME_MAX_SIZE+1];    /* Help file name*/

void MakeHelpPathName(char*);  /* Function deriving help file path */

unsigned char far win_dacbox[256][3];

int win_fastupdate;                   /* 0 for "normal" fast screen updates */

int max_colors;

BOOL bTrack = FALSE;                  /* TRUE if user is selecting a region */
BOOL bMove  = FALSE;
BOOL bMoving = FALSE;
BOOL zoomflag = FALSE;                /* TRUE if a zoom-box selected */
extern POINT DragPoint;
RECT Rect;

int Shape = SL_ZOOM;            /* shape to use for the selection rectangle */

/* pointers to various dialog-box routines */
FARPROC lpProcAbout;
FARPROC lpSelectFractal;
FARPROC lpSelectFracParams;
FARPROC lpSelectImage;
FARPROC lpSelectDoodads;
FARPROC lpSelectExtended;
FARPROC lpSelectSavePar;
FARPROC lpSelectCycle;
FARPROC lpProcStatus;
FARPROC lpSelect3D;
FARPROC lpSelect3DPlanar;
FARPROC lpSelect3DSpherical;
FARPROC lpSelectIFS3D;
FARPROC lpSelectFunnyGlasses;
FARPROC lpSelectLightSource;
FARPROC lpSelectStarfield;

extern int FileFormat;
extern unsigned char DefPath[];
extern char far StatusTitle[];
unsigned char far DialogTitle[128];
unsigned char FileName[128];
unsigned char FullPathName[FILE_MAX_DIR];
unsigned char DefSpec[13];
unsigned char DefExt[10];

HBITMAP hBitmap, oldBitmap, oldoldbitmap;              /* working bitmaps */

HANDLE hDibInfo;                /* handle to the Device-independent bitmap */
LPBITMAPINFO pDibInfo;                /* pointer to the DIB info */
HANDLE hpixels;                        /* handle to the DIB pixels */
unsigned char huge *pixels;        /* the device-independent bitmap  pixels */
extern int bytes_per_pixelline;        /* pixels/line / pixels/byte */
extern long win_bitmapsize;     /* size of the DIB in bytes */
extern        int        resave_flag;        /* resaving after a timed save */
extern        char fract_overwrite;         /* overwrite on/off */

HANDLE hClipboard1, hClipboard2, hClipboard3; /* handles to clipboard info */
LPSTR lpClipboard1, lpClipboard2;            /* pointers to clipboard info */

int last_written_y = -2;        /* last line written */
int screen_to_be_cleared = 1;        /* flag that the screen is to be cleared */
int time_to_act = 0;                /* time to take some sort of action? */
int time_to_restart = 0;        /* time to restart?  */
int time_to_reinit = 0;         /* time to reinitialize?  */
int time_to_resume = 0;                /* time to resume? */
int time_to_quit = 0;           /* time to quit? */
int time_to_save = 0;                /* time to save the file? */
int time_to_print = 0;                /* time to print the file? */
int time_to_load = 0;                /* time to load a new file? */
int time_to_cycle = 0;          /* time to begin color-cycling? */
int time_to_starfield = 0;      /* time to make a starfield? */
int time_to_orbit = 0;          /* time to activate orbits? */

int win_3dspherical = 0;          /* spherical 3D? */
int win_display3d, win_overlay3d; /* 3D flags */
extern        int        init3d[];        /* '3d=nn/nn/nn/...' values */
extern        int RAY;

extern int win_cycledir, win_cyclerand, win_cycledelay;

extern int calc_status;
extern int julibrot;

int xdots, ydots, colors;
long maxiter;
int ytop, ybottom, xleft, xright;
int xposition, yposition, win_xoffset, win_yoffset, xpagesize, ypagesize;
int win_xdots, win_ydots;
double jxxmin, jxxmax, jyymin, jyymax, jxx3rd, jyy3rd;
extern int frommandel;

int cpu, fpu;                        /* cpu, fpu flags */

extern int win_release;

char *win_choices[110];
int win_numchoices, win_choicemade;

extern int onthelist[];
extern int CountFractalList;
extern int CurrentFractal;
int MaxFormNameChoices = 80;
char FormNameChoices[80][25];
extern char FormName[];
extern char        IFSFileName[];    /* IFS code file */
extern char        IFSName[];        /* IFS code item */
double far *temp_array;
HANDLE htemp_array;

HANDLE hSaveCursor;             /* the original cursor value */
HANDLE hHourGlass;              /* the hourglass cursor value */

BOOL winfract_menustyle = FALSE;/* Menu style:
                                   FALSE = Winfract-style,
                                   TRUE  = Fractint-style */

/* far strings (near space is precious) */
char far winfract_msg01[] = "Fractint For Windows";
char far winfract_msg02[] = "WinFracMenu";
char far winfract_msg03[] = "FractintForWindowsV0010";
char far winfract_msg04[] = "WinfractAcc";
char far winfract_msg96[] = "I'm sorry, but color-cycling \nrequires a palette-based\nvideo driver";
char far winfract_msg97[] = "There isn't enough available\nmemory to run Winfract";
char far winfract_msg98[] =  "This program requires Standard\nor 386-Enhanced Mode";
char far winfract_msg99[] = "Not Enough Free Memory to Copy to the Clipboard";


int PASCAL WinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow)
HINSTANCE hInstance;
HINSTANCE hPrevInstance;
LPSTR lpCmdLine;
int nCmdShow;
{
    win_lpCmdLine = lpCmdLine;

    if (!hPrevInstance)
        if (!InitApplication(hInstance))
            return (FALSE);

    if (!InitInstance(hInstance, nCmdShow))
        return (FALSE);

    fractint_main();            /* fire up the main Fractint code */
    if(htemp_array) {
        GlobalUnlock(htemp_array);
        GlobalFree(htemp_array);
    }

    wintext_destroy();                /* destroy the text window */

    DestroyWindow(hWndCopy);    /* stop everything when it returns */

    return(0);                  /* we done when 'fractint_main' returns */
}


BOOL InitApplication(hInstance)
HANDLE hInstance;
{
    WNDCLASS  wc;

    wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = MainWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName =  winfract_msg02;
    wc.lpszClassName = winfract_msg03;

    return(RegisterClass(&wc) && RegisterMathWindows(hInstance));
}


BOOL InitInstance(hInstance, nCmdShow)
    HANDLE          hInstance;
    int             nCmdShow;
{
    DWORD WinFlags;
    unsigned int version;
    int iLoop;

    autobrowse     = FALSE;
    brwschecktype  = TRUE;
    brwscheckparms = TRUE;
    doublecaution  = TRUE;
    no_sub_images = FALSE;
    toosmall = 6;
    minbox   = 3;
    strcpy(browsemask,"*.gif");
    strcpy(browsename,"            ");
    name_stack_ptr= -1; /* init loaded files stack */

    evolving = FALSE;
    paramrangex = 4;
    opx = newopx = -2.0;
    paramrangey = 3;
    opy = newopy = -1.5;
    odpx = odpy = 0;
    gridsz = 9;
    fiddlefactor = 1;
    fiddle_reduction = 1.0;
    this_gen_rseed = (unsigned int)clock_ticks();
    srand(this_gen_rseed);
    initgene(); /*initialise pointers to lots of fractint variables for the evolution engine*/

    /* so, what kind of a computer are we on, anyway? */
    WinFlags = GetWinFlags();
    cpu = 88;                             /* determine the CPU type */
    if (WinFlags & WF_CPU186) cpu = 186;
    if (WinFlags & WF_CPU286) cpu = 286;
    if (WinFlags & WF_CPU386) cpu = 386;
    if (WinFlags & WF_CPU486) cpu = 386;
    fpu = 0;                              /* determine the FPU type */
    if (WinFlags & WF_80x87)  fpu = 87;
    if (fpu && (cpu == 386))  fpu = 387;

    version = LOWORD(GetVersion());        /* which version of Windows is it? */
    windows_version = ((LOBYTE(version) << 8) | HIBYTE(version));

    hInst = hInstance;

    hAccTable = LoadAccelerators(hInst, winfract_msg04);

    win_set_title_text();

    hMainWnd = hwnd = CreateWindow(
        winfract_msg03,
        winfract_title_text,
        WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
        140, 100,  /* initial locn instead of CW_USEDEFAULT, CW_USEDEFAULT, */
        360, 280,  /* initial size instead of CW_USEDEFAULT, CW_USEDEFAULT, */
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd) {  /* ?? can't create the initial window! */
        return (FALSE);
        }

    wintext_initialize(
        (HANDLE) hInstance,
        (HWND) hwnd,
        (LPSTR) winfract_title_text);

    /* This program doesn't run in "real" mode, so shut down right
       now to keep from mucking up Windows */
    if (!((WinFlags & WF_STANDARD) || (WinFlags & WF_ENHANCED))) {
        MessageBox (
            GetFocus(),
            winfract_msg98,
            winfract_msg01,
            MB_ICONSTOP | MB_OK);
        return(FALSE);
        }

    win_xdots = xdots;
    win_ydots = ydots;
    maxiter = 150;                   /* and a few iterations */
    xposition = yposition = 0;       /* dummy up a few pointers */
    xpagesize = ypagesize = 2000;
    set_win_offset();

    SizeWindow(hwnd);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    /* let's ensure that we have at lease 40K of free memory */
    {
    HANDLE temphandle;
    if (!(temphandle = GlobalAlloc(GMEM_FIXED,40000L)) ||
        !(htemp_array = GlobalAlloc(GMEM_FIXED, 5L * sizeof(double) * MAXPIXELS))) {
        MessageBox (
            GetFocus(),
            winfract_msg97,
            winfract_msg01,
            MB_ICONSTOP | MB_OK);
        return(FALSE);
        }
        GlobalLock(temphandle);
        GlobalUnlock(temphandle);
        GlobalFree(temphandle);
        temp_array = (double far *)GlobalLock(htemp_array);
    }

   MakeHelpPathName(szHelpFileName);

    /* so, what kind of a display are we using, anyway? */
    hDC = GetDC(NULL);
    iPalSize    = GetDeviceCaps (hDC, SIZEPALETTE);
    iRasterCaps = GetDeviceCaps (hDC, RASTERCAPS);
    iRasterCaps = (iRasterCaps & RC_PALETTE) ? TRUE : FALSE;
    if (iRasterCaps)
       iNumColors = GetDeviceCaps(hDC, SIZEPALETTE);
    else
       iNumColors = GetDeviceCaps(hDC, NUMCOLORS);
    ReleaseDC(NULL,hDC);

    /* fudges for oddball stuff (is there any?) */
    /* also, note that "true color" devices return -1 for NUMCOLORS */
    colors = iNumColors;
    if (colors < 2 || colors > 16) colors = 256;
    if (colors > 2 && colors < 16) colors = 16;
    /* adjust for Windows' 20 reserved palettes in 256-color mode */
    max_colors = 256;
    if (colors == 256  && iNumColors >= 0) max_colors = 236;

     /* Allocate enough memory for a logical palette with
      * PALETTESIZE entries and set the size and version fields
      * of the logical palette structure.
      */
     if (!(hLogPal = GlobalAlloc (GMEM_FIXED,
        (sizeof (LOGPALETTE) +
        (sizeof (PALETTEENTRY) * (PALETTESIZE)))))) {
        MessageBox (
            GetFocus(),
            winfract_msg97,
            winfract_msg01,
            MB_ICONSTOP | MB_OK);
        return(FALSE);
          }
    pLogPal = (LPLOGPALETTE)GlobalLock(hLogPal);
    pLogPal->palVersion    = 0x300;
    pLogPal->palNumEntries = PALETTESIZE;
    /* fill in intensities for all palette entry colors */
    for (iLoop = 0; iLoop < PALETTESIZE; iLoop++) {
        pLogPal->palPalEntry[iLoop].peRed   = iLoop;
        pLogPal->palPalEntry[iLoop].peGreen = 0;
        pLogPal->palPalEntry[iLoop].peBlue  = 0;
        pLogPal->palPalEntry[iLoop].peFlags = PC_EXPLICIT;
        }
    /* flip the ugly red color #1 with the pretty blue color #4 */
/*
    if (iNumColors < 0 || iNumColors > 4) {
        pLogPal->palPalEntry[1].peRed = 4;
        pLogPal->palPalEntry[4].peRed = 1;
        }
*/
    /*  create a logical color palette according the information
        in the LOGPALETTE structure. */
    hDC = GetDC(GetFocus());
    SetMapMode(hDC,MM_TEXT);
    hPal = CreatePalette ((LPLOGPALETTE) pLogPal) ;
    ReleaseDC(GetFocus(),hDC);

    /* allocate a device-independent bitmap header */
    if (!(hDibInfo = GlobalAlloc(GMEM_FIXED,
        sizeof(BITMAPINFOHEADER)+256*sizeof(PALETTEENTRY)))) {
        MessageBox (
            GetFocus(),
            winfract_msg97,
            winfract_msg01,
            MB_ICONSTOP | MB_OK);
        return(FALSE);
        }
    pDibInfo = (LPBITMAPINFO)GlobalLock(hDibInfo);
    /* fill in the header */
    pDibInfo->bmiHeader.biSize  = (long)sizeof(BITMAPINFOHEADER);
    pDibInfo->bmiHeader.biWidth  = win_xdots;
    pDibInfo->bmiHeader.biHeight = win_ydots;
    pDibInfo->bmiHeader.biSizeImage = (DWORD)win_xdots * win_ydots;
    pDibInfo->bmiHeader.biPlanes = 1;
    pDibInfo->bmiHeader.biBitCount = 8;
    pDibInfo->bmiHeader.biCompression = BI_RGB;
    pDibInfo->bmiHeader.biXPelsPerMeter = 0L;
    pDibInfo->bmiHeader.biYPelsPerMeter = 0L;
    pDibInfo->bmiHeader.biClrUsed = 0L;
    pDibInfo->bmiHeader.biClrImportant = 0L;
    default_dib_palette();

    IconWidth = GetSystemMetrics(SM_CXICON);
    IconHeight = GetSystemMetrics(SM_CYICON);
    IconSize = (IconWidth * IconHeight) >> pixelshift_per_byte;
    hIconBitmap = GlobalAlloc(GMEM_MOVEABLE, IconSize);

    /* initialize our delay counter */
    CalibrateDelay();
    win_cycledelay = 15;

    /* obtain an hourglass cursor */
    hHourGlass  = LoadCursor(NULL, IDC_WAIT);
    hSaveCursor = LoadCursor(NULL, IDC_ARROW);

    /* allocate and lock a pixel array for the initial bitmap */
    hpixels = (HANDLE) 0;
    pixels = (char huge *) NULL;
    if (hIconBitmap && clear_screen(0))
        return(TRUE);

    MessageBox (
        GetFocus(),
        winfract_msg97,
        winfract_msg01,
         MB_ICONSTOP | MB_OK);

    return (FALSE);

}

void lmemcpy(char huge *to, char huge *from, long len)
{
long i;

for (i = 0; i < len; i++)
  to[i] = from[i];
}

HWND SecondaryhWnd;
UINT Secondarymessage;
WPARAM SecondarywParam;
LPARAM SecondarylParam;


long FAR PASCAL MainWndProc(hWnd, message, wParam, lParam)
HWND hWnd;                                /* handle to main window */
UINT message;
WPARAM wParam;
LPARAM lParam;
{

    RECT tempRect;
    HMENU hMenu;

    int i;
    extern char FractintMenusStr[];
    extern char FractintPixelsStr[];

    switch (message) {

        case WM_INITMENU:
           if (!iRasterCaps || iNumColors < 16) {
               EnableMenuItem(GetMenu(hWnd), IDM_CYCLE, MF_DISABLED | MF_GRAYED);
               }
           hMenu = GetMenu(hWnd);
           if (winfract_menustyle) {
               CheckMenuItem(hMenu, IDF_FRACTINTSTYLE, MF_CHECKED);
               CheckMenuItem(hMenu, IDF_WINFRACTSTYLE, MF_UNCHECKED);
               }
           else {
               CheckMenuItem(hMenu, IDF_FRACTINTSTYLE, MF_UNCHECKED);
               CheckMenuItem(hMenu, IDF_WINFRACTSTYLE, MF_CHECKED);
               }
           if (win_fastupdate)
               CheckMenuItem(hMenu, IDM_PIXELS, MF_CHECKED);
           else
               CheckMenuItem(hMenu, IDM_PIXELS, MF_UNCHECKED);
           hWndCopy = hWnd;
           return (TRUE);

        case WM_LBUTTONDBLCLK:
            if(Zooming) {
               win_savedac();
               ExecuteZoom();
               }
            if (bMove) {
               DragPoint = MAKEPOINT(lParam);
        ExecZoom:
               /* End the selection */
               EndSelection(DragPoint, &Rect);
               ClearSelection(hWnd, &Rect, Shape);
               win_title_text(-1);

               if(PtInRect(&Rect, DragPoint)) {
                  if (abs(Rect.bottom - Rect.top ) > 5 &&
                      abs(Rect.right  - Rect.left) > 5) {
                        double xd, yd, z;
                        double tmpx, tmpy;
                        bf_t bfxd, bfyd;
                        bf_t bftmpx, bftmpy;
                        bf_t bftmpzoomx, bftmpzoomy;
                        int saved=0;
                        extern POINT Center, ZoomDim;

                        if(bf_math)
                        {
                           saved = save_stack();
                           bfxd = alloc_stack(rbflength+2);
                           bfyd  = alloc_stack(rbflength+2);
                           bftmpzoomx = alloc_stack(rbflength+2);
                           bftmpzoomy = alloc_stack(rbflength+2);
                           bftmpx = alloc_stack(rbflength+2);
                           bftmpy = alloc_stack(rbflength+2);
                         }

                        z = (double)(ZoomDim.x << 1) / xdots;
                        if(ZoomMode == IDM_ZOOMOUT) {
                            z = 1.0 / z;
                            xd = (xxmin + xxmax) / 2 - (double)(delxx * z * (Center.x + win_xoffset - (xdots/2)));
                            yd = (yymin + yymax) / 2 + (double)(delyy * z * (Center.y + win_yoffset - (ydots/2)));
                            if(bf_math) {
                               tmpx = z * (Center.x + win_xoffset - (xdots/2));
                               tmpy = z * (Center.y + win_yoffset - (ydots/2));
                               floattobf(bftmpx, tmpx);
                               floattobf(bftmpy, tmpy);
                               mult_bf(bftmpzoomx, bfxdel, bftmpx);
                               mult_bf(bftmpzoomy, bfydel, bftmpy);
                               add_bf(bftmpx, bfxmin, bfxmax);
                               add_bf(bftmpy, bfymin, bfymax);
                               sub_bf(bfxd, half_a_bf(bftmpx), bftmpzoomx);
                               add_bf(bfyd, half_a_bf(bftmpy), bftmpzoomy);
                               }
                            }
                        else {
                            xd = xxmin + (double)(delxx * (Center.x + win_xoffset));  /* BDT 11/6/91 */
                            yd = yymax - (double)(delyy * (Center.y + win_yoffset));  /* BDT 11/6/91 */
                            if(bf_math) {
                               tmpx = Center.x + win_xoffset;
                               tmpy = Center.y + win_yoffset;
                               floattobf(bftmpx, tmpx);
                               floattobf(bftmpy, tmpy);
                               mult_bf(bftmpzoomx, bfxdel, bftmpx);
                               mult_bf(bftmpzoomy, bfydel, bftmpy);
                               add_bf(bfxd, bfxmin, bftmpzoomx);
                               sub_bf(bfyd, bfymax, bftmpzoomy);
                               }
                            }
                        xxmin = xd - (double)(delxx * z * (xdots / 2));
                        xxmax = xd + (double)(delxx * z * (xdots / 2));
                        yymin = yd - (double)(delyy * z * (ydots / 2));
                        yymax = yd + (double)(delyy * z * (ydots / 2));
                        if(bf_math) {
                           tmpx = z * (xdots / 2);
                           tmpy = z * (ydots / 2);
                           floattobf(bftmpx, tmpx);
                           floattobf(bftmpy, tmpy);
                           mult_bf(bftmpzoomx, bfxdel, bftmpx);
                           mult_bf(bftmpzoomy, bfydel, bftmpy);
                           sub_bf(bfxmin, bfxd, bftmpzoomx);
                           add_bf(bfxmax, bfxd, bftmpzoomx);
                           sub_bf(bfymin, bfyd, bftmpzoomy);
                           add_bf(bfymax, bfyd, bftmpzoomy);
                           restore_stack(saved);
                           }
                        }

                      zoomflag = TRUE;
                      win_savedac();
                      time_to_restart = 1;
                      time_to_cycle = 0;
                      calc_status = 0;
                  }

                  bMove = FALSE;
                  bMoving = FALSE;
                  bTrack = FALSE;
               }

            break;

        case WM_LBUTTONDOWN:           /* message: left mouse button pressed */

            /* Start selection of region */

            if(bMove)
            {
               DragPoint = MAKEPOINT(lParam);
               bMoving   = PtInRect(&Rect, DragPoint);
               if(bMoving)
                  SetCapture(hWnd);
            }
            else if(Zooming)
               StartZoomTracking(lParam);
            else if(ZoomMode == IDM_ZOOMIN || ZoomMode == IDM_ZOOMOUT)
            {
               win_title_text(3);
               bTrack = TRUE;
               bMoving = FALSE;
               bMove = FALSE;
               SetRectEmpty(&Rect);
               StartSelection(hWnd, MAKEPOINT(lParam), &Rect,
                   (wParam & MK_SHIFT) ? (SL_EXTEND | Shape) : Shape);
            }
            break;

        case WM_MOUSEMOVE:                        /* message: mouse movement */

            /* Update the selection region */

            if (bTrack || bMoving)
                UpdateSelection(hWnd, MAKEPOINT(lParam), &Rect, Shape);
            if(CoordBoxOpen)
                UpdateCoordBox(lParam);
            if(TrackingZoom)
               TrackZoom(lParam);
            break;

        case WM_LBUTTONUP:            /* message: left mouse button released */

            if(bTrack)
            {
               bTrack = FALSE;
               bMove = TRUE;
               ReleaseCapture();
               if (abs(Rect.bottom - Rect.top ) <= 5 ||
                   abs(Rect.right  - Rect.left) <= 5) {
                   /* Zoom Box is too small - kill it off */
                   ClearSelection(hWnd, &Rect, Shape);
                   win_title_text(-1);
                   bMove = bMoving = bTrack = FALSE;
                   }
            }
            else if(bMoving)
            {
               bMoving = FALSE;
               ReleaseCapture();
            }
            else if(TrackingZoom)
               EndZoom(lParam);
            break;

        case WM_RBUTTONUP:
            {
            int xx, yy;
            win_kill_all_zooming();
            xx = LOWORD(lParam);
            yy = HIWORD(lParam);
            xx += win_xoffset;
            yy += win_yoffset;
            if (xx >= xdots || yy >= ydots || bf_math)
                break;
            if (fractalspecific[fractype].tojulia != NOFRACTAL
                && param[0] == 0.0 && param[1] == 0.0) {
                /* switch to corresponding Julia set */
                fractype = fractalspecific[fractype].tojulia;
                curfractalspecific = &fractalspecific[fractype];
                param[0] = xxmin + (xxmax - xxmin) * xx / xdots;
                param[1] = yymax - (yymax - yymin) * yy / ydots;
                jxxmin = xxmin; jxxmax = xxmax;
                jyymax = yymax; jyymin = yymin;
                jxx3rd = xx3rd; jyy3rd = yy3rd;
                frommandel = 1;
                xxmin = fractalspecific[fractype].xmin;
                xxmax = fractalspecific[fractype].xmax;
                yymin = fractalspecific[fractype].ymin;
                yymax = fractalspecific[fractype].ymax;
                xx3rd = xxmin;
                yy3rd = yymin;
                if(biomorph != -1 && bitshift != 29) {
                   xxmin *= 3.0;
                   xxmax *= 3.0;
                   yymin *= 3.0;
                   yymax *= 3.0;
                   xx3rd *= 3.0;
                   yy3rd *= 3.0;
                   }
                calc_status = 0;
                }
            else if (fractalspecific[fractype].tomandel != NOFRACTAL) {
                /* switch to corresponding Mandel set */
                fractype = fractalspecific[fractype].tomandel;
                curfractalspecific = &fractalspecific[fractype];
                if (frommandel) {
                    xxmin = jxxmin;  xxmax = jxxmax;
                    yymin = jyymin;  yymax = jyymax;
                    xx3rd = jxx3rd;  yy3rd = jyy3rd;
                    }
                else {
                    double ccreal,ccimag;
                    ccreal = (fractalspecific[fractype].xmax - fractalspecific[fractype].xmin) / 2;
                    ccimag = (fractalspecific[fractype].ymax - fractalspecific[fractype].ymin) / 2;
                    xxmin = xx3rd = param[0] - ccreal;
                    xxmax = param[0] + ccreal;
                    yymin = yy3rd = param[1] - ccimag;
                    yymax = param[1] + ccimag;
                    }
                frommandel = 0;
                param[0] = 0;
                param[1] = 0;
                calc_status = 0;
                }
            else {
                buzzer(2); /* can't switch */
                break;
                }

            ytop    = 0;
            ybottom = ydots-1;
            xleft   = 0;
            xright  = xdots-1;

            time_to_restart  = 1;
            time_to_cycle = 0;
            calc_status = 0;
            }
            break;

        case WM_CREATE:

            /* the scroll bars are hard-coded to 100 possible values */
            xposition = yposition = 0;      /* initial scroll-bar positions */
            SetScrollRange(hWnd,SB_HORZ,0,100,FALSE);
            SetScrollRange(hWnd,SB_VERT,0,100,FALSE);
            SetScrollPos(hWnd,SB_HORZ,xposition,TRUE);
            SetScrollPos(hWnd,SB_VERT,yposition,TRUE);
            InitializeParameters(hWnd);
            break;

        case WM_SIZE:
            win_kill_all_zooming();
            xpagesize = LOWORD(lParam);                /* remember the window size */
            ypagesize = HIWORD(lParam);
            set_win_offset();
            if(!ReSizing && !IsIconic(hWnd))
               ReSizeWindow(hWnd);
            break;

        case WM_MOVE:
            SaveWindowPosition(hWnd, WinfractPosStr);
            break;

        case WM_HSCROLL:
            win_kill_all_zooming();
            switch (wParam) {
               case SB_LINEDOWN:       xposition += 1; break;
               case SB_LINEUP:         xposition -= 1; break;
               case SB_PAGEDOWN:       xposition += 10; break;
               case SB_PAGEUP:         xposition -= 10; break;
               case SB_THUMBTRACK:
               case SB_THUMBPOSITION:  xposition = LOWORD(lParam);
               default:                break;
               }
            if (xposition > 100) xposition = 100;
            if (xposition <     0) xposition = 0;
            if (xposition != GetScrollPos(hWnd,SB_HORZ)) {
               SetScrollPos(hWnd,SB_HORZ,xposition,TRUE);
               InvalidateRect(hWnd,NULL,TRUE);
               }
            set_win_offset();
           break;
        case WM_VSCROLL:
            win_kill_all_zooming();
            switch (wParam) {
               case SB_LINEDOWN:       yposition += 1; break;
               case SB_LINEUP:         yposition -= 1; break;
               case SB_PAGEDOWN:       yposition += 10; break;
               case SB_PAGEUP:         yposition -= 10; break;
               case SB_THUMBTRACK:
               case SB_THUMBPOSITION:  yposition = LOWORD(lParam);
               default:                break;
               }
            if (yposition > 100) yposition = 100;
            if (yposition <     0) yposition = 0;
            if (yposition != GetScrollPos(hWnd,SB_VERT)) {
               SetScrollPos(hWnd,SB_VERT,yposition,TRUE);
               InvalidateRect(hWnd,NULL,TRUE);
               }
            set_win_offset();
            break;

        case WM_CLOSE:
            message = WM_COMMAND;
            wParam  = IDM_EXIT;
            goto GlobalExit;

        case WM_PAINT:
            if (screen_to_be_cleared && last_written_y < 0) {
                 /* an empty window */
                 screen_to_be_cleared = 0;
                 GetUpdateRect(hWnd, &tempRect, TRUE);
                 hDC = BeginPaint(hWnd,&ps);
                 if (last_written_y == -2)
                     last_written_y = -1;
                 else
                     BitBlt(hDC, 0, 0, xdots, ydots,
                         NULL, 0, 0, BLACKNESS);
                 ValidateRect(hWnd, &tempRect);
                 EndPaint(hWnd,&ps);
                 break;
                 }
            if(IsIconic(hWnd)) {
               long x, y;
               LPSTR icon;
               long lx, ly, dlx, dly;
               DWORD tSize, tWidth, tHeight;

               if((icon = GlobalLock(hIconBitmap)) == NULL)
                  break;

               dlx = ((long)xdots << 8) / IconWidth;
               dly = ((long)ydots << 8) / IconHeight;
               for(ly = y = 0; y < (long)IconHeight; y++, ly += dly)
               {
                  for(lx = x = 0; x < (long)IconWidth; x++, lx += dlx)
                  {
                     unsigned int ix, iy;
                     unsigned char color;

                     ix = (unsigned int) (lx >> 8);
                     iy = (unsigned int) (ly >> 8);
                     color = getcolor(ix, iy);

                     ix = IconWidth - (unsigned int)y - 1;
                     ix = (ix * IconWidth) + (unsigned int)x;
                     iy = ix & (unsigned int)pixels_per_bytem1;
                     ix = ix >> pixelshift_per_byte;
                     icon[ix] = (icon[ix] & win_notmask[iy]) +
                               (color << win_bitshift[iy]);
                  }
               }

               hDC = BeginPaint(hWnd, &ps);

               SelectPalette (hDC, hPal, 0);
               RealizePalette(hDC);

               tSize   = pDibInfo->bmiHeader.biSizeImage;
               tHeight = pDibInfo->bmiHeader.biHeight;
               tWidth  = pDibInfo->bmiHeader.biWidth;

               pDibInfo->bmiHeader.biSizeImage = IconSize;
               pDibInfo->bmiHeader.biHeight    = IconHeight;
               pDibInfo->bmiHeader.biWidth     = IconWidth;

               SetDIBitsToDevice(hDC,
                       2, 2,
                       IconWidth, IconHeight,
                       0, 0,
                       0, IconHeight,
                       icon, (LPBITMAPINFO)pDibInfo,
                       DIB_PAL_COLORS);

               pDibInfo->bmiHeader.biSizeImage = tSize;
               pDibInfo->bmiHeader.biHeight    = tHeight;
               pDibInfo->bmiHeader.biWidth     = tWidth;

               GlobalUnlock(hIconBitmap);

               EndPaint(hWnd, &ps);
               break;
            }
            screen_to_be_cleared = 0;
            GetUpdateRect(hWnd, &tempRect, FALSE);
            if (Zooming)
                PaintMathTools();
            if (bTrack || bMove || bMoving)
                UpdateSelection(hWnd, DragPoint, &Rect, Shape);
            hDC = BeginPaint(hWnd,&ps);
            if (last_written_y >= 0) {
              int top, bottom, left, right, xcount, ycount;
              /* bit-blit the invalidated bitmap area */
              int fromleft, fromtop;
              top    = tempRect.top;
              bottom = tempRect.bottom;
              left   = tempRect.left;
              right  = tempRect.right;
              if (bottom >  ydots) bottom = ydots;
              if (right  >= xdots) right  = xdots-1;
              if (top    >  ydots) top    = ydots;
              if (left   >= xdots) left   = xdots;
              xcount = right  - left + 1;
              ycount = bottom - top;
              fromleft  = left  + win_xoffset;
              fromtop   = win_ydots - bottom - win_yoffset;
              if (left < xdots && top < ydots) {
                  SelectPalette (hDC, hPal, 0);
                  RealizePalette(hDC);
                  SetMapMode(hDC,MM_TEXT);
                  SetDIBitsToDevice(hDC,
                       left, top,
                       xcount, ycount,
                       fromleft, fromtop,
                       0, ydots,
                       (LPSTR)pixels, (LPBITMAPINFO)pDibInfo,
                       DIB_PAL_COLORS);
                  }
              }
            ValidateRect(hWnd, &tempRect);
            EndPaint(hWnd,&ps);
            if (bTrack || bMove || bMoving)
                UpdateSelection(hWnd, DragPoint, &Rect, Shape);
            if (Zooming)
                PaintMathTools();
            last_time = time(NULL);  /* reset the "end-paint" time */
            break;

        case WM_DESTROY:
            win_kill_all_zooming();
            if (win_systempaletteused)
                win_stop_cycling();
            SaveParameters(hWnd);
            /* delete the handle to the logical palette if it has any
               color entries and quit. */
            if (pLogPal->palNumEntries)
                DeleteObject (hPal);
            time_to_quit = 1;
            time_to_cycle = 0;
            WinHelp(hWnd,szHelpFileName,HELP_QUIT,0L);
            GlobalFree(hIconBitmap);
            PostQuitMessage(0);
            hWndCopy = hWnd;
            break;

        case WM_ACTIVATE:
            if (!wParam) {  /* app. is being de-activated */
                if (win_systempaletteused)
                    win_stop_cycling();
                break;
                }

        case WM_QUERYNEWPALETTE:
            /* If palette realization causes a palette change,
             * we need to do a full redraw.
             */
             if (last_written_y >= 0) {
                 hDC = GetDC (hWnd);
                 SelectPalette (hDC, hPal, 0);
                 i = RealizePalette(hDC);
                 ReleaseDC (hWnd, hDC);
                 if (i) {
                     InvalidateRect (hWnd, (LPRECT) (NULL), 1);
                     return 1;
                     }
                 else
                     return FALSE;
                 }
             else
                 return FALSE;

        case WM_PALETTECHANGED:
            if (wParam != (WPARAM)hWnd){
                if (last_written_y >= 0) {
                    hDC = GetDC (hWnd);
                    SelectPalette (hDC, hPal, 0);
                    i = RealizePalette (hDC);
                    if (i)
                        UpdateColors (hDC);
                    else
                        InvalidateRect (hWnd, (LPRECT) (NULL), 1);
                    ReleaseDC (hWnd, hDC);
                    }
                }
            break;

        case WM_COMMAND:

GlobalExit:
            switch (wParam) {

              /* Help menu items */

               case IDM_HELP_INDEX:
               case IDF_HELP_INDEX:
                   WinHelp(hWnd,szHelpFileName,FIHELP_INDEX,0L);
                   break;

               case IDM_HELP_FRACTINT:
               case IDF_HELP_FRACTINT:
                   fractint_help();
                   break;

               case IDM_HELP_KEYBOARD:
                   WinHelp(hWnd,szHelpFileName,HELP_KEY,(DWORD)(LPSTR)"keys");
                   break;

               case IDM_HELP_HELP:
                   WinHelp(hWnd,"WINHELP.HLP",FIHELP_INDEX,0L);
                   break;

                case IDM_ABOUT:
                    lpProcAbout = MakeProcInstance((FARPROC)About, hInst);
                    DialogBox(hInst, "AboutBox", hWnd, (DLGPROC)lpProcAbout);
                    FreeProcInstance(lpProcAbout);
                    break;

                /* View menu items */

                case IDF_STATUS:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        tab_display();
                        break;
                        }
                case IDS_STATUS:
                    lpProcStatus = MakeProcInstance((FARPROC)Status, hInst);
                    DialogBox(hInst, "ShowStatus", hWnd, (DLGPROC)lpProcStatus);
                    FreeProcInstance(lpProcStatus);
                    break;

                case IDF_FRACTINTSTYLE:
                    winfract_menustyle = TRUE;
                    hMenu = GetMenu(hWnd);
                    CheckMenuItem(hMenu, IDF_FRACTINTSTYLE, MF_CHECKED);
                    CheckMenuItem(hMenu, IDF_WINFRACTSTYLE, MF_UNCHECKED);
                    SaveParamSwitch(FractintMenusStr, winfract_menustyle);
                    break;

                case IDF_WINFRACTSTYLE:
                    winfract_menustyle = FALSE;
                    hMenu = GetMenu(hWnd);
                    CheckMenuItem(hMenu, IDF_FRACTINTSTYLE, MF_UNCHECKED);
                    CheckMenuItem(hMenu, IDF_WINFRACTSTYLE, MF_CHECKED);
                    SaveParamSwitch(FractintMenusStr, winfract_menustyle);
                    break;

               /* Color-Cycling (and Zooming) hotkeys */

               case IDF_HOTCYCLERAND:
                   if(Zooming) {
                      win_savedac();
                      ExecuteZoom();
                      }
                   else if(bMove)
                   {
                       extern POINT Center;

                       DragPoint = Center;
                       goto ExecZoom;
                   }
                   else if (win_oktocycle()) {
                       win_kill_all_zooming();
                       time_to_cycle = 1;
                       win_cyclerand = 2;
                       colorstate = 1;
                       }
                   break;

               case IDF_HOTNOZOOM:
                  win_kill_all_zooming();
                  if(Zooming)
                     CancelZoom();
                  else if(bMove)
                  {
                     extern POINT Center;

                     EndSelection(Center, &Rect);

                     ClearSelection(hWnd, &Rect, Shape);
                     bMove = bMoving = bTrack = FALSE;
                     win_title_text(-1);
                  }
                  break;

               case IDF_HOTCYCLEON:
               /* Space/etc.. toggles Color-cycling parameters */
                   win_kill_all_zooming();
                   if (time_to_cycle == 1) {
                       time_to_resume = 1;
                       time_to_cycle = 0;
                       }
                   else
                       if (win_oktocycle()) {
                            time_to_cycle = 1;
                            colorstate = 1;
                            }
                   break;

               case IDF_HOTCYCLERIGHT:
                   win_kill_all_zooming();
                   if (win_oktocycle()) {
                       time_to_cycle = 1;
                       win_cycledir = -1;
                       colorstate = 1;
                       }
                   break;

               case IDF_HOTCYCLELEFT:
                   win_kill_all_zooming();
                   if (win_oktocycle()) {
                       time_to_cycle = 1;
                       win_cycledir = 1;
                       colorstate = 1;
                       }
                   break;

               case IDF_HOTCYCLERSTEP:
                   if (win_oktocycle()) {
                       time_to_cycle = 0;
                       win_cycledir = -1;
                       colorstate = 1;
                       spindac(win_cycledir,1);
                       }
                   break;

               case IDF_HOTCYCLELSTEP:
                   win_kill_all_zooming();
                   if (win_oktocycle()) {
                       time_to_cycle = 0;
                       win_cycledir = 1;
                       colorstate = 1;
                       spindac(win_cycledir,1);
                       }
                   break;

               case IDF_HOTCYCLEFAST:
                   win_kill_all_zooming();
                   if (time_to_cycle != 0 && win_cycledelay > 4)
                       win_cycledelay -= 5;
                   break;

               case IDF_HOTCYCLESLOW:
                   win_kill_all_zooming();
                   if (time_to_cycle != 0 && win_cycledelay < 100)
                       win_cycledelay += 5;
                   break;

                case IDM_ORBITS:
                    /* toggle orbits only if in pixel-by-pixel mode */
                    if (win_fastupdate)
                        time_to_orbit = 1;
                    break;

                /* the following commands all cause a flag to be set
                   which causes the main fractal engine to return to
                   its main level, call SecondaryWndProc(), and then
                   either resume, restart, or exit */
                case IDM_EXIT:
                case IDM_OPEN:
                case IDF_OPEN:
                case IDM_NEW:
                case IDM_SAVE:
                case IDF_SAVE:
                case IDM_SAVEAS:
                case IDM_PRINT:
                case IDF_PRINT:
                case IDM_3D:
                case IDF_3D:
                case IDM_3DOVER:
                case IDF_3DOVER:
                case IDM_PARFILE:
                case IDF_PARFILE:
                case IDM_SAVEPAR:
                case IDF_SAVEPAR:
                case IDM_COPY:
                case IDC_EDIT:
                case IDM_FORMULA:
                case IDF_FORMULA:
                case IDM_DOODADX:
                case IDF_DOODADX:
                case IDM_DOODADY:
                case IDF_DOODADY:
                case IDM_DOODADZ:
                case IDF_DOODADZ:
                case IDM_IFS3D:
                case IDF_IFS3D:
                case IDM_RESTART:
                case IDF_RESTART:
                case IDM_STARFIELD:
                case IDF_STARFIELD:
                case IDM_IMAGE:
                case IDF_IMAGE:
                case IDM_COORD:
                case IDM_SIZING:
                case IDM_PIXELS:
                case IDF_MAPIN:
                case IDF_MAPOUT:
                case IDM_MAPIN:
                case IDM_MAPOUT:
                case IDF_CYCLE:
                case IDM_CYCLE:
                case IDM_MATH_TOOLS:
                case IDM_ZOOMIN:
                case IDM_ZOOMOUT:
                case IDM_ZOOM:
                case IDF_PASSES:
                case IDM_PASSES:
                case IDF_BROWSER:
                case IDM_BROWSER:
                case IDF_EVOLVER:
                case IDM_EVOLVER:
                case IDM_MAINMENU:
                case IDF_MAINMENU:
                case IDF_CMDSTRING:
                    win_kill_all_zooming();
                    time_to_cycle = 0;
                    SecondaryhWnd = hWnd;
                    Secondarymessage = message;
                    SecondarywParam = wParam;
                    SecondarylParam = lParam;
                    time_to_act = 1;
                    break;

            }
            break;

        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}


void SecondaryWndProc(void)
{
    HWND hWnd;                                /* handle to main window */
    unsigned message;
    WORD wParam;
    LONG lParam;

    HMENU hMenu;

    int Return;
    int i, fchoice;
    extern char FractintMenusStr[];
    extern char FractintPixelsStr[];

    hWnd    = SecondaryhWnd;
    message = Secondarymessage;
    wParam  = SecondarywParam;
    lParam  = SecondarylParam;

    switch (message) {

        case WM_COMMAND:

            switch (wParam) {

                /* File menu items */

                case IDF_OPEN:
                case IDF_3D:
                case IDF_3DOVER:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        extern int display3d, overlay3d;
                        extern int tabmode, helpmode, initbatch;
                        extern char gifmask[];
                        int showfile;
                        char *hdg;

                        win_kill_all_zooming();
                        time_to_resume = 1;
                        initbatch = 0;
                        display3d = 0;
                        overlay3d = 0;
                        if (wParam == IDM_3D || wParam == IDM_3DOVER
                            || wParam == IDF_3D || wParam == IDF_3DOVER)
                            display3d = 1;
                        if (wParam == IDM_3DOVER || wParam == IDF_3DOVER)
                            overlay3d = 1;
                        win_display3d = 0;
                        win_overlay3d = 0;
                        stackscreen();
                        tabmode = 0;
                        showfile = -1;
                        while (showfile <= 0) {                /* image is to be loaded */
                            if (overlay3d)
                                hdg = "Select File for 3D Overlay";
                            else if (display3d)
                                hdg = "Select File for 3D Transform";
                            else
                                hdg = "Select File to Restore";
                            if (showfile < 0 &&
                                getafilename(hdg,gifmask,readname) < 0) {
                                showfile = 1;                     /* cancelled */
                                break;
                                }
                            showfile = 0;
                            tabmode = 1;
                            if (read_overlay() == 0)             /* read hdr, get video mode */
                                break;                      /* got it, exit */
                            showfile = -1;                     /* retry */
                            }
                        if (!showfile) {
                            lstrcpy(FileName, readname);
                            time_to_load = 1;
                            time_to_cycle = 0;
                            if (wParam == IDM_3D || wParam == IDM_3DOVER
                                || wParam == IDF_3D || wParam == IDF_3DOVER)
                                win_display3d = 1;
                            if (wParam == IDM_3DOVER || wParam == IDF_3DOVER)
                                win_overlay3d = 1;
                            }
                        tabmode = 1;
                        unstackscreen();
                        break;
                        }
                case IDM_NEW:
                case IDM_OPEN:
                case IDM_3D:
                case IDM_3DOVER:
                    win_display3d = 0;
                    win_overlay3d = 0;
                    lstrcpy(DialogTitle,"File to Open");
                    if (wParam == IDM_3D || wParam == IDF_3D)
                        lstrcpy(DialogTitle,"File for 3D Transform");
                    if (wParam == IDM_3DOVER || wParam == IDF_3DOVER)
                        lstrcpy(DialogTitle,"File for 3D Overlay Transform");
                    lstrcpy(FileName, readname);
                    lstrcpy(DefSpec,"*.gif");
                    lstrcpy(DefExt,".gif");
                    Return = Win_OpenFile(FileName);
                    if (Return && (wParam == IDM_3D || wParam == IDM_3DOVER)) {
                        extern int glassestype;
                        lpSelect3D = MakeProcInstance(
                            (FARPROC) Select3D, hInst);
                        Return = DialogBox(hInst, "Select3D",
                             hWnd, (DLGPROC)lpSelect3D);
                        FreeProcInstance(lpSelect3D);
                         if (glassestype) {
                             int Return2;
                             lpSelectFunnyGlasses = MakeProcInstance(
                                 (FARPROC) SelectFunnyGlasses, hInst);
                             Return2 = DialogBox(hInst, "SelectFunnyGlasses",
                                 hWnd, (DLGPROC)lpSelectFunnyGlasses);
                             FreeProcInstance(lpSelectFunnyGlasses);
                             check_funnyglasses_name();
                             }
                        if (Return && !win_3dspherical) {
                            lpSelect3DPlanar = MakeProcInstance(
                                (FARPROC) Select3DPlanar, hInst);
                            Return = DialogBox(hInst, "Select3DPlanar",
                                 hWnd, (DLGPROC)lpSelect3DPlanar);
                            FreeProcInstance(lpSelect3DPlanar);
                            }
                        if (Return && win_3dspherical) {
                            lpSelect3DSpherical = MakeProcInstance(
                                (FARPROC) Select3DSpherical, hInst);
                            Return = DialogBox(hInst, "Select3DSpherical",
                                 hWnd, (DLGPROC)lpSelect3DSpherical);
                            FreeProcInstance(lpSelect3DSpherical);
                            }
                        if (Return && (ILLUMINE || RAY)) {
                            lpSelectLightSource = MakeProcInstance(
                                (FARPROC) SelectLightSource, hInst);
                            Return = DialogBox(hInst, "SelectLightSource",
                                 hWnd, (DLGPROC)lpSelectLightSource);
                            FreeProcInstance(lpSelectLightSource);
                            }
                        }
                    if (Return) {
                        lstrcpy(readname,FileName);
                        time_to_load = 1;
                        win_kill_all_zooming();
                        time_to_cycle = 0;
                        if (wParam == IDM_3D || wParam == IDM_3DOVER
                         || wParam == IDF_3D || wParam == IDF_3DOVER)
                            win_display3d = 1;
                        if (wParam == IDM_3DOVER || wParam == IDF_3DOVER)
                            win_overlay3d = 1;
                        }
                    break;

                case IDF_SAVE:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        /* (no change) */
                        }
                case IDM_SAVE:
                case IDM_SAVEAS:
                    lstrcpy(DialogTitle,"File to SaveAs");
                    lstrcpy(FileName, savename);
                    lstrcpy(DefSpec,"*.gif");
                    lstrcpy(DefExt,".gif");
                    Return = Win_SaveFile(FileName);
                    if (Return)
                        time_to_save = 1;
                    if (time_to_save)
                    {
                       time_to_cycle = 0;
                       wsprintf(StatusTitle, "Saving:  %s", (LPSTR)FullPathName);
                       OpenStatusBox(hWnd, hInst);
                       resave_flag = fract_overwrite = 1;
                    }
                    break;

                case IDF_PRINT:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        /* (no change) */
                        }
                case IDM_PRINT:
                    time_to_print = 1;
                    time_to_cycle = 0;
                    break;

                case IDM_PARFILE:
winfract_loadpar:
                    {
                    FILE *parmfile;
                    extern char CommandFile[];
                    extern char CommandName[];
                    long point;

                    win_kill_all_zooming();
                    lstrcpy(DialogTitle,"Parameter File to Load");
                    lstrcpy(FileName, CommandFile);
                    lstrcpy(DefSpec,"*.par");
                    lstrcpy(DefExt,".par");
                    Return = Win_OpenFile(FileName);
                    if (Return) {
                        lstrcpy(LFileName, FileName);
                        lstrcpy(CommandFile, FileName);
                        get_lsys_name();
                        lstrcpy(DialogTitle,"Parameter Entry to Load");
                        win_choicemade = 0;
                        lpSelectFractal = MakeProcInstance((FARPROC)SelectFractal, hInst);
                        Return = DialogBox(hInst, "SelectFractal",
                            hWnd, (DLGPROC)lpSelectFractal);
                        FreeProcInstance(lpSelectFractal);
                        if (Return) {
                            parmfile = fopen(CommandFile,"rb");
                            memcpy((char *)&point,
                                (char *)&win_choices[win_choicemade][21],
                                4);
                            fseek(parmfile,point,SEEK_SET);
                            Return = load_commands(parmfile);
                            }
                        if (Return) {
                            if (xx3rd != xxmin || yy3rd != yymin)
                                stopmsg(4," This image uses a skewed zoom-box,\n a feature not available in Winfract.\n All Skewness has been dropped");
                            if (colorpreloaded)
                                win_savedac();
                            maxiter = maxit;
                            time_to_restart = 1;
                            time_to_cycle = 0;
                            calc_status = 0;
                            }
                        }
                    break;
                    }

                case IDM_SAVEPAR:
                    {
                    extern char CommandFile[];
                    win_kill_all_zooming();
                    lstrcpy(DialogTitle,"Parameter File to SaveAs");
                    lstrcpy(FileName, CommandFile);
                    lstrcpy(DefSpec,"*.par");
                    lstrcpy(DefExt,".par");
                    Return = Win_SaveFile(FileName);
                    if (Return) {
                        lstrcpy(LFileName, FileName);
                        lstrcpy(CommandFile, FileName);
                        lpSelectSavePar = MakeProcInstance((FARPROC)SelectSavePar, hInst);
                        Return = DialogBox(hInst, "SelectSavePar",
                             hWnd, (DLGPROC)lpSelectSavePar);
                        FreeProcInstance(lpSelectSavePar);
                        }
                    if (Return) {
                        time_to_resume = 1;
                        win_make_batch_file();
                         }
                    break;
                    }

                case IDF_SAVEPAR:
                    win_kill_all_zooming();
                    time_to_resume = 1;
                    make_batch_file();
                    break;

                case IDM_COPY:
                   win_copy_to_clipboard();

                    break;

                case IDM_EXIT:
                    win_kill_all_zooming();
                    time_to_quit = 1;
                    time_to_cycle = 0;
                    ValidateRect(hWnd, NULL);
                    hWndCopy = hWnd;
                    /* the main routine will actually call 'DestroyWindow()' */
                    break;

                /* Fractals menu items */

                case IDF_FORMULA:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        win_kill_all_zooming();
                        time_to_resume = 1;
                        julibrot = 0;                /* disable Julibrot logic */
julibrot_fudge:                                /* dive in here for Julibrots */
                        stackscreen();
                        i = get_fracttype();
                        unstackscreen();
                        SetFocus(hWnd);
                        if (i == 0) {                /* time to redraw? */
                            time_to_restart = 1;
                            time_to_cycle = 0;
                            calc_status = 0;
                            }
                        break;
                        }
                case IDM_FORMULA:
                    lstrcpy(DialogTitle,"Select a Fractal Formula");
                    win_kill_all_zooming();
                    win_numchoices = CountFractalList;
                    win_choicemade = 0;
                    CurrentFractal = fractype;
                    for (i = 0; i < win_numchoices; i++) {
                        win_choices[i] = fractalspecific[onthelist[i]].name;
                        if (onthelist[i] == fractype ||
                            fractalspecific[onthelist[i]].tofloat == fractype)
                            win_choicemade = i;
                        }
                    lpSelectFractal = MakeProcInstance((FARPROC)SelectFractal, hInst);
                    Return = DialogBox(hInst, "SelectFractal",
                        hWnd, (DLGPROC)lpSelectFractal);
                    FreeProcInstance(lpSelectFractal);
                    fchoice = win_choicemade;
                    if (Return && (onthelist[fchoice] == IFS ||
                        onthelist[fchoice] == IFS3D)) {
                        lstrcpy(DialogTitle,"IFS Filename to Load");
                        lstrcpy(FileName, IFSFileName);
                        lstrcpy(DefSpec,"*.ifs");
                        lstrcpy(DefExt,".ifs");
                        Return = Win_OpenFile(FileName);
                        if (Return) {
                            lstrcpy(IFSFileName, FileName);
                            lstrcpy(FormFileName, FileName);
                            get_formula_names();
                            lstrcpy(DialogTitle,"Select an IFS type");
                            win_choicemade = 0;
                            lpSelectFractal = MakeProcInstance((FARPROC)SelectFractal, hInst);
                            Return = DialogBox(hInst, "SelectFractal",
                                hWnd, (DLGPROC)lpSelectFractal);
                            FreeProcInstance(lpSelectFractal);
                            if (Return) {
                                lstrcpy(IFSName, win_choices[win_choicemade]);
                                Return = ! ifsload();
                                }
                            }
                        }
                    if (Return && (onthelist[fchoice] == FORMULA ||
                        onthelist[fchoice] == FFORMULA)) {
                        /* obtain the formula filename */
                        lstrcpy(DialogTitle,"Formula File to Load");
                        lstrcpy(FileName, FormFileName);
                        lstrcpy(DefSpec,"*.frm");
                        lstrcpy(DefExt,".frm");
                        Return = Win_OpenFile(FileName);
                        if (Return) {
                            lstrcpy(FormFileName, FileName);
                            get_formula_names();
                            lstrcpy(DialogTitle,"Select a Formula");
                            win_choicemade = 0;
                            lpSelectFractal = MakeProcInstance((FARPROC)SelectFractal, hInst);
                            Return = DialogBox(hInst, "SelectFractal",
                                hWnd, (DLGPROC)lpSelectFractal);
                            FreeProcInstance(lpSelectFractal);
                            if (Return)
                                Return = parse_formula_names();
                            }
                        }
                    if (Return && (onthelist[fchoice] == LSYSTEM)) {
                        lstrcpy(DialogTitle,"Lsystem File to Load");
                        lstrcpy(FileName, LFileName);
                        lstrcpy(DefSpec,"*.l");
                        lstrcpy(DefExt,".l");
                        Return = Win_OpenFile(FileName);
                        if (Return) {
                            lstrcpy(LFileName, FileName);
                            get_lsys_name();
                            lstrcpy(DialogTitle,"Select a Formula");
                            win_choicemade = 0;
                            lpSelectFractal = MakeProcInstance((FARPROC)SelectFractal, hInst);
                            Return = DialogBox(hInst, "SelectFractal",
                                hWnd, (DLGPROC)lpSelectFractal);
                            FreeProcInstance(lpSelectFractal);
                            if (Return) {
                                lstrcpy(LName, win_choices[win_choicemade]);
                                Return = !LLoad();
                                }
                            }
                        }
                    julibrot = 0;
                    if (Return) {
                        double oldxxmin, oldxxmax, oldyymin, oldyymax;
                        double oldparam[4];
                        int i;
                        oldxxmin = xxmin;
                        oldxxmax = xxmax;
                        oldyymin = yymin;
                        oldyymax = yymax;
                        for (i = 0; i < 4; i++)
                            oldparam[i] = param[i];
                        CurrentFractal = onthelist[fchoice];
                        curfractalspecific = &fractalspecific[CurrentFractal];
                        if (CurrentFractal == BIFURCATION
                            || CurrentFractal == LBIFURCATION
                            || CurrentFractal == BIFSTEWART
                            || CurrentFractal == LBIFSTEWART
                            || CurrentFractal == BIFLAMBDA
                            || CurrentFractal == LBIFLAMBDA
                            ) set_trig_array(0,"ident");
                        if (CurrentFractal == BIFEQSINPI
                            || CurrentFractal == LBIFEQSINPI
                            || CurrentFractal == BIFADSINPI
                            || CurrentFractal == LBIFADSINPI
                            ) set_trig_array(0,"sin");
                        set_default_parms();
                        if (CurrentFractal == JULIBROT || CurrentFractal == JULIBROTFP) {
                            fractype = CurrentFractal;
                            julibrot = 1;
                                stackscreen();
                            if (get_fract_params(0) < 0)
                                Return = 0;
                            unstackscreen();
                            }
                        else {
                            lpSelectFracParams = MakeProcInstance((FARPROC)SelectFracParams,
                                hInst);
                            Return = DialogBox(hInst, "SelectFracParams",
                                hWnd, (DLGPROC)lpSelectFracParams);
                            FreeProcInstance(lpSelectFracParams);
                            }
                        if (! Return) {
                            xxmin = oldxxmin;
                            xxmax = oldxxmax;
                            yymin = oldyymin;
                            yymax = oldyymax;
                            for (i = 0; i < 4; i++)
                                param[i] = oldparam[i];
                            }
                        }
                    if (Return) {
                        time_to_reinit = 1;
                        time_to_cycle = 0;
                        calc_status = 0;
                        }
                    break;

                case IDF_PARFILE:
                    if (!winfract_menustyle) {        /* winfract prompts */
                        goto winfract_loadpar; /* for now */
                        }
                case IDF_DOODADX:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_xmenu;   /* for now */
                case IDF_DOODADY:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_ymenu;   /* for now */
                case IDF_DOODADZ:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_zmenu;   /* for now */
                case IDF_PASSES:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_pmenu;   /* for now */
                case IDF_BROWSER:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_bmenu;   /* for now */
                case IDF_EVOLVER:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_emenu;   /* for now */
                case IDF_MAINMENU:
                    if (!winfract_menustyle)   /* Windows menus */
                        goto winfract_mmenu;   /* for now */
                case IDF_CMDSTRING:

                    if (winfract_menustyle) {        /* fractint prompts */
                        win_kill_all_zooming();
                        time_to_resume = 1;
                        stackscreen();
                        maxiter = maxit;
                        if (wParam == IDF_DOODADX || wParam == IDM_DOODADX)
                            i = get_toggles();
                        else if (wParam == IDF_DOODADY || wParam == IDM_DOODADY)
                            i = get_toggles2();
                        else if (wParam == IDF_DOODADZ || wParam == IDM_DOODADZ)
                            i = get_fract_params(1);
                        else if (wParam == IDF_PASSES || wParam == IDM_PASSES)
                            i = passes_options();
                        else if (wParam == IDF_BROWSER || wParam == IDM_BROWSER)
                            i = get_browse_params();
                        else if (wParam == IDF_EVOLVER || wParam == IDM_EVOLVER)
                            i = get_evolve_Parms();
                        else if (wParam == IDF_MAINMENU || wParam == IDM_MAINMENU)
                            i = main_menu(1);
                        else if (wParam == IDF_CMDSTRING)
                            i = get_cmd_string();
                        else {
                            i = get_commands();
                            if (xx3rd != xxmin || yy3rd != yymin)
                                stopmsg(4," This image uses a skewed zoom-box,\n a feature not available in Winfract.\n All Skewness has been dropped");
                            if (colorpreloaded)
                                win_savedac();
                            }
                        unstackscreen();
                        SetFocus(hWnd);
                        time_to_cycle = 0;
                        if (i > 0) {                /* time to redraw? */
                            maxiter = maxit;
                            time_to_restart = 1;
                            calc_status = 0;
                            }
                        break;
                   }

                case IDM_DOODADX:
winfract_xmenu:
                        lpSelectDoodads = MakeProcInstance((FARPROC)SelectDoodads, hInst);
                        Return = DialogBox(hInst, "SelectDoodads",
                                hWnd, (DLGPROC)lpSelectDoodads);
                        FreeProcInstance(lpSelectDoodads);
                        if (Return) {
                                win_kill_all_zooming();
                                win_savedac();
                                time_to_restart = 1;
                                time_to_cycle = 0;
                                calc_status = 0;
                                }
                        break;

                case IDM_DOODADY:
winfract_ymenu:
                        lpSelectExtended = MakeProcInstance((FARPROC)SelectExtended, hInst);
                        Return = DialogBox(hInst, "SelectExtended",
                                hWnd, (DLGPROC)lpSelectExtended);
                        FreeProcInstance(lpSelectExtended);
                        if (Return) {
                                win_kill_all_zooming();
                                win_savedac();
                                time_to_restart = 1;
                                time_to_cycle = 0;
                                calc_status = 0;
                                }
                        break;

               case IDM_DOODADZ:
winfract_zmenu:
                    CurrentFractal = fractype;
                    lpSelectFracParams = MakeProcInstance((FARPROC)SelectFracParams,
                        hInst);
                    Return = DialogBox(hInst, "SelectFracParams",
                        hWnd, (DLGPROC)lpSelectFracParams);
                    FreeProcInstance(lpSelectFracParams);
                    if (Return) {
                        win_kill_all_zooming();
                        win_savedac();
                        time_to_reinit = 1;
                        time_to_cycle = 0;
                        calc_status = 0;
                        }
                  break;

                case IDM_PASSES:
winfract_pmenu:
                  break;

                case IDM_BROWSER:
winfract_bmenu:
                  break;

                case IDM_EVOLVER:
winfract_emenu:
                  break;

                case IDM_MAINMENU:
winfract_mmenu:
                  break;

                case IDF_IFS3D:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        extern int display3d, overlay3d;
                        extern int tabmode, helpmode, initbatch;
                        extern char gifmask[];
                        tabmode = 0;
                        if (get_fract3d_params() >= 0) {
                            win_kill_all_zooming();
                            time_to_restart = 1;
                            }
                        tabmode = 1;
                        break;
                        }
                case IDM_IFS3D:
                    {
                    extern int glassestype;
                    lpSelectIFS3D = MakeProcInstance(
                        (FARPROC) SelectIFS3D, hInst);
                    Return = DialogBox(hInst, "SelectIFS3D",
                         hWnd, (DLGPROC)lpSelectIFS3D);
                    FreeProcInstance(lpSelectIFS3D);
                    if (Return) {
                         win_kill_all_zooming();
                         time_to_restart = 1;
                         if (glassestype) {
                             lpSelectFunnyGlasses = MakeProcInstance(
                                 (FARPROC) SelectFunnyGlasses, hInst);
                             Return = DialogBox(hInst, "SelectFunnyGlasses",
                                 hWnd, (DLGPROC)lpSelectFunnyGlasses);
                             FreeProcInstance(lpSelectFunnyGlasses);
                             check_funnyglasses_name();
                             }
                         }
                    break;
                    }

               case IDM_RESTART:
               case IDF_RESTART:
                   time_to_reinit  = 2;
                   time_to_cycle = 0;
                   calc_status = 0;
                   break;

               case IDF_STARFIELD:
                   if (winfract_menustyle) {        /* fractint prompts */
                       if (get_starfield_params() >= 0) {
                           win_kill_all_zooming();
                           time_to_starfield = 1;
                           time_to_cycle = 0;
                           calc_status = 0;
                           }
                       break;
                       }
               case IDM_STARFIELD:
                   lpSelectStarfield = MakeProcInstance(
                       (FARPROC) SelectStarfield, hInst);
                   Return = DialogBox(hInst, "Starfield",
                        hWnd, (DLGPROC)lpSelectStarfield);
                   FreeProcInstance(lpSelectStarfield);
                   if (Return) {
                       win_kill_all_zooming();
                       time_to_starfield = 1;
                       time_to_cycle = 0;
                       calc_status = 0;
                       }
                   break;

                /* View menu items */

                case IDF_IMAGE:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        /* (no change) */
                        }
                case IDM_IMAGE:
                        win_kill_all_zooming();
                        lpSelectImage = MakeProcInstance((FARPROC)SelectImage, hInst);
                        Return = DialogBox(hInst, "SelectImage",
                                hWnd, (DLGPROC)lpSelectImage);
                        FreeProcInstance(lpSelectImage);
                        if (Return) {
                                time_to_restart = 1;
                                time_to_cycle = 0;
                                calc_status = 0;
                                }
                        ReSizeWindow(hWnd);
                        break;

                case IDM_MATH_TOOLS:
                    MathToolBox(hWnd);
                    break;

                case IDM_ZOOMOUT:
                case IDM_ZOOMIN:
                    if(ZoomBarOpen)
                       ZoomBar(hWnd);
                    else
                       CheckMenuItem(GetMenu(hWnd), ZoomMode, MF_UNCHECKED);
                    CheckMenuItem(GetMenu(hWnd), wParam, MF_CHECKED);
                    ZoomMode = wParam;
                    break;

                case IDM_ZOOM:
                    if(!ZoomBarOpen)
                       CheckMenuItem(GetMenu(hWnd), ZoomMode, MF_UNCHECKED);
                    win_kill_all_zooming();
                    ZoomBar(hWnd);
                    break;

                case IDM_COORD:
                    CoordinateBox(hWnd);
                    break;

                case IDM_SIZING:
                    win_kill_all_zooming();
                    WindowSizing(hWnd);
                    break;

                case IDM_PIXELS:
                    {
                    BOOL profile_fastupdate;
                    hMenu = GetMenu(hWnd);
                    if (win_fastupdate) {
                        win_fastupdate = 0;
                        /* if disabling p-by-p, disable orbits, too */
                        if (show_orbit)
                            time_to_orbit = 1;
                        profile_fastupdate = FALSE;
                        CheckMenuItem(hMenu, IDM_PIXELS, MF_UNCHECKED);
                        }
                    else {
                        win_fastupdate = 2;
                        profile_fastupdate = TRUE;
                        CheckMenuItem(hMenu, IDM_PIXELS, MF_CHECKED);
                        }
                    SaveParamSwitch(FractintPixelsStr, profile_fastupdate);
                    }
                    break;

                case IDF_STATUS:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        tab_display();
                        break;
                        }
                case IDS_STATUS:
                    lpProcStatus = MakeProcInstance((FARPROC)Status, hInst);
                    DialogBox(hInst, "ShowStatus", hWnd, (DLGPROC)lpProcStatus);
                    FreeProcInstance(lpProcStatus);
                    break;

                /* Colors menu items */

                case IDF_MAPIN:
                    if (winfract_menustyle) {  /* fractint-style prompts */
                        win_kill_all_zooming();
                        time_to_resume = 1;
                        time_to_cycle = 0;
                        if (wParam == IDF_MAPIN)
                            load_palette();
                        else
                            save_palette();
                        spindac(0,1);
                        break;
                        }
                case IDF_MAPOUT:
                case IDM_MAPIN:
                case IDM_MAPOUT:
                    lstrcpy(DialogTitle,"Palette File to Load");
                    lstrcpy(FileName, MAP_name);
                    if (wParam == IDM_MAPOUT || wParam == IDF_MAPOUT)
                        lstrcpy(FileName, "mymap");
                    lstrcpy(DefSpec,"*.map");
                    lstrcpy(DefExt,".map");
                    if (wParam == IDM_MAPOUT || wParam == IDF_MAPOUT) {
                        Return = Win_SaveFile(FileName);
                        }
                    else {
                        Return = Win_OpenFile(FileName);
                        }
                    if (Return && (wParam == IDM_MAPIN || wParam == IDF_MAPIN)) {
                        win_kill_all_zooming();
                        ValidateLuts(FileName);
                        spindac(0,1);
                        }
                    if (Return && (wParam == IDM_MAPOUT || wParam == IDF_MAPOUT)) {
                        FILE *dacfile;
                        dacfile = fopen(FileName,"w");
                        if (dacfile == NULL) {
                                break;
                                }
                        fprintf(dacfile,"  0   0   0\n");
                        for (i = 1; i < 256; i++)
                                fprintf(dacfile, "%3d %3d %3d\n",
                                dacbox[i][0] << 2,
                                dacbox[i][1] << 2,
                                dacbox[i][2] << 2);
                        fclose(dacfile);
                        }
                    break;

                case IDF_CYCLE:
                    if (winfract_menustyle) {        /* Fractint prompts */
                        /* (no change) */
                        }
                case IDM_CYCLE:
                    if (!win_oktocycle())
                        break;
                    win_kill_all_zooming();
                    lpSelectCycle = MakeProcInstance((FARPROC)SelectCycle, hInst);
                    Return = DialogBox(hInst, "SelectCycle",
                    hWnd, (DLGPROC)lpSelectCycle);
                    FreeProcInstance(lpSelectCycle);
                    break;

                case IDC_EDIT:
                    if (HIWORD (lParam) == EN_ERRSPACE) {
                        MessageBox (
                              GetFocus ()
                            , "Out of memory."
                            , "Fractint For Windows"
                            , MB_ICONSTOP | MB_OK
                        );
                    }
                    break;

            }
            break;

    }
    return;
}

void win_set_title_text(void)
{
    float temp;
    char ctemp[80];

    temp = (float)(win_release / 100.0);
    sprintf(ctemp,"Fractint for Windows - Vers %5.2f", temp);
    lstrcpy(winfract_title_text, ctemp);
}

char far win_oldtitle[30];
char far win_title1[] = " (calculating)";
char far win_title2[] = " (color-cycling)";
char far win_title3[] = " (zooming ";
char far win_title4[] = " (starfield generation)";

void win_title_text(int title)
{
char newtitle[80];

lstrcpy(newtitle, winfract_title_text);

if (title < 0) {
    lstrcat(newtitle, win_oldtitle);
    }
if (title == 0) {
    win_oldtitle[0] = 0;
    lstrcat(newtitle, win_oldtitle);
    }
if (title == 1) {
    lstrcpy(win_oldtitle, win_title1);
    lstrcat(newtitle, win_oldtitle);
    }
if (title == 2) {
    lstrcat(newtitle, win_title2);
    }
if (title == 3) {
    lstrcat(newtitle, win_title3);
if (ZoomMode == IDM_ZOOMOUT)
    lstrcat(newtitle, "out)");
else
    lstrcat(newtitle, "in)");
    }
if (title == 4) {
    lstrcat(newtitle, win_title4);
    }

SetWindowText(hMainWnd, newtitle);

}

int win_oktocycle(void)
{
if (!(iRasterCaps) || iNumColors < 16) {
    stopmsg(0,
        winfract_msg96
         );
    return(0);
    }
return(1);
}

extern int win_animate_flag;

int win_stop_cycling(void)
{
HDC hDC;                      /* handle to device context           */

hDC = GetDC(GetFocus());
SetSystemPaletteUse(hDC,SYSPAL_STATIC);
ReleaseDC(GetFocus(),hDC);

time_to_cycle = 0;
win_animate_flag = 0;
restoredac();
win_systempaletteused = FALSE;
SetSysColors(COLOR_ENDCOLORS,(LPINT)win_syscolorindex,(LONG FAR *)win_syscolorold);
return(0);
}

void win_kill_all_zooming(void)
{
    if(Zooming)
        CancelZoom();
    else if(bMove)
        {
         extern POINT Center;

         EndSelection(Center, &Rect);
         ClearSelection(hWndCopy, &Rect, Shape);
         bMove = bMoving = bTrack = FALSE;
         win_title_text(-1);
         }
}

void mono_dib_palette(void)
{
int i;                /* fill in the palette index values */
    for (i = 0; i < 128; i = i+2) {
        pDibInfo->bmiColors[i  ].rgbBlue      =   0;
        pDibInfo->bmiColors[i  ].rgbGreen     =   0;
        pDibInfo->bmiColors[i  ].rgbRed       =   0;
        pDibInfo->bmiColors[i  ].rgbReserved  =   0;
        pDibInfo->bmiColors[i+1].rgbBlue      = 255;
        pDibInfo->bmiColors[i+1].rgbGreen     = 255;
        pDibInfo->bmiColors[i+1].rgbRed       = 255;
        pDibInfo->bmiColors[i+1].rgbReserved  =   0;
        }
}

int default_dib_palette(void)
{
int i, k;                /* fill in the palette index values */
int far *palette_values;        /* pointer to palette values */

    palette_values = (int far *)&pDibInfo->bmiColors[0];
    k = 0;
    for (i = 0; i < 256; i++) {
        palette_values[i] = k;
        if (++k >= iNumColors)
           if (iNumColors > 0)
               k = 0;
        }
    return(0);
}

int rgb_dib_palette(void)
{
int i;                /* fill in the palette index values */

    for (i = 0; i < 256; i++) {
        pDibInfo->bmiColors[i].rgbRed       = dacbox[i][0] << 2;
        pDibInfo->bmiColors[i].rgbGreen     = dacbox[i][1] << 2;
        pDibInfo->bmiColors[i].rgbBlue      = dacbox[i][2] << 2;
        pDibInfo->bmiColors[i].rgbReserved  =   0;
        }
    return(0);
}

int win_copy_to_clipboard(void)
{
HWND hWnd;

    hWnd    = SecondaryhWnd;

    /* allocate the memory for the BITMAPINFO structure
       (followed by the bitmap bits) */
    if (!(hClipboard1 = GlobalAlloc(GMEM_FIXED,
        sizeof(BITMAPINFOHEADER)+colors*sizeof(PALETTEENTRY)
        + win_bitmapsize))) {
        cant_clip();
        return(0);
        }
    if (!(lpClipboard1 =
        (LPSTR) GlobalLock(hClipboard1))) {
        GlobalFree(hClipboard1);
        cant_clip();
        return(0);
        }
    rgb_dib_palette();
    lmemcpy((char huge *)lpClipboard1, (char huge *)pDibInfo,
        sizeof(BITMAPINFOHEADER)+colors*sizeof(RGBQUAD)
        );
    lpClipboard1 +=
        (sizeof(BITMAPINFOHEADER))+
        (colors*sizeof(RGBQUAD));
    lmemcpy((char huge *)lpClipboard1, (char huge *)pixels,
        win_bitmapsize);

    GlobalUnlock(hClipboard1);

    /* allocate the memory for the palette info */
    if (!lpClipboard2) {
        if (!(hClipboard2 = GlobalAlloc (GMEM_FIXED,
            (sizeof (LOGPALETTE) +
            (sizeof (PALETTEENTRY) * (PALETTESIZE)))))) {
            GlobalFree(hClipboard1);
            cant_clip();
            return(0);
            }
        if (!(lpClipboard2 =
            (LPSTR) GlobalLock(hClipboard2))) {
            GlobalFree(hClipboard1);
            GlobalFree(hClipboard2);
            cant_clip();
            return(0);
            }
        }

    /* fill in the palette info */
    lpClipboard2[0] = 0;
    lpClipboard2[1] = 3;
    lpClipboard2[2] = 0;
    lpClipboard2[3] = 1;
    lmemcpy((char huge *)&lpClipboard2[4],
        (char huge *)&pDibInfo->bmiColors[0],
        PALETTESIZE*sizeof(RGBQUAD)
        );
    hClipboard3 = CreatePalette ((LPLOGPALETTE) lpClipboard2);

    hDC = GetDC(hWnd);
    hBitmap = CreateDIBitmap(hDC, &pDibInfo->bmiHeader,
        CBM_INIT, (LPSTR)pixels, pDibInfo,
        DIB_RGB_COLORS);
    ReleaseDC(hWnd, hDC);

    if (OpenClipboard(hWnd)) {
        EmptyClipboard();
        SetClipboardData(CF_PALETTE, hClipboard3);
        SetClipboardData(CF_DIB, hClipboard1);
        if(hBitmap)
            SetClipboardData(CF_BITMAP, hBitmap);
        CloseClipboard();
        }

    default_dib_palette();
}

extern char win_funnyglasses_map_name[];

void check_funnyglasses_name(void)
{
    if (win_funnyglasses_map_name[0] != 0) {
        if (ValidateLuts(win_funnyglasses_map_name) == 0) {
            win_savedac();
            }
        else {
            char temp[80];
            sprintf(temp,"Can't find map file: %s",
                win_funnyglasses_map_name);
            stopmsg(0,temp);
            }
        }
}

void MakeHelpPathName(szFileName)
char * szFileName;
{
   char *  pcFileName;
   int     nFileNameLen;

   nFileNameLen = GetModuleFileName(hInst,szFileName,EXE_NAME_MAX_SIZE);
   pcFileName = szFileName + nFileNameLen;

   while (pcFileName > szFileName) {
       if (*pcFileName == '\\' || *pcFileName == ':') {
           *(++pcFileName) = '\0';
           break;
       }
   nFileNameLen--;
   pcFileName--;
   }

   if ((nFileNameLen+13) < EXE_NAME_MAX_SIZE) {
       lstrcat(szFileName, "winfract.hlp");
   }

   else {
       lstrcat(szFileName, "?");
   }

   return;
}

int set_win_offset(void)
{
win_xoffset = (int)(((long)xposition*((long)xdots-(long)xpagesize))/100L);
win_yoffset = (int)(((long)yposition*((long)ydots-(long)ypagesize))/100L);
if (win_xoffset+xpagesize > xdots) win_xoffset = xdots-xpagesize;
if (win_yoffset+ypagesize > ydots) win_yoffset = ydots-ypagesize;
if (xpagesize >= xdots) win_xoffset = 0;
if (ypagesize >= ydots) win_yoffset = 0;
return(0);
}

void win_savedac(void)
{
    memcpy(olddacbox,dacbox,256*3); /* save the DAC */
    colorpreloaded = 1;             /* indicate it needs to be restored */

}

/*
  Read a formula file, picking off the formula names.
  Formulas use the format "  name = { ... }  name = { ... } "
*/

int get_formula_names(void)         /* get the fractal formula names */
{
   int numformulas, i;
   FILE *File;
   char msg[81], tempstring[201];

   FormName[0] = 0;                /* start by declaring failure */
   for (i = 0; i < MaxFormNameChoices; i++) {
      FormNameChoices[i][0] = 0;
      win_choices[i] = FormNameChoices[i];
      }

   if((File = fopen(FormFileName, "rt")) == NULL) {
      sprintf("I Can't find %s", FormFileName);
      stopmsg(1,msg);
      return(-1);
   }

   numformulas = 0;
   while(fscanf(File, " %20[^ \n\t({]", FormNameChoices[numformulas]) != EOF) {
      int c;

      while(c = getc(File)) {
         if(c == EOF || c == '{' || c == '\n')
            break;
      }
      if(c == EOF)
         break;
      else if(c != '\n'){
         numformulas++;
         if (numformulas >= MaxFormNameChoices) break;
skipcomments:
         if(fscanf(File, "%200[^}]", tempstring) == EOF) break;
         if (getc(File) != '}') goto skipcomments;
         if (stricmp(FormNameChoices[numformulas-1],"") == 0 ||
             stricmp(FormNameChoices[numformulas-1],"comment") == 0)
                 numformulas--;
      }
   }
   fclose(File);
   win_numchoices = numformulas;
   qsort(FormNameChoices,win_numchoices,25,
         (int(*)(const void*, const void *))strcmp);
   return(0);
}

int parse_formula_names(void)         /* parse a fractal formula name */
{

   lstrcpy(FormName, win_choices[win_choicemade]);

   if (RunForm(FormName, 0)) {
       FormName[0] = 0;         /* declare failure */
       stopmsg(0,"Can't Parse that Formula");
       return(0);
       }

return(1);
}

/* --------------------------------------------------------------------- */

int get_lsys_name(void)                /* get the Lsystem formula name */
{
   int numentries, i;
   FILE *File;
   char buf[201];
   long file_offset,name_offset;

   for (i = 0; i < MaxFormNameChoices; i++) {
      FormNameChoices[i][0] = 0;
      win_choices[i] = FormNameChoices[i];
      }

   if ((File = fopen(LFileName, "rb")) == NULL) {
      sprintf(buf,"I Can't find %s", LFileName);
      stopmsg(1,buf);
      LName[0] = 0;
      return(-1);
      }

   numentries = 0;
   file_offset = -1;
   while (1) {
      int c,len;
      do {
         ++file_offset;
         c = getc(File);
         } while (c == ' ' /* skip white space */
               || c == '\t' || c == '\n' || c == '\r');
      if (c == ';') {
         do {
            ++file_offset;
            c = getc(File);
            } while (c != '\n' && c != EOF && c != '\x1a');
         if (c == EOF || c == '\x1a') break;
         continue;
         }
      name_offset = file_offset;
      len = 0; /* next equiv roughly to fscanf(..,"%40[^ \n\r\t({\x1a]",buf) */
      while (c != ' ' && c != '\t' && c != '('
        && c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\x1a') {
         if (len < 40) buf[len++] = c;
         c = getc(File);
         ++file_offset;
         }
      buf[len] = 0;
      while (c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\x1a') {
         c = getc(File);
         ++file_offset;
         }
      if (c == '{') {
         while (c != '}' && c != EOF && c != '\x1a') {
            c = getc(File);
            ++file_offset;
            }
         if (c != '}') break;
         buf[ITEMNAMELEN] = 0;
         if (buf[0] != 0 && lstrcmpi(buf,"comment") != 0) {
            lstrcpy(FormNameChoices[numentries],buf);
            memcpy((char *)&FormNameChoices[numentries][21],
                (char *)&name_offset,4);
            if (++numentries >= MaxFormNameChoices) {
               sprintf(buf,"Too many entries in file, first %d used",
                   MaxFormNameChoices);
               stopmsg(0,buf);
               break;
               }
            }
         }
      else
         if (c == EOF || c == '\x1a') break;
      }
   fclose(File);

   win_numchoices = numentries;
   qsort(FormNameChoices,win_numchoices,25,
         (int(*)(const void *, const void *))strcmp);
   return(0);
}

BOOL cant_clip(void)
{
MessageBox (
   GetFocus(),
   winfract_msg99,
   winfract_msg01,
    MB_ICONSTOP | MB_OK);
    return(TRUE);
}
