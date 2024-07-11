//
// Edits VGA 256-color palettes.
//
//
#include "editpal.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "dir_file.h"
#include "drivers.h"
#include "field_prompt.h"
#include "find_special_colors.h"
#include "get_color.h"
#include "get_key_no_help.h"
#include "get_line.h"
#include "id_data.h"
#include "id_keys.h"
#include "memory.h"
#include "os.h"
#include "read_ticker.h"
#include "rotate.h"
#include "spindac.h"
#include "value_saver.h"
#include "zoom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <system_error>
#include <vector>

// misc. #defines
//

enum
{
    FONT_DEPTH = 8,  // font size
    CSIZE_MIN = 8,   // csize cannot be smaller than this
    CURSOR_SIZE = 5, // length of one side of the x-hair cursor
    BOX_INC = 1,
    CSIZE_INC = 2,
#ifndef XFRACT
    CURSOR_BLINK_RATE = 3, // timer ticks between cursor blinks
#else
    CURSOR_BLINK_RATE = 300, // timer ticks between cursor blinks
#endif
    FAR_RESERVE = 8192L, // amount of mem we will leave avail.
    TITLE_LEN = 17,
    CEditor_WIDTH = 8 * 3 + 4,
    CEditor_DEPTH = 8 + 4,
    STATUS_LEN = 4,
    CURS_INC = 1,
    RGBEditor_WIDTH = 62,
    RGBEditor_DEPTH = 1 + 1 + CEditor_DEPTH * 3 - 2 + 2,
    RGBEditor_BWIDTH = RGBEditor_WIDTH - (2 + CEditor_WIDTH + 1 + 2),
    RGBEditor_BDEPTH = RGBEditor_DEPTH - 4,
    PalTable_PALX = 1,
    PalTable_PALY = 2 + RGBEditor_DEPTH + 2,
    UNDO_DATA = 1,
    UNDO_DATA_SINGLE = 2,
    UNDO_ROTATE = 3,
};

constexpr int MAX_WIDTH{1024}; // palette editor cannot be wider than this

// basic data types
struct PALENTRY
{
    BYTE red;
    BYTE green;
    BYTE blue;
};

//
// Class:     Cursor
//
// Purpose:   Draw the blinking cross-hair cursor.
//
// Note:      Only one Cursor exists (referenced through the_cursor).
//            IMPORTANT: Call Cursor_Construct before you use any other
//            Cursor_ function!
//
struct Cursor
{
    int x;
    int y;
    int     hidden;       // >0 if mouse hidden
    long    last_blink;
    bool blink;
    char    t[CURSOR_SIZE];        // save line segments here
    char    b[CURSOR_SIZE];
    char    l[CURSOR_SIZE];
    char    r[CURSOR_SIZE];
};

//
// Class:     MoveBox
//
// Purpose:   Handles the rectangular move/resize box.
//
class MoveBox
{
public:
    MoveBox() = default;
    MoveBox(int x, int y, int csize, int base_width, int base_depth);

    void move(int key);
    bool process(); // returns false if ESCAPED
    bool moved() const
    {
        return m_moved;
    }
    bool should_hide() const
    {
        return m_should_hide;
    }
    int x() const
    {
        return m_x;
    }
    int y() const
    {
        return m_y;
    }
    int csize() const
    {
        return m_csize;
    }
    void set_pos(int x_, int y_)
    {
        m_x = x_;
        m_y = y_;
    }
    void set_csize(int csize_)
    {
        m_csize = csize_;
    }

private:
    void draw();
    void erase();

    int m_x{};
    int m_y{};
    int m_base_width{};
    int m_base_depth{};
    int m_csize{};
    bool m_moved{};
    bool m_should_hide{};
    std::vector<char> m_t;
    std::vector<char> m_b;
    std::vector<char> m_l;
    std::vector<char> m_r;
};

//
// Class:     CEditor
//
// Purpose:   Edits a single color component (R, G or B)
//
// Note:      Calls the "other_key" function to process keys it doesn't use.
//            The "change" function is called whenever the value is changed
//            by the CEditor.
//
class CEditor
{
public:
    CEditor() = default;
    CEditor(int x, int y, char letter, void (*other_key)(int, CEditor *, void *),
        void (*change)(CEditor *, void *), void *info);

    void draw();
    void set_pos(int x, int y);
    void set_val(int val);
    int get_val() const;
    void set_done(bool done);
    void set_hidden(bool hidden);
    int edit();

private:
    int m_x{};
    int m_y{};
    char m_letter{};
    int m_val{};
    bool m_done{};
    bool m_hidden{};
    void (*m_other_key)(int key, CEditor *ce, void *info){};
    void (*m_change)(CEditor *ce, void *info){};
    void *m_info{};
};

//
// Class:     RGBEditor
//
// Purpose:   Edits a complete color using three CEditors for R, G and B
//
class RGBEditor
{
public:
    RGBEditor() = default;
    RGBEditor(int x, int y, void (*other_key)(int, RGBEditor *, void *), void (*change)(RGBEditor *, void *),
        void *info);
    ~RGBEditor();

    void set_done(bool done);
    void set_pos(int x, int y);
    void set_hidden(bool hidden);
    void blank_sample_box();
    void update();
    void draw();
    int edit();
    void set_rgb(int pal, PALENTRY *rgb);
    PALENTRY get_rgb() const;

private:
    int m_x{};
    int m_y{};
    int m_curr{}; // 0=r, 1=g, 2=b
    int m_pal{};  // palette number
    bool m_done{};
    bool m_hidden{};
    CEditor *m_color[3]{}; // color editors 0=r, 1=g, 2=b
    void (*m_other_key)(int key, RGBEditor *e, void *info){};
    void (*m_change)(RGBEditor *e, void *info){};
    void *m_info{};
    void change(CEditor *editor);
    void other_key(int key, CEditor *ceditor);

    static void other_key_cb(int key, CEditor *ceditor, void *info);
    static void change_cb(CEditor *ceditor, void *info);
};

//
// Class:     PalTable
//
// Purpose:   This is where it all comes together.  Creates the two RGBEditors
//            and the palette. Moves the cursor, hides/restores the screen,
//            handles (S)hading, (C)opying, e(X)clude mode, the "Y" exclusion
//            mode, (Z)oom option, (H)ide palette, rotation, etc.
//
//

//
// Modes:
//   Auto:          "A", " "
//   Exclusion:     "X", "Y", " "
//   Freestyle:     "F", " "
//   S(t)ripe mode: "T", " "
//
//
class PalTable
{
public:
    PalTable();
    ~PalTable();
    void process();
    void set_hidden(bool hidden);

private:
    void draw_status(bool stripe_mode);
    void hl_pal(int pnum, int color);
    void draw();
    void set_curr(int which, int curr);
    void save_rect();
    void restore_rect();
    void set_pos(int x, int y);
    void set_csize(int csize);
    int get_cursor_color() const;
    void do_curs(int key);
    void rotate(int dir, int lo, int hi);
    void update_dac();
    void save_undo_data(int first, int last);
    void save_undo_rotate(int dir, int first, int last);
    void undo_process(int delta);
    void undo();
    void redo();
    void mk_default_palettes();
    void hide(RGBEditor *rgb, bool hidden);
    void calc_top_bottom();
    void put_band(PALENTRY *pal);
    void other_key(int key, RGBEditor *rgb);
    void change(RGBEditor *rgb);
    
    static void other_key_cb(int key, RGBEditor *rgb, void *info);
    static void change_cb(RGBEditor *rgb, void *info);

    int m_x;
    int m_y;
    int m_csize;
    int m_active; // which RGBEditor is active (0,1)
    int m_curr[2];
    RGBEditor *m_rgb[2];
    MoveBox *m_move_box;
    bool m_done;
    int m_exclude;
    bool m_auto_select;
    PALENTRY m_pal[256];
    std::FILE *m_undo_file;
    bool m_curr_changed;
    int m_num_redo;
    bool m_hidden;
    int m_stored_at;
    std::vector<char> m_saved_pixel;
    PALENTRY m_save_pal[8][256];
    PALENTRY m_fs_color;
    int m_top, m_bottom; // top and bottom colours of freestyle band
    int m_band_width;    // size of freestyle colour band
    bool m_free_style;
};

bool g_using_jiim{};
std::vector<BYTE> g_line_buff;

static const char *s_undo_file{"id.$$2"};  // file where undo list is stored
#ifdef XFRACT
bool g_editpal_cursor{};
#endif
static BYTE s_fg_color{};
static BYTE s_bg_color{};
static bool s_reserve_colors{};
static bool s_inverse{};
static float s_gamma_val{1.0f};
static Cursor s_cursor{};

// Interface to graphics stuff
static void set_pal(int pal, int r, int g, int b)
{
    g_dac_box[pal][0] = (BYTE)r;
    g_dac_box[pal][1] = (BYTE)g;
    g_dac_box[pal][2] = (BYTE)b;
    spindac(0, 1);
}

static void set_pal_range(int first, int how_many, PALENTRY *pal)
{
    std::memmove(g_dac_box+first, pal, how_many*3);
    spindac(0, 1);
}

static void get_pal_range(int first, int how_many, PALENTRY *pal)
{
    std::memmove(pal, g_dac_box+first, how_many*3);
}

