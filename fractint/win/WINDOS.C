/*
        Routines which simulate DOS functions in the existing
        Fractint for DOS
*/

#define STRICT

#include "port.h"
#include "prototyp.h"

#include <windows.h>
#include <string.h>
#include <time.h>
#include <drivinit.h>

#include "winfract.h"
#include "mathtool.h"

#ifndef USE_VARARGS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifndef TIMERINFO
                                /* define TIMERINFO stuff, if needs be */
typedef struct tagTIMERINFO {
    DWORD dwSize;
    DWORD dwmsSinceStart;
    DWORD dwmsThisVM;
    } TIMERINFO;

BOOL    far pascal TimerCount(TIMERINFO FAR *);
#endif

int stopmsg(int , char far *);
int  farread(int, VOIDFARPTR, unsigned);
int  farwrite(int, VOIDFARPTR, unsigned);

extern unsigned char FullPathName[];
extern int FileFormat;

int save_system;           /* tag identifying Fractint for Windows */
int save_release;           /* tag identifying version number */
extern int win_release;    /* tag identifying version number  (in WINDOS2.C) */

extern BOOL bTrack, bMove;             /* TRUE if user is selecting a region */
extern BOOL zoomflag;                     /* TRUE is a zoom-box selected */

extern HWND hwnd;                     /* handle to main window */
extern HANDLE hInst;

extern HANDLE hAccTable;             /* handle to accelerator table */

extern char szHelpFileName[];             /* Help file name*/

extern long maxiter;
extern int xposition, yposition, win_xoffset, win_yoffset, xpagesize, ypagesize;
extern int win_xdots, win_ydots;

extern int last_written_y;             /* last line written */
extern int screen_to_be_cleared;     /* clear screen flag */

extern int time_to_act;              /* time to take some action? */
extern int time_to_restart;             /* time to restart?  */
extern int time_to_resume;             /* time to resume? */
extern int time_to_quit;             /* time to quit? */
extern int time_to_reinit;             /* time to reinitialize? */
extern int time_to_load;             /* time to load? (DECODE) */
extern int time_to_save;             /* time to save? (ENCODE) */
extern int time_to_print;             /* time to print? (PRINTER) */
extern int time_to_cycle;             /* time to begin color-cycling? */
extern int time_to_starfield;        /* time to make a starfield? */
extern int time_to_orbit;            /* time to activate orbits? */

extern BOOL win_systempaletteused;        /* flag system palette set */

extern unsigned char far temp_array[];         /* temporary spot for Encoder rtns */

extern HANDLE hpixels;                        /* handle to the DIB pixels */
extern unsigned char huge *pixels;   /* the device-independent bitmap pixels */
int pixels_per_byte;                     /* pixels/byte in the pixmap */
long pixels_per_bytem1;              /* pixels / byte - 1 (for ANDing) */
int pixelshift_per_byte;             /* 0, 1, 2, or 3 */
int bytes_per_pixelline;             /* pixels/line / pixels/byte */
long win_bitmapsize;                     /* bitmap size, in bytes */

extern int win_overlay3d;
extern int win_display3d;


BOOL dont_wait_for_a_key = TRUE;

#ifdef __BORLANDC__

/* Too many functions defaulting to a type 'int' return that should be
   a type 'void'.  I'll just get rid of the warning message for this file
   only.  MCP 8-6-91 */

   #pragma warn -rvl

   int LPTNumber;
   int stackavail() { return(10240 + (signed int)_SP); }
#else
   int printf(const char *dummy, ...) {return(0);}
   int _bios_serialcom(){return(0);}
#endif


extern int far wintext_textmode, far wintext_AltF4hit;

int getakey(void)
{
int i;

if (time_to_orbit) {  /* activate orbits? */
    zoomflag = FALSE;
    time_to_orbit = 0;
    i = 'o';
    return(i);
    }

dont_wait_for_a_key = FALSE;
i = keypressed();
dont_wait_for_a_key = TRUE;
zoomflag = FALSE;
return(i);

}

int keypressed(void)
{
MSG msg;
int time_to;

/* is a text-mode screen active? */
if (wintext_textmode == 2 || wintext_AltF4hit) {
    if (dont_wait_for_a_key)
        return(fractint_getkeypress(0));
    else
        return(fractint_getkeypress(1));
    }

if (dont_wait_for_a_key)
    if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE) == 0) {
        time_to = time_to_act + time_to_reinit+time_to_restart+time_to_quit+
            time_to_load+time_to_save+time_to_print+time_to_cycle+
            time_to_resume+time_to_starfield;
        if (time_to_orbit) {  /* activate orbits? */
            time_to = 'o';
            }
        /* bail out if nothing is happening */
        return(time_to);
        }

