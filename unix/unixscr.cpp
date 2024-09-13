/* Unixscr.c
 * This file contains routines for the Unix port of Id.
 * It uses the current window for text and creates an X window for graphics.
 *
 * This file Copyright 1991 Ken Shirriff.  It may be used according to the
 * fractint license conditions, blah blah blah.
 *
 * Some of the X stuff is based on xloadimage by Jim Frost.
 * The FindWindowRoot routine is from ssetroot by Tom LaStrange.
 * Other root window stuff is based on xmartin, by Ed Kubaitis.
 * Some of the colormap stuff is from Mike Yang (mikey@sgi.com).
 * Some of the zoombox code is from Bill Broadley.
 * David Sanderson straightened out a bunch of include file problems.
 */
#include "unixscr.h"

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "editpal.h"
#include "goodbye.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "os.h"
#include "mouse.h"
#include "rotate.h"
#include "spindac.h"
#include "video_mode.h"
#include "zoom.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#ifdef FPUERR
#include <floatingpoint.h>
#endif

#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef LINUX
#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif
#endif

constexpr int ctl(int code)
{
    return code & 0x1f;
}

static int iocount;

// Check if there is a character waiting for us.
#define input_pending() (ioctl(0, FIONREAD, &iocount), (int)iocount)

// external variables (set in the id.cfg file, but findable here

extern  int g_dot_mode;        // video access method (= 19)
extern  int g_screen_x_dots, g_screen_y_dots;     // total # of dots on the screen
extern  int g_logical_screen_x_offset, g_logical_screen_y_offset;     // offset of drawing area
extern  int g_colors;         // maximum colors available
extern  int g_adapter;
extern bool g_got_real_dac;
extern bool g_inside_help;
extern float g_final_aspect_ratio;
extern  float   g_screen_aspect;

extern VIDEOINFO x11_video_table[];

// the video-palette array (named after the VGA adapter's video-DAC)

extern unsigned char dacbox[256][3];

extern int g_color_cycle_range_hi;

extern void fpe_handler(int signum);

//extern WINDOW *curwin;

static int onroot = 0;
static int fullscreen = 0;
static int sharecolor = 0;
static int privatecolor = 0;
static int fixcolors = 0;
static int synch = 0; // Run X events synchronously (debugging)
static int simple_input = 0; // Use simple input (debugging)
static int resize_flag = 0; // Main window being resized ?
static int drawing_or_drawn = 0; // Is image (being) drawn ?

static char const *Xdisplay = "";
static char *Xgeometry = nullptr;

static int unixDisk = 0; // Flag if we use the disk video mode

static int old_fcntl;

static int doesBacking;

/*
 * The pixtab stuff is so we can map from pixel values 0-n to
 * the actual color table entries which may be anything.
 */
static int usepixtab = 0;
static int ipixtab[256];
static unsigned long pixtab[256];
typedef unsigned long XPixel;

static XPixel cmap_pixtab[256]; // for faking a LUTs on non-LUT visuals
static bool cmap_pixtab_alloced = false;
static unsigned long do_fake_lut(int idx)
{
    return g_fake_lut ? cmap_pixtab[idx] : idx;
}
#define FAKE_LUT(idx_) do_fake_lut(idx_)

static int fastmode = 0; // Don't draw pixels 1 at a time
static int alarmon = 0; // 1 if the refresh alarm is on
static int doredraw = 0; // 1 if we have a redraw waiting

// Static routines
static Window FindRootWindow();
static Window pr_dwm_root(Display *dpy, Window pwin);
static int errhand(Display *dp, XErrorEvent *xe);
static int getachar();
static int handleesc();
static int translatekey(int ch);
static int xcmapstuff();
static void xhandleevents();
static void RemoveRootPixmap();
static void doneXwindow();
static void initdacbox();
static void setredrawscreen();
static void clearXwindow();
#ifdef FPUERR
static void continue_hdl(int sig, int code, struct sigcontext *scp,
                         char *addr);
#endif

static int mousefkey[4][4] /* [button][dir] */ = {
    {ID_KEY_RIGHT_ARROW, ID_KEY_LEFT_ARROW, ID_KEY_DOWN_ARROW, ID_KEY_UP_ARROW},
    {0, 0, ID_KEY_PAGE_DOWN, ID_KEY_PAGE_UP},
    {ID_KEY_CTL_PLUS, ID_KEY_CTL_MINUS, ID_KEY_CTL_DEL, ID_KEY_CTL_INSERT},
    {ID_KEY_CTL_END, ID_KEY_CTL_HOME, ID_KEY_CTL_PAGE_DOWN, ID_KEY_CTL_PAGE_UP}
};

/*
 *----------------------------------------------------------------------
 *
 * errhand --
 *
 *  Called on an X server error.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Prints the error message.
 *
 *----------------------------------------------------------------------
 */
static int errhand(Display *dp, XErrorEvent *xe)
{
    char buf[200];
    fflush(stdout);
    std::printf("X Error: %d %d %d %d\n", xe->type, xe->error_code,
           xe->request_code, xe->minor_code);
    XGetErrorText(dp, xe->error_code, buf, 200);
    std::printf("%s\n", buf);
    return 0;
}

#ifdef FPUERR
/*
 *----------------------------------------------------------------------
 *
 * continue_hdl --
 *
 *  Handle an IEEE fpu error.
 *  This routine courtesy of Ulrich Hermes
 *  <hermes@olymp.informatik.uni-dortmund.de>
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Clears flag.
 *
 *----------------------------------------------------------------------
 */
static void
continue_hdl(int sig, int code, struct sigcontext *scp, char *addr)
{
    //      if you want to get all messages enable this statement.
    //  std::printf("ieee exception code %x occurred at pc %X\n",code,scp->sc_pc);
    //  clear all excaption flags
    char out[20];
    ieee_flags("clear", "exception", "all", out);
}
#endif

#define DEFX 640
#define DEFY 480
#define DEFXY "640x480+0+0"

static Display *Xdp = nullptr;
static Window Xw;
static GC Xgc = nullptr;
static Visual *Xvi;
static Screen *Xsc;
static Colormap Xcmap;
static int Xdepth;
static XImage *Ximage =nullptr;
static int Xdscreen;
static Pixmap   Xpixmap = 0;
static int Xwinwidth = DEFX, Xwinheight = DEFY;
static XSizeHints *size_hints = nullptr;
static int gravity;
static Window Xroot;
static int xlastcolor = -1;
static int xlastfcn = GXcopy;
static BYTE *pixbuf = nullptr;