static void rotate_pal(PALENTRY *pal, int dir, int lo, int hi)
{
    // rotate in either direction
    PALENTRY hold;
    int      size;

    size  = 1 + (hi-lo);

    if (dir > 0)
    {
        while (dir-- > 0)
        {
            std::memmove(&hold, &pal[hi],  3);
            std::memmove(&pal[lo+1], &pal[lo], 3*(size-1));
            std::memmove(&pal[lo], &hold, 3);
        }
    }

    else if (dir < 0)
    {
        while (dir++ < 0)
        {
            std::memmove(&hold, &pal[lo], 3);
            std::memmove(&pal[lo], &pal[lo+1], 3*(size-1));
            std::memmove(&pal[hi], &hold,  3);
        }
    }
}

static void clip_put_line(int row, int start, int stop, BYTE const *pixels)
{
    if (row < 0 || row >= g_screen_y_dots || start > g_screen_x_dots || stop < 0)
    {
        return ;
    }

    if (start < 0)
    {
        pixels += -start;
        start = 0;
    }

    if (stop >= g_screen_x_dots)
    {
        stop = g_screen_x_dots - 1;
    }

    if (start > stop)
    {
        return ;
    }

    put_line(row, start, stop, pixels);
}

static void clip_get_line(int row, int start, int stop, BYTE *pixels)
{
    if (row < 0 || row >= g_screen_y_dots || start > g_screen_x_dots || stop < 0)
    {
        return ;
    }

    if (start < 0)
    {
        pixels += -start;
        start = 0;
    }

    if (stop >= g_screen_x_dots)
    {
        stop = g_screen_x_dots - 1;
    }

    if (start > stop)
    {
        return ;
    }

    get_line(row, start, stop, pixels);
}

void clip_put_color(int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= g_screen_x_dots || y >= g_screen_y_dots)
    {
        return ;
    }

    g_put_color(x, y, color);
}

int clip_get_color(int x, int y)
{
    if (x < 0 || y < 0 || x >= g_screen_x_dots || y >= g_screen_y_dots)
    {
        return 0;
    }

    return getcolor(x, y);
}

static void hor_line(int x, int y, int width, int color)
{
    std::memset(&g_line_buff[0], color, width);
    clip_put_line(y, x, x+width-1, &g_line_buff[0]);
}

static void ver_line(int x, int y, int depth, int color)
{
    while (depth-- > 0)
    {
        clip_put_color(x, y++, color);
    }
}

void get_row(int x, int y, int width, char *buff)
{
    clip_get_line(y, x, x+width-1, (BYTE *)buff);
}

void put_row(int x, int y, int width, char const *buff)
{
    clip_put_line(y, x, x+width-1, (BYTE *)buff);
}

static void ver_get_row(int x, int y, int depth, char *buff)
{
    while (depth-- > 0)
    {
        *buff++ = (char)clip_get_color(x, y++);
    }
}

static void ver_put_row(int x, int y, int depth, char const *buff)
{
    while (depth-- > 0)
    {
        clip_put_color(x, y++, (BYTE)(*buff++));
    }
}

static void fill_rect(int x, int y, int width, int depth, int color)
{
    while (depth-- > 0)
    {
        hor_line(x, y++, width, color);
    }
}

static void rect(int x, int y, int width, int depth, int color)
{
    hor_line(x, y, width, color);
    hor_line(x, y+depth-1, width, color);

    ver_line(x, y, depth, color);
    ver_line(x+width-1, y, depth, color);
}

static void displayf(int x, int y, int fg, int bg, char const *format, ...)
{
    char buff[81];

    std::va_list arg_list;

    va_start(arg_list, format);
    std::vsnprintf(buff, std::size(buff), format, arg_list);
    va_end(arg_list);

    driver_display_string(x, y, fg, bg, buff);
}

// create smooth shades between two colors
static void mk_pal_range(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
{
    double rm = (double) ((int) p2->red - (int) p1->red) / num;
    double gm = (double) ((int) p2->green - (int) p1->green) / num;
    double bm = (double) ((int) p2->blue - (int) p1->blue) / num;

    for (int curr = 0; curr < num; curr += skip)
    {
        if (s_gamma_val == 1)
        {
            pal[curr].red   = (BYTE)((p1->red   == p2->red) ? p1->red   :
                                     (int) p1->red   + (int)(rm * curr));
            pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
                                     (int) p1->green + (int)(gm * curr));
            pal[curr].blue  = (BYTE)((p1->blue  == p2->blue) ? p1->blue  :
                                     (int) p1->blue  + (int)(bm * curr));
        }
        else
        {
            pal[curr].red   = (BYTE)((p1->red   == p2->red) ? p1->red   :
                                     (int)(p1->red   + std::pow(curr/(double)(num-1), static_cast<double>(s_gamma_val))*num*rm));
            pal[curr].green = (BYTE)((p1->green == p2->green) ? p1->green :
                                     (int)(p1->green + std::pow(curr/(double)(num-1), static_cast<double>(s_gamma_val))*num*gm));
            pal[curr].blue  = (BYTE)((p1->blue  == p2->blue) ? p1->blue  :
                                     (int)(p1->blue  + std::pow(curr/(double)(num-1), static_cast<double>(s_gamma_val))*num*bm));
        }
    }
}

//  Swap RG GB & RB columns
static void rot_col_r_g(PALENTRY pal[], int num)
{
    for (int curr = 0; curr <= num; curr++)
    {
        int dummy = pal[curr].red;
        pal[curr].red = pal[curr].green;
        pal[curr].green = (BYTE)dummy;
    }
}

static void rot_col_g_b(PALENTRY pal[], int num)
{
    for (int curr = 0; curr <= num; curr++)
    {
        int const dummy = pal[curr].green;
        pal[curr].green = pal[curr].blue;
        pal[curr].blue = (BYTE)dummy;
    }
}

static void rot_col_b_r(PALENTRY pal[], int num)
{
    for (int curr = 0; curr <= num; curr++)
    {
        int const dummy = pal[curr].red;
        pal[curr].red = pal[curr].blue;
        pal[curr].blue = (BYTE)dummy;
    }
}

// convert a range of colors to grey scale
static void pal_range_to_grey(PALENTRY pal[], int first, int how_many)
{
    for (PALENTRY *curr = &pal[first]; how_many > 0; how_many--, curr++)
    {
        BYTE val = (BYTE)(((int)curr->red*30 + (int)curr->green*59 + (int)curr->blue*11) / 100);
        curr->blue = (BYTE)val;
        curr->green = curr->blue;
        curr->red = curr->green;
    }
}

// convert a range of colors to their inverse
static void pal_range_to_negative(PALENTRY pal[], int first, int how_many)
{
    for (PALENTRY *curr = &pal[first]; how_many > 0; how_many--, curr++)
    {
        curr->red   = (BYTE)(63 - curr->red);
        curr->green = (BYTE)(63 - curr->green);
        curr->blue  = (BYTE)(63 - curr->blue);
    }
}

// draw and horizontal/vertical dotted lines
static void hor_dot_line(int x, int y, int width)
{
    BYTE *ptr = &g_line_buff[0];
    for (int ctr = 0; ctr < width; ctr++, ptr++)
    {
        *ptr = (BYTE)((ctr&2) ? s_bg_color : s_fg_color);
    }

    put_row(x, y, width, (char *) &g_line_buff[0]);
}

static void ver_dot_line(int x, int y, int depth)
{
    for (int ctr = 0; ctr < depth; ctr++, y++)
    {
        clip_put_color(x, y, (ctr&2) ? s_bg_color : s_fg_color);
    }
}

static void dot_rect(int x, int y, int width, int depth)
{
    hor_dot_line(x, y, width);
    hor_dot_line(x, y+depth-1, width);

    ver_dot_line(x, y, depth);
    ver_dot_line(x+width-1, y, depth);
}

// misc. routines
static bool is_reserved(int color)
{
    return s_reserve_colors && (color == s_fg_color || color == s_bg_color);
}

static bool is_in_box(int x, int y, int bx, int by, int bw, int bd)
{
    return (x >= bx) && (y >= by) && (x < bx+bw) && (y < by+bd);
}

static void draw_diamond(int x, int y, int color)
{
    g_put_color(x+2, y+0,    color);
    hor_line(x+1, y+1, 3, color);
    hor_line(x+0, y+2, 5, color);
    hor_line(x+1, y+3, 3, color);
    g_put_color(x+2, y+4,    color);
}

// private:
static  void    Cursor__Draw();
static  void    Cursor__Save();
static  void    Cursor__Restore();

void Cursor_Construct()
{
    s_cursor.x = g_screen_x_dots / 2;
    s_cursor.y = g_screen_y_dots / 2;
    s_cursor.hidden = 1;
    s_cursor.blink = false;
    s_cursor.last_blink = 0;
}

MoveBox::MoveBox(int x, int y, int csize, int base_width, int base_depth) :
    m_x(x),
    m_y(y),
    m_base_width(base_width),
    m_base_depth(base_depth),
    m_csize(csize)
{
    m_t.resize(g_screen_x_dots);
    m_b.resize(g_screen_x_dots);
    m_l.resize(g_screen_y_dots);
    m_r.resize(g_screen_y_dots);
}