while (GetMessage(&msg, 0, 0, 0)) {

    if (!TranslateAccelerator(hwnd, hAccTable, &msg)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }

    CheckMathTools();
    if (!bTrack && !bMove) {                  /* don't do this if mouse-button is down */
        time_to = time_to_act+time_to_reinit+time_to_restart+time_to_quit+
            time_to_load+time_to_save+time_to_print+time_to_cycle+
            time_to_starfield;
        if (time_to_orbit) {  /* activate orbits? */
            time_to = 'o';
            }
        if (dont_wait_for_a_key || time_to)
            return(time_to);
        }

    }

if (!dont_wait_for_a_key)
    time_to_quit = 1;

    /* bail out if nothing is happening */
    time_to = time_to_act+time_to_reinit+time_to_restart+time_to_quit+
        time_to_load+time_to_save+time_to_print+time_to_cycle;
    if (time_to_orbit) {  /* activate orbits? */
        time_to = 'o';
        }
    return(time_to);

}

int  farread(int handle, VOIDFARPTR buf, unsigned len)
{
int i;

    i = _lread(handle, buf, len);
    return(i);

}

int  farwrite(int handle, VOIDFARPTR buf, unsigned len)
{

    return(_lwrite(handle, buf, len));

}


extern int win_fastupdate;

time_t last_time;
time_t update_time;
long minimum_update;
long pixelsout;
int top_changed, bottom_changed;

/* Made global, MCP 6-16-91 */
unsigned char win_andmask[8];
unsigned char win_notmask[8];
unsigned char win_bitshift[8];

void putcolor_a(int x, int y, int color)
{
RECT tempRect;                         /* temporary rectangle structure */
long i;
int temp_top_changed, temp_bottom_changed;
time_t this_time;

last_written_y = y;
if (y < top_changed) top_changed = y;
if (y > bottom_changed) bottom_changed = y;

i = win_ydots-1-y;
i = (i * win_xdots) + x;

if (x >= 0 && x < xdots && y >= 0 && y < ydots) {
    if (pixelshift_per_byte == 0) {
          pixels[i] = color % colors;
          }
     else {
          unsigned int j;
          j = (unsigned int)(i & pixels_per_bytem1);
          i = i >> pixelshift_per_byte;
          pixels[i] = (pixels[i] & win_notmask[j]) +
              (((unsigned char)(color % colors)) << win_bitshift[j]);
          }

     /* check the time every nnn pixels */
     if (win_fastupdate || ++pixelsout > 100) {
          pixelsout = 0;
          this_time = time(NULL);
          /* time to update the screen? */
          if (win_fastupdate || (this_time - last_time) > update_time ||
              ((int)(minimum_update*(this_time-last_time))) > (bottom_changed-top_changed)) {
              temp_top_changed = top_changed - win_yoffset;
              temp_bottom_changed = bottom_changed - win_yoffset;
              if (!(temp_top_changed >= ypagesize || temp_bottom_changed < 0)) {
                  if (temp_top_changed    < 0) temp_top_changed    = 0;
                  if (temp_bottom_changed < 0) temp_bottom_changed = 0;
                  if (temp_top_changed    > ypagesize) temp_top_changed    = ypagesize;
                  if (temp_bottom_changed > ypagesize) temp_bottom_changed = ypagesize;
                  tempRect.top = temp_top_changed;
                  tempRect.bottom = temp_bottom_changed+1;
                  tempRect.left = 0;
                  tempRect.right = xdots;
                  if (win_fastupdate == 1) {
                      tempRect.left =  x-win_xoffset;
                      tempRect.right = x-win_xoffset+1;
                      }
                  InvalidateRect(hwnd, &tempRect, FALSE);
/*
                  EndDeferWindowPos(BeginDeferWindowPos(0));
*/
                  }
              if (win_fastupdate) {
                  extern int kbdcount;
                  if (kbdcount > 5)
                      kbdcount = 5;
                  win_fastupdate = 1;
                  }
              keypressed();    /* force a look-see at the screen */
              last_time = this_time;
              top_changed = win_ydots;
              bottom_changed = 0;
              }
          }
     }

}

