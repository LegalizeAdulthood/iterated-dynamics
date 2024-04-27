/* Unixscr.c
 * This file contains routines for the Unix port.
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
#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "diskvid.h"
#include "drivers.h"
#include "editpal.h"
#include "fixed_pt.h"
#include "fractals.h"
#include "goodbye.h"
#include "helpdefs.h"
#include "id_data.h"
#include "id_keys.h"
#include "os.h"
#include "read_ticker.h"
#include "slideshw.h"
#include "text_screen.h"
#include "video_mode.h"
#include "zoom.h"

#include <sys/ioctl.h>
#include <sys/time.h>
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
#include <string>
#include <vector>

#include "x11_frame.h"
#include "x11_text.h"
#include "x11_plot.h"

#ifdef LINUX
#define FNDELAY O_NDELAY
#endif

// external variables (set in the id.cfg file, but findable here

extern bool slowdisplay;
extern  int g_dot_mode;        // video access method (= 19)
extern  int g_screen_x_dots, g_screen_y_dots;     // total # of dots on the screen
extern  int g_logical_screen_x_offset, g_logical_screen_y_offset;     // offset of drawing area
extern  int g_colors;         // maximum colors available
extern  int g_init_mode;
extern  int g_adapter;
extern bool g_got_real_dac;
extern bool g_inside_help;
extern float g_final_aspect_ratio;
extern  float   g_screen_aspect;
extern VIDEOINFO x11_video_table[];

// the video-palette array (named after the VGA adapter's video-DAC)

extern unsigned char g_dac_box[256][3];

extern int g_color_cycle_range_hi;

typedef unsigned long XPixel;

constexpr inline int ctl(int code)
{
    return code & 0x1f;
}

enum
{
    TEXT_WIDTH = 80,
    TEXT_HEIGHT = 25,
    MOUSE_SCALE = 1
};

constexpr const char *const FONT{"-*-*-medium-r-*-*-9-*-*-*-c-*-iso8859-*"};
constexpr const int DEFX{640};
constexpr const int DEFY{480};
constexpr const char *const DEFXY{"640x480+0+0"};

// The pixtab stuff is so we can map from pixel values 0-n to
// the actual color table entries which may be anything.
//
class X11Driver : public Driver
{
public:
    ~X11Driver() override = default;

    bool init(int *argc, char **argv) override;
    bool validate_mode(VIDEOINFO *mode) override;
    void get_max_screen(int *width, int *height) override;
    void terminate() override;
    void pause() override;
    void resume() override;
    void schedule_alarm(int secs) override;
    void window() override;
    bool resize() override;
    void redraw() override;
    int read_palette() override;
    int write_palette() override;
    int read_pixel(int x, int y) override;
    void write_pixel(int x, int y, int color) override;
    void read_span(int y, int x, int lastx, BYTE *pixels) override;
    void write_span(int y, int x, int lastx, BYTE *pixels) override;
    void get_truecolor(int x, int y, int *r, int *g, int *b, int *a) override;
    void put_truecolor(int x, int y, int r, int g, int b, int a) override;
    void set_line_mode(int mode) override;
    void draw_line(int x1, int y1, int x2, int y2, int color) override;
    void display_string(int x, int y, int fg, int bg, char const *text) override;
    void save_graphics() override;
    void restore_graphics() override;
    int get_key() override;
    int key_cursor(int row, int col) override;
    int key_pressed() override;
    int wait_key_pressed(int timeout) override;
    void unget_key(int key) override;
    void shell() override;
    void set_video_mode(VIDEOINFO *mode) override;
    void put_string(int row, int col, int attr, char const *msg) override;
    bool is_text() override;
    void set_for_text() override;
    void set_clear() override;
    void move_cursor(int row, int col) override;
    void hide_text_cursor() override;
    void set_attr(int row, int col, int attr, int count) override;
    void scroll_up(int top, int bot) override;
    void stack_screen() override;
    void unstack_screen() override;
    void discard_screen() override;
    int init_fm() override;
    void buzzer(buzzer_codes kind) override;
    bool sound_on(int frequency) override;
    void sound_off() override;
    void mute() override;
    bool diskp() override;
    int get_char_attr() override;
    void put_char_attr(int char_attr) override;
    void delay(int ms) override;
    void set_keyboard_timeout(int ms) override;
    void flush() override;

    void setredrawscreen()
    {
        m_need_redraw = true;
    }

private:
    int check_arg(int argc, char **argv, int *i);
    void doneXwindow();
    void erase_text_screen();
    void select_visual();
    void clearXwindow();
    int xcmapstuff();
    int start_video();
    void get_max_size(unsigned *width, unsigned *height, bool *center_x, bool *center_y);
    void center_window(bool center_x, bool center_y);
    int getachar();
    int get_a_char_delay();
    int handle_esc();
    int ev_key_press(XKeyEvent *xevent);
    void ev_key_release(XKeyEvent *xevent);
    void ev_expose(XExposeEvent *xevent);
    void ev_button_press(XEvent *xevent);
    void ev_motion_notify(XEvent *xevent);
    void handle_events();
    int input_pending();
    Window pr_dwmroot(Display *dpy, Window pwin);
    Window FindRootWindow();
    void RemoveRootPixmap();
    void load_font();
    void flush_output();
    unsigned long do_fake_lut(int idx)
    {
        return m_fake_lut ? m_cmap_pixtab[idx] : idx;
    }

    bool m_on_root{};              // = false;
    bool m_full_screen{};          // = false;
    bool m_share_color{};          // = false;
    bool m_private_color{};        // = false;
    int m_fix_colors{};            // = 0;
    bool m_sync{};                 // = false; Run X events synchronously (debugging)
    std::string m_display;         //
    std::string m_geometry;        //
    bool m_use_pixtab{};           // = false;
    unsigned long m_pixtab[256]{}; //
    int m_inv_pixtab[256]{};       //
    XPixel m_cmap_pixtab[256]{};   // for faking a LUTs on non-LUT visuals
    bool m_have_cmap_pixtab{};     //
    bool m_fake_lut{};             //
    bool m_fast_mode{};            // = false; Don't draw pixels 1 at a time
    bool m_alarm_on{};             // = false; true if the refresh alarm is on
    bool m_need_redraw{};          // = false; true if we have a redraw waiting
    Display *m_dpy{};              // = nullptr;
    Window m_window{None};         //
    GC Xgc{None};                  // = nullptr;
    Visual *Xvi{};                 //
    Screen *Xsc{};                 //
    Colormap Xcmap{None};          //
    int Xdepth{};                  //
    XImage *Ximage{};              //
    char *Xdata{};                 //
    int Xdscreen{};                //
    Pixmap Xpixmap{None};          // = None;
    int Xwinwidth{DEFX};           //
    int Xwinheight{DEFY};          //
    Window Xroot{None};            //
    int xlastcolor{-1};            // = -1;
    int xlastfcn{GXcopy};          // = GXcopy;
    BYTE *pixbuf{};                // = nullptr;
    XColor cols[256]{};            //
    bool XZoomWaiting{};           // = false;
    char const *x_font_name{FONT}; // = FONT;
    XFontStruct *font_info{};      // = nullptr;
    int key_buffer{};              // = 0; Buffered X key
    unsigned char *fontPtr{};      // = nullptr;
    char text_screen[TEXT_HEIGHT][TEXT_WIDTH]{};
    int text_attr[TEXT_HEIGHT][TEXT_WIDTH]{};
    BYTE *font_table{};       // = nullptr;
    bool text_not_graphics{}; // true when displaying text
    bool ctl_mode{};          // rubber banding and event processing data
    bool shift_mode{};        //
    int button_num{};         //
    int last_x{};             //
    int last_y{};             //
    int dx{};                 //
    int dy{};                 //
    x11_frame_window frame_;  //
    x11_text_window text_;    //
    x11_plot_window plot_;    //
};

#ifdef FPUERR
static void continue_hdl(int sig, int code, struct sigcontext *scp,
                         char *addr);
#endif

static const int mousefkey[4][4] /* [button][dir] */ = {
    {ID_KEY_RIGHT_ARROW, ID_KEY_LEFT_ARROW, ID_KEY_DOWN_ARROW, ID_KEY_UP_ARROW},
    {0, 0, ID_KEY_PAGE_DOWN, ID_KEY_PAGE_UP},
    {ID_KEY_CTL_PLUS, ID_KEY_CTL_MINUS, ID_KEY_CTL_DEL, ID_KEY_CTL_INSERT},
    {ID_KEY_CTL_END, ID_KEY_CTL_HOME, ID_KEY_CTL_PAGE_DOWN, ID_KEY_CTL_PAGE_UP}
};

