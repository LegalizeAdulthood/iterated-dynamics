// SPDX-License-Identifier: GPL-3.0-only
//
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
#include "mouse.h"
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
#include <ctime>
#include <string>
#include <vector>

#include "x11_frame.h"
#include "x11_text.h"
#include "x11_plot.h"
#include "intro.h"

#ifdef LINUX
#define FNDELAY O_NDELAY
#endif

// external variables (set in the id.cfg file, but findable here

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
constexpr const int DEFAULT_WIDTH{640};
constexpr const int DEFAULT_HEIGHT{480};
constexpr const char *const DEFXY{"640x480+0+0"};

// The pixtab stuff is so we can map from pixel values 0-n to
// the actual color table entries which may be anything.
//
class X11Driver : public Driver
{
public:
    ~X11Driver() override = default;

    const std::string &get_name() const
    {
        return m_name;
    }
    const std::string &get_description() const
    {
        return m_description;
    }
    bool init(int *argc, char **argv) override;
    bool validate_mode(VIDEOINFO *mode) override;
    void get_max_screen(int &width, int &height) override;
    void terminate() override;
    void pause() override;
    void resume() override;
    void schedule_alarm(int secs) override;
    void create_window() override;
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
    void set_for_graphics() override;
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
    bool diskp() const override;
    int get_char_attr() override;
    void put_char_attr(int char_attr) override;
    void delay(int ms) override;
    void set_keyboard_timeout(int ms) override;
    void flush() override;
    void debug_text(const char *text) override;
    void get_cursor_pos(int &x, int &y) const override
    {
        // TODO
        x = 0;
        y = 0;
    }

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
    int end_video();
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

    std::string m_name{"x11"};
    std::string m_description{"X11 Window"};
    bool m_on_root{};                 //
    bool m_full_screen{};             //
    bool m_share_color{};             //
    bool m_private_color{};           //
    int m_fix_colors{};               //
    bool m_sync{};                    // Run X events synchronously (debugging)
    std::string m_display;            //
    std::string m_geometry;           //
    bool m_use_pixtab{};              //
    unsigned long m_pixtab[256]{};    //
    int m_inv_pixtab[256]{};          //
    XPixel m_cmap_pixtab[256]{};      // for faking a LUTs on non-LUT visuals
    bool m_have_cmap_pixtab{};        //
    bool m_fake_lut{};                //
    bool m_fast_mode{};               // Don't draw pixels 1 at a time
    bool m_alarm_on{};                // true if the refresh alarm is on
    bool m_need_redraw{};             // true if we have a redraw waiting
    Display *m_dpy{};                 //
    Window m_window{None};            //
    GC m_gc{None};                    //
    Visual *m_visual{};               //
    Screen *m_screen{};               //
    Colormap m_colormap{None};        //
    int m_depth{};                    //
    XImage *m_image{};                //
    int m_dpy_screen{};               //
    Pixmap m_pixmap{None};            //
    int m_min_width{DEFAULT_WIDTH};   //
    int m_min_height{DEFAULT_HEIGHT}; //
    Window m_root_window{None};       //
    int xlastcolor{-1};               //
    int xlastfcn{GXcopy};             //
    std::vector<BYTE> m_pixels;       //
    XColor m_colors[256]{};           //
    std::string x_font_name{FONT};    //
    XFontStruct *m_font_info{};       //
    int m_key_buffer{};               // Buffered X key
    char m_text_screen[TEXT_HEIGHT][TEXT_WIDTH]{};
    int m_text_attr[TEXT_HEIGHT][TEXT_WIDTH]{};
    bool m_text_not_graphics{}; // true when displaying text
    bool m_ctl_mode{};          // rubber banding and event processing data
    bool m_shift_mode{};        //
    int m_button_num{};         //
    int m_last_x{};             //
    int m_last_y{};             //
    int m_dx{};                 //
    int m_dy{};                 //
    x11_frame_window m_frame;   //
    x11_text_window m_text;     //
    x11_plot_window m_plot;     //
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
        g_slow_display = true;
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