int getcolor(int x, int y)
{
long i;

i = win_ydots-1-y;
i = (i * win_xdots) + x;

if (x >= 0 && x < xdots && y >= 0 && y < ydots) {
    if (pixelshift_per_byte == 0) {
          return(pixels[i]);
          }
     else {
          unsigned int j;
          j = (unsigned int)(i & pixels_per_bytem1);
          i = i >> pixelshift_per_byte;
          return((int)((pixels[i] & win_andmask[j]) >> win_bitshift[j]));
          }
     }
else
     return(0);
}

int put_line(int rownum, int leftpt, int rightpt, BYTE *localvalues)
{
int i, len;
long startloc;

len = rightpt - leftpt;
if (rightpt >= xdots) len = xdots - 1 - leftpt;
startloc = win_ydots-1-rownum;
startloc = (startloc * win_xdots) + leftpt;

if (rownum < 0 || rownum >= ydots || leftpt < 0) {
    return(0);
    }

if (pixelshift_per_byte == 0) {
    for (i = 0; i <= len; i++)
        pixels[startloc+i] = localvalues[i];
    }
else {
    unsigned int j;
    long k;
    for (i = 0; i <= len; i++) {
        k = startloc + i;
        j = (unsigned int)(k & pixels_per_bytem1);
        k = k >> pixelshift_per_byte;
        pixels[k] = (pixels[k] & win_notmask[j]) +
            (((unsigned char)(localvalues[i] % colors)) << win_bitshift[j]);
        }
    }
pixelsout += len;
if (win_fastupdate)
    win_fastupdate = 2;  /* force 'putcolor()' to update a whole scanline */
putcolor(leftpt, rownum, localvalues[0]);
}

int get_line(int rownum, int leftpt, int rightpt, BYTE *localvalues)
{
int i, len;
long startloc;

len = rightpt - leftpt;
if (rightpt >= xdots) len = xdots - 1 - leftpt;
startloc = win_ydots-1-rownum;
startloc = (startloc * win_xdots) + leftpt;

if (rownum < 0 || rownum >= ydots || leftpt < 0 || rightpt >= xdots) {
    for (i = 0; i <= len; i++)
        localvalues[i] = 0;
    return(0);
    }

if (pixelshift_per_byte == 0) {
    for (i = 0; i <= len; i++)
        localvalues[i] = pixels[startloc+i];
    }
else {
    unsigned int j;
    long k;
    for (i = 0; i <= len; i++) {
        k = startloc + i;
        j = (unsigned int)(k & pixels_per_bytem1);
        k = k >> pixelshift_per_byte;
        localvalues[i] = (pixels[k] & win_andmask[j]) >> win_bitshift[j];
        }
    }
}

extern int rowcount;

int out_line(BYTE *localvalues, int numberofdots)
{
    put_line(rowcount++, 0, numberofdots, localvalues);
    return (0);
}

extern LPBITMAPINFO pDibInfo;                /* pointer to the DIB info */

int clear_screen(int forceclear)
{
long numdots;
int i;

/* set up the videoentry values */
strcpy(videoentry.name,   "Windows Video Image");
strcpy(videoentry.comment,"Generated using Winfract");
videoentry.keynum      = 40;
videoentry.videomodeax = 3;
videoentry.videomodebx = 0;
videoentry.videomodecx = 0;
videoentry.videomodedx = 0;
videoentry.dotmode     = 1;
videoentry.xdots       = xdots;
videoentry.ydots       = ydots;
videoentry.colors      = colors;

win_xdots = (xdots+3) & 0xfffc;
win_ydots = ydots;
pixelshift_per_byte = 0;
pixels_per_byte   = 1;
pixels_per_bytem1 = 0;
if (colors == 16) {
    win_xdots = (xdots+7) & 0xfff8;
    pixelshift_per_byte = 1;
    pixels_per_byte = 2;
    pixels_per_bytem1 = 1;
    win_andmask[0] = 0xf0;  win_notmask[0] = 0x0f; win_bitshift[0] = 4;
    win_andmask[1] = 0x0f;  win_notmask[1] = 0xf0; win_bitshift[1] = 0;
    }
if (colors == 2) {
    win_xdots = (xdots+31) & 0xffe0;
    pixelshift_per_byte = 3;
    pixels_per_byte = 8;
    pixels_per_bytem1 = 7;
    win_andmask[0] = 0x80;  win_notmask[0] = 0x7f; win_bitshift[0] = 7;
    for (i = 1; i < 8; i++) {
        win_andmask[i] = win_andmask[i-1] >> 1;
        win_notmask[i] = (win_notmask[i-1] >> 1) + 0x80;
        win_bitshift[i] = win_bitshift[i-1] - 1;
        }
    }

numdots = (long)win_xdots * (long) win_ydots;
update_time = 2;
/* disable the long delay logic
if (numdots > 200000L) update_time = 4;
if (numdots > 400000L) update_time = 8;
*/
last_time = time(NULL) - update_time + 1;
minimum_update = 7500/xdots;        /* assume 75,000 dots/sec drawing speed */

last_written_y = -1;
pixelsout = 0;
top_changed = win_ydots;
bottom_changed = 0;

bytes_per_pixelline = win_xdots >> pixelshift_per_byte;

/* Create the Device-independent Bitmap entries */
pDibInfo->bmiHeader.biWidth  = win_xdots;
pDibInfo->bmiHeader.biHeight = win_ydots;
pDibInfo->bmiHeader.biSizeImage = (DWORD)bytes_per_pixelline * win_ydots;
pDibInfo->bmiHeader.biBitCount = 8 / pixels_per_byte;

/* hard to believe, but this is the fast way to clear the pixel map */
if (hpixels) {
     GlobalUnlock(hpixels);
     GlobalFree(hpixels);
     }

win_bitmapsize = (numdots >> pixelshift_per_byte)+1;

if (!(hpixels = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, win_bitmapsize)))
     return(0);