extern bool editpal_cursor;
extern void Cursor_SetPos();

#define SENS 1
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define SIGN(x) ((x) > 0 ? 1 : -1)

#define SHELL "/bin/csh"

#define DRAW_INTERVAL 6

#define DRIVER_MODE(width_, height_) \
    { 0, width_, height_, 256, nullptr, "                         " }
static const VIDEOINFO modes[] =
{
    // 4:3 aspect ratio
    DRIVER_MODE(800, 600),
    DRIVER_MODE(1024, 768),
    DRIVER_MODE(1280, 960),
    DRIVER_MODE(1400, 1050),
    DRIVER_MODE(1600, 1200),
    DRIVER_MODE(2048, 1536),

    // 16:9 aspect ratio
    DRIVER_MODE(854, 480),
    DRIVER_MODE(1280, 720),
    DRIVER_MODE(1366, 768),
    DRIVER_MODE(1920, 1080),

    // 8:5 (16:10) aspect ratio
    DRIVER_MODE(1280, 800),
    DRIVER_MODE(1440, 900),
    DRIVER_MODE(1680, 1050),
    DRIVER_MODE(1920, 1200),
    DRIVER_MODE(2560, 1600)
};

/*
 *----------------------------------------------------------------------
 *
 * check_arg --
 *
 *  See if we want to do something with the argument.
 *
 * Results:
 *  Returns 1 if we parsed the argument.
 *
 * Side effects:
 *  Increments i if we use more than 1 argument.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::check_arg(int argc, char **argv, int *i)
{
    if (std::strcmp(argv[*i], "-display") == 0 && (*i)+1 < argc)
    {
        m_display = argv[(*i)+1];
        (*i)++;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-fullscreen") == 0)
    {
        m_full_screen = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-onroot") == 0)
    {
        m_on_root = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-share") == 0)
    {
        m_share_color = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-fast") == 0)
    {
        m_fast_mode = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-slowdisplay") == 0)
    {
        slowdisplay = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-sync") == 0)
    {
        m_sync = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-private") == 0)
    {
        m_private_color = true;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-fixcolors") == 0 && *i+1 < argc)
    {
        m_fix_colors = std::atoi(argv[(*i)+1]);
        (*i)++;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-geometry") == 0 && *i+1 < argc)
    {
        m_geometry = argv[(*i)+1];
        (*i)++;
        return 1;
    }
    else if (std::strcmp(argv[*i], "-fn") == 0 && *i+1 < argc)
    {
        x_font_name = argv[(*i)+1];
        (*i)++;
        return 1;
    }
    else
    {
        return 0;
    }
}

/*----------------------------------------------------------------------
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
void X11Driver::doneXwindow()
{
    if (m_dpy == nullptr)
        return;

    if (Xgc)
        XFreeGC(m_dpy, Xgc);

    if (Xpixmap)
    {
        XFreePixmap(m_dpy, Xpixmap);
        Xpixmap = None;
    }
    XFlush(m_dpy);
    m_dpy = nullptr;
}

/*----------------------------------------------------------------------
 *
 * initdacbox --
 *
 * Put something nice in the dac.
 *
 * The conditions are:
 *  Colors 1 and 2 should be bright so ifs fractals show up.
 *  Color 15 should be bright for lsystem.
 *  Color 1 should be bright for bifurcation.
 *  Colors 1, 2, 3 should be distinct for periodicity.
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
static void initdacbox()
{
    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = (i >> 5)*8+7;
        g_dac_box[i][1] = (((i+16) & 28) >> 2)*8+7;
        g_dac_box[i][2] = (((i+2) & 3))*16+15;
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

void X11Driver::erase_text_screen()
{
    for (int r = 0; r < TEXT_HEIGHT; r++)
        for (int c = 0; c < TEXT_WIDTH; c++)
        {
            text_attr[r][c] = 0;
            text_screen[r][c] = ' ';
        }
}

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
    std::fprintf(stderr, "X Error: %d %d %d %d\n", xe->type, xe->error_code,
           xe->request_code, xe->minor_code);
    XGetErrorText(dp, xe->error_code, buf, 200);
    std::fprintf(stderr, "%s\n", buf);
    fflush(stderr);
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
    char out[20];
    //        if you want to get all messages enable this statement.
    //  std::printf("ieee exception code %x occurred at pc %X\n", code, scp->sc_pc);
    //    clear all excaption flags
    ieee_flags("clear", "exception", "all", out);
}
#endif

void X11Driver::select_visual()
{
    Xvi = XDefaultVisualOfScreen(Xsc);
    Xdepth = DefaultDepth(m_dpy, Xdscreen);

    switch (Xvi->c_class)
    {
    case StaticGray:
    case StaticColor:
        g_colors = (Xdepth <= 8) ? Xvi->map_entries : 256;
        g_got_real_dac = false;
        m_fake_lut = false;
        break;

    case GrayScale:
    case PseudoColor:
        g_colors = (Xdepth <= 8) ? Xvi->map_entries : 256;
        g_got_real_dac = true;
        m_fake_lut = false;
        break;

    case TrueColor:
    case DirectColor:
        g_colors = 256;
        g_got_real_dac = false;
        m_fake_lut = true;
        break;

    default:
        // those should be all the visual classes
        assert(1);
        break;
    }
    if (g_colors > 256)
        g_colors = 256;
}

/*----------------------------------------------------------------------
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
void X11Driver::clearXwindow()
{
    if (m_fake_lut)
    {
        for (int j = 0; j < Ximage->height; j++)
            for (int i = 0; i < Ximage->width; i++)
                XPutPixel(Ximage, i, j, m_cmap_pixtab[m_pixtab[0]]);
    }
    else if (m_pixtab[0] != 0)
    {
        /*
         * Initialize image to m_pixtab[0].
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
                Ximage->data[i] = m_pixtab[0];
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
    XSetForeground(m_dpy, Xgc, do_fake_lut(m_pixtab[0]));
    if (m_on_root)
        XFillRectangle(m_dpy, Xpixmap, Xgc,
                       0, 0, Xwinwidth, Xwinheight);
    XFillRectangle(m_dpy, m_window, Xgc,
                   0, 0, Xwinwidth, Xwinheight);
    flush();
}

/*----------------------------------------------------------------------
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
int X11Driver::xcmapstuff()
{
    int ncells;

    if (m_on_root)
    {
        m_private_color = false;
    }
    for (int i = 0; i < g_colors; i++)
    {
        m_pixtab[i] = i;
        m_inv_pixtab[i] = 999;
    }
    if (!g_got_real_dac)
    {
        Xcmap = DefaultColormapOfScreen(Xsc);
        if (m_fake_lut)
            x11_write_palette(&pub);
    }
    else if (m_share_color)
    {
        g_got_real_dac = false;
    }
    else if (m_private_color)
    {
        Xcmap = XCreateColormap(m_dpy, m_window, Xvi, AllocAll);
        XSetWindowColormap(m_dpy, m_window, Xcmap);
    }
    else
    {
        Xcmap = DefaultColormap(m_dpy, Xdscreen);
        for (int powr = Xdepth; powr >= 1; powr--)
        {
            ncells = 1 << powr;
            if (ncells > g_colors)
                continue;
            if (XAllocColorCells(m_dpy, Xcmap, False, nullptr, 0, m_pixtab,
                                 (unsigned int) ncells))
            {
                g_colors = ncells;
                m_use_pixtab = true;
                break;
            }
        }
        if (!m_use_pixtab)
        {
            std::printf("Couldn't allocate any colors\n");
            g_got_real_dac = false;
        }
    }
    for (int i = 0; i < g_colors; i++)
    {
        m_inv_pixtab[m_pixtab[i]] = i;
    }
    /* We must make sure if any color uses position 0, that it is 0.
     * This is so we can clear the image with std::memset.
     * So, suppose 0 = cmap 42, cmap 0 = fractint 55.
     * Then want 0 = cmap 0, cmap 42 = fractint 55.
     * I.e. m_pixtab[55] = 42, m_inv_pixtab[42] = 55.
     */
    if (m_inv_pixtab[0] == 999)
    {
        m_inv_pixtab[0] = 0;
    }
    else if (m_inv_pixtab[0] != 0)
    {
        int other;
        other = m_inv_pixtab[0];
        m_pixtab[other] = m_pixtab[0];
        m_inv_pixtab[m_pixtab[other]] = other;
        m_pixtab[0] = 0;
        m_inv_pixtab[0] = 0;
    }

    if (!g_got_real_dac && g_colors == 2 && BlackPixelOfScreen(Xsc) != 0)
    {
        m_inv_pixtab[0] = 1;
        m_pixtab[0] = m_inv_pixtab[0];
        m_inv_pixtab[1] = 0;
        m_pixtab[1] = m_inv_pixtab[1];
        m_use_pixtab = true;
    }

    return g_colors;
}

