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
    void Process();
    void SetHidden(bool hidden);

private:
    void DrawStatus(bool stripe_mode);
    void HlPal(int pnum, int color);
    void Draw();
    void SetCurr(int which, int curr);
    void SaveRect();
    void RestoreRect();
    void SetPos(int x, int y);
    void SetCSize(int csize);
    int GetCursorColor() const;
    void DoCurs(int key);
    void Rotate(int dir, int lo, int hi);
    void UpdateDAC();
    void SaveUndoData(int first, int last);
    void SaveUndoRotate(int dir, int first, int last);
    void UndoProcess(int delta);
    void Undo();
    void Redo();
    void MkDefaultPalettes();
    void Hide(RGBEditor *rgb, bool hidden);
    void CalcTopBottom();
    void PutBand(PALENTRY *pal);
    void other_key(int key, RGBEditor *rgb);
    void change(RGBEditor *rgb);
    
    static void other_key_cb(int key, RGBEditor *rgb, void *info);
    static void change_cb(RGBEditor *rgb, void *info);

    int x;
    int y;
    int           csize;
    int           active;   // which RGBEditor is active (0,1)
    int           curr[2];
    RGBEditor    *rgb[2];
    MoveBox      *movebox;
    bool done;
    int exclude;
    bool auto_select;
    PALENTRY      pal[256];
    std::FILE         *undo_file;
    bool curr_changed;
    int           num_redo;
    bool hidden;
    int           stored_at;
    std::vector<char> saved_pixels;
    PALENTRY     save_pal[8][256];
    PALENTRY      fs_color;
    int           top, bottom; // top and bottom colours of freestyle band
    int           bandwidth; //size of freestyle colour band
    bool freestyle;

};

bool g_using_jiim{};
std::vector<BYTE> g_line_buff;

static const char *s_undo_file{"id.$$2"};  // file where undo list is stored
#ifdef XFRACT
static bool s_editpal_cursor{};
#endif
static BYTE s_fg_color{};
static BYTE s_bg_color{};
static bool s_reserve_colors{};
static bool s_inverse{};
static float s_gamma_val{1.0f};
static Cursor s_cursor{};

// Interface to graphics stuff
static void setpal(int pal, int r, int g, int b)
{
    g_dac_box[pal][0] = (BYTE)r;
    g_dac_box[pal][1] = (BYTE)g;
    g_dac_box[pal][2] = (BYTE)b;
    spindac(0, 1);
}

static void setpalrange(int first, int how_many, PALENTRY *pal)
{
    std::memmove(g_dac_box+first, pal, how_many*3);
    spindac(0, 1);
}

static void getpalrange(int first, int how_many, PALENTRY *pal)
{
    std::memmove(pal, g_dac_box+first, how_many*3);
}

static void rotatepal(PALENTRY *pal, int dir, int lo, int hi)
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

void clip_putcolor(int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= g_screen_x_dots || y >= g_screen_y_dots)
    {
        return ;
    }

    g_put_color(x, y, color);
}

int clip_getcolor(int x, int y)
{
    if (x < 0 || y < 0 || x >= g_screen_x_dots || y >= g_screen_y_dots)
    {
        return 0;
    }

    return getcolor(x, y);
}

static void hline(int x, int y, int width, int color)
{
    std::memset(&g_line_buff[0], color, width);
    clip_put_line(y, x, x+width-1, &g_line_buff[0]);
}

static void vline(int x, int y, int depth, int color)
{
    while (depth-- > 0)
    {
        clip_putcolor(x, y++, color);
    }
}

void getrow(int x, int y, int width, char *buff)
{
    clip_get_line(y, x, x+width-1, (BYTE *)buff);
}

void putrow(int x, int y, int width, char const *buff)
{
    clip_put_line(y, x, x+width-1, (BYTE *)buff);
}

static void vgetrow(int x, int y, int depth, char *buff)
{
    while (depth-- > 0)
    {
        *buff++ = (char)clip_getcolor(x, y++);
    }
}

static void vputrow(int x, int y, int depth, char const *buff)
{
    while (depth-- > 0)
    {
        clip_putcolor(x, y++, (BYTE)(*buff++));
    }
}

static void fillrect(int x, int y, int width, int depth, int color)
{
    while (depth-- > 0)
    {
        hline(x, y++, width, color);
    }
}

