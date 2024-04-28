#include "port.h"

#include "id.h"
#include "id_data.h"
#include "plot3d.h"
#include "rotate.h"

#include <crtdbg.h>
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <windowsx.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "plot.h"
#include "ods.h"

#define PLOT_TIMER_ID 1

static Plot *s_plot = nullptr;
static LPCSTR s_window_class = "IdPlot";

static const BYTE font_8x8[8][1024/8] =
{
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x14, 0x14, 0x08, 0x00, 0x18, 0x08,
        0x04, 0x10, 0x08, 0x08, 0x00, 0x00, 0x00, 0x01,
        0x1C, 0x08, 0x1C, 0x1C, 0x0C, 0x3E, 0x1C, 0x3E,
        0x1C, 0x1C, 0x00, 0x00, 0x04, 0x00, 0x20, 0x1C,
        0x1C, 0x1C, 0x3C, 0x1C, 0x3C, 0x3E, 0x3E, 0x1C,
        0x22, 0x1C, 0x0E, 0x22, 0x10, 0x41, 0x22, 0x1C,
        0x1C, 0x1C, 0x3C, 0x1C, 0x3E, 0x22, 0x22, 0x41,
        0x22, 0x22, 0x3E, 0x1C, 0x40, 0x1C, 0x08, 0x00,
        0x10, 0x00, 0x10, 0x00, 0x02, 0x00, 0x0C, 0x00,
        0x20, 0x00, 0x00, 0x20, 0x18, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x0C, 0x08, 0x30, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x14, 0x14, 0x1E, 0x32, 0x28, 0x08,
        0x08, 0x08, 0x49, 0x08, 0x00, 0x00, 0x00, 0x02,
        0x22, 0x18, 0x22, 0x22, 0x14, 0x20, 0x22, 0x02,
        0x22, 0x22, 0x0C, 0x0C, 0x08, 0x00, 0x10, 0x22,
        0x22, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20, 0x22,
        0x22, 0x08, 0x04, 0x22, 0x10, 0x63, 0x32, 0x22,
        0x12, 0x22, 0x22, 0x22, 0x08, 0x22, 0x22, 0x41,
        0x22, 0x22, 0x02, 0x10, 0x20, 0x04, 0x14, 0x00,
        0x08, 0x1C, 0x10, 0x00, 0x02, 0x00, 0x12, 0x00,
        0x20, 0x08, 0x08, 0x20, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x10, 0x08, 0x08, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x7F, 0x28, 0x34, 0x10, 0x00,
        0x10, 0x04, 0x2A, 0x08, 0x00, 0x00, 0x00, 0x04,
        0x22, 0x08, 0x02, 0x02, 0x24, 0x20, 0x20, 0x04,
        0x22, 0x22, 0x0C, 0x0C, 0x10, 0x7F, 0x08, 0x02,
        0x2E, 0x22, 0x22, 0x20, 0x22, 0x20, 0x20, 0x20,
        0x22, 0x08, 0x04, 0x24, 0x10, 0x55, 0x2A, 0x22,
        0x12, 0x22, 0x22, 0x20, 0x08, 0x22, 0x22, 0x41,
        0x14, 0x14, 0x04, 0x10, 0x10, 0x04, 0x22, 0x00,
        0x00, 0x02, 0x1C, 0x1C, 0x0E, 0x1C, 0x10, 0x1D,
        0x2C, 0x00, 0x00, 0x24, 0x08, 0xB6, 0x2C, 0x1C,
        0x2C, 0x1A, 0x2C, 0x1C, 0x1C, 0x24, 0x22, 0x41,
        0x22, 0x12, 0x3C, 0x10, 0x08, 0x08, 0x30, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x14, 0x1C, 0x08, 0x28, 0x00,
        0x10, 0x04, 0x1C, 0x7F, 0x00, 0x7F, 0x00, 0x08,
        0x2A, 0x08, 0x04, 0x0C, 0x3E, 0x3C, 0x3C, 0x08,
        0x1C, 0x1E, 0x00, 0x00, 0x20, 0x00, 0x04, 0x04,
        0x2A, 0x3E, 0x3C, 0x20, 0x22, 0x3C, 0x3E, 0x2E,
        0x3E, 0x08, 0x04, 0x38, 0x10, 0x49, 0x2A, 0x22,
        0x1C, 0x22, 0x3C, 0x1C, 0x08, 0x22, 0x14, 0x2A,
        0x08, 0x08, 0x08, 0x10, 0x08, 0x04, 0x00, 0x00,
        0x00, 0x1E, 0x12, 0x20, 0x12, 0x22, 0x38, 0x22,
        0x32, 0x08, 0x08, 0x28, 0x08, 0x49, 0x12, 0x22,
        0x12, 0x24, 0x30, 0x20, 0x08, 0x24, 0x22, 0x41,
        0x14, 0x12, 0x04, 0x20, 0x08, 0x04, 0x49, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x7F, 0x0A, 0x16, 0x46, 0x00,
        0x10, 0x04, 0x2A, 0x08, 0x0C, 0x00, 0x00, 0x10,
        0x22, 0x08, 0x08, 0x02, 0x04, 0x02, 0x22, 0x10,
        0x22, 0x02, 0x0C, 0x0C, 0x10, 0x7F, 0x08, 0x08,
        0x2E, 0x22, 0x22, 0x20, 0x22, 0x20, 0x20, 0x22,
        0x22, 0x08, 0x24, 0x24, 0x10, 0x41, 0x26, 0x22,
        0x10, 0x22, 0x28, 0x02, 0x08, 0x22, 0x14, 0x2A,
        0x14, 0x08, 0x10, 0x10, 0x04, 0x04, 0x00, 0x00,
        0x00, 0x22, 0x12, 0x20, 0x12, 0x3E, 0x10, 0x22,
        0x22, 0x08, 0x08, 0x30, 0x08, 0x49, 0x12, 0x22,
        0x12, 0x24, 0x20, 0x18, 0x08, 0x24, 0x22, 0x49,
        0x08, 0x12, 0x08, 0x10, 0x08, 0x08, 0x06, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x14, 0x3C, 0x26, 0x44, 0x00,
        0x08, 0x08, 0x49, 0x08, 0x0C, 0x00, 0x0C, 0x20,
        0x22, 0x08, 0x10, 0x22, 0x04, 0x22, 0x22, 0x10,
        0x22, 0x22, 0x0C, 0x0C, 0x08, 0x00, 0x10, 0x00,
        0x20, 0x22, 0x22, 0x22, 0x22, 0x20, 0x20, 0x22,
        0x22, 0x08, 0x24, 0x22, 0x10, 0x41, 0x22, 0x22,
        0x10, 0x22, 0x24, 0x22, 0x08, 0x22, 0x08, 0x14,
        0x22, 0x08, 0x20, 0x10, 0x02, 0x04, 0x00, 0x00,
        0x00, 0x22, 0x12, 0x20, 0x12, 0x20, 0x10, 0x1E,
        0x22, 0x08, 0x08, 0x28, 0x08, 0x41, 0x12, 0x22,
        0x1C, 0x1C, 0x20, 0x04, 0x08, 0x24, 0x14, 0x55,
        0x14, 0x0E, 0x10, 0x10, 0x08, 0x08, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x14, 0x08, 0x00, 0x3A, 0x00,
        0x04, 0x10, 0x08, 0x08, 0x04, 0x00, 0x0C, 0x40,
        0x1C, 0x1C, 0x3E, 0x1C, 0x0E, 0x1C, 0x1C, 0x10,
        0x1C, 0x1C, 0x00, 0x04, 0x04, 0x00, 0x20, 0x08,
        0x1C, 0x22, 0x3C, 0x1C, 0x3C, 0x3E, 0x20, 0x1C,
        0x22, 0x1C, 0x18, 0x22, 0x1E, 0x41, 0x22, 0x1C,
        0x10, 0x1C, 0x22, 0x1C, 0x08, 0x1C, 0x08, 0x14,
        0x22, 0x08, 0x3E, 0x1C, 0x01, 0x1C, 0x00, 0x00,
        0x00, 0x1D, 0x2C, 0x1C, 0x0D, 0x1C, 0x10, 0x02,
        0x22, 0x08, 0x08, 0x24, 0x08, 0x41, 0x12, 0x1C,
        0x10, 0x04, 0x20, 0x38, 0x08, 0x1A, 0x08, 0x22,
        0x22, 0x02, 0x3C, 0x0C, 0x08, 0x30, 0x00, 0x00
    },
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1C,
        0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x10, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x1C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};