void MoveBox::draw()
{
    int width = m_base_width + m_csize * 16 + 1;
    int depth = m_base_depth + m_csize * 16 + 1;

    get_row(m_x, m_y, width, &m_t[0]);
    get_row(m_x, m_y + depth - 1, width, &m_b[0]);

    ver_get_row(m_x, m_y, depth, &m_l[0]);
    ver_get_row(m_x + width - 1, m_y, depth, &m_r[0]);

    hor_dot_line(m_x, m_y, width);
    hor_dot_line(m_x, m_y + depth - 1, width);

    ver_dot_line(m_x, m_y, depth);
    ver_dot_line(m_x + width - 1, m_y, depth);
}

void MoveBox::erase()
{
    int width = m_base_width + m_csize * 16 + 1;
    int depth = m_base_depth + m_csize * 16 + 1;

    ver_put_row(m_x, m_y, depth, &m_l[0]);
    ver_put_row(m_x + width - 1, m_y, depth, &m_r[0]);

    put_row(m_x, m_y, width, &m_t[0]);
    put_row(m_x, m_y + depth - 1, width, &m_b[0]);
}

CEditor::CEditor(int x, int y, char letter, void (*other_key)(int, CEditor *, void *),
    void (*change)(CEditor *, void *), void *info) :
    m_x(x),
    m_y(y),
    m_letter(letter),
    m_other_key(other_key),
    m_change(change),
    m_info(info)
{
}

void CEditor::draw()
{
    if (m_hidden)
    {
        return;
    }

    Cursor_Hide();
    displayf(m_x + 2, m_y + 2, s_fg_color, s_bg_color, "%c%02d", m_letter, m_val);
    Cursor_Show();
}

void CEditor::set_pos(int x, int y)
{
    m_x = x;
    m_y = y;
}

void CEditor::set_val(int val)
{
    m_val = val;
}

int CEditor::get_val() const
{
    return m_val;
}

void CEditor::set_done(bool done)
{
    m_done = done;
}

void CEditor::set_hidden(bool hidden)
{
    m_hidden = hidden;
}

int CEditor::edit()
{
    int key = 0;
    int diff;

    m_done = false;

    if (!m_hidden)
    {
        Cursor_Hide();
        rect(m_x, m_y, CEditor_WIDTH, CEditor_DEPTH, s_fg_color);
        Cursor_Show();
    }

#ifdef XFRACT
    Cursor_StartMouseTracking();
#endif
    while (!m_done)
    {
        Cursor_WaitKey();
        key = driver_get_key();

        switch (key)
        {
        case ID_KEY_PAGE_UP:
            if (m_val < 63)
            {
                m_val += 5;
                if (m_val > 63)
                {
                    m_val = 63;
                }
                draw();
                m_change(this, m_info);
            }
            break;

        case '+':
        case ID_KEY_CTL_PLUS:
            diff = 1;
            while (driver_key_pressed() == key)
            {
                driver_get_key();
                ++diff;
            }
            if (m_val < 63)
            {
                m_val += diff;
                if (m_val > 63)
                {
                    m_val = 63;
                }
                draw();
                m_change(this, m_info);
            }
            break;

        case ID_KEY_PAGE_DOWN:
            if (m_val > 0)
            {
                m_val -= 5;
                if (m_val < 0)
                {
                    m_val = 0;
                }
                draw();
                m_change(this, m_info);
            }
            break;

        case '-':
        case ID_KEY_CTL_MINUS:
            diff = 1;
            while (driver_key_pressed() == key)
            {
                driver_get_key();
                ++diff;
            }
            if (m_val > 0)
            {
                m_val -= diff;
                if (m_val < 0)
                {
                    m_val = 0;
                }
                draw();
                m_change(this, m_info);
            }
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            m_val = (key - '0') * 10;
            if (m_val > 63)
            {
                m_val = 63;
            }
            draw();
            m_change(this, m_info);
            break;

        default:
            m_other_key(key, this, m_info);
            break;
        } // switch
    }     // while
#ifdef XFRACT
    Cursor_EndMouseTracking();
#endif

    if (!m_hidden)
    {
        Cursor_Hide();
        rect(m_x, m_y, CEditor_WIDTH, CEditor_DEPTH, s_bg_color);
        Cursor_Show();
    }

    return key;
}

void MoveBox::move(int key)
{
    bool done = false;
    bool first = true;
    int xoff = 0;
    int yoff = 0;

    while (!done)
    {
        switch (key)
        {
        case ID_KEY_CTL_RIGHT_ARROW:
            xoff += BOX_INC * 4;
            break;
        case ID_KEY_RIGHT_ARROW:
            xoff += BOX_INC;
            break;
        case ID_KEY_CTL_LEFT_ARROW:
            xoff -= BOX_INC * 4;
            break;
        case ID_KEY_LEFT_ARROW:
            xoff -= BOX_INC;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
            yoff += BOX_INC * 4;
            break;
        case ID_KEY_DOWN_ARROW:
            yoff += BOX_INC;
            break;
        case ID_KEY_CTL_UP_ARROW:
            yoff -= BOX_INC * 4;
            break;
        case ID_KEY_UP_ARROW:
            yoff -= BOX_INC;
            break;

        default:
            done = true;
        }

        if (!done)
        {
            if (!first)
            {
                driver_get_key(); // delete key from buffer
            }
            else
            {
                first = false;
            }
            key = driver_key_pressed(); // peek at the next one...
        }
    }

    xoff += m_x;
    yoff += m_y; // (xoff,yoff) = new position

    if (xoff < 0)
    {
        xoff = 0;
    }
    if (yoff < 0)
    {
        yoff = 0;
    }

    if (xoff + m_base_width + m_csize * 16 + 1 > g_screen_x_dots)
    {
        xoff = g_screen_x_dots - (m_base_width + m_csize * 16 + 1);
    }

    if (yoff + m_base_depth + m_csize * 16 + 1 > g_screen_y_dots)
    {
        yoff = g_screen_y_dots - (m_base_depth + m_csize * 16 + 1);
    }

    if (xoff != m_x || yoff != m_y)
    {
        erase();
        m_y = yoff;
        m_x = xoff;
        draw();
    }
}

bool MoveBox::process()
{
    int     key;
    int orig_x = m_x;
    int orig_y = m_y;
    int orig_csize = m_csize;

    draw();

#ifdef XFRACT
    Cursor_StartMouseTracking();
#endif
    while (true)
    {
        Cursor_WaitKey();
        key = driver_get_key();

        if (key == ID_KEY_ENTER || key == ID_KEY_ENTER_2 || key == ID_KEY_ESC || key == 'H' || key == 'h')
        {
            m_moved = m_x != orig_x || m_y != orig_y || m_csize != orig_csize;
            break;
        }

        switch (key)
        {
        case ID_KEY_UP_ARROW:
        case ID_KEY_DOWN_ARROW:
        case ID_KEY_LEFT_ARROW:
        case ID_KEY_RIGHT_ARROW:
        case ID_KEY_CTL_UP_ARROW:
        case ID_KEY_CTL_DOWN_ARROW:
        case ID_KEY_CTL_LEFT_ARROW:
        case ID_KEY_CTL_RIGHT_ARROW:
            move(key);
            break;

        case ID_KEY_PAGE_UP:   // shrink
            if (m_csize > CSIZE_MIN)
            {
                int t = m_csize - CSIZE_INC;
                int change;

                if (t < CSIZE_MIN)
                {
                    t = CSIZE_MIN;
                }

                erase();

                change = m_csize - t;
                m_csize = t;
                m_x += (change*16) / 2;
                m_y += (change*16) / 2;
                draw();
            }
            break;

        case ID_KEY_PAGE_DOWN:   // grow
        {
            int max_width = std::min(g_screen_x_dots, MAX_WIDTH);

            if (m_base_depth+(m_csize+CSIZE_INC)*16+1 < g_screen_y_dots
                && m_base_width+(m_csize+CSIZE_INC)*16+1 < max_width)
            {
                erase();
                m_x -= (CSIZE_INC*16) / 2;
                m_y -= (CSIZE_INC*16) / 2;
                m_csize += CSIZE_INC;
                if (m_y+m_base_depth+m_csize*16+1 > g_screen_y_dots)
                {
                    m_y = g_screen_y_dots - (m_base_depth+m_csize*16+1);
                }
                if (m_x+m_base_width+m_csize*16+1 > max_width)
                {
                    m_x = max_width - (m_base_width+m_csize*16+1);
                }
                if (m_y < 0)
                {
                    m_y = 0;
                }
                if (m_x < 0)
                {
                    m_x = 0;
                }
                draw();
            }
        }
        break;
        }
    }

#ifdef XFRACT
    Cursor_EndMouseTracking();
#endif

    erase();

    m_should_hide = key == 'H' || key == 'h';

    return key != ID_KEY_ESC;
}

static void Cursor__Draw()
{
    int color;

    find_special_colors();
    color = s_cursor.blink ? g_color_medium : g_color_dark;

    ver_line(s_cursor.x, s_cursor.y-CURSOR_SIZE-1, CURSOR_SIZE, color);
    ver_line(s_cursor.x, s_cursor.y+2,             CURSOR_SIZE, color);

    hor_line(s_cursor.x-CURSOR_SIZE-1, s_cursor.y, CURSOR_SIZE, color);
    hor_line(s_cursor.x+2,             s_cursor.y, CURSOR_SIZE, color);
}