if (!(pixels = (char huge *)GlobalLock(hpixels))) {
     GlobalFree(hpixels);
     return(0);
     }

/* adjust the colors for B&W or default */
if (colors == 2) {
    dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
    dacbox[1][0] = dacbox[1][1] = dacbox[1][2] = 63;
    spindac(0,1);
    }
else
    restoredac();   /* color palette */

screen_to_be_cleared = 1;
InvalidateRect(hwnd, NULL, TRUE);

if (forceclear)
    keypressed();                /* force a look-see at the screen */

return(1);
}

void flush_screen(void)
{

last_written_y = 0;

InvalidateRect(hwnd, NULL, FALSE);

}

void buzzer(int i)
{

MessageBeep(0);

}


#define MAXFARMEMALLOCS  50                /* max active farmemallocs */
int   farmemallocinit = 0;                /* any memory been allocated yet?   */
HANDLE farmemallochandles[MAXFARMEMALLOCS];                        /* handles  */
void far *farmemallocpointers[MAXFARMEMALLOCS]; /* pointers */

void far * cdecl farmemalloc(long bytecount)
{
int i;
HANDLE temphandle;
void far *temppointer;

if (!farmemallocinit) {         /* never been here yet - initialize */
    farmemallocinit = 1;
    for (i = 0; i < MAXFARMEMALLOCS; i++) {
        farmemallochandles[i] = (HANDLE)0;
        farmemallocpointers[i] = NULL;
        }
    }

for (i = 0; i < MAXFARMEMALLOCS; i++)  /* look for a free handle */
    if (farmemallochandles[i] == (HANDLE)0) break;

if (i == MAXFARMEMALLOCS)        /* uh-oh - no more handles */
   return(NULL);                /* can't get far memory this way */

if (!(temphandle = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, bytecount)))
     return(NULL);                /* can't allocate the memory */

if ((temppointer = (void far *)GlobalLock(temphandle)) == NULL) {
     GlobalFree(temphandle);
     return(NULL);                /* ?? can't lock the memory ?? */
     }

farmemallochandles[i] =  temphandle;
farmemallocpointers[i] = temppointer;
return(temppointer);
}

void farmemfree(void far *bytepointer)
{
int i;

if (bytepointer == (void far *)NULL) return;

for (i = 0; i < MAXFARMEMALLOCS; i++)        /* search for a matching pointer */
    if (farmemallocpointers[i] == bytepointer)
         break;
if (i < MAXFARMEMALLOCS) {                /* got one */
    GlobalUnlock(farmemallochandles[i]);
    GlobalFree(farmemallochandles[i]);
    farmemallochandles[i] = (HANDLE)0;
    }

}

void debugmessage(char *msg1, char *msg2)
{
MessageBox (
    GetFocus(),
    msg2,
    msg1,
    MB_ICONASTERISK | MB_OK);

}

void texttempmsg(char far *msg1)
{
MessageBox (
    GetFocus(),
    msg1,
    "Encoder",
    MB_ICONASTERISK | MB_OK);
}