static int step = 0;
static int cyclic[][3] = {
    {1, 3, 5}, {1, 5, 3}, {3, 1, 5}, {3, 5, 1}, {5, 1, 3}, {5, 3, 1},
    {1, 3, 7}, {1, 7, 3}, {3, 1, 7}, {3, 7, 1}, {7, 1, 3}, {7, 3, 1},
    {1, 5, 7}, {1, 7, 5}, {5, 1, 7}, {5, 7, 1}, {7, 1, 5}, {7, 5, 1},
    {3, 5, 7}, {3, 7, 5}, {5, 3, 7}, {5, 7, 3}, {7, 3, 5}, {7, 5, 3}
};

static void
select_visual()
{
    Xvi = XDefaultVisualOfScreen(Xsc);
    Xdepth = DefaultDepth(Xdp, Xdscreen);

    switch (Xvi->c_class)
    {
    case StaticGray:
    case StaticColor:
        g_colors = 1 << Xdepth;
        g_got_real_dac = false;
        g_fake_lut = false;
        g_is_true_color = false;
        break;

    case GrayScale:
    case PseudoColor:
        g_colors = 1 << Xdepth;
        g_got_real_dac = true;
        g_fake_lut = false;
        g_is_true_color = false;
        break;

    case TrueColor:
    case DirectColor:
        g_colors = 256;
        g_got_real_dac = false;
        g_fake_lut = true;
        g_is_true_color = false;
        break;

    default:
        // those should be all the visual classes
        assert(1);
        break;
    }
    if (g_colors > 256)
        g_colors = 256;
}

/*
 *----------------------------------------------------------------------
 *
 * initUnixWindow --
 *
 *  Make the X window.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Makes window.
 *
 *----------------------------------------------------------------------
 */

void
initUnixWindow()
{
    XSetWindowAttributes Xwatt;
    XGCValues Xgcvals;

    if (Xdp != nullptr)
    {
        // We are already initialized
        return;
    }

    if (!simple_input)
    {
        old_fcntl = fcntl(0, F_GETFL);
        fcntl(0, F_SETFL, FNDELAY);
    }

    g_adapter = 0;

    /* We have to do some X stuff even for disk video, to parse the geometry
     * string */

    if (unixDisk)
    {
        int offx, offy;
        fastmode = 0;
        g_fake_lut = false;
        g_is_true_color = false;
        g_got_real_dac = true;
        g_colors = 256;
        for (int i = 0; i < g_colors; i++)
        {
            pixtab[i] = i;
            ipixtab[i] = i;
        }
        if (fixcolors > 0)
        {
            g_colors = fixcolors;
        }
        if (Xgeometry)
        {
            XParseGeometry(Xgeometry, &offx, &offy, (unsigned int *) &Xwinwidth,
                           (unsigned int *) &Xwinheight);
        }
        Xwinwidth &= -4;
        Xwinheight &= -4;
        g_screen_x_dots = Xwinwidth;
        g_screen_y_dots = Xwinheight;
    }
    else
    {  // Use X window
        size_hints = XAllocSizeHints();
        if (size_hints == nullptr)
        {
            std::fprintf(stderr, "Could not allocate memory for X size hints \n");
            std::fprintf(stderr, "Note: id can run without X in -disk mode\n");
            exit(-1);
        }

        if (Xgeometry)
        {
            size_hints->flags = USSize | PBaseSize | PResizeInc;
        }
        else
        {
            size_hints->flags = PSize | PBaseSize | PResizeInc;
        }
        size_hints->base_width = 320;
        size_hints->base_height = 200;
        size_hints->width_inc = 4;
        size_hints->height_inc = 4;

        Xdp = XOpenDisplay(Xdisplay);
        if (Xdp == nullptr)
        {
            std::fprintf(stderr, "Could not open display %s\n", Xdisplay);
            std::fprintf(stderr, "Note: id can run without X in -disk mode\n");
            exit(-1);
        }
        Xdscreen = XDefaultScreen(Xdp);
        if (Xgeometry && !onroot)
        {
            int offx, offy;
            XParseGeometry(Xgeometry, &offx, &offy, (unsigned int *) &Xwinwidth,
                           (unsigned int *) &Xwinheight);
        }
        if (synch)
        {
            XSynchronize(Xdp, True);
        }
        XSetErrorHandler(errhand);
        Xsc = ScreenOfDisplay(Xdp, Xdscreen);
        select_visual();
        if (fixcolors > 0)
        {
            g_colors = fixcolors;
        }

        if (fullscreen || onroot)
        {
            Xwinwidth = DisplayWidth(Xdp, Xdscreen);
            Xwinheight = DisplayHeight(Xdp, Xdscreen);
        }
        g_screen_x_dots = Xwinwidth;
        g_screen_y_dots = Xwinheight;

        Xwatt.background_pixel = BlackPixelOfScreen(Xsc);
        Xwatt.bit_gravity = StaticGravity;
        doesBacking = DoesBackingStore(Xsc);
        if (doesBacking)
        {
            Xwatt.backing_store = Always;
        }
        else
        {
            Xwatt.backing_store = NotUseful;
        }
        if (onroot)
        {
            Xroot = FindRootWindow();
            RemoveRootPixmap();
            Xgc = XCreateGC(Xdp, Xroot, 0, &Xgcvals);
            Xpixmap = XCreatePixmap(Xdp, Xroot, Xwinwidth, Xwinheight, Xdepth);
            Xw = Xroot;
            XFillRectangle(Xdp, Xpixmap, Xgc, 0, 0, Xwinwidth, Xwinheight);
            XSetWindowBackgroundPixmap(Xdp, Xroot, Xpixmap);
        }
        else
        {
            Xroot = DefaultRootWindow(Xdp);
            int Xwinx = 0;
            int Xwiny = 0;
            Xw = XCreateWindow(Xdp, Xroot, Xwinx, Xwiny, Xwinwidth,
                               Xwinheight, 0, Xdepth, InputOutput, CopyFromParent,
                               CWBackPixel | CWBitGravity | CWBackingStore, &Xwatt);
            XStoreName(Xdp, Xw, "Id");
            Xgc = XCreateGC(Xdp, Xw, 0, &Xgcvals);
        }
        g_colors = xcmapstuff();
        if (g_color_cycle_range_hi == 255)
            g_color_cycle_range_hi = g_colors-1;

        XSetWMNormalHints(Xdp, Xw, size_hints);

        if (!onroot)
        {
            XSetBackground(Xdp, Xgc, FAKE_LUT(pixtab[0]));
            XSetForeground(Xdp, Xgc, FAKE_LUT(pixtab[1]));
            Xwatt.background_pixel = FAKE_LUT(pixtab[0]);
            XChangeWindowAttributes(Xdp, Xw, CWBackPixel, &Xwatt);
            XMapWindow(Xdp, Xw);
        }
    }

    writevideopalette();

    x11_video_table[0].xdots = g_screen_x_dots;
    x11_video_table[0].ydots = g_screen_y_dots;
    x11_video_table[0].colors = g_colors;
}
/*
 *----------------------------------------------------------------------
 *
 * doneXwindow --
 *
 *  Clean up the X stuff.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Frees window, etc.
 *
 *----------------------------------------------------------------------
 */