static void Cursor__Save()
{
    ver_get_row(s_cursor.x, s_cursor.y-CURSOR_SIZE-1, CURSOR_SIZE, s_cursor.t);
    ver_get_row(s_cursor.x, s_cursor.y+2,             CURSOR_SIZE, s_cursor.b);

    get_row(s_cursor.x-CURSOR_SIZE-1, s_cursor.y,  CURSOR_SIZE, s_cursor.l);
    get_row(s_cursor.x+2,             s_cursor.y,  CURSOR_SIZE, s_cursor.r);
}

static void Cursor__Restore()
{
    ver_put_row(s_cursor.x, s_cursor.y-CURSOR_SIZE-1, CURSOR_SIZE, s_cursor.t);
    ver_put_row(s_cursor.x, s_cursor.y+2,             CURSOR_SIZE, s_cursor.b);

    put_row(s_cursor.x-CURSOR_SIZE-1, s_cursor.y,  CURSOR_SIZE, s_cursor.l);
    put_row(s_cursor.x+2,             s_cursor.y,  CURSOR_SIZE, s_cursor.r);
}

void Cursor_SetPos(int x, int y)
{
    if (!s_cursor.hidden)
    {
        Cursor__Restore();
    }

    s_cursor.x = x;
    s_cursor.y = y;

    if (!s_cursor.hidden)
    {
        Cursor__Save();
        Cursor__Draw();
    }
}

void Cursor_Move(int xoff, int yoff)
{
    if (!s_cursor.hidden)
    {
        Cursor__Restore();
    }

    s_cursor.x += xoff;
    s_cursor.y += yoff;

    if (s_cursor.x < 0)
    {
        s_cursor.x = 0;
    }
    if (s_cursor.y < 0)
    {
        s_cursor.y = 0;
    }
    if (s_cursor.x >= g_screen_x_dots)
    {
        s_cursor.x = g_screen_x_dots-1;
    }
    if (s_cursor.y >= g_screen_y_dots)
    {
        s_cursor.y = g_screen_y_dots-1;
    }

    if (!s_cursor.hidden)
    {
        Cursor__Save();
        Cursor__Draw();
    }
}

int Cursor_GetX()
{
    return s_cursor.x;
}

int Cursor_GetY()
{
    return s_cursor.y;
}

void Cursor_Hide()
{
    if (s_cursor.hidden++ == 0)
    {
        Cursor__Restore();
    }
}

void Cursor_Show()
{
    if (--s_cursor.hidden == 0)
    {
        Cursor__Save();
        Cursor__Draw();
    }
}

#ifdef XFRACT
void Cursor_StartMouseTracking()
{
    g_editpal_cursor = true;
}

void Cursor_EndMouseTracking()
{
    g_editpal_cursor = false;
}
#endif

// See if the cursor should blink yet, and blink it if so
void Cursor_CheckBlink()
{
    long tick;
    tick = readticker();

    if ((tick - s_cursor.last_blink) > CURSOR_BLINK_RATE)
    {
        s_cursor.blink = !s_cursor.blink;
        s_cursor.last_blink = tick;
        if (!s_cursor.hidden)
        {
            Cursor__Draw();
        }
    }
    else if (tick < s_cursor.last_blink)
    {
        s_cursor.last_blink = tick;
    }
}

int Cursor_WaitKey()   // blink cursor while waiting for a key
{
    while (!driver_wait_key_pressed(1))
    {
        Cursor_CheckBlink();
    }

    return driver_key_pressed();
}

RGBEditor::RGBEditor(int x, int y, void (*other_key)(int, RGBEditor *, void *),
    void (*change)(RGBEditor *, void *), void *info) :
    m_x(x),
    m_y(y),
    m_pal(1),
    m_other_key(other_key),
    m_change(change),
    m_info(info)
{
    static char letter[] = "RGB";

    for (int ctr = 0; ctr < 3; ctr++)
    {
        m_color[ctr] = new CEditor(0, 0, letter[ctr], other_key_cb, change_cb, this);
    }
}

RGBEditor::~RGBEditor()
{
    delete m_color[0];
    delete m_color[1];
    delete m_color[2];
}

void RGBEditor::set_done(bool done)
{
    this->m_done = done;
}

void RGBEditor::set_pos(int x, int y)
{
    this->m_x = x;
    this->m_y = y;
    m_color[0]->set_pos(x + 2, y + 2);
    m_color[1]->set_pos(x + 2, y + 2 + CEditor_DEPTH - 1);
    m_color[2]->set_pos(x + 2, y + 2 + CEditor_DEPTH - 1 + CEditor_DEPTH - 1);
}

void RGBEditor::set_hidden(bool hidden)
{
    this->m_hidden = hidden;
    m_color[0]->set_hidden(hidden);
    m_color[1]->set_hidden(hidden);
    m_color[2]->set_hidden(hidden);
}

void RGBEditor::blank_sample_box()
{
    if (m_hidden)
    {
        return;
    }

    Cursor_Hide();
    fill_rect(
        m_x + 2 + CEditor_WIDTH + 1 + 1, m_y + 2 + 1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, s_bg_color);
    Cursor_Show();
}

void RGBEditor::update()
{
    if (m_hidden)
    {
        return;
    }

    int x1 = m_x + 2 + CEditor_WIDTH + 1 + 1;
    int y1 = m_y + 2 + 1;

    Cursor_Hide();

    if (m_pal >= g_colors)
    {
        fill_rect(x1, y1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, s_bg_color);
        draw_diamond(x1 + (RGBEditor_BWIDTH - 5) / 2, y1 + (RGBEditor_BDEPTH - 5) / 2, s_fg_color);
    }

    else if (is_reserved(m_pal))
    {
        int x2 = x1 + RGBEditor_BWIDTH - 3;
        int y2 = y1 + RGBEditor_BDEPTH - 3;

        fill_rect(x1, y1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, s_bg_color);
        driver_draw_line(x1, y1, x2, y2, s_fg_color);
        driver_draw_line(x1, y2, x2, y1, s_fg_color);
    }
    else
    {
        fill_rect(x1, y1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, m_pal);
    }

    m_color[0]->draw();
    m_color[1]->draw();
    m_color[2]->draw();
    Cursor_Show();
}

void RGBEditor::draw()
{
    if (m_hidden)
    {
        return;
    }

    Cursor_Hide();
    dot_rect(m_x, m_y, RGBEditor_WIDTH, RGBEditor_DEPTH);
    fill_rect(m_x + 1, m_y + 1, RGBEditor_WIDTH - 2, RGBEditor_DEPTH - 2, s_bg_color);
    rect(m_x + 1 + CEditor_WIDTH + 2, m_y + 2, RGBEditor_BWIDTH, RGBEditor_BDEPTH, s_fg_color);
    update();
    Cursor_Show();
}

int RGBEditor::edit()
{
    int key = 0;

    m_done = false;

    if (!m_hidden)
    {
        Cursor_Hide();
        rect(m_x, m_y, RGBEditor_WIDTH, RGBEditor_DEPTH, s_fg_color);
        Cursor_Show();
    }

    while (!m_done)
    {
        key = m_color[m_curr]->edit();
    }

    if (!m_hidden)
    {
        Cursor_Hide();
        dot_rect(m_x, m_y, RGBEditor_WIDTH, RGBEditor_DEPTH);
        Cursor_Show();
    }

    return key;
}

void RGBEditor::set_rgb(int pal, PALENTRY *rgb)
{
    this->m_pal = pal;
    m_color[0]->set_val(rgb->red);
    m_color[1]->set_val(rgb->green);
    m_color[2]->set_val(rgb->blue);
}

PALENTRY RGBEditor::get_rgb() const
{
    PALENTRY pal;

    pal.red = (BYTE) m_color[0]->get_val();
    pal.green = (BYTE) m_color[1]->get_val();
    pal.blue = (BYTE) m_color[2]->get_val();

    return pal;
}

void RGBEditor::other_key_cb(int key, CEditor *ceditor, void *info)
{
    static_cast<RGBEditor *>(info)->other_key(key, ceditor);
}

void RGBEditor::change(CEditor *editor)
{
    if (m_pal < g_colors && !is_reserved(m_pal))
    {
        set_pal(m_pal, m_color[0]->get_val(), m_color[1]->get_val(), m_color[2]->get_val());
    }

    m_change(this, m_info);
}

void RGBEditor::other_key(int key, CEditor *ceditor)
{
    switch (key)
    {
    case 'R':
    case 'r':
        if (m_curr != 0)
        {
            m_curr = 0;
            ceditor->set_done(true);
        }
        break;

    case 'G':
    case 'g':
        if (m_curr != 1)
        {
            m_curr = 1;
            ceditor->set_done(true);
        }
        break;

    case 'B':
    case 'b':
        if (m_curr != 2)
        {
            m_curr = 2;
            ceditor->set_done(true);
        }
        break;

    case ID_KEY_DELETE:      // move to next CEditor
    case ID_KEY_CTL_ENTER_2: // double click rt mouse also!
        if (++m_curr > 2)
        {
            m_curr = 0;
        }
        ceditor->set_done(true);
        break;

    case ID_KEY_INSERT: // move to prev CEditor
        if (--m_curr < 0)
        {
            m_curr = 2;
        }
        ceditor->set_done(true);
        break;

    default:
        m_other_key(key, this, m_info);
        if (m_done)
        {
            ceditor->set_done(true);
        }
        break;
    }
}