int stopmsg(int flags, char far *msg1)
{
int result;

if (! (flags & 4)) MessageBeep(0);

result = IDOK;

if (!(flags & 2))
    MessageBox (
        0,
        msg1,
        "Fractint for Windows",
        MB_TASKMODAL | MB_ICONASTERISK | MB_OK);
else
    result = MessageBox (
        0,
        msg1,
        "Fractint for Windows",
        MB_TASKMODAL | MB_ICONQUESTION | MB_OKCANCEL);

if (result == 0 || result == IDOK || result == IDYES)
    return(0);
else
    return(-1);
}

extern char readname[];
extern int fileydots, filexdots, filecolors;
extern int     iNumColors;    /* Number of colors supported by device               */
extern int     iRasterCaps;   /* Raster capabilities                               */

int win_load(void)
{
int i;

time_to_load = 0;

    start_wait();
    if ((i = read_overlay()) >= 0 && (!win_display3d ||
        xdots < filexdots || ydots < fileydots)) {
        if (win_display3d) stopmsg(0,
            "3D and Overlay3D file image sizes must be\nat least as large as the display image.\nAltering your display image to match the file.");
        xdots = filexdots;
        ydots = fileydots;
        colors = filecolors;
        if (colors > 16) colors = 256;
        if (colors >  2 && colors < 16) colors = 16;
        if (xdots < 50) xdots = 50;
        if (xdots > 2048) xdots = 2048;
        if (ydots < 50) ydots = 50;
        if (ydots > 2048) ydots = 2048;
        set_win_offset();
        clear_screen(0);
        }
    end_wait();
    return(i);
}

void win_save(void)
{
    time_to_save = 0;
    save_system = 1;
    save_release = win_release;

    /* MCP 10-27-91 */
    if(FileFormat != ID_BMP)
       savetodisk(readname);
    else
       SaveBitmapFile(hwnd, FullPathName);
    CloseStatusBox();
}

/*
        Delay code - still not so good for a multi-tasking environment,
        but what the hell...
*/

DWORD DelayCount;

DWORD DelayMillisecond(void)
{
   DWORD i;

   i = 0;
   while(i != DelayCount)
      i++;
   return(i);
}

void delay(DWORD milliseconds)
{
   DWORD n, i, j;

if (DelayCount == 0) {      /* use version 3.1's 1ms timer */

    TIMERINFO timerinfo;

    timerinfo.dwSize = sizeof(timerinfo);
    TimerCount(&timerinfo);
    n = timerinfo.dwmsSinceStart;
    i = n + milliseconds;
    for (;;) {
        keypressed();        /* let the other folks in */
        TimerCount(&timerinfo);
        j = timerinfo.dwmsSinceStart;
        if (j < n || j >= i)
            break;
        }

   }
else {
   for(n = 0; n < milliseconds; n++)
      DelayMillisecond();
   }

}

void CalibrateDelay(void)
{
    DWORD Now, Time, Delta, TimeAdj;

    /* this logic switches tothe fast timer logic supplied by TimerCount */
    DelayCount = 0;
    return;

   DelayCount = 128;

   /* Determine the Windows timer resolution.  It's usually 38ms in
      version 3.0, but that may change latter. */
   Now = Time = GetCurrentTime();
   while(Time == Now)
      Now = GetCurrentTime();

   /* Logrithmic Adjust */
   Delta = Now - Time;
   Time = Now;
   while(Time == Now)
   {
      delay(Delta);
      Now = GetCurrentTime();
      if(Time == Now)
      {
         /* Resynch */
         Time = Now = GetCurrentTime();
         while(Time == Now)
            Now = GetCurrentTime();
         Time = Now;
         DelayCount <<= 1;
      }
   }

   /* Linear Adjust */
   Time = Now;
   TimeAdj = (DelayCount - (DelayCount >> 1)) >> 1;
   DelayCount -= TimeAdj;
   while(TimeAdj > 16)
   {
      delay(Delta);
      TimeAdj >>= 1;
      if(GetCurrentTime() == Now)
         DelayCount += TimeAdj;
      else
         DelayCount -= TimeAdj;

      /* Resynch */
      Time = Now = GetCurrentTime();
      while(Time == Now)
         Now = GetCurrentTime();
      Time = Now;
   }
}

/*
        Color-cycling logic
        includes variable-delay capabilities
*/

extern int win_cycledir, win_cyclerand, win_cyclefreq, win_cycledelay;