int X11Driver::start_video()
{
    clearXwindow();
    return 0;
}

int X11Driver::end_video()
{
    return 0;             // set flag: video ended
}


int X11Driver::getachar()
{
    if (0)
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

/*----------------------------------------------------------------------
 *
 * translate_key --
 *
 *  Translate an input key into MSDOS format.  The purpose of this
 *  routine is to do the mappings like U -> PAGE_UP.
 *
 * Results:
 *  New character;
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
static int translate_key(int ch)
{
    if (ch >= 'a' && ch <= 'z')
        return ch;
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
        case ctl('E'):
            return ID_KEY_CTL_END;
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
        case ctl('D'):
            return ID_KEY_CTL_DEL;
        case '!':
            return ID_KEY_F1;
        case '@':
            return ID_KEY_F2;
        case '#':
            return ID_KEY_F3;
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

int X11Driver::get_a_char_delay()
{
    int ch = getachar();
    if (ch == -1)
    {
        delay(250); // Wait 1/4 sec to see if a control sequence follows
        ch = getachar();
    }
    return ch;
}

/* handle_esc --
 *
 *  Handle an escape key.  This may be an escape key sequence
 *  indicating a function key was pressed.
 */
int X11Driver::handle_esc()
{
    // SUN escape key sequences
    int ch1 = get_a_char_delay();
    if (ch1 != '[')       // See if we have esc [
        return ID_KEY_ESC;
    ch1 = get_a_char_delay();
    if (ch1 == -1)
        return ID_KEY_ESC;
    switch (ch1)
    {
    case 'A':     // esc [ A
        return ID_KEY_UP_ARROW;
    case 'B':     // esc [ B
        return ID_KEY_DOWN_ARROW;
    case 'C':     // esc [ C
        return ID_KEY_RIGHT_ARROW;
    case 'D':     // esc [ D
        return ID_KEY_LEFT_ARROW;
    default:
        break;
    }
    int ch2 = get_a_char_delay();
    if (ch2 == '~')
    {     // esc [ ch1 ~
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
        int ch3 = get_a_char_delay();
        if (ch3 != '~')
        {   // esc [ ch1 ch2 ~
            return ID_KEY_ESC;
        }
        if (ch1 == '1')
        {
            switch (ch2)
            {
            case '1': // esc [ 1 1 ~
                return ID_KEY_F1;
            case '2': // esc [ 1 2 ~
                return ID_KEY_F2;
            case '3': // esc [ 1 3 ~
                return ID_KEY_F3;
            case '4': // esc [ 1 4 ~
                return ID_KEY_F4;
            case '5': // esc [ 1 5 ~
                return ID_KEY_F5;
            case '6': // esc [ 1 6 ~
                return ID_KEY_F6;
            case '7': // esc [ 1 7 ~
                return ID_KEY_F7;
            case '8': // esc [ 1 8 ~
                return ID_KEY_F8;
            case '9': // esc [ 1 9 ~
                return ID_KEY_F9;
            default:
                return ID_KEY_ESC;
            }
        }
        else if (ch1 == '2')
        {
            switch (ch2)
            {
            case '0': // esc [ 2 0 ~
                return ID_KEY_F10;
            case '8': // esc [ 2 8 ~
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

/* ev_key_press
 *
 * Translate keypress into appropriate character code,
 * according to defines in id.h
 */
int X11Driver::ev_key_press(XKeyEvent *xevent)
{
    int charcount;
    char buffer[1];
    KeySym keysym;
    charcount = XLookupString(xevent, buffer, 1, &keysym, nullptr);
    switch (keysym)
    {
    case XK_Control_L:
    case XK_Control_R:
        ctl_mode = true;
        return 1;

    case XK_Shift_L:
    case XK_Shift_R:
        shift_mode = true;
        break;
    case XK_Home:
    case XK_R7:
        key_buffer = ctl_mode ? ID_KEY_CTL_HOME : ID_KEY_HOME;
        return 1;
    case XK_Left:
    case XK_R10:
        key_buffer = ctl_mode ? ID_KEY_CTL_LEFT_ARROW : ID_KEY_LEFT_ARROW;
        return 1;
    case XK_Right:
    case XK_R12:
        key_buffer = ctl_mode ? ID_KEY_CTL_RIGHT_ARROW : ID_KEY_RIGHT_ARROW;
        return 1;
    case XK_Down:
    case XK_R14:
        key_buffer = ctl_mode ? ID_KEY_CTL_DOWN_ARROW : ID_KEY_DOWN_ARROW;
        return 1;
    case XK_Up:
    case XK_R8:
        key_buffer = ctl_mode ? ID_KEY_CTL_UP_ARROW : ID_KEY_UP_ARROW;
        return 1;
    case XK_Insert:
        key_buffer = ctl_mode ? ID_KEY_CTL_INSERT : ID_KEY_INSERT;
        return 1;
    case XK_Delete:
        key_buffer = ctl_mode ? ID_KEY_CTL_DEL : ID_KEY_DELETE;
        return 1;
    case XK_End:
    case XK_R13:
        key_buffer = ctl_mode ? ID_KEY_CTL_END : ID_KEY_END;
        return 1;
    case XK_Help:
        key_buffer = ID_KEY_F1;
        return 1;
    case XK_Prior:
    case XK_R9:
        key_buffer = ctl_mode ? ID_KEY_CTL_PAGE_UP : ID_KEY_PAGE_UP;
        return 1;
    case XK_Next:
    case XK_R15:
        key_buffer = ctl_mode ? ID_KEY_CTL_PAGE_DOWN : ID_KEY_PAGE_DOWN;
        return 1;
    case XK_F1:
    case XK_L1:
        key_buffer = shift_mode ? ID_KEY_SF1: ID_KEY_F1;
        return 1;
    case XK_F2:
    case XK_L2:
        key_buffer = shift_mode ? ID_KEY_SF2: ID_KEY_F2;
        return 1;
    case XK_F3:
    case XK_L3:
        key_buffer = shift_mode ? ID_KEY_SF3: ID_KEY_F3;
        return 1;
    case XK_F4:
    case XK_L4:
        key_buffer = shift_mode ? ID_KEY_SF4: ID_KEY_F4;
        return 1;
    case XK_F5:
    case XK_L5:
        key_buffer = shift_mode ? ID_KEY_SF5: ID_KEY_F5;
        return 1;
    case XK_F6:
    case XK_L6:
        key_buffer = shift_mode ? ID_KEY_SF6: ID_KEY_F6;
        return 1;
    case XK_F7:
    case XK_L7:
        key_buffer = shift_mode ? ID_KEY_SF7: ID_KEY_F7;
        return 1;
    case XK_F8:
    case XK_L8:
        key_buffer = shift_mode ? ID_KEY_SF8: ID_KEY_F8;
        return 1;
    case XK_F9:
    case XK_L9:
        key_buffer = shift_mode ? ID_KEY_SF9: ID_KEY_F9;
        return 1;
    case XK_F10:
    case XK_L10:
        key_buffer = shift_mode ? ID_KEY_SF10: ID_KEY_F10;
        return 1;
    case '+':
        key_buffer = ctl_mode ? ID_KEY_CTL_PLUS : '+';
        return 1;
    case '-':
        key_buffer = ctl_mode ? ID_KEY_CTL_MINUS : '-';
        return 1;
        break;
    case XK_Return:
    case XK_KP_Enter:
        key_buffer = ctl_mode ? ctl('T') : '\n';
        return 1;
    }
    if (charcount == 1)
    {
        key_buffer = buffer[0];
        if (key_buffer == '\003')
        {
            goodbye();
        }
    }
    return 0;
}

/* ev_key_release
 *
 * toggle modifier key state for shit and contol keys, otherwise ignore
 */
void X11Driver::ev_key_release(XKeyEvent *xevent)
{
    char buffer[1];
    KeySym keysym;
    XLookupString(xevent, buffer, 1, &keysym, nullptr);
    switch (keysym)
    {
    case XK_Control_L:
    case XK_Control_R:
        ctl_mode = false;
        break;
    case XK_Shift_L:
    case XK_Shift_R:
        shift_mode = false;
        break;
    }
}

void X11Driver::ev_expose(XExposeEvent *xevent)
{
    if (text_not_graphics)
    {
        // if text window, refresh text
    }
    else
    {
        // refresh graphics
        int x, y, w, h;
        x = xevent->x;
        y = xevent->y;
        w = xevent->width;
        h = xevent->height;
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
            XPutImage(m_dpy, m_window, Xgc, Ximage, x, y, x, y,
                      xevent->width, xevent->height);
        }
    }
}

void X11Driver::ev_button_press(XEvent *xevent)
{
    bool done = false;
    bool banding = false;
    int bandx0, bandy0, bandx1, bandy1;

    if (g_look_at_mouse == 3 || !g_zoom_off)
    {
        last_x = xevent->xbutton.x;
        last_y = xevent->xbutton.y;
        return;
    }

    bandx0 = xevent->xbutton.x;
    bandx1 = bandx0;
    bandy0 = xevent->xbutton.y;
    bandy1 = bandy0;
    while (!done)
    {
        XNextEvent(m_dpy, xevent);
        switch (xevent->type)
        {
        case MotionNotify:
            while (XCheckWindowEvent(m_dpy, m_window, PointerMotionMask, xevent))
                ;
            if (banding)
                XDrawRectangle(m_dpy, m_window, Xgc, MIN(bandx0, bandx1),
                               MIN(bandy0, bandy1), ABS(bandx1-bandx0),
                               ABS(bandy1-bandy0));
            bandx1 = xevent->xmotion.x;
            bandy1 = xevent->xmotion.y;
            if (ABS(bandx1-bandx0)*g_final_aspect_ratio > ABS(bandy1-bandy0))
                bandy1 =
                    SIGN(bandy1-bandy0)*ABS(bandx1-bandx0)*g_final_aspect_ratio + bandy0;
            else
                bandx1 =
                    SIGN(bandx1-bandx0)*ABS(bandy1-bandy0)/g_final_aspect_ratio + bandx0;

            if (!banding)
            {
                /* Don't start rubber-banding until the mouse
                   gets moved.  Otherwise a click messes up the
                   window */
                if (ABS(bandx1-bandx0) > 10 || ABS(bandy1-bandy0) > 10)
                {
                    banding = true;
                    XSetForeground(m_dpy, Xgc, do_fake_lut(g_colors-1));
                    XSetFunction(m_dpy, Xgc, GXxor);
                }
            }
            if (banding)
            {
                XDrawRectangle(m_dpy, m_window, Xgc, MIN(bandx0, bandx1),
                               MIN(bandy0, bandy1), ABS(bandx1-bandx0),
                               ABS(bandy1-bandy0));
            }
            XFlush(m_dpy);
            break;

        case ButtonRelease:
            done = true;
            break;
        }
    }

    if (!banding)
        return;

    XDrawRectangle(m_dpy, m_window, Xgc, MIN(bandx0, bandx1),
                   MIN(bandy0, bandy1), ABS(bandx1-bandx0),
                   ABS(bandy1-bandy0));
    if (bandx1 == bandx0)
        bandx1 = bandx0+1;
    if (bandy1 == bandy0)
        bandy1 = bandy0+1;
    g_zoom_box_rotation = 0;
    g_zoom_box_skew = 0;
    g_zoom_box_x = (MIN(bandx0, bandx1)-g_logical_screen_x_offset)/g_logical_screen_x_size_dots;
    g_zoom_box_y = (MIN(bandy0, bandy1)-g_logical_screen_y_offset)/g_logical_screen_y_size_dots;
    g_zoom_box_width = ABS(bandx1-bandx0)/g_logical_screen_x_size_dots;
    g_zoom_box_height = g_zoom_box_width;
    if (!g_inside_help)
        key_buffer = ID_KEY_ENTER;
    if (xlastcolor != -1)
        XSetForeground(m_dpy, Xgc, do_fake_lut(xlastcolor));
    XSetFunction(m_dpy, Xgc, xlastfcn);
    XZoomWaiting = true;
    drawbox(0);
}

void X11Driver::ev_motion_notify(XEvent *xevent)
{
    if (editpal_cursor && !g_inside_help)
    {
        while (XCheckWindowEvent(m_dpy, m_window, PointerMotionMask, xevent))
            ;

        if (xevent->xmotion.state & Button2Mask ||
                (xevent->xmotion.state & (Button1Mask | Button3Mask)))
        {
            button_num = 3;
        }
        else if (xevent->xmotion.state & Button1Mask)
        {
            button_num = 1;
        }
        else if (xevent->xmotion.state & Button3Mask)
        {
            button_num = 2;
        }
        else
        {
            button_num = 0;
        }

        if (g_look_at_mouse == 3 && button_num != 0)
        {
            dx += (xevent->xmotion.x-last_x)/MOUSE_SCALE;
            dy += (xevent->xmotion.y-last_y)/MOUSE_SCALE;
            last_x = xevent->xmotion.x;
            last_y = xevent->xmotion.y;
        }
        else
        {
            Cursor_SetPos(xevent->xmotion.x, xevent->xmotion.y);
            key_buffer = ID_KEY_ENTER;
        }
    }
}

void X11Driver::handle_events()
{
    XEvent xevent;

    if (m_need_redraw)
        redraw();

    while (XPending(m_dpy) && !key_buffer)
    {
        XNextEvent(m_dpy, &xevent);
        switch (xevent.type)
        {
        case KeyRelease:
            ev_key_release(&xevent.xkey);
            break;

        case KeyPress:
            if (ev_key_press(&xevent.xkey))
                return;
            break;

        case MotionNotify:
            ev_motion_notify(&xevent);
            break;

        case ButtonPress:
            ev_button_press(&xevent);
            break;

        case Expose:
            ev_expose(&xevent.xexpose);
            break;
        }
    }

    if (!key_buffer && editpal_cursor && !g_inside_help && g_look_at_mouse == 3 &&
            (dx != 0 || dy != 0))
    {
        if (ABS(dx) > ABS(dy))
        {
            if (dx > 0)
            {
                key_buffer = mousefkey[button_num][0]; // right
                dx--;
            }
            else if (dx < 0)
            {
                key_buffer = mousefkey[button_num][1]; // left
                dx++;
            }
        }
        else
        {
            if (dy > 0)
            {
                key_buffer = mousefkey[button_num][2]; // down
                dy--;
            }
            else if (dy < 0)
            {
                key_buffer = mousefkey[button_num][3]; // up
                dy++;
            }
        }
    }
}

// Check if there is a character waiting for us.
int X11Driver::input_pending()
{
    return XPending(m_dpy);
}

Window X11Driver::pr_dwmroot(Display *dpy, Window pwin)
{
    // search for DEC Window Manager root
    XWindowAttributes pxwa, cxwa;
    Window  root, parent, *child;
    unsigned int nchild;

    if (!XGetWindowAttributes(dpy, pwin, &pxwa))
    {
        std::printf("Search for root: XGetWindowAttributes failed\n");
        return RootWindow(dpy, Xdscreen);
    }
    if (XQueryTree(dpy, pwin, &root, &parent, &child, &nchild))
    {
        for (unsigned int i = 0U; i < nchild; i++)
        {
            if (!XGetWindowAttributes(dpy, child[i], &cxwa))
            {
                std::printf("Search for root: XGetWindowAttributes failed\n");
                return RootWindow(dpy, Xdscreen);
            }
            if (pxwa.width == cxwa.width && pxwa.height == cxwa.height)
                return pr_dwmroot(dpy, child[i]);
        }
        return pwin;
    }
    else
    {
        std::printf("Id: failed to find root window\n");
        return RootWindow(dpy, Xdscreen);
    }
}

Window X11Driver::FindRootWindow()
{
    Xroot = RootWindow(m_dpy, Xdscreen);
    Xroot = pr_dwmroot(m_dpy, Xroot); // search for DEC wm root

    {   // search for swm/tvtwm root (from ssetroot by Tom LaStrange)
        Window rootReturn, parentReturn, *children;
        unsigned int numChildren;

        Atom SWM_VROOT = XInternAtom(m_dpy, "__SWM_VROOT", False);
        XQueryTree(m_dpy, Xroot, &rootReturn, &parentReturn,
                   &children, &numChildren);
        for (int i = 0; i < numChildren; i++)
        {
            Atom actual_type;
            int actual_format;
            unsigned long nitems, bytesafter;
            Window *newRoot = nullptr;

            if (XGetWindowProperty(m_dpy, children[i], SWM_VROOT,
                                   0L, 1L,
                                   False, XA_WINDOW,
                                   &actual_type, &actual_format,
                                   &nitems, &bytesafter,
                                   (unsigned char **) &newRoot) == Success &&
                    newRoot)
            {
                Xroot = *newRoot;
                break;
            }
        }
    }
    return Xroot;
}

void X11Driver::RemoveRootPixmap()
{
    Atom prop, type;
    int format;
    unsigned long nitems, after;
    Pixmap *pm;

    prop = XInternAtom(m_dpy, "_XSETROOT_ID", False);
    if (XGetWindowProperty(m_dpy, Xroot, prop, 0L, 1L, 1,
                           AnyPropertyType, &type, &format, &nitems, &after,
                           (unsigned char **) &pm) == Success && nitems == 1)
    {
        if (type == XA_PIXMAP && format == 32 && after == 0)
        {
            XKillClient(m_dpy, (XID)*pm);
            XFree((char *)pm);
        }
    }
}

void X11Driver::load_font()
{
    font_info = XLoadQueryFont(m_dpy, x_font_name);
    if (font_info == nullptr)
        font_info = XLoadQueryFont(m_dpy, "6x12");
}

void X11DRiver::flush()
{
    XSync(m_dpy, False);
}

void fpe_handler(int signum)
{
    g_overflow = true;
}

bool X11Driver::init(int *argc, char **argv)
{
    /*
     * Check a bunch of important conditions
     */
    if (sizeof(short) != 2)
    {
        std::fprintf(stderr, "Error: need short to be 2 bytes\n");
        exit(-1);
    }
    if (sizeof(long) < sizeof(FLOAT4))
    {
        std::fprintf(stderr, "Error: need sizeof(long) >= sizeof(FLOAT4)\n");
        exit(-1);
    }

    initdacbox();

    signal(SIGFPE, fpe_handler);
#ifdef FPUERR
    signal(SIGABRT, SIG_IGN);
    /*
      setup the IEEE-handler to forget all common ( invalid,
      divide by zero, overflow ) signals. Here we test, if
      such ieee trapping is supported.
    */
    if (ieee_handler("set", "common", continue_hdl) != 0)
        std::printf("ieee trapping not supported here \n");
#endif

    // filter out x11 arguments
    {
        int count = *argc;
        std::vector<char *> filtered;
        for (int i = 0; i < count; i++)
        {
            if (! check_arg(i, argv, &i))
            {
                filtered.push_back(argv[i]);
            }
        }
        std::copy(filtered.begin(), filtered.end(), argv);
        *argc = filtered.size();
    }

    m_dpy = XOpenDisplay(m_display.c_str());
    if (m_dpy == nullptr)
    {
        terminate();
        return false;
    }
    Xdscreen = XDefaultScreen(m_dpy);
    if (m_sync)
        XSynchronize(m_dpy, True);
    XSetErrorHandler(errhand);

    {
        int const width = WidthOfScreen(DefaultScreenOfDisplay(m_dpy));
        int const height = HeightOfScreen(DefaultScreenOfDisplay(m_dpy));

        for (auto m : modes)
        {
            if (m.xdots <= width && m.ydots <= height)
            {
                add_video_mode(drv, &m);
            }
        }
    }

    int screen_num = DefaultScreen(m_dpy);
    frame_.initialize(m_dpy, screen_num, m_geometry.c_str());
    plot_.initialize(m_dpy, screen_num, frame_.window());
    text_.initialize(m_dpy, screen_num, frame_.window());

    return true;
}

bool X11Driver::validate_mode(VIDEOINFO *mode)
{
    return false;
}

void X11Driver::terminate()
{
    doneXwindow();
}

void X11Driver::pause()
{
}

void X11Driver::resume()
{
}

static void handle_sig_alarm()
{
    ((X11Driver *) g_driver)->setredrawscreen();
}
void X11Driver::schedule_alarm(int secs)
{
    if (!m_fast_mode)
        return;

    signal(SIGALRM, (SignalHandler) handle_sig_alarm);
    if (secs)
        alarm(1);
    else
        alarm(DRAW_INTERVAL);

    m_alarm_on = true;
}

void X11Driver::get_max_size(unsigned *width, unsigned *height, bool *center_x, bool *center_y)
{
    *width = text_.max_width();
    *height = text_.max_height();
    if (g_video_table[g_adapter].xdots > *width)
    {
        *width = g_video_table[g_adapter].xdots;
        *center_x = false;
    }
    if (g_video_table[g_adapter].ydots > *height)
    {
        *height = g_video_table[g_adapter].ydots;
        *center_y = false;
    }
}

void X11Driver::center_window(bool center_x, bool center_y)
{
    struct x11_point
    {
        int x;
        int y;
    };
    x11_point text_pos{};
    x11_point plot_pos{};

    if (center_x)
    {
        plot_pos.x = (frame_.width() - plot_.width())/2;
    }
    else
    {
        text_pos.x = (frame_.width() - text_.max_width())/2;
    }

    if (center_y)
    {
        plot_pos.y = (frame_.height() - plot_.height())/2;
    }
    else
    {
        text_pos.y = (frame_.height() - text_.max_height())/2;
    }

    plot_.set_position(plot_pos.x, plot_pos.y);
    text_.set_position(text_pos.x, text_pos.y);
}

void frame_window(int width, int height)
{
}

void X11Driver::window()
{
    unsigned width;
    unsigned height;
    bool center_x = true;
    bool center_y = true;
    get_max_size(&width, &height, &center_x, &center_y);
    frame_.window(width, height);
    text_.text_on();
    center_window(center_x, center_y);
#if 0
    base.wintext.hWndParent = g_frame.window;
    wintext_texton(&base.wintext);
    plot_window(&plot, g_frame.window);
    center_windows(center_x, center_y);

    XSetWindowAttributes Xwatt;
    XGCValues Xgcvals;
    int Xwinx = 0, Xwiny = 0;

    g_adapter = 0;

    /* We have to do some X stuff even for disk video, to parse the geometry
     * string */

    if (!m_geometry.empty() && !m_on_root)
        XGeometry(m_dpy, Xdscreen, m_geometry.c_str(), DEFXY, 0, 1, 1, 0, 0,
                  &Xwinx, &Xwiny, &Xwinwidth, &Xwinheight);
    if (m_sync)
        XSynchronize(m_dpy, True);
    XSetErrorHandler(errhand);
    Xsc = ScreenOfDisplay(m_dpy, Xdscreen);
    select_visual();
    if (m_fix_colors > 0)
        g_colors = m_fix_colors;

    if (m_full_screen || m_on_root)
    {
        Xwinwidth = DisplayWidth(m_dpy, Xdscreen);
        Xwinheight = DisplayHeight(m_dpy, Xdscreen);
    }
    g_screen_x_dots = Xwinwidth;
    g_screen_y_dots = Xwinheight;

    Xwatt.background_pixel = BlackPixelOfScreen(Xsc);
    Xwatt.bit_gravity = StaticGravity;
    const int doesBacking = DoesBackingStore(Xsc);
    if (doesBacking)
        Xwatt.backing_store = Always;
    else
        Xwatt.backing_store = NotUseful;
    if (m_on_root)
    {
        Xroot = FindRootWindow();
        RemoveRootPixmap();
        Xgc = XCreateGC(m_dpy, Xroot, 0, &Xgcvals);
        Xpixmap = XCreatePixmap(m_dpy, Xroot,
                                    Xwinwidth, Xwinheight, Xdepth);
        m_window = Xroot;
        XFillRectangle(m_dpy, Xpixmap, Xgc, 0, 0, Xwinwidth, Xwinheight);
        XSetWindowBackgroundPixmap(m_dpy, Xroot, Xpixmap);
    }
    else
    {
        Xroot = DefaultRootWindow(m_dpy);
        m_window = XCreateWindow(m_dpy, Xroot, Xwinx, Xwiny,
                               Xwinwidth, Xwinheight, 0, Xdepth,
                               InputOutput, CopyFromParent,
                               CWBackPixel | CWBitGravity | CWBackingStore,
                               &Xwatt);
        XStoreName(m_dpy, m_window, "id");
        Xgc = XCreateGC(m_dpy, m_window, 0, &Xgcvals);
    }
    g_colors = xcmapstuff();
    if (g_color_cycle_range_hi == 255)
        g_color_cycle_range_hi = g_colors-1;

    {
        unsigned long event_mask = KeyPressMask | KeyReleaseMask | ExposureMask;
        if (! m_on_root)
            event_mask |= ButtonPressMask | ButtonReleaseMask | PointerMotionMask;
        XSelectInput(m_dpy, m_window, event_mask);
    }

    if (!m_on_root)
    {
        XSetBackground(m_dpy, Xgc, do_fake_lut(m_pixtab[0]));
        XSetForeground(m_dpy, Xgc, do_fake_lut(m_pixtab[1]));
        Xwatt.background_pixel = do_fake_lut(m_pixtab[0]);
        XChangeWindowAttributes(m_dpy, m_window, CWBackPixel, &Xwatt);
        XMapWindow(m_dpy, m_window);
    }

    resize(drv);
    flush(drv);
    write_palette(drv);

    x11_video_table[0].xdots = g_screen_x_dots;
    x11_video_table[0].ydots = g_screen_y_dots;
    x11_video_table[0].colors = g_colors;
#endif
}

/*----------------------------------------------------------------------
 *
 * resize --
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
bool X11Driver::resize()
{
    static int oldx = -1, oldy = -1;
    int junki;
    unsigned int junkui;
    Window junkw;
    unsigned int width, height;
    Status status;

    XGetGeometry(m_dpy, m_window, &junkw, &junki, &junki, &width, &height,
                 &junkui, &junkui);

    if (oldx != width || oldy != height)
    {
        g_screen_x_dots = width;
        g_screen_y_dots = height;
        x11_video_table[0].xdots = g_screen_x_dots;
        x11_video_table[0].ydots = g_screen_y_dots;
        oldx = g_screen_x_dots;
        oldy = g_screen_y_dots;
        Xwinwidth = g_screen_x_dots;
        Xwinheight = g_screen_y_dots;
        g_screen_aspect = g_screen_y_dots/(float) g_screen_x_dots;
        g_final_aspect_ratio = g_screen_aspect;
        int Xpad = 9;
        int Xmwidth;
        if (Xdepth == 1)
        {
            Xmwidth = 1 + g_screen_x_dots/8;
        }
        else if (Xdepth <= 8)
        {
            Xmwidth = g_screen_x_dots;
        }
        else if (Xdepth <= 16)
        {
            Xmwidth = 2*g_screen_x_dots;
            Xpad = 16;
        }
        else
        {
            Xmwidth = 4*g_screen_x_dots;
            Xpad = 32;
        }
        if (pixbuf != nullptr)
            free(pixbuf);
        pixbuf = (BYTE *) malloc(Xwinwidth *sizeof(BYTE));
        if (Ximage != nullptr)
        {
            free(Ximage->data);
            XDestroyImage(Ximage);
        }
        Ximage = XCreateImage(m_dpy, Xvi, Xdepth, ZPixmap, 0, nullptr, g_screen_x_dots,
                                  g_screen_y_dots, Xpad, Xmwidth);
        if (Ximage == nullptr)
        {
            std::printf("XCreateImage failed\n");
            terminate();
            exit(-1);
        }
        Ximage->data = (char *) malloc(Ximage->bytes_per_line * Ximage->height);
        if (Ximage->data == nullptr)
        {
            std::fprintf(stderr, "Malloc failed: %d\n", Ximage->bytes_per_line *
                    Ximage->height);
            exit(-1);
        }
        clearXwindow();
        return true;
    }
    else
    {
        return false;
    }
}


/*
 *----------------------------------------------------------------------
 *
 * redraw --
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
void X11Driver::redraw()
{
    if (m_alarm_on)
    {
        XPutImage(m_dpy, m_window, Xgc, Ximage, 0, 0, 0, 0,
                  g_screen_x_dots, g_screen_y_dots);
        if (m_on_root)
            XPutImage(m_dpy, Xpixmap, Xgc, Ximage, 0, 0, 0, 0,
                      g_screen_x_dots, g_screen_y_dots);
        m_alarm_on = false;
    }
    m_need_redraw = false;
}

/*
 *----------------------------------------------------------------------
 *
 * read_palette --
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
int X11Driver::read_palette()
{
    if (!g_got_real_dac)
        return -1;
    for (int i = 0; i < 256; i++)
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
 * write_palette --
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
int X11Driver::write_palette()
{
    if (!g_got_real_dac)
    {
        if (m_fake_lut)
        {
            // !g_got_real_dac, m_fake_lut => truecolor, directcolor displays
            static unsigned char last_dac[256][3];
            static bool last_dac_inited = false;

            for (int i = 0; i < 256; i++)
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

                    if (m_have_cmap_pixtab)
                    {
                        XFreeColors(m_dpy, Xcmap, m_cmap_pixtab + i, 1, None);
                    }
                    if (XAllocColor(m_dpy, Xcmap, &cols[i]))
                    {
                        m_cmap_pixtab[i] = cols[i].pixel;
                    }
                    else
                    {
                        assert(1);
                        std::printf("Allocating color %d failed.\n", i);
                    }

                    last_dac[i][0] = g_dac_box[i][0];
                    last_dac[i][1] = g_dac_box[i][1];
                    last_dac[i][2] = g_dac_box[i][2];
                }
            }
            m_have_cmap_pixtab = true;
            last_dac_inited = true;
        }
        else
        {
            // !g_got_real_dac, !m_fake_lut => static color, static gray displays
            assert(1);
        }
    }
    else
    {
        // g_got_real_dac => grayscale or pseudocolor displays
        for (int i = 0; i < 256; i++)
        {
            cols[i].pixel = m_pixtab[i];
            cols[i].flags = DoRed | DoGreen | DoBlue;
            cols[i].red = g_dac_box[i][0]*1024;
            cols[i].green = g_dac_box[i][1]*1024;
            cols[i].blue = g_dac_box[i][2]*1024;
        }
        XStoreColors(m_dpy, Xcmap, cols, g_colors);
        XFlush(m_dpy);
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * read_pixel --
 *
 *  Read a point from the screen
 *
 * Results:
 *  Value of point.
 *
 * Side effects:
 *  None.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::read_pixel(int x, int y)
{
    if (m_fake_lut)
    {
        XPixel pixel = XGetPixel(Ximage, x, y);
        for (int i = 0; i < 256; i++)
            if (m_cmap_pixtab[i] == pixel)
                return i;
        return 0;
    }
    else
        return m_inv_pixtab[XGetPixel(Ximage, x, y)];
}

/*
 *----------------------------------------------------------------------
 *
 * write_pixel --
 *
 *  Write a point to the screen
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Draws point.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::write_pixel(int x, int y, int color)
{
#ifndef NDEBUG // Debugging checks
    if (color >= g_colors || color < 0)
    {
        std::printf("Color %d too big %d\n", color, g_colors);
    }
    if (x >= g_screen_x_dots || x < 0 || y >= g_screen_y_dots || y < 0)
    {
        std::printf("Bad coord %d %d\n", x, y);
    }
#endif
    if (xlastcolor != color)
    {
        XSetForeground(m_dpy, Xgc, do_fake_lut(m_pixtab[color]));
        xlastcolor = color;
    }
    XPutPixel(Ximage, x, y, do_fake_lut(m_pixtab[color]));
    if (m_fast_mode && g_help_mode != help_labels::HELP_PALETTE_EDITOR)
    {
        if (!m_alarm_on)
        {
            schedule_alarm(drv, 0);
        }
    }
    else
    {
        XDrawPoint(m_dpy, m_window, Xgc, x, y);
        if (m_on_root)
        {
            XDrawPoint(m_dpy, Xpixmap, Xgc, x, y);
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * read_span --
 *
 *  Reads a line of pixels from the screen.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Gets pixels
 *
 *----------------------------------------------------------------------
 */
void X11Driver::read_span(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx-x+1;
    for (int i = 0; i < width; i++)
    {
        pixels[i] = read_pixel(drv, x+i, y);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * write_span --
 *
 *  Write a line of pixels to the screen.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Draws pixels.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::write_span(int y, int x, int lastx, BYTE *pixels)
{
    int width;
    BYTE *pixline;

#if 1
    if (x == lastx)
    {
        write_pixel(drv, x, y, pixels[0]);
        return;
    }
    width = lastx-x+1;
    if (m_use_pixtab)
    {
        for (int i = 0; i < width; i++)
        {
            pixbuf[i] = m_pixtab[pixels[i]];
        }
        pixline = pixbuf;
    }
    else
    {
        pixline = pixels;
    }
    for (int i = 0; i < width; i++)
    {
        XPutPixel(Ximage, x+i, y, do_fake_lut(pixline[i]));
    }
    if (m_fast_mode && g_help_mode != help_labels::HELP_PALETTE_EDITOR)
    {
        if (!m_alarm_on)
        {
            schedule_alarm(drv, 0);
        }
    }
    else
    {
        XPutImage(m_dpy, m_window, Xgc, Ximage, x, y, x, y, width, 1);
        if (m_on_root)
        {
            XPutImage(m_dpy, Xpixmap, Xgc, Ximage, x, y, x, y, width, 1);
        }
    }
#else
    width = lastx-x+1;
    for (int i = 0; i < width; i++)
    {
        write_pixel(x+i, y, pixels[i]);
    }
#endif
}

void X11Driver::get_truecolor(int x, int y, int *r, int *g, int *b, int *a)
{
}

void X11Driver::put_truecolor(int x, int y, int r, int g, int b, int a)
{
}

/*
 *----------------------------------------------------------------------
 *
 * set_line_mode --
 *
 *  Set line mode to 0=draw or 1=xor.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Sets mode.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::set_line_mode(int mode)
{
    xlastcolor = -1;
    if (mode == 0)
    {
        XSetFunction(m_dpy, Xgc, GXcopy);
        xlastfcn = GXcopy;
    }
    else
    {
        XSetForeground(m_dpy, Xgc, do_fake_lut(g_colors-1));
        xlastcolor = -1;
        XSetFunction(m_dpy, Xgc, GXxor);
        xlastfcn = GXxor;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * draw_line --
 *
 *  Draw a line.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Modifies window.
 *
 *----------------------------------------------------------------------
 */
void X11Driver::draw_line(int x1, int y1, int x2, int y2, int color)
{
    XDrawLine(m_dpy, m_window, Xgc, x1, y1, x2, y2);
}

void X11Driver::display_string(int x, int y, int fg, int bg, char const *text)
{
}

void X11Driver::save_graphics()
{
}

void X11Driver::restore_graphics()
{
}

/*----------------------------------------------------------------------
 *
 * get_key --
 *
 *  Get a key from the keyboard or the X server.
 *  Blocks if block = 1.
 *
 * Results:
 *  Key, or 0 if no key and not blocking.
 *  Times out after .5 second.
 *
 * Side effects:
 *  Processes X events.
 *
 *----------------------------------------------------------------------
 */
int X11Driver::get_key()
{
    int block = 1;
    static int skipcount = 0;

    while (1)
    {
        // Don't check X events every time, since that is expensive
        skipcount++;
        if (block == 0 && skipcount < 25)
            break;
        skipcount = 0;

        handle_events();

        if (key_buffer)
        {
            int ch = key_buffer;
            key_buffer = 0;
            skipcount = 9999; // If we got a key, check right away next time
            return translate_key(ch);
        }

        if (!block)
            break;

        {
            fd_set reads;
            struct timeval tout;
            int status;

            FD_ZERO(&reads);
            // See http://llvm.org/bugs/show_bug.cgi?id=8920
#if !defined(__clang_analyzer__)
            FD_SET(0, &reads);
#endif
            tout.tv_sec = 0;
            tout.tv_usec = 500000;

            // See http://llvm.org/bugs/show_bug.cgi?id=8920
#if !defined(__clang_analyzer__)
            FD_SET(ConnectionNumber(m_dpy), &reads);
#endif
            status = select(ConnectionNumber(m_dpy) + 1, &reads, nullptr, nullptr, &tout);

            if (status <= 0)
                return 0;
        }
    }

    return 0;
}

int X11Driver::key_cursor(int row, int col)
{
    return 0;
}

void X11Driver::flush_output()
{
    static time_t start = 0;
    static long ticks_per_second = 0;
    static long last = 0;
    static long frames_per_second = 10;

    if (!ticks_per_second)
    {
        if (!start)
        {
            time(&start);
            last = readticker();
        }
        else
        {
            time_t now = time(nullptr);
            long now_ticks = readticker();
            if (now > start)
            {
                ticks_per_second = (now_ticks - last)/((long)(now - start));
            }
        }
    }
    else
    {
        long now = readticker();
        if ((now - last)*frames_per_second > ticks_per_second)
        {
            flush();
            frame_.pump_messages(false);
            last = now;
        }
    }
}

int X11Driver::key_pressed()
{
    if (int const ch = key_buffer)
    {
        return ch;
    }

    flush_output();
    int const ch = handle_special_keys(frame_.get_key_press(0));
    key_buffer = ch;

    return ch;
}

int X11Driver::wait_key_pressed(int timeout)
{
    return 0;
}

void X11Driver::unget_key(int key)
{
}

/*
 *----------------------------------------------------------------------
 *
 * shell
 *
 *  Exit to a unix shell.
 *
 * Results:
 *  None.
 *
 * Side effects:
 *  Goes to shell
 *
 *----------------------------------------------------------------------
 */
void X11Driver::shell()
{
    SignalHandler sigint;
    std::string shell{getenv("SHELL")};
    int pid, donepid;

    sigint = (SignalHandler) signal(SIGINT, SIG_IGN);
    if (shell.empty())
    {
        shell = SHELL;
    }

    char *argv0 = strdup(shell.c_str());
    char *const argv[2] = { argv0, nullptr };

    // Fork the shell; it should be something like an xterm
    pid = fork();
    if (pid < 0)
        perror("fork to shell");
    if (pid == 0)
    {
        execvp(shell.c_str(), argv);
        perror("fork to shell");
        exit(1);
    }
    free(argv0);

    // Wait for the shell to finish
    while (1)
    {
        donepid = wait(nullptr);
        if (donepid < 0 || donepid == pid)
            break;
    }

    signal(SIGINT, (SignalHandler) sigint);
    putchar('\n');
}

void X11Driver::set_video_mode(VIDEOINFO *mode)
{
    if (g_disk_flag)
        enddisk();
    end_video();
    g_good_mode = true;
    switch (g_dot_mode)
    {
    case 0:               // text
        break;

    case 19: // X window
        start_video();
        set_for_graphics();
        break;

    default:
        std::printf("Bad mode %d\n", g_dot_mode);
        exit(-1);
    }
    if (g_dot_mode != 0)
    {
        read_palette();
        g_and_color = g_colors-1;
        g_box_count =0;
    }
}

void X11Driver::put_string(int row, int col, int attr, char const *msg)
{
    assert(text_not_graphics);
    if (row != -1)
    {
        g_text_row = row;
    }
    if (col != -1)
    {
        g_text_col = col;
    }

    int r = g_text_rbase + g_text_row;
    int c = g_text_cbase + g_text_col;
    assert(r >= 0 && r < X11_TEXT_MAX_ROW);
    assert(c >= 0 && c < X11_TEXT_MAX_COL);
    text_.put_string(c, r, attr, msg, &g_text_row, &g_text_col);
}

bool X11Driver::is_text()
{
    return text_not_graphics;
}

void X11Driver::set_for_text()
{
    text_not_graphics = true;
    text_.show();
    plot_.hide();
}

void X11Driver::set_for_graphics()
{
    text_not_graphics = false;
    plot_.show();
    text_.hide();
}

void X11Driver::set_clear()
{
    if (text_not_graphics)
    {
        text_.clear();
    }
    else
    {
        plot_.clear();
    }
}

void X11Driver::move_cursor(int row, int col)
{
    // TODO: draw reverse video text cursor at new position
    std::fprintf(stderr, "X11Driver::move_cursor(%d,%d)\n", row, col);
}

void X11Driver::hide_text_cursor()
{
    // TODO: erase cursor if currently drawn
    std::fprintf(stderr, "X11Driver::hide_text_cursor\n");
}

void X11Driver::set_attr(int row, int col, int attr, int count)
{
    int i = col;

    while (count)
    {
        assert(row < TEXT_HEIGHT);
        assert(i < TEXT_WIDTH);
        text_attr[row][i] = attr;
        if (++i == TEXT_WIDTH)
        {
            i = 0;
            row++;
        }
        count--;
    }
    // TODO: refresh text
    std::fprintf(stderr, "X11Driver::set_attr(%d,%d, %d): %d\n", row, col, count, attr);
}

void X11Driver::scroll_up(int top, int bot)
{
    assert(bot <= TEXT_HEIGHT);
    for (int r = top; r < bot; r++)
        for (int c = 0; c < TEXT_WIDTH; c++)
        {
            text_attr[r][c] = text_attr[r+1][c];
            text_screen[r][c] = text_screen[r+1][c];
        }
    for (int c = 0; c < TEXT_WIDTH; c++)
    {
        text_attr[bot][c] = 0;
        text_screen[bot][c] = ' ';
    }
    // TODO: draw text
    std::fprintf(stderr, "X11Driver::scroll_up(%d, %d)\n", top, bot);
}

void X11Driver::stack_screen()
{
    // TODO
    std::fprintf(stderr, "X11Driver::stack_screen\n");
}

void X11Driver::unstack_screen()
{
    // TODO
    std::fprintf(stderr, "X11Driver::unstack_screen\n");
}

void X11Driver::discard_screen()
{
    // TODO
    std::fprintf(stderr, "X11Driver::discard_screen\n");
}

int X11Driver::init_fm()
{
    // TODO
    return 0;
}

void X11Driver::buzzer(buzzer_codes kind)
{
    // TODO
    std::fprintf(stderr, "X11Driver::buzzer(%d)\n", static_cast<int>(kind));
}

bool X11Driver::sound_on(int freq)
{
    // TODO
    std::fprintf(stderr, "X11Driver::sound_on(%d)\n", freq);
    return false;
}

void X11Driver::sound_off()
{
    // TODO
    std::fprintf(stderr, "X11Driver::sound_off\n");
}

void X11Driver::mute()
{
    // TODO
}

bool X11Driver::diskp()
{
    // TODO
    return false;
}

int X11Driver::get_char_attr()
{
    // TODO
    return 0;
}

void X11Driver::put_char_attr(int char_attr)
{
    // TODO
}

void X11Driver::delay(int ms)
{
    // TODO
}

void X11Driver::get_max_screen(int *width, int *height)
{
    // TODO
}

void X11Driver::set_keyboard_timeout(int ms)
{
    // TODO
}

static X11Driver x11_driver_info;

Driver *x11_driver = &x11_driver_info;