void RGBEditor::change_cb(CEditor *ceditor, void *info)
{
    static_cast<RGBEditor *>(info)->change(ceditor);
}

void PalTable::process()
{
    get_pal_range(0, g_colors, m_pal);

    // Make sure all palette entries are 0-63

    for (int ctr = 0; ctr < 768; ctr++)
    {
        ((char *)m_pal)[ctr] &= 63;
    }

    update_dac();

    m_rgb[0]->set_rgb(m_curr[0], &m_pal[m_curr[0]]);
    m_rgb[1]->set_rgb(m_curr[1], &m_pal[m_curr[0]]);

    if (!m_hidden)
    {
        m_move_box->set_pos(m_x, m_y);
        m_move_box->set_csize(m_csize);
        if (!m_move_box->process())
        {
            set_pal_range(0, g_colors, m_pal);
            return ;
        }

        set_pos(m_move_box->x(), m_move_box->y());
        set_csize(m_move_box->csize());

        if (m_move_box->should_hide())
        {
            set_hidden(true);
            s_reserve_colors = false;
        }
        else
        {
            s_reserve_colors = true;
            save_rect();
            draw();
        }
    }

    set_curr(m_active,          get_cursor_color());
    set_curr((m_active == 1)?0:1, get_cursor_color());
    Cursor_Show();
    mk_default_palettes();
    m_done = false;

    while (!m_done)
    {
        m_rgb[m_active]->edit();
    }

    Cursor_Hide();
    restore_rect();
    set_pal_range(0, g_colors, m_pal);
}

// - everything else -
void PalTable::draw_status(bool stripe_mode)
{
    int width = 1 + (m_csize * 16) + 1 + 1;

    if (!m_hidden && (width - (RGBEditor_WIDTH * 2 + 4) >= STATUS_LEN * 8))
    {
        int x = this->m_x + 2 + RGBEditor_WIDTH;
        int y = this->m_y + PalTable_PALY - 10;
        int color = get_cursor_color();
        if (color < 0 || color >= g_colors) // hmm, the border returns -1
        {
            color = 0;
        }
        Cursor_Hide();

        {
            char buff[80];
            std::snprintf(buff, std::size(buff), "%c%c%c%c", m_auto_select ? 'A' : ' ',
                (m_exclude == 1)       ? 'X'
                    : (m_exclude == 2) ? 'Y'
                                     : ' ',
                m_free_style ? 'F' : ' ', stripe_mode ? 'T' : ' ');
            driver_display_string(x, y, s_fg_color, s_bg_color, buff);

            y = y - 10;
            std::snprintf(buff, std::size(buff), "%-3d", color); // assumes 8-bit color, 0-255 values
            driver_display_string(x, y, s_fg_color, s_bg_color, buff);
        }
        Cursor_Show();
    }
}

void PalTable::hl_pal(int pnum, int color)
{
    if (m_hidden)
    {
        return;
    }

    int x = this->m_x + PalTable_PALX + (pnum % 16) * m_csize;
    int y = this->m_y + PalTable_PALY + (pnum / 16) * m_csize;

    Cursor_Hide();

    const int size = m_csize + 1;
    if (color < 0)
    {
        dot_rect(x, y, size, size);
    }
    else
    {
        rect(x, y, size, size, color);
    }

    Cursor_Show();
}

void PalTable::draw()
{
    if (m_hidden)
    {
        return;
    }

    Cursor_Hide();

    int width = 1 + (m_csize * 16) + 1 + 1;

    rect(m_x, m_y, width, 2 + RGBEditor_DEPTH + 2 + (m_csize * 16) + 1 + 1, s_fg_color);

    fill_rect(m_x + 1, m_y + 1, width - 2, 2 + RGBEditor_DEPTH + 2 + (m_csize * 16) + 1 + 1 - 2, s_bg_color);

    hor_line(m_x, m_y + PalTable_PALY - 1, width, s_fg_color);

    if (width - (RGBEditor_WIDTH * 2 + 4) >= TITLE_LEN * 8)
    {
        int center = (width - TITLE_LEN * 8) / 2;

        displayf(m_x + center, m_y + RGBEditor_DEPTH / 2 - 12, s_fg_color, s_bg_color, ID_PROGRAM_NAME);
    }

    m_rgb[0]->draw();
    m_rgb[1]->draw();

    for (int pal = 0; pal < 256; pal++)
    {
        int xoff = PalTable_PALX + (pal % 16) * m_csize;
        int yoff = PalTable_PALY + (pal / 16) * m_csize;

        if (pal >= g_colors)
        {
            fill_rect(m_x + xoff + 1, m_y + yoff + 1, m_csize - 1, m_csize - 1, s_bg_color);
            draw_diamond(m_x + xoff + m_csize / 2 - 1, m_y + yoff + m_csize / 2 - 1, s_fg_color);
        }

        else if (is_reserved(pal))
        {
            int x1 = m_x + xoff + 1;
            int y1 = m_y + yoff + 1;
            int x2 = x1 + m_csize - 2;
            int y2 = y1 + m_csize - 2;
            fill_rect(m_x + xoff + 1, m_y + yoff + 1, m_csize - 1, m_csize - 1, s_bg_color);
            driver_draw_line(x1, y1, x2, y2, s_fg_color);
            driver_draw_line(x1, y2, x2, y1, s_fg_color);
        }
        else
        {
            fill_rect(m_x + xoff + 1, m_y + yoff + 1, m_csize - 1, m_csize - 1, pal);
        }
    }

    if (m_active == 0)
    {
        hl_pal(m_curr[1], -1);
        hl_pal(m_curr[0], s_fg_color);
    }
    else
    {
        hl_pal(m_curr[0], -1);
        hl_pal(m_curr[1], s_fg_color);
    }

    draw_status(false);

    Cursor_Show();
}

void PalTable::set_curr(int which, int curr)
{
    bool redraw = which < 0;

    if (redraw)
    {
        which = m_active;
        curr = this->m_curr[which];
    }
    else if (curr == this->m_curr[which] || curr < 0)
    {
        return;
    }

    Cursor_Hide();

    hl_pal(this->m_curr[0], s_bg_color);
    hl_pal(this->m_curr[1], s_bg_color);
    hl_pal(m_top, s_bg_color);
    hl_pal(m_bottom, s_bg_color);

    if (m_free_style)
    {
        this->m_curr[which] = curr;

        calc_top_bottom();

        hl_pal(m_top, -1);
        hl_pal(m_bottom, -1);
        hl_pal(this->m_curr[m_active], s_fg_color);

        m_rgb[which]->set_rgb(this->m_curr[which], &m_fs_color);
        m_rgb[which]->update();

        update_dac();

        Cursor_Show();

        return;
    }

    this->m_curr[which] = curr;

    if (this->m_curr[0] != this->m_curr[1])
    {
        hl_pal(this->m_curr[m_active == 0 ? 1 : 0], -1);
    }
    hl_pal(this->m_curr[m_active], s_fg_color);

    m_rgb[which]->set_rgb(this->m_curr[which], &(m_pal[this->m_curr[which]]));

    if (redraw)
    {
        int other = (which == 0) ? 1 : 0;
        m_rgb[other]->set_rgb(this->m_curr[other], &(m_pal[this->m_curr[other]]));
        m_rgb[0]->update();
        m_rgb[1]->update();
    }
    else
    {
        m_rgb[which]->update();
    }

    if (m_exclude)
    {
        update_dac();
    }

    Cursor_Show();

    m_curr_changed = false;
}

void PalTable::save_rect()
{
    int const width = PalTable_PALX + m_csize * 16 + 1 + 1;
    int const depth = PalTable_PALY + m_csize * 16 + 1 + 1;

    m_saved_pixel.resize(width * depth);
    Cursor_Hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        get_row(m_x, m_y + yoff, width, &m_saved_pixel[yoff * width]);
        hor_line(m_x, m_y + yoff, width, s_bg_color);
    }
    Cursor_Show();
}

void PalTable::restore_rect()
{
    if (m_hidden)
    {
        return;
    }

    int width = PalTable_PALX + m_csize * 16 + 1 + 1;
    int depth = PalTable_PALY + m_csize * 16 + 1 + 1;

    Cursor_Hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        put_row(m_x, m_y + yoff, width, &m_saved_pixel[yoff * width]);
    }
    Cursor_Show();
}

void PalTable::set_pos(int x, int y)
{
    int width = PalTable_PALX + m_csize * 16 + 1 + 1;

    this->m_x = x;
    this->m_y = y;

    m_rgb[0]->set_pos(x + 2, y + 2);
    m_rgb[1]->set_pos(x + width - 2 - RGBEditor_WIDTH, y + 2);
}

void PalTable::set_csize(int csize)
{
    this->m_csize = csize;
    set_pos(m_x, m_y);
}