extern HANDLE  hPal;           /* Palette Handle */
extern LPLOGPALETTE pLogPal;  /* pointer to the application's logical palette */
extern unsigned char far win_dacbox[256][3];
#define PALETTESIZE 256               /* dull-normal VGA                    */

static int win_fsteps[] = {54, 24, 8};

int win_animate_flag = 0;
int win_syscolorindex[21];
DWORD win_syscolorold[21];
DWORD win_syscolornew[21];

extern int debugflag;

void win_cycle(void)
{
int istep, jstep, fstep, step, oldstep, last, next, maxreg;
int incr, fromred, fromblue, fromgreen, tored, toblue, togreen;
HDC hDC;                      /* handle to device context            */

fstep = 1;                                /* randomization frequency        */
oldstep = 1;                                /* single-step                        */
step = 256;                                /* single-step                        */
incr = 999;                                /* ready to randomize                */
maxreg = 256;                                /* maximum register to rotate        */
last = maxreg-1;                        /* last box that was filled        */
next = 1;                                /* next box to be filled        */
if (win_cycledir < 0) {
        last = 1;
        next = maxreg;
        }

win_title_text(2);

srand((unsigned)time(NULL));                /* randomize things                */

hDC = GetDC(GetFocus());

win_animate_flag = 1;
SetPaletteEntries(hPal, 0, pLogPal->palNumEntries, pLogPal->palPalEntry);
SelectPalette (hDC, hPal, 1);

if ((iNumColors == 16 || debugflag == 1000) && !win_systempaletteused) {
    int i;
    DWORD white, black;
    win_systempaletteused = TRUE;
    white = 0xffffff00;
    black = 0;
    for (i = 0; i <= COLOR_ENDCOLORS; i++) {
        win_syscolorindex[i] = i;
        win_syscolorold[i] = GetSysColor(i);
        win_syscolornew[i] = black;
        }
    win_syscolornew[COLOR_BTNTEXT] = white;
    win_syscolornew[COLOR_CAPTIONTEXT] = white;
    win_syscolornew[COLOR_GRAYTEXT] = white;
    win_syscolornew[COLOR_HIGHLIGHTTEXT] = white;
    win_syscolornew[COLOR_MENUTEXT] = white;
    win_syscolornew[COLOR_WINDOWTEXT] = white;
    win_syscolornew[COLOR_WINDOWFRAME] = white;
    win_syscolornew[COLOR_INACTIVECAPTION] = white;
    win_syscolornew[COLOR_INACTIVEBORDER] = white;
    SetSysColors(COLOR_ENDCOLORS,(LPINT)win_syscolorindex,(LONG FAR *)win_syscolornew);
    SetSystemPaletteUse(hDC,SYSPAL_NOSTATIC);
    UnrealizeObject(hPal);
    }

while (time_to_cycle) {
    if (win_cyclerand) {
        for (istep = 0; istep < step; istep++) {
            jstep = next + (istep * win_cycledir);
            if (jstep <=          0) jstep += maxreg-1;
            if (jstep >= maxreg) jstep -= maxreg-1;
            if (++incr > fstep) {        /* time to randomize        */
                incr = 1;
                fstep = ((win_fsteps[win_cyclefreq]*
                    (rand() >> 8)) >> 6) + 1;
                fromred   = dacbox[last][0];
                fromgreen = dacbox[last][1];
                fromblue  = dacbox[last][2];
                tored          = rand() >> 9;
                togreen   = rand() >> 9;
                toblue          = rand() >> 9;
                }
            dacbox[jstep][0] = fromred         + (((tored   - fromred  )*incr)/fstep);
            dacbox[jstep][1] = fromgreen + (((togreen - fromgreen)*incr)/fstep);
            dacbox[jstep][2] = fromblue  + (((toblue  - fromblue )*incr)/fstep);
            }
        }
        if (step >= 256) step = oldstep;

    spindac(win_cycledir,step);
    delay(win_cycledelay);
    AnimatePalette(hPal, 0, pLogPal->palNumEntries, pLogPal->palPalEntry);
    RealizePalette(hDC);
    keypressed();
    if (win_cyclerand == 2) {
        win_cyclerand = 1;
        step = 256;
        }
    }

win_animate_flag = 0;
ReleaseDC(GetFocus(),hDC);

win_title_text(0);

}

/* cursor routines */

extern HANDLE hSaveCursor;               /* the original cursor value */
extern HANDLE hHourGlass;               /* the hourglass cursor value */