static void
plot_set_dirty_region(Plot *p, int xmin, int ymin, int xmax, int ymax)
{
    RECT *r = &p->m_dirty_region;

    _ASSERTE(xmin < xmax);
    _ASSERTE(ymin < ymax);
    _ASSERTE((r->left <= r->right) && (r->top <= r->bottom));
    if (r->left < 0)
    {
        r->left = xmin;
        r->right = xmax;
        r->top = ymin;
        r->bottom = ymax;
        p->m_dirty = true;
    }
    else
    {
        if (xmin < r->left)
        {
            r->left = xmin;
            p->m_dirty = true;
        }
        if (xmax > r->right)
        {
            r->right = xmax;
            p->m_dirty = true;
        }
        if (ymin < r->top)
        {
            r->top = ymin;
            p->m_dirty = true;
        }
        if (ymax > r->bottom)
        {
            r->bottom = ymax;
            p->m_dirty = true;
        }
    }
}

/* init_pixels
 *
 * Resize the pixel array to sxdots by sydots and initialize it to zero.
 * Any existing pixel array is freed.
 */
static void
init_pixels(Plot *p)
{
    p->m_pixels.clear();
    p->m_saved_pixels.clear();
    p->m_width = g_screen_x_dots;
    p->m_height = g_screen_y_dots;
    p->m_row_len = p->m_width * sizeof(BYTE);
    p->m_row_len = ((p->m_row_len + 3)/4)*4;
    p->m_pixels_len = p->m_row_len * p->m_height;
    _ASSERTE(p->m_pixels_len > 0);
    p->m_pixels.resize(p->m_pixels_len);
    std::memset(&p->m_pixels[0], 0, p->m_pixels_len);
    p->m_dirty = false;
    {
        RECT dirty_rect = { -1, -1, -1, -1 };
        p->m_dirty_region = dirty_rect;
    }
    {
        BITMAPINFOHEADER *h = &p->m_bmi.bmiHeader;

        h->biSize = sizeof(p->m_bmi.bmiHeader);
        h->biWidth = p->m_width;
        h->biHeight = p->m_height;
        h->biPlanes = 1;
        h->biBitCount = 8;
        h->biCompression = BI_RGB;
        h->biSizeImage = 0;
        h->biClrUsed = 256;
    }
}