static void
doneXwindow()
{
    if (Xdp == nullptr)
    {
        return;
    }
    if (Xgc)
    {
        XFreeGC(Xdp, Xgc);
    }
    if (Xpixmap)
    {
        XFreePixmap(Xdp, Xpixmap);
        Xpixmap = (Pixmap)nullptr;
    }
    XFlush(Xdp);
    if (size_hints)
    {
        XFree(size_hints);
    }
    /*
    XCloseDisplay(Xdp);
    */
    Xdp = nullptr;
}

/*
 *----------------------------------------------------------------------
 *
 * clearXwindow --
 *
 *  Clears X window.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Clears window.
 *
 *----------------------------------------------------------------------
 */
static void
clearXwindow()
{
    if (g_fake_lut)
    {
        for (int j = 0; j < Ximage->height; j++)
            for (int i = 0; i < Ximage->width; i++)
                XPutPixel(Ximage, i, j, cmap_pixtab[pixtab[0]]);
    }
    else if (pixtab[0] != 0)
    {
        /*
         * Initialize image to pixtab[0].
         */
        if (g_colors == 2)
        {
            for (int i = 0; i < Ximage->bytes_per_line; i++)
            {
                Ximage->data[i] = 0xff;
            }
        }
        else
        {
            for (int i = 0; i < Ximage->bytes_per_line; i++)
            {
                Ximage->data[i] = pixtab[0];
            }
        }
        for (int i = 1; i < Ximage->height; i++)
        {
            std::memcpy(
                Ximage->data + i*Ximage->bytes_per_line,
                Ximage->data,
                Ximage->bytes_per_line);
        }
    }
    else
    {
        /*
         * Initialize image to 0's.
         */
        std::memset(Ximage->data, 0, Ximage->bytes_per_line*Ximage->height);
    }
    xlastcolor = -1;
    XSetForeground(Xdp, Xgc, FAKE_LUT(pixtab[0]));
    if (onroot)
    {
        XFillRectangle(Xdp, Xpixmap, Xgc, 0, 0, Xwinwidth, Xwinheight);
    }
    XFillRectangle(Xdp, Xw, Xgc, 0, 0, Xwinwidth, Xwinheight);
    xsync();
    drawing_or_drawn = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * initdacbox --
 *
 * Put something nice in the dac.
 *
 * The conditions are:
 *  Colors 1 and 2 should be bright so ifs fractals show up.
 *  Color 15 should be bright for lsystem.
 *  Color 1 should be bright for bifurcation.
 *  Colors 1,2,3 should be distinct for periodicity.
 *  The color map should look good for mandelbrot.
 *  The color map should be good if only 128 colors are used.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Loads the dac.
 *
 *----------------------------------------------------------------------
 */
static void
initdacbox()
{
    int s0 = step & 1;
    int sp = step/2;

    if (sp)
    {
        --sp;
        s0 = 1-s0;
        for (int i = 0; i < 256; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                int k = (i*(cyclic[sp][j])) & 127;
                if (k < 64)
                    g_dac_box[i][j] = k;
                else
                    g_dac_box[i][j] = (127 - k);
            }
        }
    }
    else
    {
        for (int i = 0; i < 256; i++)
        {
            g_dac_box[i][0] = (i >> 5)*8+7;
            g_dac_box[i][1] = (((i+16)&28) >> 2)*8+7;
            g_dac_box[i][2] = (((i+2)&3))*16+15;
        }
        g_dac_box[0][2] = 0;
        g_dac_box[0][1] = g_dac_box[0][2];
        g_dac_box[0][0] = g_dac_box[0][1];
        g_dac_box[1][2] = 63;
        g_dac_box[1][1] = g_dac_box[1][2];
        g_dac_box[1][0] = g_dac_box[1][1];
        g_dac_box[2][0] = 47;
        g_dac_box[2][2] = 63;
        g_dac_box[2][1] = g_dac_box[2][2];
    }
    if (s0)
        for (int i = 0; i < 256; i++)
            for (int j = 0; j < 3; j++)
                g_dac_box[i][j] = 63 - g_dac_box[i][j];

}

/*
 *----------------------------------------------------------------------
 *
 * resizeWindow --
 *
 *  Look after resizing the window if necessary.
 *
 * Results:
 *  Returns 1 for resize, 0 for no resize.
 *
 * Side effects:
 *  May reallocate data structures.
 *
 *----------------------------------------------------------------------
 */