int PalTable::get_cursor_color() const
{
    int x = Cursor_GetX();
    int y = Cursor_GetY();
    int color = getcolor(x, y);

    if (is_reserved(color))
    {
        if (is_in_box(x, y, this->m_x, this->m_y, 1 + (m_csize * 16) + 1 + 1,
                2 + RGBEditor_DEPTH + 2 + (m_csize * 16) + 1 + 1))
        {
            // is the cursor over the editor?
            x -= this->m_x + PalTable_PALX;
            y -= this->m_y + PalTable_PALY;
            int size = m_csize;

            if (x < 0 || y < 0 || x > size * 16 || y > size * 16)
            {
                return -1;
            }

            if (x == size * 16)
            {
                --x;
            }
            if (y == size * 16)
            {
                --y;
            }

            return (y / size) * 16 + x / size;
        }
        else
        {
            return color;
        }
    }

    return color;
}

void PalTable::do_curs(int key)
{
    bool done = false;
    bool first = true;
    int xoff = 0;
    int yoff = 0;

    while (!done)
    {
        switch (key)
        {
        case ID_KEY_CTL_RIGHT_ARROW:
            xoff += CURS_INC * 4;
            break;
        case ID_KEY_RIGHT_ARROW:
            xoff += CURS_INC;
            break;
        case ID_KEY_CTL_LEFT_ARROW:
            xoff -= CURS_INC * 4;
            break;
        case ID_KEY_LEFT_ARROW:
            xoff -= CURS_INC;
            break;
        case ID_KEY_CTL_DOWN_ARROW:
            yoff += CURS_INC * 4;
            break;
        case ID_KEY_DOWN_ARROW:
            yoff += CURS_INC;
            break;
        case ID_KEY_CTL_UP_ARROW:
            yoff -= CURS_INC * 4;
            break;
        case ID_KEY_UP_ARROW:
            yoff -= CURS_INC;
            break;

        default:
            done = true;
        }

        if (!done)
        {
            if (!first)
            {
                driver_get_key(); // delete key from buffer
            }
            else
            {
                first = false;
            }
            key = driver_key_pressed(); // peek at the next one...
        }
    }

    Cursor_Move(xoff, yoff);

    if (m_auto_select)
    {
        set_curr(m_active, get_cursor_color());
    }
}

void PalTable::rotate(int dir, int lo, int hi)
{
    rotate_pal(m_pal, dir, lo, hi);

    Cursor_Hide();

    // update the DAC.

    update_dac();

    // update the editors.

    m_rgb[0]->set_rgb(m_curr[0], &(m_pal[m_curr[0]]));
    m_rgb[1]->set_rgb(m_curr[1], &(m_pal[m_curr[1]]));
    m_rgb[0]->update();
    m_rgb[1]->update();

    Cursor_Show();
}

void PalTable::update_dac()
{
    if (m_exclude)
    {
        std::memset(g_dac_box, 0, 256 * 3);
        if (m_exclude == 1)
        {
            int a = m_curr[m_active];
            std::memmove(g_dac_box[a], &m_pal[a], 3);
        }
        else
        {
            int a = m_curr[0];
            int b = m_curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            std::memmove(g_dac_box[a], &m_pal[a], 3 * (1 + (b - a)));
        }
    }
    else
    {
        std::memmove(g_dac_box[0], m_pal, 3 * g_colors);

        if (m_free_style)
        {
            put_band((PALENTRY *) g_dac_box); // apply band to g_dac_box
        }
    }

    if (!m_hidden)
    {
        if (s_inverse)
        {
            std::memset(g_dac_box[s_fg_color], 0, 3);  // g_dac_box[fg] = (0,0,0)
            std::memset(g_dac_box[s_bg_color], 48, 3); // g_dac_box[bg] = (48,48,48)
        }
        else
        {
            std::memset(g_dac_box[s_bg_color], 0, 3);  // g_dac_box[bg] = (0,0,0)
            std::memset(g_dac_box[s_fg_color], 48, 3); // g_dac_box[fg] = (48,48,48)
        }
    }

    spindac(0, 1);
}

void PalTable::save_undo_data(int first, int last)
{
    if (m_undo_file == nullptr)
    {
        return;
    }

    int num;

    num = (last - first) + 1;
    std::fseek(m_undo_file, 0, SEEK_CUR);
    if (num == 1)
    {
        putc(UNDO_DATA_SINGLE, m_undo_file);
        putc(first, m_undo_file);
        std::fwrite(m_pal + first, 3, 1, m_undo_file);
        putw(1 + 1 + 3 + sizeof(int), m_undo_file);
    }
    else
    {
        putc(UNDO_DATA, m_undo_file);
        putc(first, m_undo_file);
        putc(last, m_undo_file);
        std::fwrite(m_pal + first, 3, num, m_undo_file);
        putw(1 + 2 + (num * 3) + sizeof(int), m_undo_file);
    }

    m_num_redo = 0;
}

void PalTable::save_undo_rotate(int dir, int first, int last)
{
    if (m_undo_file == nullptr)
    {
        return;
    }

    std::fseek(m_undo_file, 0, SEEK_CUR);
    putc(UNDO_ROTATE, m_undo_file);
    putc(first, m_undo_file);
    putc(last, m_undo_file);
    putw(dir, m_undo_file);
    putw(1 + 2 + sizeof(int), m_undo_file);

    m_num_redo = 0;
}

void PalTable::undo_process(int delta)
{
    // delta = -1 for undo, +1 for redo
    int cmd = getc(m_undo_file);

    switch (cmd)
    {
    case UNDO_DATA:
    case UNDO_DATA_SINGLE:
    {
        int first;
        int last;
        int num;
        PALENTRY temp[256];

        if (cmd == UNDO_DATA)
        {
            first = (unsigned char) getc(m_undo_file);
            last = (unsigned char) getc(m_undo_file);
        }
        else // UNDO_DATA_SINGLE
        {
            last = (unsigned char) getc(m_undo_file);
            first = last;
        }

        num = (last - first) + 1;
        if (std::fread(temp, 3, num, m_undo_file) != num)
        {
            throw std::system_error(errno, std::system_category(), "UndoProcess  failed fread");
        }

        std::fseek(m_undo_file, -(num * 3), SEEK_CUR); // go to start of undo/redo data
        std::fwrite(m_pal + first, 3, num, m_undo_file); // write redo/undo data

        std::memmove(m_pal + first, temp, num * 3);

        update_dac();

        m_rgb[0]->set_rgb(m_curr[0], &(m_pal[m_curr[0]]));
        m_rgb[1]->set_rgb(m_curr[1], &(m_pal[m_curr[1]]));
        m_rgb[0]->update();
        m_rgb[1]->update();
        break;
    }

    case UNDO_ROTATE:
    {
        int first = (unsigned char) getc(m_undo_file);
        int last = (unsigned char) getc(m_undo_file);
        int dir = getw(m_undo_file);
        rotate(delta * dir, first, last);
        break;
    }

    default:
        break;
    }

    std::fseek(m_undo_file, 0, SEEK_CUR); // to put us in read mode
    getw(m_undo_file);                    // read size
}

void PalTable::undo()
{
    int size;
    long pos;

    if (ftell(m_undo_file) <= 0) // at beginning of file?
    {
        //   nothing to undo -- exit
        return;
    }

    std::fseek(m_undo_file, -(int) sizeof(int), SEEK_CUR); // go back to get size

    size = getw(m_undo_file);
    std::fseek(m_undo_file, -size, SEEK_CUR); // go to start of undo

    pos = ftell(m_undo_file);

    undo_process(-1);

    std::fseek(m_undo_file, pos, SEEK_SET); // go to start of me block

    ++m_num_redo;
}

void PalTable::redo()
{
    if (m_num_redo <= 0)
    {
        return;
    }

    std::fseek(m_undo_file, 0, SEEK_CUR); // to make sure we are in "read" mode
    undo_process(1);

    --m_num_redo;
}

void PalTable::mk_default_palettes()
{
    for (int i = 0; i < 8; i++) // copy original palette to save areas
    {
        std::memcpy(m_save_pal[i], m_pal, 256 * sizeof(PALENTRY));
    }
}

void PalTable::hide(RGBEditor *rgb, bool hidden)
{
    if (hidden)
    {
        restore_rect();
        set_hidden(true);
        s_reserve_colors = false;
        if (m_auto_select)
        {
            set_curr(m_active, get_cursor_color());
        }
    }
    else
    {
        set_hidden(false);
        s_reserve_colors = true;
        if (m_stored_at == NOWHERE) // do we need to save screen?
        {
            save_rect();
        }
        draw();
        if (m_auto_select)
        {
            set_curr(m_active, get_cursor_color());
        }
        rgb->set_done(true);
    }
}

void PalTable::calc_top_bottom()
{
    if (m_curr[m_active] < m_band_width)
    {
        m_bottom = 0;
    }
    else
    {
        m_bottom = (m_curr[m_active]) - m_band_width;
    }

    if (m_curr[m_active] > (255 - m_band_width))
    {
        m_top = 255;
    }
    else
    {
        m_top = (m_curr[m_active]) + m_band_width;
    }
}