static void plot_OnPaint(HWND window)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(window, &ps);
    RECT *r = &ps.rcPaint;
    int width = r->right - r->left;
    int height = r->bottom - r->top;

    _ASSERTE(width >= 0 && height >= 0);
    if (width > 0 && height > 0)
    {
        DWORD status;
        status = StretchDIBits(dc,
                               0, 0, s_plot->m_width, s_plot->m_height,
                               0, 0, s_plot->m_width, s_plot->m_height,
                               &s_plot->m_pixels[0], &s_plot->m_bmi, DIB_RGB_COLORS, SRCCOPY);
        _ASSERTE(status != GDI_ERROR);
    }
    EndPaint(window, &ps);
}

static LRESULT CALLBACK plot_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
    _ASSERTE(s_plot != nullptr);
    switch (message)
    {
    case WM_PAINT:
        HANDLE_WM_PAINT(window, wp, lp, plot_OnPaint);
        break;

    default:
        return DefWindowProc(window, message, wp, lp);
    }

    return 0;
}

int plot_init(Plot *p, HINSTANCE instance, LPCSTR title)
{
    WNDCLASS  wc;
    int result;

    p->m_instance = instance;
    std::strcpy(p->m_title, title);

    result = GetClassInfo(p->m_instance, s_window_class, &wc);
    if (!result)
    {
        wc.style = 0;
        wc.lpfnWndProc = plot_proc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = p->m_instance;
        wc.hIcon = nullptr;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        wc.lpszMenuName =  p->m_title;
        wc.lpszClassName = s_window_class;

        result = RegisterClass(&wc);
    }

    return result;
}

void plot_terminate(Plot *p)
{
    {
        HBITMAP rendering = (HBITMAP) SelectObject(p->m_memory_dc, (HGDIOBJ) p->m_backup);
        _ASSERTE(rendering == p->m_rendering);
    }
    DeleteObject(p->m_rendering);
    DeleteDC(p->m_memory_dc);
    DestroyWindow(p->m_window);
}

static void plot_create_backing_store(Plot *p)
{
    {
        HDC dc = GetDC(p->m_window);
        p->m_memory_dc = CreateCompatibleDC(dc);
        _ASSERTE(p->m_memory_dc);
        ReleaseDC(p->m_window, dc);
    }

    p->m_rendering = CreateCompatibleBitmap(p->m_memory_dc, p->m_width, p->m_height);
    _ASSERTE(p->m_rendering);
    p->m_backup = (HBITMAP) SelectObject(p->m_memory_dc, (HGDIOBJ) p->m_rendering);

    p->m_font = CreateFont(8, 8, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
                          OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                          DEFAULT_PITCH | FF_MODERN, "Courier");
    _ASSERTE(p->m_font);
    SelectObject(p->m_memory_dc, (HGDIOBJ) p->m_font);
    SetBkMode(p->m_memory_dc, OPAQUE);
}
void plot_window(Plot *p, HWND parent)
{
    if (nullptr == p->m_window)
    {
        init_pixels(p);
        s_plot = p;
        p->m_parent = parent;
        p->m_window = CreateWindow(s_window_class,
                                  p->m_title,
                                  parent ? WS_CHILD : WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT,               // default horizontal position
                                  CW_USEDEFAULT,               // default vertical position
                                  p->m_width,
                                  p->m_height,
                                  parent, nullptr, p->m_instance,
                                  nullptr);

        plot_create_backing_store(p);
    }
}