    if (m_gc)
        XFreeGC(m_dpy, m_gc);

    if (m_pixmap)
    {
        XFreePixmap(m_dpy, m_pixmap);
        m_pixmap = None;
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
            m_text_attr[r][c] = 0;
            m_text_screen[r][c] = ' ';
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
    m_visual = XDefaultVisualOfScreen(m_screen);
    m_depth = DefaultDepth(m_dpy, m_dpy_screen);

    switch (m_visual->c_class)
    {
    case StaticGray:
    case StaticColor:
        g_colors = (m_depth <= 8) ? m_visual->map_entries : 256;
        g_got_real_dac = false;
        m_fake_lut = false;
        break;

    case GrayScale:
    case PseudoColor:
        g_colors = (m_depth <= 8) ? m_visual->map_entries : 256;
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
        for (int j = 0; j < m_image->height; j++)
            for (int i = 0; i < m_image->width; i++)
                XPutPixel(m_image, i, j, m_cmap_pixtab[m_pixtab[0]]);
    }
    else if (m_pixtab[0] != 0)
    {
        /*
         * Initialize image to m_pixtab[0].
         */
        if (g_colors == 2)
        {
            for (int i = 0; i < m_image->bytes_per_line; i++)
            {
                m_image->data[i] = 0xff;
            }
        }
        else
        {
            for (int i = 0; i < m_image->bytes_per_line; i++)
            {
                m_image->data[i] = m_pixtab[0];
            }
        }
        for (int i = 1; i < m_image->height; i++)
        {
            std::memcpy(
                m_image->data + i*m_image->bytes_per_line,
                m_image->data,
                m_image->bytes_per_line);
        }
    }
    else
    {
        /*
         * Initialize image to 0's.
         */
        std::memset(m_image->data, 0, m_image->bytes_per_line*m_image->height);
    }
    xlastcolor = -1;
    XSetForeground(m_dpy, m_gc, do_fake_lut(m_pixtab[0]));
    if (m_on_root)
        XFillRectangle(m_dpy, m_pixmap, m_gc,
                       0, 0, m_min_width, m_min_height);
    XFillRectangle(m_dpy, m_window, m_gc,
                   0, 0, m_min_width, m_min_height);
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
        m_colormap = DefaultColormapOfScreen(m_screen);
        if (m_fake_lut)
            write_palette();
    }
    else if (m_share_color)
    {
        g_got_real_dac = false;
    }
    else if (m_private_color)
    {
        m_colormap = XCreateColormap(m_dpy, m_window, m_visual, AllocAll);
        XSetWindowColormap(m_dpy, m_window, m_colormap);
    }
    else
    {
        m_colormap = DefaultColormap(m_dpy, m_dpy_screen);
        for (int powr = m_depth; powr >= 1; powr--)
        {
            ncells = 1 << powr;
            if (ncells > g_colors)
                continue;
            if (XAllocColorCells(m_dpy, m_colormap, False, nullptr, 0, m_pixtab,
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

    if (!g_got_real_dac && g_colors == 2 && BlackPixelOfScreen(m_screen) != 0)
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
        m_ctl_mode = true;
        return 1;

    case XK_Shift_L:
    case XK_Shift_R:
        m_shift_mode = true;
        break;
    case XK_Home:
    case XK_R7:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_HOME : ID_KEY_HOME;
        return 1;
    case XK_Left:
    case XK_R10:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_LEFT_ARROW : ID_KEY_LEFT_ARROW;
        return 1;
    case XK_Right:
    case XK_R12:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_RIGHT_ARROW : ID_KEY_RIGHT_ARROW;
        return 1;
    case XK_Down:
    case XK_R14:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_DOWN_ARROW : ID_KEY_DOWN_ARROW;
        return 1;
    case XK_Up:
    case XK_R8:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_UP_ARROW : ID_KEY_UP_ARROW;
        return 1;
    case XK_Insert:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_INSERT : ID_KEY_INSERT;
        return 1;
    case XK_Delete:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_DEL : ID_KEY_DELETE;
        return 1;
    case XK_End:
    case XK_R13:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_END : ID_KEY_END;
        return 1;
    case XK_Help:
        m_key_buffer = ID_KEY_F1;
        return 1;
    case XK_Prior:
    case XK_R9:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_PAGE_UP : ID_KEY_PAGE_UP;
        return 1;
    case XK_Next:
    case XK_R15:
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_PAGE_DOWN : ID_KEY_PAGE_DOWN;
        return 1;
    case XK_F1:
    case XK_L1:
        m_key_buffer = m_shift_mode ? ID_KEY_SF1: ID_KEY_F1;
        return 1;
    case XK_F2:
    case XK_L2:
        m_key_buffer = m_shift_mode ? ID_KEY_SF2: ID_KEY_F2;
        return 1;
    case XK_F3:
    case XK_L3:
        m_key_buffer = m_shift_mode ? ID_KEY_SF3: ID_KEY_F3;
        return 1;
    case XK_F4:
    case XK_L4:
        m_key_buffer = m_shift_mode ? ID_KEY_SF4: ID_KEY_F4;
        return 1;
    case XK_F5:
    case XK_L5:
        m_key_buffer = m_shift_mode ? ID_KEY_SF5: ID_KEY_F5;
        return 1;
    case XK_F6:
    case XK_L6:
        m_key_buffer = m_shift_mode ? ID_KEY_SF6: ID_KEY_F6;
        return 1;
    case XK_F7:
    case XK_L7:
        m_key_buffer = m_shift_mode ? ID_KEY_SF7: ID_KEY_F7;
        return 1;
    case XK_F8:
    case XK_L8:
        m_key_buffer = m_shift_mode ? ID_KEY_SF8: ID_KEY_F8;
        return 1;
    case XK_F9:
    case XK_L9:
        m_key_buffer = m_shift_mode ? ID_KEY_SF9: ID_KEY_F9;
        return 1;
    case XK_F10:
    case XK_L10:
        m_key_buffer = m_shift_mode ? ID_KEY_SF10: ID_KEY_F10;
        return 1;
    case '+':
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_PLUS : '+';
        return 1;
    case '-':
        m_key_buffer = m_ctl_mode ? ID_KEY_CTL_MINUS : '-';
        return 1;
        break;
    case XK_Return:
    case XK_KP_Enter:
        m_key_buffer = m_ctl_mode ? ctl('T') : '\n';
        return 1;
    }
    if (charcount == 1)
    {
        m_key_buffer = buffer[0];
        if (m_key_buffer == '\003')
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
        m_ctl_mode = false;
        break;
    case XK_Shift_L:
    case XK_Shift_R:
        m_shift_mode = false;
        break;
    }
}

void X11Driver::ev_expose(XExposeEvent *xevent)
{
    if (m_text_not_graphics)
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
            XPutImage(m_dpy, m_window, m_gc, m_image, x, y, x, y,
                      xevent->width, xevent->height);
        }
    }
}

void X11Driver::ev_button_press(XEvent *xevent)
{
    bool done = false;
    bool banding = false;
    int bandx0, bandy0, bandx1, bandy1;

    if (g_look_at_mouse == +MouseLook::POSITION || !g_zoom_off)
    {
        m_last_x = xevent->xbutton.x;
        m_last_y = xevent->xbutton.y;
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
                XDrawRectangle(m_dpy, m_window, m_gc, MIN(bandx0, bandx1),
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
                    XSetForeground(m_dpy, m_gc, do_fake_lut(g_colors-1));
                    XSetFunction(m_dpy, m_gc, GXxor);
                }
            }
            if (banding)
            {
                XDrawRectangle(m_dpy, m_window, m_gc, MIN(bandx0, bandx1),
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

    XDrawRectangle(m_dpy, m_window, m_gc, MIN(bandx0, bandx1),
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
        m_key_buffer = ID_KEY_ENTER;
    if (xlastcolor != -1)
        XSetForeground(m_dpy, m_gc, do_fake_lut(xlastcolor));
    XSetFunction(m_dpy, m_gc, xlastfcn);
    drawbox(0);
}

void X11Driver::ev_motion_notify(XEvent *xevent)
{
    if (g_cursor_mouse_tracking && !g_inside_help)
    {
        while (XCheckWindowEvent(m_dpy, m_window, PointerMotionMask, xevent))
            ;

        if (xevent->xmotion.state & Button2Mask ||
                (xevent->xmotion.state & (Button1Mask | Button3Mask)))
        {
            m_button_num = 3;
        }
        else if (xevent->xmotion.state & Button1Mask)
        {
            m_button_num = 1;
        }
        else if (xevent->xmotion.state & Button3Mask)
        {
            m_button_num = 2;
        }
        else
        {
            m_button_num = 0;
        }

        if (g_look_at_mouse == +MouseLook::POSITION && m_button_num != 0)
        {
            m_dx += (xevent->xmotion.x-m_last_x)/MOUSE_SCALE;
            m_dy += (xevent->xmotion.y-m_last_y)/MOUSE_SCALE;
            m_last_x = xevent->xmotion.x;
            m_last_y = xevent->xmotion.y;
        }
        else
        {
            //Cursor_SetPos(xevent->xmotion.x, xevent->xmotion.y);
            m_key_buffer = ID_KEY_ENTER;
        }
    }
}

void X11Driver::handle_events()
{
    XEvent xevent;

    if (m_need_redraw)
        redraw();

    while (XPending(m_dpy) && !m_key_buffer)
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

    if (!m_key_buffer && g_cursor_mouse_tracking && !g_inside_help && g_look_at_mouse == +MouseLook::POSITION &&
            (m_dx != 0 || m_dy != 0))
    {
        if (ABS(m_dx) > ABS(m_dy))
        {
            if (m_dx > 0)
            {
                m_key_buffer = mousefkey[m_button_num][0]; // right
                m_dx--;
            }
            else if (m_dx < 0)
            {
                m_key_buffer = mousefkey[m_button_num][1]; // left
                m_dx++;
            }
        }
        else
        {
            if (m_dy > 0)
            {
                m_key_buffer = mousefkey[m_button_num][2]; // down
                m_dy--;
            }
            else if (m_dy < 0)
            {
                m_key_buffer = mousefkey[m_button_num][3]; // up
                m_dy++;
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
        return RootWindow(dpy, m_dpy_screen);
    }
    if (XQueryTree(dpy, pwin, &root, &parent, &child, &nchild))
    {
        for (unsigned int i = 0U; i < nchild; i++)
        {
            if (!XGetWindowAttributes(dpy, child[i], &cxwa))
            {
                std::printf("Search for root: XGetWindowAttributes failed\n");
                return RootWindow(dpy, m_dpy_screen);
            }
            if (pxwa.width == cxwa.width && pxwa.height == cxwa.height)
                return pr_dwmroot(dpy, child[i]);
        }
        return pwin;
    }
    else
    {
        std::printf("Id: failed to find root window\n");
        return RootWindow(dpy, m_dpy_screen);
    }
}

Window X11Driver::FindRootWindow()
{
    m_root_window = RootWindow(m_dpy, m_dpy_screen);
    m_root_window = pr_dwmroot(m_dpy, m_root_window); // search for DEC wm root

    {   // search for swm/tvtwm root (from ssetroot by Tom LaStrange)
        Window rootReturn, parentReturn, *children;
        unsigned int numChildren;

        Atom SWM_VROOT = XInternAtom(m_dpy, "__SWM_VROOT", False);
        XQueryTree(m_dpy, m_root_window, &rootReturn, &parentReturn,
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
                m_root_window = *newRoot;
                break;
            }
        }
    }
    return m_root_window;
}

void X11Driver::RemoveRootPixmap()
{
    Atom prop, type;
    int format;
    unsigned long nitems, after;
    Pixmap *pm;

    prop = XInternAtom(m_dpy, "_XSETROOT_ID", False);
    if (XGetWindowProperty(m_dpy, m_root_window, prop, 0L, 1L, 1,
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
    m_font_info = XLoadQueryFont(m_dpy, x_font_name.c_str());
    if (m_font_info == nullptr)
        m_font_info = XLoadQueryFont(m_dpy, "6x12");
}

void X11Driver::flush()
{
    XSync(m_dpy, False);
}

void X11Driver::debug_text(const char *text)
{
    std::fprintf(stderr, "%s", text);
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
    m_dpy_screen = XDefaultScreen(m_dpy);
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
                add_video_mode(this, &m);
            }
        }
    }

    int screen_num = DefaultScreen(m_dpy);
    m_frame.initialize(m_dpy, screen_num, m_geometry.c_str());
    m_plot.initialize(m_dpy, screen_num, m_frame.window());
    m_text.initialize(m_dpy, screen_num, m_frame.window());

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
    *width = m_text.max_width();
    *height = m_text.max_height();
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
        plot_pos.x = (m_frame.width() - m_plot.width())/2;
    }
    else
    {
        text_pos.x = (m_frame.width() - m_text.max_width())/2;
    }

    if (center_y)
    {
        plot_pos.y = (m_frame.height() - m_plot.height())/2;
    }
    else
    {
        text_pos.y = (m_frame.height() - m_text.max_height())/2;
    }

    m_plot.set_position(plot_pos.x, plot_pos.y);
    m_text.set_position(text_pos.x, text_pos.y);
}

void frame_window(int width, int height)
{
}

void X11Driver::create_window()
{
    unsigned width;
    unsigned height;
    bool center_x = true;
    bool center_y = true;
    get_max_size(&width, &height, &center_x, &center_y);
    m_frame.window(width, height);
    m_text.text_on();
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
        XGeometry(m_dpy, m_dpy_screen, m_geometry.c_str(), DEFXY, 0, 1, 1, 0, 0,
                  &Xwinx, &Xwiny, &m_min_width, &m_min_height);
    if (m_sync)
        XSynchronize(m_dpy, True);
    XSetErrorHandler(errhand);
    m_screen = ScreenOfDisplay(m_dpy, m_dpy_screen);
    select_visual();
    if (m_fix_colors > 0)
        g_colors = m_fix_colors;

    if (m_full_screen || m_on_root)
    {
        m_min_width = DisplayWidth(m_dpy, m_dpy_screen);
        m_min_height = DisplayHeight(m_dpy, m_dpy_screen);
    }
    g_screen_x_dots = m_min_width;
    g_screen_y_dots = m_min_height;

    Xwatt.background_pixel = BlackPixelOfScreen(m_screen);
    Xwatt.bit_gravity = StaticGravity;
    const int doesBacking = DoesBackingStore(m_screen);
    if (doesBacking)
        Xwatt.backing_store = Always;
    else
        Xwatt.backing_store = NotUseful;
    if (m_on_root)
    {
        m_root_window = FindRootWindow();
        RemoveRootPixmap();
        m_gc = XCreateGC(m_dpy, m_root_window, 0, &Xgcvals);
        m_pixmap = XCreatePixmap(m_dpy, m_root_window,
                                    m_min_width, m_min_height, m_depth);
        m_window = m_root_window;
        XFillRectangle(m_dpy, m_pixmap, m_gc, 0, 0, m_min_width, m_min_height);
        XSetWindowBackgroundPixmap(m_dpy, m_root_window, m_pixmap);
    }
    else
    {
        m_root_window = DefaultRootWindow(m_dpy);
        m_window = XCreateWindow(m_dpy, m_root_window, Xwinx, Xwiny,
                               m_min_width, m_min_height, 0, m_depth,
                               InputOutput, CopyFromParent,
                               CWBackPixel | CWBitGravity | CWBackingStore,
                               &Xwatt);
        XStoreName(m_dpy, m_window, "id");
        m_gc = XCreateGC(m_dpy, m_window, 0, &Xgcvals);
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
        XSetBackground(m_dpy, m_gc, do_fake_lut(m_pixtab[0]));
        XSetForeground(m_dpy, m_gc, do_fake_lut(m_pixtab[1]));
        Xwatt.background_pixel = do_fake_lut(m_pixtab[0]);
        XChangeWindowAttributes(m_dpy, m_window, CWBackPixel, &Xwatt);
        XMapWindow(m_dpy, m_window);
    }

    resize();
    flush();
    write_palette();

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
        m_min_width = g_screen_x_dots;
        m_min_height = g_screen_y_dots;
        g_screen_aspect = g_screen_y_dots/(float) g_screen_x_dots;
        g_final_aspect_ratio = g_screen_aspect;
        int Xpad = 9;
        int Xmwidth;
        if (m_depth == 1)
        {
            Xmwidth = 1 + g_screen_x_dots/8;
        }
        else if (m_depth <= 8)
        {
            Xmwidth = g_screen_x_dots;
        }
        else if (m_depth <= 16)
        {
            Xmwidth = 2*g_screen_x_dots;
            Xpad = 16;
        }
        else
        {
            Xmwidth = 4*g_screen_x_dots;
            Xpad = 32;
        }
        m_pixels.resize(m_min_width);
        if (m_image != nullptr)
        {
            free(m_image->data);
            XDestroyImage(m_image);
        }
        m_image = XCreateImage(m_dpy, m_visual, m_depth, ZPixmap, 0, nullptr, g_screen_x_dots,
                                  g_screen_y_dots, Xpad, Xmwidth);
        if (m_image == nullptr)
        {
            std::printf("XCreateImage failed\n");
            terminate();
            exit(-1);
        }
        m_image->data = (char *) malloc(m_image->bytes_per_line * m_image->height);
        if (m_image->data == nullptr)
        {
            std::fprintf(stderr, "Malloc failed: %d\n", m_image->bytes_per_line *
                    m_image->height);
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
        XPutImage(m_dpy, m_window, m_gc, m_image, 0, 0, 0, 0,
                  g_screen_x_dots, g_screen_y_dots);
        if (m_on_root)
            XPutImage(m_dpy, m_pixmap, m_gc, m_image, 0, 0, 0, 0,
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
        g_dac_box[i][0] = m_colors[i].red/1024;
        g_dac_box[i][1] = m_colors[i].green/1024;
        g_dac_box[i][2] = m_colors[i].blue/1024;
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
                    m_colors[i].flags = DoRed | DoGreen | DoBlue;
                    m_colors[i].red = g_dac_box[i][0]*1024;
                    m_colors[i].green = g_dac_box[i][1]*1024;
                    m_colors[i].blue = g_dac_box[i][2]*1024;

                    if (m_have_cmap_pixtab)
                    {
                        XFreeColors(m_dpy, m_colormap, m_cmap_pixtab + i, 1, None);
                    }
                    if (XAllocColor(m_dpy, m_colormap, &m_colors[i]))
                    {
                        m_cmap_pixtab[i] = m_colors[i].pixel;
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
            m_colors[i].pixel = m_pixtab[i];
            m_colors[i].flags = DoRed | DoGreen | DoBlue;
            m_colors[i].red = g_dac_box[i][0]*1024;
            m_colors[i].green = g_dac_box[i][1]*1024;
            m_colors[i].blue = g_dac_box[i][2]*1024;
        }
        XStoreColors(m_dpy, m_colormap, m_colors, g_colors);
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
        XPixel pixel = XGetPixel(m_image, x, y);
        for (int i = 0; i < 256; i++)
            if (m_cmap_pixtab[i] == pixel)
                return i;
        return 0;
    }
    else
        return m_inv_pixtab[XGetPixel(m_image, x, y)];
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
        XSetForeground(m_dpy, m_gc, do_fake_lut(m_pixtab[color]));
        xlastcolor = color;
    }
    XPutPixel(m_image, x, y, do_fake_lut(m_pixtab[color]));
    if (m_fast_mode && g_help_mode != help_labels::HELP_PALETTE_EDITOR)
    {
        if (!m_alarm_on)
        {
            schedule_alarm(0);
        }
    }
    else
    {
        XDrawPoint(m_dpy, m_window, m_gc, x, y);
        if (m_on_root)
        {
            XDrawPoint(m_dpy, m_pixmap, m_gc, x, y);
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
        pixels[i] = read_pixel(x+i, y);
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
    const BYTE *pixline;

#if 1
    if (x == lastx)
    {
        write_pixel(x, y, pixels[0]);
        return;
    }
    width = lastx-x+1;
    if (m_use_pixtab)
    {
        m_pixels.resize(width);
        for (int i = 0; i < width; i++)
        {
            m_pixels[i] = m_pixtab[pixels[i]];
        }
        pixline = m_pixels.data();
    }
    else
    {
        pixline = pixels;
    }
    for (int i = 0; i < width; i++)
    {
        XPutPixel(m_image, x+i, y, do_fake_lut(pixline[i]));
    }
    if (m_fast_mode && g_help_mode != help_labels::HELP_PALETTE_EDITOR)
    {
        if (!m_alarm_on)
        {
            schedule_alarm(0);
        }
    }
    else
    {
        XPutImage(m_dpy, m_window, m_gc, m_image, x, y, x, y, width, 1);
        if (m_on_root)
        {
            XPutImage(m_dpy, m_pixmap, m_gc, m_image, x, y, x, y, width, 1);
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
        XSetFunction(m_dpy, m_gc, GXcopy);
        xlastfcn = GXcopy;
    }
    else
    {
        XSetForeground(m_dpy, m_gc, do_fake_lut(g_colors-1));
        xlastcolor = -1;
        XSetFunction(m_dpy, m_gc, GXxor);
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
    XDrawLine(m_dpy, m_window, m_gc, x1, y1, x2, y2);
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

        if (m_key_buffer)
        {
            int ch = m_key_buffer;
            m_key_buffer = 0;
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
    static std::time_t start = 0;
    static long ticks_per_second = 0;
    static long last = 0;
    static long frames_per_second = 10;

    if (!ticks_per_second)
    {
        if (!start)
        {
            std::time(&start);
            last = readticker();
        }
        else
        {
            std::time_t now = std::time(nullptr);
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
            m_frame.pump_messages(false);
            last = now;
        }
    }
}

int X11Driver::key_pressed()
{
    if (int const ch = m_key_buffer)
    {
        return ch;
    }

    flush_output();
    int const ch = handle_special_keys(m_frame.get_key_press(0));
    m_key_buffer = ch;

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
    assert(m_text_not_graphics);
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
    m_text.put_string(c, r, attr, msg, &g_text_row, &g_text_col);
}

bool X11Driver::is_text()
{
    return m_text_not_graphics;
}

void X11Driver::set_for_text()
{
    m_text_not_graphics = true;
    m_text.show();
    m_plot.hide();
}

void X11Driver::set_for_graphics()
{
    m_text_not_graphics = false;
    m_plot.show();
    m_text.hide();
}

void X11Driver::set_clear()
{
    if (m_text_not_graphics)
    {
        m_text.clear();
    }
    else
    {
        m_plot.clear();
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
        m_text_attr[row][i] = attr;
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
            m_text_attr[r][c] = m_text_attr[r+1][c];
            m_text_screen[r][c] = m_text_screen[r+1][c];
        }
    for (int c = 0; c < TEXT_WIDTH; c++)
    {
        m_text_attr[bot][c] = 0;
        m_text_screen[bot][c] = ' ';
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

bool X11Driver::diskp() const
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

void X11Driver::get_max_screen(int &width, int &height)
{
    // TODO
}

void X11Driver::set_keyboard_timeout(int ms)
{
    // TODO
}

static X11Driver x11_driver_info;

Driver *x11_driver = &x11_driver_info;