void PalTable::other_key(int key, RGBEditor *rgb)
{
    switch (key)
    {
    case '\\': // move/resize
    {
        if (m_hidden)
        {
            break; // cannot move a hidden pal
        }
        Cursor_Hide();
        restore_rect();
        m_move_box->set_pos(m_x, m_y);
        m_move_box->set_csize(m_csize);
        if (m_move_box->process())
        {
            if (m_move_box->should_hide())
            {
                set_hidden(true);
            }
            else if (m_move_box->moved())
            {
                set_pos(m_move_box->x(), m_move_box->y());
                set_csize(m_move_box->csize());
                save_rect();
            }
        }
        draw();
        Cursor_Show();

        this->m_rgb[m_active]->set_done(true);

        if (m_auto_select)
        {
            set_curr(m_active, get_cursor_color());
        }
        break;
    }

    case 'Y': // exclude range
    case 'y':
        if (m_exclude == 2)
        {
            m_exclude = 0;
        }
        else
        {
            m_exclude = 2;
        }
        update_dac();
        break;

    case 'X':
    case 'x': // exclude current entry
        if (m_exclude == 1)
        {
            m_exclude = 0;
        }
        else
        {
            m_exclude = 1;
        }
        update_dac();
        break;

    case ID_KEY_RIGHT_ARROW:
    case ID_KEY_LEFT_ARROW:
    case ID_KEY_UP_ARROW:
    case ID_KEY_DOWN_ARROW:
    case ID_KEY_CTL_RIGHT_ARROW:
    case ID_KEY_CTL_LEFT_ARROW:
    case ID_KEY_CTL_UP_ARROW:
    case ID_KEY_CTL_DOWN_ARROW:
        do_curs(key);
        break;

    case ID_KEY_ESC:
        m_done = true;
        rgb->set_done(true);
        break;

    case ' ': // select the other palette register
        m_active = (m_active == 0) ? 1 : 0;
        if (m_auto_select)
        {
            set_curr(m_active, get_cursor_color());
        }
        else
        {
            set_curr(-1, 0);
        }

        if (m_exclude || m_free_style)
        {
            update_dac();
        }

        rgb->set_done(true);
        break;

    case ID_KEY_ENTER:   // set register to color under cursor.  useful when not
    case ID_KEY_ENTER_2: // in auto_select mode

        if (m_free_style)
        {
            save_undo_data(m_bottom, m_top);
            put_band(m_pal);
        }

        set_curr(m_active, get_cursor_color());

        if (m_exclude || m_free_style)
        {
            update_dac();
        }

        rgb->set_done(true);
        break;

    case 'D': // copy (Duplicate?) color in inactive to color in active
    case 'd':
    {
        int a = m_active;
        int b = (a == 0) ? 1 : 0;
        PALENTRY t;

        t = this->m_rgb[b]->get_rgb();
        Cursor_Hide();

        this->m_rgb[a]->set_rgb(m_curr[a], &t);
        this->m_rgb[a]->update();
        change_cb(this->m_rgb[a], this);
        update_dac();

        Cursor_Show();
        break;
    }

    case '=': // create a shade range between the two entries
    {
        int a = m_curr[0];
        int b = m_curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        save_undo_data(a, b);

        if (a != b)
        {
            mk_pal_range(&m_pal[a], &m_pal[b], &m_pal[a], b - a, 1);
            update_dac();
        }

        break;
    }

    case '!': // swap r<->g
    {
        int a = m_curr[0];
        int b = m_curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        save_undo_data(a, b);

        if (a != b)
        {
            rot_col_r_g(&m_pal[a], b - a);
            update_dac();
        }

        break;
    }

    case '@': // swap g<->b
    case '"': // UK keyboards
    case 151: // French keyboards
    {
        int a = m_curr[0];
        int b = m_curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        save_undo_data(a, b);

        if (a != b)
        {
            rot_col_g_b(&m_pal[a], b - a);
            update_dac();
        }

        break;
    }

    case '#': // swap r<->b
    case 156: // UK keyboards (pound sign)
    case '$': // For French keyboards
    {
        int a = m_curr[0];
        int b = m_curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        save_undo_data(a, b);

        if (a != b)
        {
            rot_col_b_r(&m_pal[a], b - a);
            update_dac();
        }

        break;
    }

    case 'T':
    case 't': // s(T)ripe mode
    {
        Cursor_Hide();
        draw_status(true);
        const int key2 = getakeynohelp();
        Cursor_Show();

        if (key2 >= '1' && key2 <= '9')
        {
            int a = m_curr[0];
            int b = m_curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            save_undo_data(a, b);

            if (a != b)
            {
                mk_pal_range(&m_pal[a], &m_pal[b], &m_pal[a], b - a, key2 - '0');
                update_dac();
            }
        }

        break;
    }

    case 'M': // set gamma
    case 'm':
    {
        int i;
        char buf[20];
        std::snprintf(buf, std::size(buf), "%.3f", 1. / s_gamma_val);
        driver_stack_screen();
        i = field_prompt("Enter gamma value", nullptr, buf, 20, nullptr);
        driver_unstack_screen();
        if (i != -1)
        {
            std::sscanf(buf, "%f", &s_gamma_val);
            if (s_gamma_val == 0)
            {
                s_gamma_val = 0.0000000001F;
            }
            s_gamma_val = (float) (1.0 / s_gamma_val);
        }
    }
    break;
    case 'A': // toggle auto-select mode
    case 'a':
        m_auto_select = !m_auto_select;
        if (m_auto_select)
        {
            set_curr(m_active, get_cursor_color());
            if (m_exclude)
            {
                update_dac();
            }
        }
        break;

    case 'H':
    case 'h': // toggle hide/display of palette editor
        Cursor_Hide();
        hide(rgb, !m_hidden);
        Cursor_Show();
        break;

    case '.': // rotate once
    case ',':
    {
        int dir = (key == '.') ? 1 : -1;

        save_undo_rotate(dir, g_color_cycle_range_lo, g_color_cycle_range_hi);
        rotate(dir, g_color_cycle_range_lo, g_color_cycle_range_hi);
        break;
    }

    case '>': // continuous rotation (until a key is pressed)
    case '<':
    {
        int dir;
        long tick;
        int diff = 0;

        Cursor_Hide();

        if (!m_hidden)
        {
            this->m_rgb[0]->blank_sample_box();
            this->m_rgb[1]->blank_sample_box();
            this->m_rgb[0]->set_hidden(true);
            this->m_rgb[1]->set_hidden(true);
        }

        do
        {
            dir = (key == '>') ? 1 : -1;

            while (!driver_key_pressed())
            {
                tick = readticker();
                rotate(dir, g_color_cycle_range_lo, g_color_cycle_range_hi);
                diff += dir;
                while (readticker() == tick) // wait until a tick passes
                {
                }
            }

            key = driver_get_key();
        } while (key == '<' || key == '>');

        if (!m_hidden)
        {
            this->m_rgb[0]->set_hidden(false);
            this->m_rgb[1]->set_hidden(false);
            this->m_rgb[0]->update();
            this->m_rgb[1]->update();
        }

        if (diff != 0)
        {
            save_undo_rotate(diff, g_color_cycle_range_lo, g_color_cycle_range_hi);
        }

        Cursor_Show();
        break;
    }

    case 'I': // invert the fg & bg colors
    case 'i':
        s_inverse = !s_inverse;
        update_dac();
        break;

    case 'V':
    case 'v': // set the reserved colors to the editor colors
        if (m_curr[0] >= g_colors || m_curr[1] >= g_colors || m_curr[0] == m_curr[1])
        {
            driver_buzzer(buzzer_codes::PROBLEM);
            break;
        }

        s_fg_color = (BYTE) m_curr[0];
        s_bg_color = (BYTE) m_curr[1];

        if (!m_hidden)
        {
            Cursor_Hide();
            update_dac();
            draw();
            Cursor_Show();
        }

        this->m_rgb[m_active]->set_done(true);
        break;

    case 'O': // set rotate_lo and rotate_hi to editors
    case 'o':
        if (m_curr[0] > m_curr[1])
        {
            g_color_cycle_range_lo = m_curr[1];
            g_color_cycle_range_hi = m_curr[0];
        }
        else
        {
            g_color_cycle_range_lo = m_curr[0];
            g_color_cycle_range_hi = m_curr[1];
        }
        break;

    case ID_KEY_F2: // restore a palette
    case ID_KEY_F3:
    case ID_KEY_F4:
    case ID_KEY_F5:
    case ID_KEY_F6:
    case ID_KEY_F7:
    case ID_KEY_F8:
    case ID_KEY_F9:
    {
        int which = key - ID_KEY_F2;

        Cursor_Hide();

        save_undo_data(0, 255);
        std::memcpy(m_pal, m_save_pal[which], 256 * sizeof(PALENTRY));
        update_dac();

        set_curr(-1, 0);
        Cursor_Show();
        this->m_rgb[m_active]->set_done(true);
        break;
    }

    case ID_KEY_SF2: // save a palette
    case ID_KEY_SF3:
    case ID_KEY_SF4:
    case ID_KEY_SF5:
    case ID_KEY_SF6:
    case ID_KEY_SF7:
    case ID_KEY_SF8:
    case ID_KEY_SF9:
    {
        int which = key - ID_KEY_SF2;
        std::memcpy(m_save_pal[which], m_pal, 256 * sizeof(PALENTRY));
        break;
    }

    case 'L': // load a .map palette
    case 'l':
    {
        save_undo_data(0, 255);

        load_palette();
#ifndef XFRACT
        get_pal_range(0, g_colors, m_pal);
#else
        getpalrange(0, 256, pal);
#endif
        update_dac();
        this->m_rgb[0]->set_rgb(m_curr[0], &(m_pal[m_curr[0]]));
        this->m_rgb[0]->update();
        this->m_rgb[1]->set_rgb(m_curr[1], &(m_pal[m_curr[1]]));
        this->m_rgb[1]->update();
        break;
    }

    case 'S': // save a .map palette
    case 's':
    {
#ifndef XFRACT
        set_pal_range(0, g_colors, m_pal);
#else
        setpalrange(0, 256, pal);
#endif
        save_palette();
        update_dac();
        break;
    }

    case 'C': // color cycling sub-mode
    case 'c':
    {
        bool oldhidden = m_hidden;

        save_undo_data(0, 255);

        Cursor_Hide();
        if (!oldhidden)
        {
            hide(rgb, true);
        }
        set_pal_range(0, g_colors, m_pal);
        ::rotate(0);
        get_pal_range(0, g_colors, m_pal);
        update_dac();
        if (!oldhidden)
        {
            this->m_rgb[0]->set_rgb(m_curr[0], &(m_pal[m_curr[0]]));
            this->m_rgb[1]->set_rgb(m_curr[1], &(m_pal[m_curr[1]]));
            hide(rgb, false);
        }
        Cursor_Show();
        break;
    }

    case 'F':
    case 'f': // toggle freestyle palette edit mode
        m_free_style = !m_free_style;

        set_curr(-1, 0);

        if (!m_free_style) // if turning off...
        {
            update_dac();
        }

        break;

    case ID_KEY_CTL_DEL: // rt plus down
        if (m_band_width > 0)
        {
            m_band_width--;
        }
        else
        {
            m_band_width = 0;
        }
        set_curr(-1, 0);
        break;

    case ID_KEY_CTL_INSERT: // rt plus up
        if (m_band_width < 255)
        {
            m_band_width++;
        }
        else
        {
            m_band_width = 255;
        }
        set_curr(-1, 0);
        break;

    case 'W': // convert to greyscale
    case 'w':
    {
        switch (m_exclude)
        {
        case 0: // normal mode.  convert all colors to grey scale
            save_undo_data(0, 255);
            pal_range_to_grey(m_pal, 0, 256);
            break;

        case 1: // 'x' mode. convert current color to grey scale.
            save_undo_data(m_curr[m_active], m_curr[m_active]);
            pal_range_to_grey(m_pal, m_curr[m_active], 1);
            break;

        case 2: // 'y' mode.  convert range between editors to grey.
        {
            int a = m_curr[0];
            int b = m_curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            save_undo_data(a, b);
            pal_range_to_grey(m_pal, a, 1 + (b - a));
            break;
        }
        }

        update_dac();
        this->m_rgb[0]->set_rgb(m_curr[0], &(m_pal[m_curr[0]]));
        this->m_rgb[0]->update();
        this->m_rgb[1]->set_rgb(m_curr[1], &(m_pal[m_curr[1]]));
        this->m_rgb[1]->update();
        break;
    }

    case 'N': // convert to negative color
    case 'n':
    {
        switch (m_exclude)
        {
        case 0: // normal mode.  convert all colors to grey scale
            save_undo_data(0, 255);
            pal_range_to_negative(m_pal, 0, 256);
            break;

        case 1: // 'x' mode. convert current color to grey scale.
            save_undo_data(m_curr[m_active], m_curr[m_active]);
            pal_range_to_negative(m_pal, m_curr[m_active], 1);
            break;

        case 2: // 'y' mode.  convert range between editors to grey.
        {
            int a = m_curr[0];
            int b = m_curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            save_undo_data(a, b);
            pal_range_to_negative(m_pal, a, 1 + (b - a));
            break;
        }
        }

        update_dac();
        this->m_rgb[0]->set_rgb(m_curr[0], &(m_pal[m_curr[0]]));
        this->m_rgb[0]->update();
        this->m_rgb[1]->set_rgb(m_curr[1], &(m_pal[m_curr[1]]));
        this->m_rgb[1]->update();
        break;
    }

    case 'U': // Undo
    case 'u':
        undo();
        break;

    case 'e': // Redo
    case 'E':
        redo();
        break;

    } // switch
    draw_status(false);
}