int
resizeWindow()
{
    static int oldx = -1, oldy = -1;
    int junki;
    unsigned int junkui;
    Window junkw;
    unsigned int width, height;

    if (unixDisk)
        return 0;
    if (resize_flag)
    {
        Window root, parent, *children;
        resize_flag = 0;
        XQueryTree(Xdp, Xw, &root, &parent, &children, &junkui);
        if (!parent)
            return 0;
        XGetGeometry(Xdp, parent, &root, &junki, &junki,
                     &width, &height, &junkui, &junkui);
        XResizeWindow(Xdp, Xw, width, height);
        XSync(Xdp, False);
        usleep(100000);
    }
    else
        XGetGeometry(Xdp, Xw, &junkw, &junki, &junki, &width, &height,
                     &junkui, &junkui);

    if (oldx != width || oldy != height)
    {
        g_screen_x_dots = width & -4;
        g_screen_y_dots = height & -4;
        x11_video_table[0].xdots = g_screen_x_dots;
        x11_video_table[0].ydots = g_screen_y_dots;
        oldx = g_screen_x_dots;
        oldy = g_screen_y_dots;
        Xwinwidth = g_screen_x_dots;
        Xwinheight = g_screen_y_dots;
        g_screen_aspect = g_screen_y_dots/(float)g_screen_x_dots;
        g_final_aspect_ratio = g_screen_aspect;
        int Xpad = 8;  // default, unless changed below
        int Xmwidth;
        if (Xdepth == 1)
            Xmwidth = 1 + g_screen_x_dots/8;
        else if (Xdepth <= 8)
            Xmwidth = g_screen_x_dots;
        else if (Xdepth <= 16)
        {  // 15 or 16 bpp
            Xmwidth = 2*g_screen_x_dots;
            Xpad = 16;
        }
        else
        {  // 24 or 32 bpp
            Xmwidth = 4*g_screen_x_dots;
            Xpad = 32;
        }
        if (pixbuf != nullptr)
        {
            free(pixbuf);
        }
        pixbuf = (BYTE *) malloc(Xwinwidth *sizeof(BYTE));
        if (Ximage != nullptr)
            XDestroyImage(Ximage);
        Ximage = XCreateImage(Xdp, Xvi, Xdepth, ZPixmap, 0, nullptr, g_screen_x_dots,
                              g_screen_y_dots, Xpad, Xmwidth);
        if (Ximage == nullptr)
        {
            std::fprintf(stderr, "XCreateImage failed\n");
            exit(-1);
        }
        Ximage->data = static_cast<char *>(malloc(Ximage->bytes_per_line * Ximage->height));
        if (Ximage->data == nullptr)
        {
            std::fprintf(stderr, "Malloc failed: %d\n", Ximage->bytes_per_line *
                    Ximage->height);
            exit(-1);
        }
        clearXwindow();
        return 1;
    }
    else
    {
        return 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * xcmapstuff --
 *
 *  Set up the colormap appropriately
 *
 * Results:
 *  Number of colors.
 *
 * Side effects:
 *  Sets colormap.
 *
 *----------------------------------------------------------------------
 */
static int
xcmapstuff()
{
    int ncells;

    if (onroot)
    {
        privatecolor = 0;
    }
    for (int i = 0; i < g_colors; i++)
    {
        pixtab[i] = i;
        ipixtab[i] = 999;
    }
    if (!g_got_real_dac)
    {
        Xcmap = DefaultColormapOfScreen(Xsc);
        if (g_fake_lut)
            writevideopalette();
    }
    else if (sharecolor)
    {
        g_got_real_dac = false;
    }
    else if (privatecolor)
    {
        Xcmap = XCreateColormap(Xdp, Xw, Xvi, AllocAll);
        XSetWindowColormap(Xdp, Xw, Xcmap);
    }
    else
    {
        Xcmap = DefaultColormap(Xdp, Xdscreen);
        for (int powr = Xdepth; powr >= 1; powr--)
        {
            ncells = 1 << powr;
            if (ncells > g_colors)
                continue;
            if (XAllocColorCells(Xdp, Xcmap, False, nullptr, 0, pixtab,
                                 (unsigned int) ncells))
            {
                g_colors = ncells;
                std::fprintf(stderr, "%d colors\n", g_colors);
                usepixtab = 1;
                break;
            }
        }
        if (!usepixtab)
        {
            std::fprintf(stderr, "Couldn't allocate any colors\n");
            g_got_real_dac = false;
        }
    }
    for (int i = 0; i < g_colors; i++)
    {
        ipixtab[pixtab[i]] = i;
    }
    /* We must make sure if any color uses position 0, that it is 0.
     * This is so we can clear the image with std::memset.
     * So, suppose id 0 = cmap 42, cmap 0 = fractint 55.
     * Then want id 0 = cmap 0, cmap 42 = fractint 55.
     * I.e. pixtab[55] = 42, ipixtab[42] = 55.
     */
    if (ipixtab[0] == 999)
    {
        ipixtab[0] = 0;
    }
    else if (ipixtab[0] != 0)
    {
        int other;
        other = ipixtab[0];
        pixtab[other] = pixtab[0];
        ipixtab[pixtab[other]] = other;
        pixtab[0] = 0;
        ipixtab[0] = 0;
    }

    if (!g_got_real_dac && g_colors == 2 && BlackPixelOfScreen(Xsc) != 0)
    {
        ipixtab[0] = 1;
        pixtab[0] = ipixtab[0];
        ipixtab[1] = 0;
        pixtab[1] = ipixtab[1];
        usepixtab = 1;
    }

    return g_colors;
}

XColor cols[256];

/*
 *----------------------------------------------------------------------
 *
 * readvideopalette --
 *  Reads the current video palette into g_dac_box.
 *
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Fills in g_dac_box.
 *
 *----------------------------------------------------------------------
 */
int readvideopalette()
{
    if (!g_got_real_dac && g_is_true_color && g_true_mode != true_color_mode::default_color)
        return -1;
    for (int i = 0; i < g_colors; i++)
    {
        g_dac_box[i][0] = cols[i].red/1024;
        g_dac_box[i][1] = cols[i].green/1024;
        g_dac_box[i][2] = cols[i].blue/1024;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * writevideopalette --
 *  Writes g_dac_box into the video palette.
 *
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Changes the displayed colors.
 *
 *----------------------------------------------------------------------
 */
int writevideopalette()
{
    if (!g_got_real_dac)
    {
        if (g_fake_lut)
        {
            // !g_got_real_dac, g_fake_lut => truecolor, directcolor displays
            static unsigned char last_dac[256][3];
            static bool last_dac_inited = false;

            for (int i = 0; i < g_colors; i++)
            {
                if (!last_dac_inited ||
                        last_dac[i][0] != g_dac_box[i][0] ||
                        last_dac[i][1] != g_dac_box[i][1] ||
                        last_dac[i][2] != g_dac_box[i][2])
                {
                    cols[i].flags = DoRed | DoGreen | DoBlue;
                    cols[i].red = g_dac_box[i][0]*1024;
                    cols[i].green = g_dac_box[i][1]*1024;
                    cols[i].blue = g_dac_box[i][2]*1024;

                    if (XAllocColor(Xdp, Xcmap, &cols[i]))
                    {
                        cmap_pixtab[i] = cols[i].pixel;
                    }
                    else
                    {
                        assert(1);
                        std::fprintf(stderr, "Allocating color %d failed.\n", i);
                    }

                    last_dac[i][0] = g_dac_box[i][0];
                    last_dac[i][1] = g_dac_box[i][1];
                    last_dac[i][2] = g_dac_box[i][2];
                }
            }
            cmap_pixtab_alloced = true;
            last_dac_inited = true;
        }
        else
        {
            // !g_got_real_dac, !g_fake_lut => static color, static gray displays
            assert(1);
        }
    }
    else
    {
        // g_got_real_dac => grayscale or pseudocolor displays
        for (int i = 0; i < g_colors; i++)
        {
            cols[i].pixel = pixtab[i];
            cols[i].flags = DoRed | DoGreen | DoBlue;
            cols[i].red = g_dac_box[i][0]*1024;
            cols[i].green = g_dac_box[i][1]*1024;
            cols[i].blue = g_dac_box[i][2]*1024;
        }
        if (!unixDisk)
        {
            XStoreColors(Xdp, Xcmap, cols, g_colors);
            XFlush(Xdp);

            /* None of these changed the colors without redrawing the fractal
            XSetWindowColormap(Xdp, Xw, Xcmap);
            XDrawRectangle(Xdp, Xw, Xgc, 0, 0, Xwinwidth, Xwinheight);
            XPutImage(Xdp,Xw,Xgc,Ximage,0,0,0,0,Xwinwidth,Xwinheight);
            XCopyArea(Xdp,Xw,Xw,Xgc,0,0,Xwinwidth,Xwinheight,0,0);
            XFlush(Xdp);
            */
        }
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * xsync --
 *
 *  Sync the x server
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Modifies window.
 *
 *----------------------------------------------------------------------
 */
void
xsync()
{
    if (!unixDisk)
    {
        XSync(Xdp, False);
    }
}
/*
 *----------------------------------------------------------------------
 *
 * getachar --
 *
 *  Gets a character.
 *
 * Results:
 *  Key.
 *
 * Side effects:
 *  Reads key.
 *
 *----------------------------------------------------------------------
 */
static int
getachar()
{
    if (simple_input)
    {
        return getchar();
    }
    else
    {
        char ch;
        int status;
        status = read(0, &ch, 1);
        if (status < 0)
        {
            return -1;
        }
        else
        {
            return ch;
        }
    }
}

static int xbufkey = 0;     // Buffered X key

/*
 *----------------------------------------------------------------------
 *
 * translatekey --
 *
 *  Translate an input key into MSDOS format.  The purpose of this
 *  routine is to do the mappings like U -> ID_KEY_PAGE_UP.
 *
 * Results:
 *  New character;
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static int
translatekey(int ch)
{
    if (ch >= 'a' && ch <= 'z')
    {
        return ch;
    }
    else
    {
        switch (ch)
        {
        case 'I':
            return ID_KEY_INSERT;
        case 'D':
            return ID_KEY_DELETE;
        case 'U':
            return ID_KEY_PAGE_UP;
        case 'N':
            return ID_KEY_PAGE_DOWN;
        case ctl('O'):
            return ID_KEY_CTL_HOME;
        case 'H':
            return ID_KEY_LEFT_ARROW;
        case 'L':
            return ID_KEY_RIGHT_ARROW;
        case 'K':
            return ID_KEY_UP_ARROW;
        case 'J':
            return ID_KEY_DOWN_ARROW;
        case 1115:
            return ID_KEY_CTL_LEFT_ARROW;
        case 1116:
            return ID_KEY_CTL_RIGHT_ARROW;
        case 1141:
            return ID_KEY_CTL_UP_ARROW;
        case 1145:
            return ID_KEY_CTL_DOWN_ARROW;
        case 'O':
            return ID_KEY_HOME;
        case 'E':
            return ID_KEY_END;
        case '\n':
            return ID_KEY_ENTER;
        case ctl('T'):
            return ID_KEY_CTL_ENTER;
        case -2:
            return ID_KEY_CTL_ENTER_2;
        case ctl('U'):
            return ID_KEY_CTL_PAGE_UP;
        case ctl('N'):
            return ID_KEY_CTL_PAGE_DOWN;
        case '{':
            return ID_KEY_CTL_MINUS;
        case '}':
            return ID_KEY_CTL_PLUS;
            // we need ^I for tab
        case ctl('D'):
            return ID_KEY_CTL_DEL;
        case '!':
            return ID_KEY_F1;
        case '@':
            return ID_KEY_F2;
        case '$':
            return ID_KEY_F4;
        case '%':
            return ID_KEY_F5;
        case '^':
            return ID_KEY_F6;
        case '&':
            return ID_KEY_F7;
        case '*':
            return ID_KEY_F8;
        case '(':
            return ID_KEY_F9;
        case ')':
            return ID_KEY_F10;
        default:
            return ch;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * handleesc --
 *
 *  Handle an escape key.  This may be an escape key sequence
 *  indicating a function key was pressed.
 *
 * Results:
 *  Key.
 *
 * Side effects:
 *  Reads keys.
 *
 *----------------------------------------------------------------------
 */
static int
handleesc()
{
    if (simple_input)
    {
        return ID_KEY_ESC;
    }
    // SUN escape key sequences
    int ch1 = getachar();
    if (ch1 == -1)
    {
        driver_delay(250); // Wait 1/4 sec to see if a control sequence follows
        ch1 = getachar();
    }
    if (ch1 != '[')
    {       // See if we have esc [
        return ID_KEY_ESC;
    }
    ch1 = getachar();
    if (ch1 == -1)
    {
        driver_delay(250); // Wait 1/4 sec to see if a control sequence follows
        ch1 = getachar();
    }
    if (ch1 == -1)
    {
        return ID_KEY_ESC;
    }
    switch (ch1)
    {
    case 'A':       // esc [ A
        return ID_KEY_UP_ARROW;
    case 'B':       // esc [ B
        return ID_KEY_DOWN_ARROW;
    case 'C':       // esc [ C
        return ID_KEY_RIGHT_ARROW;
    case 'D':       // esc [ D
        return ID_KEY_LEFT_ARROW;
    default:
        break;
    }
    int ch2 = getachar();
    if (ch2 == -1)
    {
        driver_delay(250); // Wait 1/4 sec to see if a control sequence follows
        ch2 = getachar();
    }
    if (ch2 == '~')
    {       // esc [ ch1 ~
        switch (ch1)
        {
        case '2':       // esc [ 2 ~
            return ID_KEY_INSERT;
        case '3':       // esc [ 3 ~
            return ID_KEY_DELETE;
        case '5':       // esc [ 5 ~
            return ID_KEY_PAGE_UP;
        case '6':       // esc [ 6 ~
            return ID_KEY_PAGE_DOWN;
        default:
            return ID_KEY_ESC;
        }
    }
    else if (ch2 == -1)
    {
        return ID_KEY_ESC;
    }
    else
    {
        int ch3 = getachar();
        if (ch3 == -1)
        {
            driver_delay(250); // Wait 1/4 sec to see if a control sequence follows
            ch3 = getachar();
        }
        if (ch3 != '~')
        {   // esc [ ch1 ch2 ~
            return ID_KEY_ESC;
        }
        if (ch1 == '1')
        {
            switch (ch2)
            {
            case '1':   // esc [ 1 1 ~
                return ID_KEY_F1;
            case '2':   // esc [ 1 2 ~
                return ID_KEY_F2;
            case '3':   // esc [ 1 3 ~
                return ID_KEY_F3;
            case '4':   // esc [ 1 4 ~
                return ID_KEY_F4;
            case '5':   // esc [ 1 5 ~
                return ID_KEY_F5;
            case '6':   // esc [ 1 6 ~
                return ID_KEY_F6;
            case '7':   // esc [ 1 7 ~
                return ID_KEY_F7;
            case '8':   // esc [ 1 8 ~
                return ID_KEY_F8;
            case '9':   // esc [ 1 9 ~
                return ID_KEY_F9;
            default:
                return ID_KEY_ESC;
            }
        }
        else if (ch1 == '2')
        {
            switch (ch2)
            {
            case '0':   // esc [ 2 0 ~
                return ID_KEY_F10;
            case '8':   // esc [ 2 8 ~
                return ID_KEY_F1;  // HELP
            default:
                return ID_KEY_ESC;
            }
        }
        else
        {
            return ID_KEY_ESC;
        }
    }
}

bool g_x_zoom_waiting = false;

#define SENS 1
#define ABS(x) ((x) > 0?(x):-(x))
#define MIN(x, y) ((x) < (y)?(x):(y))
#define SIGN(x) ((x) > 0?1:-1)

/*
 *----------------------------------------------------------------------
 *
 * xhandleevents --
 *
 *  Handles X events.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Does event action.
 *
 *----------------------------------------------------------------------
 */
static void
xhandleevents()
{
    XEvent xevent;
    int drawn;
    static int ctl_mode = 0;
    static int shift_mode = 0;
    int bandx0, bandy0, bandx1, bandy1;
    static int bnum = 0;
    static int lastx, lasty;
    static int dx, dy;

    if (doredraw)
    {
        redrawscreen();
    }

    while (XPending(Xdp) && !xbufkey)
    {
        XNextEvent(Xdp, &xevent);

        switch (((XAnyEvent *)&xevent)->type)
        {
        case KeyRelease:
        {
            char buffer[1];
            KeySym keysym;
            XLookupString(&xevent.xkey, buffer, 1, &keysym, nullptr);
            switch (keysym)
            {
            case XK_Control_L:
            case XK_Control_R:
                ctl_mode = 0;
                break;
            case XK_Shift_L:
            case XK_Shift_R:
                shift_mode = 0;
                break;
            }
        }
        break;
        case KeyPress:
        {
            int charcount;
            char buffer[1];
            KeySym keysym;
            charcount = XLookupString(&xevent.xkey, buffer, 1, &keysym, nullptr);
            switch (keysym)
            {
            case XK_Control_L:
            case XK_Control_R:
                ctl_mode = 1;
                return;
            case XK_Shift_L:
            case XK_Shift_R:
                shift_mode = 1;
                break;
            case XK_Home:
            case XK_R7:
                xbufkey = ctl_mode ? ID_KEY_CTL_HOME : ID_KEY_HOME;
                return;
            case XK_Left:
            case XK_R10:
                xbufkey = ctl_mode ? ID_KEY_CTL_LEFT_ARROW : ID_KEY_LEFT_ARROW;
                return;
            case XK_Right:
            case XK_R12:
                xbufkey = ctl_mode ? ID_KEY_CTL_RIGHT_ARROW : ID_KEY_RIGHT_ARROW;
                return;
            case XK_Down:
            case XK_R14:
                xbufkey = ctl_mode ? ID_KEY_CTL_DOWN_ARROW : ID_KEY_DOWN_ARROW;
                return;
            case XK_Up:
            case XK_R8:
                xbufkey = ctl_mode ? ID_KEY_CTL_UP_ARROW : ID_KEY_UP_ARROW;
                return;
            case XK_Insert:
                xbufkey = ctl_mode ? ID_KEY_CTL_INSERT : ID_KEY_INSERT;
                return;
            case XK_Delete:
                xbufkey = ctl_mode ? ID_KEY_CTL_DEL : ID_KEY_DELETE;
                return;
            case XK_End:
            case XK_R13:
                xbufkey = ctl_mode ? ID_KEY_CTL_END : ID_KEY_END;
                return;
            case XK_Help:
                xbufkey = ID_KEY_F1;
                return;
            case XK_Prior:
            case XK_R9:
                xbufkey = ctl_mode ? ID_KEY_CTL_PAGE_UP : ID_KEY_PAGE_UP;
                return;
            case XK_Next:
            case XK_R15:
                xbufkey = ctl_mode ? ID_KEY_CTL_PAGE_DOWN : ID_KEY_PAGE_DOWN;
                return;
            case XK_F1:
            case XK_L1:
                xbufkey = shift_mode ? ID_KEY_SF1 : ID_KEY_F1;
                return;
            case XK_F2:
            case XK_L2:
                xbufkey = shift_mode ? ID_KEY_SF2: ID_KEY_F2;
                return;
            case XK_F3:
            case XK_L3:
                xbufkey = shift_mode ? ID_KEY_SF3: ID_KEY_F3;
                return;
            case XK_F4:
            case XK_L4:
                xbufkey = shift_mode ? ID_KEY_SF4: ID_KEY_F4;
                return;
            case XK_F5:
            case XK_L5:
                xbufkey = shift_mode ? ID_KEY_SF5: ID_KEY_F5;
                return;
            case XK_F6:
            case XK_L6:
                xbufkey = shift_mode ? ID_KEY_SF6: ID_KEY_F6;
                return;
            case XK_F7:
            case XK_L7:
                xbufkey = shift_mode ? ID_KEY_SF7: ID_KEY_F7;
                return;
            case XK_F8:
            case XK_L8:
                xbufkey = shift_mode ? ID_KEY_SF8: ID_KEY_F8;
                return;
            case XK_F9:
            case XK_L9:
                xbufkey = shift_mode ? ID_KEY_SF9: ID_KEY_F9;
                return;
            case XK_F10:
            case XK_L10:
                xbufkey = shift_mode ? ID_KEY_SF10: ID_KEY_F10;
                return;
            case '+':
                xbufkey = ctl_mode ? ID_KEY_CTL_PLUS : '+';
                return;
            case '-':
                xbufkey = ctl_mode ? ID_KEY_CTL_MINUS : '-';
                return;
                break;
            case XK_Return:
            case XK_KP_Enter:
                xbufkey = ctl_mode ? ctl('T') : '\n';
                return;
            }
            if (charcount == 1)
            {
                xbufkey = buffer[0];
                if (xbufkey == '\003')
                {
                    goodbye();
                }
            }
        }
        break;
        case MotionNotify:
        {
            if (g_editpal_cursor && !g_inside_help)
            {
                while (XCheckWindowEvent(Xdp, Xw, PointerMotionMask,
                                         &xevent))
                {
                }

                if (xevent.xmotion.state&Button2Mask ||
                        (xevent.xmotion.state&Button1Mask &&
                         xevent.xmotion.state&Button3Mask))
                {
                    bnum = 3;
                }
                else if (xevent.xmotion.state&Button1Mask)
                {
                    bnum = 1;
                }
                else if (xevent.xmotion.state&Button3Mask)
                {
                    bnum = 2;
                }
                else
                {
                    bnum = 0;
                }

#define MSCALE 1

                if (g_look_at_mouse == +MouseLook::POSITION && bnum != 0)
                {
                    dx += (xevent.xmotion.x-lastx)/MSCALE;
                    dy += (xevent.xmotion.y-lasty)/MSCALE;
                    lastx = xevent.xmotion.x;
                    lasty = xevent.xmotion.y;
                }
                else
                {
                    Cursor_SetPos(xevent.xmotion.x, xevent.xmotion.y);
                    xbufkey = ID_KEY_ENTER;
                }

            }
        }
        break;
        case ButtonPress:
        {
            int done = 0;
            int banding = 0;
            if (g_look_at_mouse == +MouseLook::POSITION || !g_zoom_off)
            {
                lastx = xevent.xbutton.x;
                lasty = xevent.xbutton.y;
                break;
            }
            bandx0 = xevent.xbutton.x;
            bandx1 = bandx0;
            bandy0 = xevent.xbutton.y;
            bandy1 = bandy0;
            while (!done)
            {
                XNextEvent(Xdp, &xevent);
                switch (xevent.type)
                {
                case MotionNotify:
                    while (XCheckWindowEvent(Xdp, Xw, PointerMotionMask,
                                             &xevent))
                    {
                    }
                    if (banding)
                    {
                        XDrawRectangle(Xdp, Xw, Xgc, MIN(bandx0, bandx1),
                                       MIN(bandy0, bandy1), ABS(bandx1-bandx0),
                                       ABS(bandy1-bandy0));
                    }
                    bandx1 = xevent.xmotion.x;
                    bandy1 = xevent.xmotion.y;
                    if (ABS(bandx1-bandx0)*g_final_aspect_ratio >
                            ABS(bandy1-bandy0))
                    {
                        bandy1 = SIGN(bandy1-bandy0)*ABS(bandx1-bandx0)*
                                 g_final_aspect_ratio + bandy0;
                    }
                    else
                    {
                        bandx1 = SIGN(bandx1-bandx0)*ABS(bandy1-bandy0)/
                                 g_final_aspect_ratio + bandx0;
                    }
                    if (!banding)
                    {
                        /* Don't start rubber-banding until the mouse
                           gets moved.  Otherwise a click messes up the
                           window */
                        if (ABS(bandx1-bandx0) > 10 ||
                                ABS(bandy1-bandy0) > 10)
                        {
                            banding = 1;
                            XSetForeground(Xdp, Xgc, g_colors-1);
                            XSetFunction(Xdp, Xgc, GXxor);
                        }
                    }
                    if (banding)
                    {
                        XDrawRectangle(Xdp, Xw, Xgc, MIN(bandx0, bandx1),
                                       MIN(bandy0, bandy1), ABS(bandx1-bandx0),
                                       ABS(bandy1-bandy0));
                    }
                    XFlush(Xdp);
                    break;
                case ButtonRelease:
                    done = 1;
                    break;
                }
            }
            if (!banding)
            {
                break;
            }
            XDrawRectangle(Xdp, Xw, Xgc, MIN(bandx0, bandx1),
                           MIN(bandy0, bandy1), ABS(bandx1-bandx0),
                           ABS(bandy1-bandy0));
            if (bandx1 == bandx0)
            {
                bandx1 = bandx0+1;
            }
            if (bandy1 == bandy0)
            {
                bandy1 = bandy0+1;
            }
            g_zoom_box_rotation = 0;
            g_zoom_box_skew = 0;
            g_zoom_box_x = (MIN(bandx0, bandx1)-g_logical_screen_x_offset)/g_logical_screen_x_size_dots;
            g_zoom_box_y = (MIN(bandy0, bandy1)-g_logical_screen_y_offset)/g_logical_screen_y_size_dots;
            g_zoom_box_width = ABS(bandx1-bandx0)/g_logical_screen_x_size_dots;
            g_zoom_box_height = g_zoom_box_width;
            if (!g_inside_help)
            {
                xbufkey = ID_KEY_ENTER;
            }
            if (xlastcolor != -1)
            {
                XSetForeground(Xdp, Xgc, xlastcolor);
            }
            XSetFunction(Xdp, Xgc, xlastfcn);
            g_x_zoom_waiting = true;
            drawbox(0);
        }
        break;
        case ConfigureNotify:
            XSelectInput(Xdp, Xw, KeyPressMask|KeyReleaseMask|ExposureMask|
                         ButtonPressMask|ButtonReleaseMask|PointerMotionMask);
            resize_flag = 1;
            drawn = drawing_or_drawn;
            if (resizeWindow())
            {
                if (drawn)
                {
                    xbufkey = 'D';
                    return;
                }
            }
            XSelectInput(Xdp, Xw, KeyPressMask|KeyReleaseMask|ExposureMask|
                         ButtonPressMask|ButtonReleaseMask|
                         PointerMotionMask|StructureNotifyMask);
            break;
        case Expose:
            if (!doesBacking)
            {
                int x, y, w, h;
                x = xevent.xexpose.x;
                y = xevent.xexpose.y;
                w = xevent.xexpose.width;
                h = xevent.xexpose.height;
                if (x+w > g_screen_x_dots)
                {
                    w = g_screen_x_dots-x;
                }
                if (y+h > g_screen_y_dots)
                {
                    h = g_screen_y_dots-y;
                }
                if (x < g_screen_x_dots && y < g_screen_y_dots && w > 0 && h > 0)
                {

                    XPutImage(Xdp, Xw, Xgc, Ximage, xevent.xexpose.x,
                              xevent.xexpose.y, xevent.xexpose.x,
                              xevent.xexpose.y, xevent.xexpose.width,
                              xevent.xexpose.height);
                }
            }
            break;
        default:
            /* All events selected by StructureNotifyMask
            * except ConfigureNotify are thrown away here,
            * since nothing is done with them */
            break;
        }  // End switch
    }  // End while

    if (!xbufkey && g_editpal_cursor && !g_inside_help && g_look_at_mouse == +MouseLook::POSITION &&
            (dx != 0 || dy != 0))
    {
        if (ABS(dx) > ABS(dy))
        {
            if (dx > 0)
            {
                xbufkey = mousefkey[bnum][0]; // right
                dx--;
            }
            else if (dx < 0)
            {
                xbufkey = mousefkey[bnum][1]; // left
                dx++;
            }
        }
        else
        {
            if (dy > 0)
            {
                xbufkey = mousefkey[bnum][2]; // down
                dy--;
            }
            else if (dy < 0)
            {
                xbufkey = mousefkey[bnum][3]; // up
                dy++;
            }
        }
    }

}

#define w_root Xroot
#define dpy Xdp
#define scr Xdscreen
/*
 *----------------------------------------------------------------------
 *
 * pr_dwm_root --
 *
 *  Search for a dec window manager root window.
 *
 * Results:
 *  Returns the root window.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static Window pr_dwm_root(Display *dpy, Window pwin)
{
    // search for DEC Window Manager root
    XWindowAttributes pxwa, cxwa;
    Window  root, parent, *child;

    if (!XGetWindowAttributes(dpy, pwin, &pxwa))
    {
        std::printf("Search for root: XGetWindowAttributes failed\n");
        return RootWindow(dpy, scr);
    }
    unsigned int nchild;
    if (XQueryTree(dpy, pwin, &root, &parent, &child, &nchild))
    {
        for (unsigned int i = 0U; i < nchild; i++)
        {
            if (!XGetWindowAttributes(dpy, child[i], &cxwa))
            {
                std::printf("Search for root: XGetWindowAttributes failed\n");
                return RootWindow(dpy, scr);
            }
            if (pxwa.width == cxwa.width && pxwa.height == cxwa.height)
            {
                return pr_dwm_root(dpy, child[i]);
            }
        }
        return pwin;
    }
    else
    {
        std::printf("id: failed to find root window\n");
        return RootWindow(dpy, scr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FindRootWindow --
 *
 *  Find the root or virtual root window.
 *
 * Results:
 *  Returns the root window.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static Window
FindRootWindow()
{
    w_root = RootWindow(dpy, scr);
    w_root = pr_dwm_root(dpy, w_root); // search for DEC wm root

    {   // search for swm/tvtwm root (from ssetroot by Tom LaStrange)
        Window rootReturn, parentReturn, *children;
        unsigned int numChildren;
        Atom SWM_VROOT = XInternAtom(dpy, "__SWM_VROOT", False);
        XQueryTree(dpy, w_root, &rootReturn, &parentReturn, &children, &numChildren);
        for (int i = 0; i < numChildren; i++)
        {
            Atom actual_type;
            int actual_format;
            unsigned long nitems, bytesafter;
            Window *newRoot = nullptr;
            if (XGetWindowProperty(dpy, children[i], SWM_VROOT, 0L, 1L,
                                   False, XA_WINDOW, &actual_type, &actual_format, &nitems, &bytesafter,
                                   (unsigned char **) &newRoot) == Success && newRoot)
            {
                w_root = *newRoot;
                break;
            }
        }
    }
    return w_root;
}

/*
 *----------------------------------------------------------------------
 *
 * RemoveRootPixmap --
 *
 *  Clean up old pixmap on the root window.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Pixmap is cleaned up.
 *
 *----------------------------------------------------------------------
 */
static void
RemoveRootPixmap()
{
    Atom prop, type;
    int format;
    unsigned long nitems, after;
    Pixmap *pm;

    prop = XInternAtom(Xdp, "_XSETROOT_ID", False);
    if (XGetWindowProperty(Xdp, Xroot, prop, 0L, 1L, 1, AnyPropertyType,
                           &type, &format, &nitems, &after, (unsigned char **)&pm) ==
            Success && nitems == 1)
    {
        if (type == XA_PIXMAP && format == 32 && after == 0)
        {
            XKillClient(Xdp, (XID)*pm);
            XFree((char *)pm);
        }
    }
}
static unsigned char *fontPtr = nullptr;
/*
 *----------------------------------------------------------------------
 *
 * xgetfont --
 *
 *  Get an 8x8 font.
 *
 * Results:
 *  Returns a pointer to the bits.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
unsigned char *
xgetfont()
{
    XFontStruct *font_info;
    XImage *font_image;
    char str[8];
    int width;
    Pixmap font_pixmap;
    XGCValues values;
    GC font_gc;

    fontPtr = (unsigned char *)malloc(128*8);
    std::memset(fontPtr, 0, 128*8);

    xlastcolor = -1;
#define FONT "-*-*-medium-r-*-*-9-*-*-*-*-*-iso8859-*"
    font_info = XLoadQueryFont(Xdp, FONT);
    if (font_info == nullptr)
    {
        std::printf("No %s\n", FONT);
    }
    if (font_info == nullptr || font_info->max_bounds.width > 8 ||
            font_info->max_bounds.width != font_info->min_bounds.width)
    {
        std::printf("Bad font: %s\n", FONT);
        sleep(2);
        font_info = XLoadQueryFont(Xdp, "6x12");
    }
    if (font_info == nullptr)
        return nullptr;
    width = font_info->max_bounds.width;
    if (font_info->max_bounds.width > 8 ||
            font_info->max_bounds.width != font_info->min_bounds.width)
    {
        std::printf("Bad font\n");
        return nullptr;
    }

    font_pixmap = XCreatePixmap(Xdp, Xw, 64, 8, Xdepth);
    assert(font_pixmap);
    values.background = 0;
    values.foreground = 1;
    values.font = font_info->fid;
    font_gc = XCreateGC(Xdp, font_pixmap,
                        GCForeground | GCBackground | GCFont, &values);
    assert(font_gc);

    for (int i = 0; i < 128; i += 8)
    {
        for (int j = 0; j < 8; j++)
        {
            str[j] = i+j;
        }

        XDrawImageString(Xdp, font_pixmap, Xgc, 0, 8, str, 8);

        font_image = XGetImage(Xdp, font_pixmap, 0, 0, 64, 8, AllPlanes, XYPixmap);
        assert(font_image);
        for (int j = 0; j < 8; j++)
        {
            for (int k = 0; k < 8; k++)
            {
                for (int l = 0; l < width; l++)
                {
                    if (XGetPixel(font_image, j*width+l, k))
                    {
                        fontPtr[(i+j)*8+k] = (fontPtr[(i+j)*8+k] << 1) | 1;
                    }
                    else
                    {
                        fontPtr[(i+j)*8+k] = (fontPtr[(i+j)*8+k] << 1);
                    }
                }
            }
        }
        XDestroyImage(font_image);
    }

    XFreeGC(Xdp, font_gc);
    XFreePixmap(Xdp, font_pixmap);

    return fontPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * setredrawscreen --
 *
 *  Set the screen refresh flag
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Sets the flag.
 *
 *----------------------------------------------------------------------
 */
static void
setredrawscreen()
{
    doredraw = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * redrawscreen --
 *
 *  Refresh the screen.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Redraws the screen.
 *
 *----------------------------------------------------------------------
 */
void
redrawscreen()
{
    if (alarmon)
    {
        XPutImage(Xdp, Xw, Xgc, Ximage, 0, 0, 0, 0, g_screen_x_dots, g_screen_y_dots);
        if (onroot)
        {
            XPutImage(Xdp, Xpixmap, Xgc, Ximage, 0, 0, 0, 0, g_screen_x_dots, g_screen_y_dots);
        }
        alarmon = 0;
    }
    doredraw = 0;
    if (!unixDisk)
        XSelectInput(Xdp, Xw, KeyPressMask|KeyReleaseMask|ExposureMask|
                     ButtonPressMask|ButtonReleaseMask|
                     PointerMotionMask|StructureNotifyMask);
}