void plot_write_pixel(Plot *p, int x, int y, int color)
{
    _ASSERTE(p->m_pixels.size() == p->m_width* p->m_height);
    _ASSERTE(x >= 0 && x < p->m_width);
    _ASSERTE(y >= 0 && y < p->m_height);
    p->m_pixels[(p->m_height - y - 1)*p->m_row_len + x] = (BYTE)(color & 0xFF);
    plot_set_dirty_region(p, x, y, x+1, y+1);
}

int plot_read_pixel(Plot *p, int x, int y)
{
    _ASSERTE(p->m_pixels.size() == p->m_width* p->m_height);
    _ASSERTE(x >= 0 && x < p->m_width);
    _ASSERTE(y >= 0 && y < p->m_height);
    return (int) p->m_pixels[(p->m_height - 1 - y)*p->m_row_len + x];
}

void plot_write_span(Plot *p, int y, int x, int lastx, const BYTE *pixels)
{
    int width = lastx-x+1;

    for (int i = 0; i < width; i++)
    {
        plot_write_pixel(p, x+i, y, pixels[i]);
    }
    plot_set_dirty_region(p, x, y, lastx+1, y+1);
}

void plot_flush(Plot *p)
{
    if (p->m_dirty)
    {
        RECT r = { -1, -1, -1, -1 };
        InvalidateRect(p->m_window, nullptr, FALSE);
        p->m_dirty = false;
        p->m_dirty_region = r;
    }
}

void plot_read_span(Plot *p, int y, int x, int lastx, BYTE *pixels)
{
    plot_flush(p);
    int width = lastx - x + 1;
    for (int i = 0; i < width; i++)
    {
        pixels[i] = plot_read_pixel(p, x + i, y);
    }
}

void plot_set_line_mode(Plot *p, int mode)
{
}

void plot_draw_line(Plot *p, int x1, int y1, int x2, int y2, int color)
{
    draw_line(x1, y1, x2, y2, color);
}

int plot_resize(Plot *p)
{
    if ((g_screen_x_dots == p->m_width) && (g_screen_y_dots == p->m_height))
    {
        return 0;
    }

    init_pixels(p);
    BOOL status = SetWindowPos(p->m_window, nullptr, 0, 0, p->m_width, p->m_height, SWP_NOZORDER | SWP_NOMOVE);
    _ASSERTE(status);

    return !0;
}

int plot_read_palette(Plot *p)
{
    if (!g_got_real_dac)
    {
        return -1;
    }

    for (int i = 0; i < 256; i++)
    {
        g_dac_box[i][0] = p->m_clut[i][0];
        g_dac_box[i][1] = p->m_clut[i][1];
        g_dac_box[i][2] = p->m_clut[i][2];
    }
    return 0;
}

int plot_write_palette(Plot *p)
{
    for (int i = 0; i < 256; i++)
    {
        p->m_clut[i][0] = g_dac_box[i][0];
        p->m_clut[i][1] = g_dac_box[i][1];
        p->m_clut[i][2] = g_dac_box[i][2];

        p->m_bmi.bmiColors[i].rgbRed = g_dac_box[i][0]*4;
        p->m_bmi.bmiColors[i].rgbGreen = g_dac_box[i][1]*4;
        p->m_bmi.bmiColors[i].rgbBlue = g_dac_box[i][2]*4;
    }
    plot_redraw(p);

    return 0;
}

static VOID CALLBACK redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
    InvalidateRect(window, nullptr, FALSE);
    KillTimer(window, PLOT_TIMER_ID);
}

void plot_schedule_alarm(Plot *p, int secs)
{
    UINT_PTR result = SetTimer(p->m_window, PLOT_TIMER_ID, secs, redraw);
    if (!result)
    {
        DWORD error = GetLastError();
        _ASSERTE(result);
    }
}

void plot_clear(Plot *p)
{
    RECT r = { 0, 0, p->m_width, p->m_height };
    p->m_dirty_region = r;
    p->m_dirty = true;
    std::memset(&p->m_pixels[0], 0, p->m_pixels_len);
}

void plot_redraw(Plot *p)
{
    InvalidateRect(p->m_window, nullptr, FALSE);
}

void plot_display_string(Plot *p, int x, int y, int fg, int bg, char const *text)
{
    while (*text)
    {
        for (int row = 0; row < 8; row++)
        {
            int x1 = x;
            int col = 8;
            BYTE pixel = font_8x8[row][static_cast<unsigned char>(*text)];
            while (col-- > 0)
            {
                int color = (pixel & (1 << col)) ? fg : bg;
                plot_write_pixel(p, x1++, y + row, color);
            }
        }
        x += 8;
        text++;
    }
}

void plot_save_graphics(Plot *p)
{
    p->m_saved_pixels = p->m_pixels;
}

void plot_restore_graphics(Plot *p)
{
    p->m_pixels = p->m_saved_pixels;
    plot_redraw(p);
}