static void rect(int x, int y, int width, int depth, int color)
{
    hline(x, y, width, color);
    hline(x, y+depth-1, width, color);

    vline(x, y, depth, color);
    vline(x+width-1, y, depth, color);
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
static void mkpalrange(PALENTRY *p1, PALENTRY *p2, PALENTRY pal[], int num, int skip)
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
static void rotcolrg(PALENTRY pal[], int num)
{
    for (int curr = 0; curr <= num; curr++)
    {
        int dummy = pal[curr].red;
        pal[curr].red = pal[curr].green;
        pal[curr].green = (BYTE)dummy;
    }
}

static void rotcolgb(PALENTRY pal[], int num)
{
    for (int curr = 0; curr <= num; curr++)
    {
        int const dummy = pal[curr].green;
        pal[curr].green = pal[curr].blue;
        pal[curr].blue = (BYTE)dummy;
    }
}

static void rotcolbr(PALENTRY pal[], int num)
{
    for (int curr = 0; curr <= num; curr++)
    {
        int const dummy = pal[curr].red;
        pal[curr].red = pal[curr].blue;
        pal[curr].blue = (BYTE)dummy;
    }
}

// convert a range of colors to grey scale
static void palrangetogrey(PALENTRY pal[], int first, int how_many)
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
static void palrangetonegative(PALENTRY pal[], int first, int how_many)
{
    for (PALENTRY *curr = &pal[first]; how_many > 0; how_many--, curr++)
    {
        curr->red   = (BYTE)(63 - curr->red);
        curr->green = (BYTE)(63 - curr->green);
        curr->blue  = (BYTE)(63 - curr->blue);
    }
}

// draw and horizontal/vertical dotted lines
static void hdline(int x, int y, int width)
{
    BYTE *ptr = &g_line_buff[0];
    for (int ctr = 0; ctr < width; ctr++, ptr++)
    {
        *ptr = (BYTE)((ctr&2) ? s_bg_color : s_fg_color);
    }

    putrow(x, y, width, (char *) &g_line_buff[0]);
}

static void vdline(int x, int y, int depth)
{
    for (int ctr = 0; ctr < depth; ctr++, y++)
    {
        clip_putcolor(x, y, (ctr&2) ? s_bg_color : s_fg_color);
    }
}

static void drect(int x, int y, int width, int depth)
{
    hdline(x, y, width);
    hdline(x, y+depth-1, width);

    vdline(x, y, depth);
    vdline(x+width-1, y, depth);
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
    hline(x+1, y+1, 3, color);
    hline(x+0, y+2, 5, color);
    hline(x+1, y+3, 3, color);
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

    getrow(m_x, m_y, width, &m_t[0]);
    getrow(m_x, m_y + depth - 1, width, &m_b[0]);

    vgetrow(m_x, m_y, depth, &m_l[0]);
    vgetrow(m_x + width - 1, m_y, depth, &m_r[0]);

    hdline(m_x, m_y, width);
    hdline(m_x, m_y + depth - 1, width);

    vdline(m_x, m_y, depth);
    vdline(m_x + width - 1, m_y, depth);
}

void MoveBox::erase()
{
    int width = m_base_width + m_csize * 16 + 1;
    int depth = m_base_depth + m_csize * 16 + 1;

    vputrow(m_x, m_y, depth, &m_l[0]);
    vputrow(m_x + width - 1, m_y, depth, &m_r[0]);

    putrow(m_x, m_y, width, &m_t[0]);
    putrow(m_x, m_y + depth - 1, width, &m_b[0]);
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

    vline(s_cursor.x, s_cursor.y-CURSOR_SIZE-1, CURSOR_SIZE, color);
    vline(s_cursor.x, s_cursor.y+2,             CURSOR_SIZE, color);

    hline(s_cursor.x-CURSOR_SIZE-1, s_cursor.y, CURSOR_SIZE, color);
    hline(s_cursor.x+2,             s_cursor.y, CURSOR_SIZE, color);
}

static void Cursor__Save()
{
    vgetrow(s_cursor.x, s_cursor.y-CURSOR_SIZE-1, CURSOR_SIZE, s_cursor.t);
    vgetrow(s_cursor.x, s_cursor.y+2,             CURSOR_SIZE, s_cursor.b);

    getrow(s_cursor.x-CURSOR_SIZE-1, s_cursor.y,  CURSOR_SIZE, s_cursor.l);
    getrow(s_cursor.x+2,             s_cursor.y,  CURSOR_SIZE, s_cursor.r);
}

static void Cursor__Restore()
{
    vputrow(s_cursor.x, s_cursor.y-CURSOR_SIZE-1, CURSOR_SIZE, s_cursor.t);
    vputrow(s_cursor.x, s_cursor.y+2,             CURSOR_SIZE, s_cursor.b);

    putrow(s_cursor.x-CURSOR_SIZE-1, s_cursor.y,  CURSOR_SIZE, s_cursor.l);
    putrow(s_cursor.x+2,             s_cursor.y,  CURSOR_SIZE, s_cursor.r);
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
    s_editpal_cursor = true;
}

void Cursor_EndMouseTracking()
{
    s_editpal_cursor = false;
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
    fillrect(
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
        fillrect(x1, y1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, s_bg_color);
        draw_diamond(x1 + (RGBEditor_BWIDTH - 5) / 2, y1 + (RGBEditor_BDEPTH - 5) / 2, s_fg_color);
    }

    else if (is_reserved(m_pal))
    {
        int x2 = x1 + RGBEditor_BWIDTH - 3;
        int y2 = y1 + RGBEditor_BDEPTH - 3;

        fillrect(x1, y1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, s_bg_color);
        driver_draw_line(x1, y1, x2, y2, s_fg_color);
        driver_draw_line(x1, y2, x2, y1, s_fg_color);
    }
    else
    {
        fillrect(x1, y1, RGBEditor_BWIDTH - 2, RGBEditor_BDEPTH - 2, m_pal);
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
    drect(m_x, m_y, RGBEditor_WIDTH, RGBEditor_DEPTH);
    fillrect(m_x + 1, m_y + 1, RGBEditor_WIDTH - 2, RGBEditor_DEPTH - 2, s_bg_color);
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
        drect(m_x, m_y, RGBEditor_WIDTH, RGBEditor_DEPTH);
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
        setpal(m_pal, m_color[0]->get_val(), m_color[1]->get_val(), m_color[2]->get_val());
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

void PalTable::Process()
{
    getpalrange(0, g_colors, pal);

    // Make sure all palette entries are 0-63

    for (int ctr = 0; ctr < 768; ctr++)
    {
        ((char *)pal)[ctr] &= 63;
    }

    UpdateDAC();

    rgb[0]->set_rgb(curr[0], &pal[curr[0]]);
    rgb[1]->set_rgb(curr[1], &pal[curr[0]]);

    if (!hidden)
    {
        movebox->set_pos(x, y);
        movebox->set_csize(csize);
        if (!movebox->process())
        {
            setpalrange(0, g_colors, pal);
            return ;
        }

        SetPos(movebox->x(), movebox->y());
        SetCSize(movebox->csize());

        if (movebox->should_hide())
        {
            SetHidden(true);
            s_reserve_colors = false;
        }
        else
        {
            s_reserve_colors = true;
            SaveRect();
            Draw();
        }
    }

    SetCurr(active,          GetCursorColor());
    SetCurr((active == 1)?0:1, GetCursorColor());
    Cursor_Show();
    MkDefaultPalettes();
    done = false;

    while (!done)
    {
        rgb[active]->edit();
    }

    Cursor_Hide();
    RestoreRect();
    setpalrange(0, g_colors, pal);
}

// - everything else -
void PalTable::DrawStatus(bool stripe_mode)
{
    int width = 1 + (csize * 16) + 1 + 1;

    if (!hidden && (width - (RGBEditor_WIDTH * 2 + 4) >= STATUS_LEN * 8))
    {
        int x = this->x + 2 + RGBEditor_WIDTH;
        int y = this->y + PalTable_PALY - 10;
        int color = GetCursorColor();
        if (color < 0 || color >= g_colors) // hmm, the border returns -1
        {
            color = 0;
        }
        Cursor_Hide();

        {
            char buff[80];
            std::snprintf(buff, std::size(buff), "%c%c%c%c", auto_select ? 'A' : ' ',
                (exclude == 1)       ? 'X'
                    : (exclude == 2) ? 'Y'
                                     : ' ',
                freestyle ? 'F' : ' ', stripe_mode ? 'T' : ' ');
            driver_display_string(x, y, s_fg_color, s_bg_color, buff);

            y = y - 10;
            std::snprintf(buff, std::size(buff), "%-3d", color); // assumes 8-bit color, 0-255 values
            driver_display_string(x, y, s_fg_color, s_bg_color, buff);
        }
        Cursor_Show();
    }
}

void PalTable::HlPal(int pnum, int color)
{
    if (hidden)
    {
        return;
    }

    int x = this->x + PalTable_PALX + (pnum % 16) * csize;
    int y = this->y + PalTable_PALY + (pnum / 16) * csize;

    Cursor_Hide();

    const int size = csize + 1;
    if (color < 0)
    {
        drect(x, y, size, size);
    }
    else
    {
        rect(x, y, size, size, color);
    }

    Cursor_Show();
}

void PalTable::Draw()
{
    if (hidden)
    {
        return;
    }

    Cursor_Hide();

    int width = 1 + (csize * 16) + 1 + 1;

    rect(x, y, width, 2 + RGBEditor_DEPTH + 2 + (csize * 16) + 1 + 1, s_fg_color);

    fillrect(x + 1, y + 1, width - 2, 2 + RGBEditor_DEPTH + 2 + (csize * 16) + 1 + 1 - 2, s_bg_color);

    hline(x, y + PalTable_PALY - 1, width, s_fg_color);

    if (width - (RGBEditor_WIDTH * 2 + 4) >= TITLE_LEN * 8)
    {
        int center = (width - TITLE_LEN * 8) / 2;

        displayf(x + center, y + RGBEditor_DEPTH / 2 - 12, s_fg_color, s_bg_color, ID_PROGRAM_NAME);
    }

    rgb[0]->draw();
    rgb[1]->draw();

    for (int pal = 0; pal < 256; pal++)
    {
        int xoff = PalTable_PALX + (pal % 16) * csize;
        int yoff = PalTable_PALY + (pal / 16) * csize;

        if (pal >= g_colors)
        {
            fillrect(x + xoff + 1, y + yoff + 1, csize - 1, csize - 1, s_bg_color);
            draw_diamond(x + xoff + csize / 2 - 1, y + yoff + csize / 2 - 1, s_fg_color);
        }

        else if (is_reserved(pal))
        {
            int x1 = x + xoff + 1;
            int y1 = y + yoff + 1;
            int x2 = x1 + csize - 2;
            int y2 = y1 + csize - 2;
            fillrect(x + xoff + 1, y + yoff + 1, csize - 1, csize - 1, s_bg_color);
            driver_draw_line(x1, y1, x2, y2, s_fg_color);
            driver_draw_line(x1, y2, x2, y1, s_fg_color);
        }
        else
        {
            fillrect(x + xoff + 1, y + yoff + 1, csize - 1, csize - 1, pal);
        }
    }

    if (active == 0)
    {
        HlPal(curr[1], -1);
        HlPal(curr[0], s_fg_color);
    }
    else
    {
        HlPal(curr[0], -1);
        HlPal(curr[1], s_fg_color);
    }

    DrawStatus(false);

    Cursor_Show();
}

void PalTable::SetCurr(int which, int curr)
{
    bool redraw = which < 0;

    if (redraw)
    {
        which = active;
        curr = this->curr[which];
    }
    else if (curr == this->curr[which] || curr < 0)
    {
        return;
    }

    Cursor_Hide();

    HlPal(this->curr[0], s_bg_color);
    HlPal(this->curr[1], s_bg_color);
    HlPal(top, s_bg_color);
    HlPal(bottom, s_bg_color);

    if (freestyle)
    {
        this->curr[which] = curr;

        CalcTopBottom();

        HlPal(top, -1);
        HlPal(bottom, -1);
        HlPal(this->curr[active], s_fg_color);

        rgb[which]->set_rgb(this->curr[which], &fs_color);
        rgb[which]->update();

        UpdateDAC();

        Cursor_Show();

        return;
    }

    this->curr[which] = curr;

    if (this->curr[0] != this->curr[1])
    {
        HlPal(this->curr[active == 0 ? 1 : 0], -1);
    }
    HlPal(this->curr[active], s_fg_color);

    rgb[which]->set_rgb(this->curr[which], &(pal[this->curr[which]]));

    if (redraw)
    {
        int other = (which == 0) ? 1 : 0;
        rgb[other]->set_rgb(this->curr[other], &(pal[this->curr[other]]));
        rgb[0]->update();
        rgb[1]->update();
    }
    else
    {
        rgb[which]->update();
    }

    if (exclude)
    {
        UpdateDAC();
    }

    Cursor_Show();

    curr_changed = false;
}

void PalTable::SaveRect()
{
    int const width = PalTable_PALX + csize * 16 + 1 + 1;
    int const depth = PalTable_PALY + csize * 16 + 1 + 1;

    saved_pixels.resize(width * depth);
    Cursor_Hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        getrow(x, y + yoff, width, &saved_pixels[yoff * width]);
        hline(x, y + yoff, width, s_bg_color);
    }
    Cursor_Show();
}

void PalTable::RestoreRect()
{
    if (hidden)
    {
        return;
    }

    int width = PalTable_PALX + csize * 16 + 1 + 1;
    int depth = PalTable_PALY + csize * 16 + 1 + 1;

    Cursor_Hide();
    for (int yoff = 0; yoff < depth; yoff++)
    {
        putrow(x, y + yoff, width, &saved_pixels[yoff * width]);
    }
    Cursor_Show();
}

void PalTable::SetPos(int x, int y)
{
    int width = PalTable_PALX + csize * 16 + 1 + 1;

    this->x = x;
    this->y = y;

    rgb[0]->set_pos(x + 2, y + 2);
    rgb[1]->set_pos(x + width - 2 - RGBEditor_WIDTH, y + 2);
}

void PalTable::SetCSize(int csize)
{
    this->csize = csize;
    SetPos(x, y);
}

int PalTable::GetCursorColor() const
{
    int x = Cursor_GetX();
    int y = Cursor_GetY();
    int color = getcolor(x, y);

    if (is_reserved(color))
    {
        if (is_in_box(x, y, this->x, this->y, 1 + (csize * 16) + 1 + 1,
                2 + RGBEditor_DEPTH + 2 + (csize * 16) + 1 + 1))
        {
            // is the cursor over the editor?
            x -= this->x + PalTable_PALX;
            y -= this->y + PalTable_PALY;
            int size = csize;

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

void PalTable::DoCurs(int key)
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

    if (auto_select)
    {
        SetCurr(active, GetCursorColor());
    }
}

void PalTable::Rotate(int dir, int lo, int hi)
{
    rotatepal(pal, dir, lo, hi);

    Cursor_Hide();

    // update the DAC.

    UpdateDAC();

    // update the editors.

    rgb[0]->set_rgb(curr[0], &(pal[curr[0]]));
    rgb[1]->set_rgb(curr[1], &(pal[curr[1]]));
    rgb[0]->update();
    rgb[1]->update();

    Cursor_Show();
}

void PalTable::UpdateDAC()
{
    if (exclude)
    {
        std::memset(g_dac_box, 0, 256 * 3);
        if (exclude == 1)
        {
            int a = curr[active];
            std::memmove(g_dac_box[a], &pal[a], 3);
        }
        else
        {
            int a = curr[0];
            int b = curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            std::memmove(g_dac_box[a], &pal[a], 3 * (1 + (b - a)));
        }
    }
    else
    {
        std::memmove(g_dac_box[0], pal, 3 * g_colors);

        if (freestyle)
        {
            PutBand((PALENTRY *) g_dac_box); // apply band to g_dac_box
        }
    }

    if (!hidden)
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

void PalTable::SaveUndoData(int first, int last)
{
    if (undo_file == nullptr)
    {
        return;
    }

    int num;

    num = (last - first) + 1;
    std::fseek(undo_file, 0, SEEK_CUR);
    if (num == 1)
    {
        putc(UNDO_DATA_SINGLE, undo_file);
        putc(first, undo_file);
        std::fwrite(pal + first, 3, 1, undo_file);
        putw(1 + 1 + 3 + sizeof(int), undo_file);
    }
    else
    {
        putc(UNDO_DATA, undo_file);
        putc(first, undo_file);
        putc(last, undo_file);
        std::fwrite(pal + first, 3, num, undo_file);
        putw(1 + 2 + (num * 3) + sizeof(int), undo_file);
    }

    num_redo = 0;
}

void PalTable::SaveUndoRotate(int dir, int first, int last)
{
    if (undo_file == nullptr)
    {
        return;
    }

    std::fseek(undo_file, 0, SEEK_CUR);
    putc(UNDO_ROTATE, undo_file);
    putc(first, undo_file);
    putc(last, undo_file);
    putw(dir, undo_file);
    putw(1 + 2 + sizeof(int), undo_file);

    num_redo = 0;
}

void PalTable::UndoProcess(int delta)
{
    // delta = -1 for undo, +1 for redo
    int cmd = getc(undo_file);

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
            first = (unsigned char) getc(undo_file);
            last = (unsigned char) getc(undo_file);
        }
        else // UNDO_DATA_SINGLE
        {
            last = (unsigned char) getc(undo_file);
            first = last;
        }

        num = (last - first) + 1;
        if (std::fread(temp, 3, num, undo_file) != num)
        {
            throw std::system_error(errno, std::system_category(), "UndoProcess  failed fread");
        }

        std::fseek(undo_file, -(num * 3), SEEK_CUR); // go to start of undo/redo data
        std::fwrite(pal + first, 3, num, undo_file); // write redo/undo data

        std::memmove(pal + first, temp, num * 3);

        UpdateDAC();

        rgb[0]->set_rgb(curr[0], &(pal[curr[0]]));
        rgb[1]->set_rgb(curr[1], &(pal[curr[1]]));
        rgb[0]->update();
        rgb[1]->update();
        break;
    }

    case UNDO_ROTATE:
    {
        int first = (unsigned char) getc(undo_file);
        int last = (unsigned char) getc(undo_file);
        int dir = getw(undo_file);
        Rotate(delta * dir, first, last);
        break;
    }

    default:
        break;
    }

    std::fseek(undo_file, 0, SEEK_CUR); // to put us in read mode
    getw(undo_file);                    // read size
}

void PalTable::Undo()
{
    int size;
    long pos;

    if (ftell(undo_file) <= 0) // at beginning of file?
    {
        //   nothing to undo -- exit
        return;
    }

    std::fseek(undo_file, -(int) sizeof(int), SEEK_CUR); // go back to get size

    size = getw(undo_file);
    std::fseek(undo_file, -size, SEEK_CUR); // go to start of undo

    pos = ftell(undo_file);

    UndoProcess(-1);

    std::fseek(undo_file, pos, SEEK_SET); // go to start of me block

    ++num_redo;
}

void PalTable::Redo()
{
    if (num_redo <= 0)
    {
        return;
    }

    std::fseek(undo_file, 0, SEEK_CUR); // to make sure we are in "read" mode
    UndoProcess(1);

    --num_redo;
}

void PalTable::MkDefaultPalettes()
{
    for (int i = 0; i < 8; i++) // copy original palette to save areas
    {
        std::memcpy(save_pal[i], pal, 256 * sizeof(PALENTRY));
    }
}

void PalTable::Hide(RGBEditor *rgb, bool hidden)
{
    if (hidden)
    {
        RestoreRect();
        SetHidden(true);
        s_reserve_colors = false;
        if (auto_select)
        {
            SetCurr(active, GetCursorColor());
        }
    }
    else
    {
        SetHidden(false);
        s_reserve_colors = true;
        if (stored_at == NOWHERE) // do we need to save screen?
        {
            SaveRect();
        }
        Draw();
        if (auto_select)
        {
            SetCurr(active, GetCursorColor());
        }
        rgb->set_done(true);
    }
}

void PalTable::CalcTopBottom()
{
    if (curr[active] < bandwidth)
    {
        bottom = 0;
    }
    else
    {
        bottom = (curr[active]) - bandwidth;
    }

    if (curr[active] > (255 - bandwidth))
    {
        top = 255;
    }
    else
    {
        top = (curr[active]) + bandwidth;
    }
}

void PalTable::other_key(int key, RGBEditor *rgb)
{
    switch (key)
    {
    case '\\': // move/resize
    {
        if (hidden)
        {
            break; // cannot move a hidden pal
        }
        Cursor_Hide();
        RestoreRect();
        movebox->set_pos(x, y);
        movebox->set_csize(csize);
        if (movebox->process())
        {
            if (movebox->should_hide())
            {
                SetHidden(true);
            }
            else if (movebox->moved())
            {
                SetPos(movebox->x(), movebox->y());
                SetCSize(movebox->csize());
                SaveRect();
            }
        }
        Draw();
        Cursor_Show();

        this->rgb[active]->set_done(true);

        if (auto_select)
        {
            SetCurr(active, GetCursorColor());
        }
        break;
    }

    case 'Y': // exclude range
    case 'y':
        if (exclude == 2)
        {
            exclude = 0;
        }
        else
        {
            exclude = 2;
        }
        UpdateDAC();
        break;

    case 'X':
    case 'x': // exclude current entry
        if (exclude == 1)
        {
            exclude = 0;
        }
        else
        {
            exclude = 1;
        }
        UpdateDAC();
        break;

    case ID_KEY_RIGHT_ARROW:
    case ID_KEY_LEFT_ARROW:
    case ID_KEY_UP_ARROW:
    case ID_KEY_DOWN_ARROW:
    case ID_KEY_CTL_RIGHT_ARROW:
    case ID_KEY_CTL_LEFT_ARROW:
    case ID_KEY_CTL_UP_ARROW:
    case ID_KEY_CTL_DOWN_ARROW:
        DoCurs(key);
        break;

    case ID_KEY_ESC:
        done = true;
        rgb->set_done(true);
        break;

    case ' ': // select the other palette register
        active = (active == 0) ? 1 : 0;
        if (auto_select)
        {
            SetCurr(active, GetCursorColor());
        }
        else
        {
            SetCurr(-1, 0);
        }

        if (exclude || freestyle)
        {
            UpdateDAC();
        }

        rgb->set_done(true);
        break;

    case ID_KEY_ENTER:   // set register to color under cursor.  useful when not
    case ID_KEY_ENTER_2: // in auto_select mode

        if (freestyle)
        {
            SaveUndoData(bottom, top);
            PutBand(pal);
        }

        SetCurr(active, GetCursorColor());

        if (exclude || freestyle)
        {
            UpdateDAC();
        }

        rgb->set_done(true);
        break;

    case 'D': // copy (Duplicate?) color in inactive to color in active
    case 'd':
    {
        int a = active;
        int b = (a == 0) ? 1 : 0;
        PALENTRY t;

        t = this->rgb[b]->get_rgb();
        Cursor_Hide();

        this->rgb[a]->set_rgb(curr[a], &t);
        this->rgb[a]->update();
        change_cb(this->rgb[a], this);
        UpdateDAC();

        Cursor_Show();
        break;
    }

    case '=': // create a shade range between the two entries
    {
        int a = curr[0];
        int b = curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        SaveUndoData(a, b);

        if (a != b)
        {
            mkpalrange(&pal[a], &pal[b], &pal[a], b - a, 1);
            UpdateDAC();
        }

        break;
    }

    case '!': // swap r<->g
    {
        int a = curr[0];
        int b = curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        SaveUndoData(a, b);

        if (a != b)
        {
            rotcolrg(&pal[a], b - a);
            UpdateDAC();
        }

        break;
    }

    case '@': // swap g<->b
    case '"': // UK keyboards
    case 151: // French keyboards
    {
        int a = curr[0];
        int b = curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        SaveUndoData(a, b);

        if (a != b)
        {
            rotcolgb(&pal[a], b - a);
            UpdateDAC();
        }

        break;
    }

    case '#': // swap r<->b
    case 156: // UK keyboards (pound sign)
    case '$': // For French keyboards
    {
        int a = curr[0];
        int b = curr[1];

        if (a > b)
        {
            int t = a;
            a = b;
            b = t;
        }

        SaveUndoData(a, b);

        if (a != b)
        {
            rotcolbr(&pal[a], b - a);
            UpdateDAC();
        }

        break;
    }

    case 'T':
    case 't': // s(T)ripe mode
    {
        Cursor_Hide();
        DrawStatus(true);
        const int key2 = getakeynohelp();
        Cursor_Show();

        if (key2 >= '1' && key2 <= '9')
        {
            int a = curr[0];
            int b = curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            SaveUndoData(a, b);

            if (a != b)
            {
                mkpalrange(&pal[a], &pal[b], &pal[a], b - a, key2 - '0');
                UpdateDAC();
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
        auto_select = !auto_select;
        if (auto_select)
        {
            SetCurr(active, GetCursorColor());
            if (exclude)
            {
                UpdateDAC();
            }
        }
        break;

    case 'H':
    case 'h': // toggle hide/display of palette editor
        Cursor_Hide();
        Hide(rgb, !hidden);
        Cursor_Show();
        break;

    case '.': // rotate once
    case ',':
    {
        int dir = (key == '.') ? 1 : -1;

        SaveUndoRotate(dir, g_color_cycle_range_lo, g_color_cycle_range_hi);
        Rotate(dir, g_color_cycle_range_lo, g_color_cycle_range_hi);
        break;
    }

    case '>': // continuous rotation (until a key is pressed)
    case '<':
    {
        int dir;
        long tick;
        int diff = 0;

        Cursor_Hide();

        if (!hidden)
        {
            this->rgb[0]->blank_sample_box();
            this->rgb[1]->blank_sample_box();
            this->rgb[0]->set_hidden(true);
            this->rgb[1]->set_hidden(true);
        }

        do
        {
            dir = (key == '>') ? 1 : -1;

            while (!driver_key_pressed())
            {
                tick = readticker();
                Rotate(dir, g_color_cycle_range_lo, g_color_cycle_range_hi);
                diff += dir;
                while (readticker() == tick) // wait until a tick passes
                {
                }
            }

            key = driver_get_key();
        } while (key == '<' || key == '>');

        if (!hidden)
        {
            this->rgb[0]->set_hidden(false);
            this->rgb[1]->set_hidden(false);
            this->rgb[0]->update();
            this->rgb[1]->update();
        }

        if (diff != 0)
        {
            SaveUndoRotate(diff, g_color_cycle_range_lo, g_color_cycle_range_hi);
        }

        Cursor_Show();
        break;
    }

    case 'I': // invert the fg & bg colors
    case 'i':
        s_inverse = !s_inverse;
        UpdateDAC();
        break;

    case 'V':
    case 'v': // set the reserved colors to the editor colors
        if (curr[0] >= g_colors || curr[1] >= g_colors || curr[0] == curr[1])
        {
            driver_buzzer(buzzer_codes::PROBLEM);
            break;
        }

        s_fg_color = (BYTE) curr[0];
        s_bg_color = (BYTE) curr[1];

        if (!hidden)
        {
            Cursor_Hide();
            UpdateDAC();
            Draw();
            Cursor_Show();
        }

        this->rgb[active]->set_done(true);
        break;

    case 'O': // set rotate_lo and rotate_hi to editors
    case 'o':
        if (curr[0] > curr[1])
        {
            g_color_cycle_range_lo = curr[1];
            g_color_cycle_range_hi = curr[0];
        }
        else
        {
            g_color_cycle_range_lo = curr[0];
            g_color_cycle_range_hi = curr[1];
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

        SaveUndoData(0, 255);
        std::memcpy(pal, save_pal[which], 256 * sizeof(PALENTRY));
        UpdateDAC();

        SetCurr(-1, 0);
        Cursor_Show();
        this->rgb[active]->set_done(true);
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
        std::memcpy(save_pal[which], pal, 256 * sizeof(PALENTRY));
        break;
    }

    case 'L': // load a .map palette
    case 'l':
    {
        SaveUndoData(0, 255);

        load_palette();
#ifndef XFRACT
        getpalrange(0, g_colors, pal);
#else
        getpalrange(0, 256, pal);
#endif
        UpdateDAC();
        this->rgb[0]->set_rgb(curr[0], &(pal[curr[0]]));
        this->rgb[0]->update();
        this->rgb[1]->set_rgb(curr[1], &(pal[curr[1]]));
        this->rgb[1]->update();
        break;
    }

    case 'S': // save a .map palette
    case 's':
    {
#ifndef XFRACT
        setpalrange(0, g_colors, pal);
#else
        setpalrange(0, 256, pal);
#endif
        save_palette();
        UpdateDAC();
        break;
    }

    case 'C': // color cycling sub-mode
    case 'c':
    {
        bool oldhidden = hidden;

        SaveUndoData(0, 255);

        Cursor_Hide();
        if (!oldhidden)
        {
            Hide(rgb, true);
        }
        setpalrange(0, g_colors, pal);
        rotate(0);
        getpalrange(0, g_colors, pal);
        UpdateDAC();
        if (!oldhidden)
        {
            this->rgb[0]->set_rgb(curr[0], &(pal[curr[0]]));
            this->rgb[1]->set_rgb(curr[1], &(pal[curr[1]]));
            Hide(rgb, false);
        }
        Cursor_Show();
        break;
    }

    case 'F':
    case 'f': // toggle freestyle palette edit mode
        freestyle = !freestyle;

        SetCurr(-1, 0);

        if (!freestyle) // if turning off...
        {
            UpdateDAC();
        }

        break;

    case ID_KEY_CTL_DEL: // rt plus down
        if (bandwidth > 0)
        {
            bandwidth--;
        }
        else
        {
            bandwidth = 0;
        }
        SetCurr(-1, 0);
        break;

    case ID_KEY_CTL_INSERT: // rt plus up
        if (bandwidth < 255)
        {
            bandwidth++;
        }
        else
        {
            bandwidth = 255;
        }
        SetCurr(-1, 0);
        break;

    case 'W': // convert to greyscale
    case 'w':
    {
        switch (exclude)
        {
        case 0: // normal mode.  convert all colors to grey scale
            SaveUndoData(0, 255);
            palrangetogrey(pal, 0, 256);
            break;

        case 1: // 'x' mode. convert current color to grey scale.
            SaveUndoData(curr[active], curr[active]);
            palrangetogrey(pal, curr[active], 1);
            break;

        case 2: // 'y' mode.  convert range between editors to grey.
        {
            int a = curr[0];
            int b = curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            SaveUndoData(a, b);
            palrangetogrey(pal, a, 1 + (b - a));
            break;
        }
        }

        UpdateDAC();
        this->rgb[0]->set_rgb(curr[0], &(pal[curr[0]]));
        this->rgb[0]->update();
        this->rgb[1]->set_rgb(curr[1], &(pal[curr[1]]));
        this->rgb[1]->update();
        break;
    }

    case 'N': // convert to negative color
    case 'n':
    {
        switch (exclude)
        {
        case 0: // normal mode.  convert all colors to grey scale
            SaveUndoData(0, 255);
            palrangetonegative(pal, 0, 256);
            break;

        case 1: // 'x' mode. convert current color to grey scale.
            SaveUndoData(curr[active], curr[active]);
            palrangetonegative(pal, curr[active], 1);
            break;

        case 2: // 'y' mode.  convert range between editors to grey.
        {
            int a = curr[0];
            int b = curr[1];

            if (a > b)
            {
                int t = a;
                a = b;
                b = t;
            }

            SaveUndoData(a, b);
            palrangetonegative(pal, a, 1 + (b - a));
            break;
        }
        }

        UpdateDAC();
        this->rgb[0]->set_rgb(curr[0], &(pal[curr[0]]));
        this->rgb[0]->update();
        this->rgb[1]->set_rgb(curr[1], &(pal[curr[1]]));
        this->rgb[1]->update();
        break;
    }

    case 'U': // Undo
    case 'u':
        Undo();
        break;

    case 'e': // Redo
    case 'E':
        Redo();
        break;

    } // switch
    DrawStatus(false);
}

void PalTable::PutBand(PALENTRY *pal)
{
    int r;
    int b;
    int a;

    // clip top and bottom values to stop them running off the end of the DAC

    CalcTopBottom();

    // put bands either side of current colour

    a = curr[active];
    b = bottom;
    r = top;

    pal[a] = fs_color;

    if (r != a && a != b)
    {
        mkpalrange(&pal[a], &pal[r], &pal[a], r-a, 1);
        mkpalrange(&pal[b], &pal[a], &pal[b], a-b, 1);
    }

}

void PalTable::SetHidden(bool hidden)
{
    this->hidden = hidden;
    rgb[0]->set_hidden(hidden);
    rgb[1]->set_hidden(hidden);
    UpdateDAC();
}

void PalTable::change(RGBEditor *rgb)
{
    int       pnum = curr[active];

    if (freestyle)
    {
        fs_color = rgb->get_rgb();
        UpdateDAC();
        return;
    }

    if (!curr_changed)
    {
        SaveUndoData(pnum, pnum);
        curr_changed = true;
    }

    pal[pnum] = rgb->get_rgb();

    if (curr[0] == curr[1])
    {
        int      other = active == 0 ? 1 : 0;
        PALENTRY color;

        color = this->rgb[active]->get_rgb();
        this->rgb[other]->set_rgb(curr[other], &color);

        Cursor_Hide();
        this->rgb[other]->update();
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
    rgb[0] = new RGBEditor(0, 0, &PalTable::other_key_cb, &PalTable::change_cb, this);
    rgb[1] = new RGBEditor(0, 0, &PalTable::other_key_cb, &PalTable::change_cb, this);
    movebox = new MoveBox(0, 0, 0, PalTable_PALX + 1, PalTable_PALY + 1);
    active      = 0;
    curr[0]     = 1;
    curr[1]     = 1;
    auto_select = true;
    exclude     = 0;
    hidden      = false;
    stored_at   = NOWHERE;
    fs_color.red   = 42;
    fs_color.green = 42;
    fs_color.blue  = 42;
    freestyle      = false;
    bandwidth      = 15;
    top            = 255;
    bottom         = 0 ;
    undo_file    = dir_fopen(g_temp_dir.c_str(), s_undo_file, "w+b");
    curr_changed = false;
    num_redo     = 0;

    rgb[0]->set_rgb(curr[0], &pal[curr[0]]);
    rgb[1]->set_rgb(curr[1], &pal[curr[0]]);

    if (g_video_scroll)
    {
        SetPos(g_video_start_x, g_video_start_y);
        csize = ((g_vesa_y_res-(PalTable_PALY+1+1)) / 2) / 16;
    }
    else
    {
        SetPos(0, 0);
        csize = ((g_screen_y_dots-(PalTable_PALY+1+1)) / 2) / 16;
    }

    if (csize < CSIZE_MIN)
    {
        csize = CSIZE_MIN;
    }
    SetCSize(csize);
}

PalTable::~PalTable()
{
    if (undo_file != nullptr)
    {
        std::fclose(undo_file);
        dir_remove(g_temp_dir.c_str(), s_undo_file);
    }

    delete rgb[0];
    delete rgb[1];
    delete movebox;
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
    pt.Process();

    g_line_buff.clear();
}