void start_wait(void)
{
   hSaveCursor = (HANDLE)SetClassWord(hwnd, GCW_HCURSOR, (WORD)hHourGlass);
}

void end_wait(void)
{
   SetClassWord(hwnd, GCW_HCURSOR, (WORD)hSaveCursor);
}

/* video-mode routines */

extern int    viewwindow;                /* 0 for full screen, 1 for window */
extern float  viewreduction;                /* window auto-sizing */
extern float  finalaspectratio;         /* for view shape and rotation */
extern int    viewxdots,viewydots;        /* explicit view sizing */
extern int    fileydots, filexdots, filecolors;
extern float  fileaspectratio;
extern short    skipxdots,skipydots;        /* for decoder, when reducing image */


int get_video_mode(struct fractal_info *info,struct ext_blk_3 *dummy)
{
   viewwindow = viewxdots = viewydots = 0;
   fileaspectratio = (float)(0.75);
   skipxdots = skipydots = 0;
   return(0);
}


void spindac(int direction, int step)
{
int i, j, k;
int cycle_start, cycle_fin;
extern int rotate_lo,rotate_hi;
char tempdacbox;

cycle_start = 0;
cycle_fin = 255;
if (time_to_cycle) {
   cycle_start = rotate_lo;
   cycle_fin = rotate_hi;
   }

for (k = 0; k < step; k++) {
    if (direction > 0) {
        for (j = 0; j < 3; j++) {
            tempdacbox = dacbox[cycle_fin][j];
            for (i = cycle_fin; i > cycle_start; i--)
                dacbox[i][j] = dacbox[i-1][j];
            dacbox[cycle_start][j] = tempdacbox;
            }
        }
    if (direction < 0) {
        for (j = 0; j < 3; j++) {
            tempdacbox = dacbox[cycle_start][j];
            for (i = cycle_start; i < cycle_fin; i++)
                dacbox[i][j] = dacbox[i+1][j];
            dacbox[cycle_fin][j] = tempdacbox;
            }
        }
    }

    /* fill in intensities for all palette entry colors */
    for (i = 0; i < 256; i++) {
        pLogPal->palPalEntry[i].peRed        = ((BYTE)dacbox[i][0]) << 2;
        pLogPal->palPalEntry[i].peGreen = ((BYTE)dacbox[i][1]) << 2;
        pLogPal->palPalEntry[i].peBlue        = ((BYTE)dacbox[i][2]) << 2;
        pLogPal->palPalEntry[i].peFlags = PC_RESERVED;
        }

    if (!win_animate_flag) {
        HDC hDC;
        hDC = GetDC(GetFocus());
        SetPaletteEntries(hPal, 0, pLogPal->palNumEntries, pLogPal->palPalEntry);
        SelectPalette (hDC, hPal, 1);
        RealizePalette(hDC);
        ReleaseDC(GetFocus(),hDC);
        /* for non-palette-based adapters, redraw the image */
        if (!iRasterCaps) {
            InvalidateRect(hwnd, NULL, FALSE);
            }
        }
}

void restoredac(void)
{
int iLoop;
int j;

    /* fill in intensities for all palette entry colors */
    for (iLoop = 0; iLoop < PALETTESIZE; iLoop++)
        for (j = 0; j < 3; j++)
            dacbox[iLoop][j] = win_dacbox[iLoop][j];
    spindac(0,1);
}

int win_thinking = 0;

int thinking(int waiting, char far *dummy)
{
if (waiting && ! win_thinking) {
    win_thinking = 1;
    start_wait();
    }
if (!waiting)
    end_wait();
return(keypressed());
}

extern HWND far wintext_hWndCopy;                /* a Global copy of hWnd */

/* Call for help caused by pressing F1 inside the "fractint-style"
   prompting routines */
void winfract_help(void)
{
        WinHelp(wintext_hWndCopy,szHelpFileName,FIHELP_INDEX,0L);
}

int far_strnicmp(char far *string1, char far *string2, int maxlen) {
int i;
unsigned char j, k;
for (i = 0;i < maxlen ; i++) {
    j = string1[i];
    k = string2[i];
    if (j >= 'a' && j <= 'z') j -= ('a' - 'A');
    if (k >= 'a' && k <= 'z') k -= ('a' - 'A');
    if (j-k != 0)
        return(j-k);
    }
return(0);
}

void far_memcpy(void far *string1, void far *string2, int maxlen) {
int i;
for (i = 0;i < maxlen ; i++)
    ((char far *)string1)[i] = ((char far *)string2)[i];
}