void PalTable::put_band(PALENTRY *pal)
{
    int r;
    int b;
    int a;

    // clip top and bottom values to stop them running off the end of the DAC

    calc_top_bottom();

    // put bands either side of current colour

    a = m_curr[m_active];
    b = m_bottom;
    r = m_top;

    pal[a] = m_fs_color;

    if (r != a && a != b)
    {
        mk_pal_range(&pal[a], &pal[r], &pal[a], r-a, 1);
        mk_pal_range(&pal[b], &pal[a], &pal[b], a-b, 1);
    }

}

void PalTable::set_hidden(bool hidden)
{
    this->m_hidden = hidden;
    m_rgb[0]->set_hidden(hidden);
    m_rgb[1]->set_hidden(hidden);
    update_dac();
}

void PalTable::change(RGBEditor *rgb)
{
    int       pnum = m_curr[m_active];

    if (m_free_style)
    {
        m_fs_color = rgb->get_rgb();
        update_dac();
        return;
    }

    if (!m_curr_changed)
    {
        save_undo_data(pnum, pnum);
        m_curr_changed = true;
    }

    m_pal[pnum] = rgb->get_rgb();

    if (m_curr[0] == m_curr[1])
    {
        int      other = m_active == 0 ? 1 : 0;
        PALENTRY color;

        color = this->m_rgb[m_active]->get_rgb();
        this->m_rgb[other]->set_rgb(m_curr[other], &color);

        Cursor_Hide();
        this->m_rgb[other]->update();
        Cursor_Show();
    }

}

void PalTable::change_cb(RGBEditor *rgb, void *info)
{
    static_cast<PalTable *>(info)->change(rgb);
}

void PalTable::other_key_cb(int key, RGBEditor *rgb, void *info)
{
    static_cast<PalTable *>(info)->other_key(key, rgb);
}

PalTable::PalTable()
{
    m_rgb[0] = new RGBEditor(0, 0, &PalTable::other_key_cb, &PalTable::change_cb, this);
    m_rgb[1] = new RGBEditor(0, 0, &PalTable::other_key_cb, &PalTable::change_cb, this);
    m_move_box = new MoveBox(0, 0, 0, PalTable_PALX + 1, PalTable_PALY + 1);
    m_active      = 0;
    m_curr[0]     = 1;
    m_curr[1]     = 1;
    m_auto_select = true;
    m_exclude     = 0;
    m_hidden      = false;
    m_stored_at   = NOWHERE;
    m_fs_color.red   = 42;
    m_fs_color.green = 42;
    m_fs_color.blue  = 42;
    m_free_style      = false;
    m_band_width      = 15;
    m_top            = 255;
    m_bottom         = 0 ;
    m_undo_file    = dir_fopen(g_temp_dir.c_str(), s_undo_file, "w+b");
    m_curr_changed = false;
    m_num_redo     = 0;

    m_rgb[0]->set_rgb(m_curr[0], &m_pal[m_curr[0]]);
    m_rgb[1]->set_rgb(m_curr[1], &m_pal[m_curr[0]]);

    if (g_video_scroll)
    {
        set_pos(g_video_start_x, g_video_start_y);
        m_csize = ((g_vesa_y_res-(PalTable_PALY+1+1)) / 2) / 16;
    }
    else
    {
        set_pos(0, 0);
        m_csize = ((g_screen_y_dots-(PalTable_PALY+1+1)) / 2) / 16;
    }

    if (m_csize < CSIZE_MIN)
    {
        m_csize = CSIZE_MIN;
    }
    set_csize(m_csize);
}

PalTable::~PalTable()
{
    if (m_undo_file != nullptr)
    {
        std::fclose(m_undo_file);
        dir_remove(g_temp_dir.c_str(), s_undo_file);
    }

    delete m_rgb[0];
    delete m_rgb[1];
    delete m_move_box;
}

void EditPalette()
{
    ValueSaver saved_look_at_mouse(g_look_at_mouse, 3);
    ValueSaver saved_logical_screen_x_offset(g_logical_screen_x_offset, 0);
    ValueSaver saved_logical_screen_y_offset(g_logical_screen_y_offset, 0);

    if (g_screen_x_dots < 133 || g_screen_y_dots < 174)
    {
        return; // prevents crash when physical screen is too small
    }

    g_plot = g_put_color;

    g_line_buff.resize(std::max(g_screen_x_dots, g_screen_y_dots));

    s_reserve_colors = true;
    s_inverse = false;
    s_fg_color = (BYTE)(255%g_colors);
    s_bg_color = (BYTE)(s_fg_color-1);

    Cursor_Construct();
    PalTable pt;
    pt.process();

    g_line_buff.clear();
}