int far_memcmp(void far *string1, void far *string2, int maxlen) {
int i;
unsigned char j, k;
for (i = 0;i < maxlen ; i++) {
    j = ((char far *)string1)[i];
    k = ((char far *)string2)[i];
    if (j-k != 0)
        return(j-k);
    }
return(0);
}

/*
; *************** Far string/memory functions *********
*/

long timer_start,timer_interval;        /* timer(...) start & total */
extern int  timerflag;
extern int  show_orbit;
extern        int        dotmode;                /* video access method            */
extern        long        maxit;                        /* try this many iterations */

int check_key()
{
   int key;
   if((key = keypressed()) != 0) {
      if(key != 'o' && key != 'O') {
         return(-1);
      }
      getakey();
      if (dotmode != 11)
         show_orbit = 1 - show_orbit;
   }
   return(0);
}

/* timer function:
     timer(0,(*fractal)())                fractal engine
     timer(1,NULL,int width)                decoder
     timer(2)                                encoder
  */
#ifndef USE_VARARGS
int timer(int timertype,int(*subrtn)(),...)
#else
int timer(va_alist)
va_dcl
#endif
{
   va_list arg_marker;        /* variable arg list */
   char *timestring;
   time_t ltime;
   FILE *fp;
   int out;
   int i;
   int do_bench;

#ifndef USE_VARARGS
   va_start(arg_marker,subrtn);
#else
   int timertype;
   int (*subrtn)();
   va_start(arg_marker);
   timertype = va_arg(arg_marker, int);
   subrtn = ( int (*)()) va_arg(arg_marker, int *);
#endif

   do_bench = timerflag; /* record time? */
   if (timertype == 2)         /* encoder, record time only if debug=200 */
      do_bench = (debugflag == 200);
   if(do_bench)
      fp=fopen("bench","a");
   timer_start = clock_ticks();
   switch(timertype) {
      case 0:
         out = (*subrtn)();
         break;
      case 1:
         i = va_arg(arg_marker,int);
         out = decoder(i);             /* not indirect, safer with overlays */
         break;
      case 2:
         out = encoder();             /* not indirect, safer with overlays */
         break;
      }
   /* next assumes CLK_TCK is 10^n, n>=2 */
   timer_interval = (clock_ticks() - timer_start) / (CLK_TCK/100);

   if(do_bench) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      switch(timertype) {
         case 1:
            fprintf(fp,"decode ");
            break;
         case 2:
            fprintf(fp,"encode ");
            break;
         }
      fprintf(fp,"%s type=%s resolution = %dx%d maxiter=%d",
          timestring,
          curfractalspecific->name,
          xdots,
          ydots,
          maxit);
      fprintf(fp," time= %ld.%02ld secs\n",timer_interval/100,timer_interval%100);
      if(fp != NULL)
         fclose(fp);
      }
   return(out);
}

/* dummy out the environment entries, as we never use them */
#ifndef QUICKC
int _setenvp(void){return(0);}
#endif

extern double zwidth;

void clear_zoombox(void)
{
   zwidth = 0;
   drawbox(0);
   reset_zoom_corners();
}

extern double xxmin, xxmax, xx3rd, yymin, yymax, yy3rd;
extern double sxmin, sxmax, sx3rd, symin, symax, sy3rd;

void reset_zoom_corners(void)
{
   xxmin = sxmin;
   xxmax = sxmax;
   xx3rd = sx3rd;
   yymax = symax;
   yymin = symin;
   yy3rd = sy3rd;
   if(bf_math)
   {
      copy_bf(bfxmin,bfsxmin);
      copy_bf(bfxmax,bfsxmax);
      copy_bf(bfymin,bfsymin);
      copy_bf(bfymax,bfsymax);
      copy_bf(bfx3rd,bfsx3rd);
      copy_bf(bfy3rd,bfsy3rd);
   }
}

/*  fake videotable stuff  */

struct videoinfo far videotable[1];

void vidmode_keyname(int num, char *string)
{
    strcpy(string,"    ");  /* fill in a blank videomode name */
}

int check_vidmode_keyname(char *dummy)
{
    return(1);  /* yeah, sure - that's a good videomode name. */
}

int check_vidmode_key(int dummy1, int dummy2)
{
    /* fill in a videomode structure that looks just like the current image */
    videotable[0].xdots = xdots;
    videotable[0].ydots = ydots;
    return 0;  /* yeah, sure - that's a good key. */
}
