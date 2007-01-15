#define _CRTDBG_MAP_ALLOC
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <crtdbg.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

#include "port.h"
#include "prototyp.h"
#include "fractint.h"

#include "plot.h"
#include "ods.h"

#define TIMER_ID 1

static Plot *s_plot = NULL;
static LPCSTR s_window_class = "FractIntPlot";

static void
plot_set_dirty_region(Plot *p, int xmin, int ymin, int xmax, int ymax)
{
	RECT *r = &p->dirty_region;

	_ASSERTE(xmin < xmax);
	_ASSERTE(ymin < ymax);
	_ASSERTE((r->left <= r->right) && (r->top <= r->bottom));
	if (r->left < 0)
	{
		r->left = xmin;
		r->right = xmax;
		r->top = ymin;
		r->bottom = ymax;
		p->dirty = TRUE;
	}
	else
	{
		if (xmin < r->left)
		{
			r->left = xmin;
			p->dirty = TRUE;
		}
		if (xmax > r->right)
		{
			r->right = xmax;
			p->dirty = TRUE;
		}
		if (ymin < r->top)
		{
			r->top = ymin;
			p->dirty = TRUE;
		}
		if (ymax > r->bottom)
		{
			r->bottom = ymax;
			p->dirty = TRUE;
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
	if (p->pixels != NULL)
	{
		free(p->pixels);
	}
	p->width = sxdots;
	p->height = sydots;
	p->row_len = p->width * sizeof(p->pixels[0]);
	p->row_len = ((p->row_len + 3)/4)*4;
	p->pixels_len = p->row_len * p->height;
	_ASSERTE(p->pixels_len > 0);
	p->pixels = (BYTE *) malloc(p->pixels_len);
	memset(p->pixels, 0, p->pixels_len);
	p->dirty = FALSE;
	{
		RECT dirty_rect = { -1, -1, -1, -1 };
		p->dirty_region = dirty_rect;
	}
	{
		BITMAPINFOHEADER *h = &p->bmi.bmiHeader;

		h->biSize = sizeof(p->bmi.bmiHeader);
		h->biWidth = p->width;
		h->biHeight = p->height;
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
#if 0
		DWORD status = StretchDIBits(dc,
			r->left, r->top, width, height,
			r->left, r->top, width, height,
			s_plot->pixels, &s_plot->bmi, DIB_RGB_COLORS, SRCCOPY);
#else
		DWORD status = StretchDIBits(dc,
			0, 0, s_plot->width, s_plot->height,
			0, 0, s_plot->width, s_plot->height,
			s_plot->pixels, &s_plot->bmi, DIB_RGB_COLORS, SRCCOPY);
#endif
		_ASSERTE(status != GDI_ERROR);
	}
	EndPaint(window, &ps);
}

static LRESULT CALLBACK plot_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	_ASSERTE(s_plot != NULL);
	switch (message)
	{
	case WM_PAINT: HANDLE_WM_PAINT(window, wp, lp, plot_OnPaint); break;

	default: return DefWindowProc(window, message, wp, lp);
	}

	return 0;
}

/*----------------------------------------------------------------------
*
* init_clut --
*
* Put something nice in the dac.
*
* The conditions are:
*	Colors 1 and 2 should be bright so ifs fractals show up.
*	Color 15 should be bright for lsystem.
*	Color 1 should be bright for bifurcation.
*	Colors 1, 2, 3 should be distinct for periodicity.
*	The color map should look good for mandelbrot.
*	The color map should be good if only 128 colors are used.
*
* Results:
*	None.
*
* Side effects:
*	Loads the dac.
*
*----------------------------------------------------------------------
*/
static void
init_clut(BYTE clut[256][3])
{
	int i;
	for (i=0;i < 256;i++)
	{
		clut[i][0] = (i >> 5)*8+7;
		clut[i][1] = (((i+16) & 28) >> 2)*8+7;
		clut[i][2] = (((i+2) & 3))*16+15;
	}
	clut[0][0] = clut[0][1] = clut[0][2] = 0;
	clut[1][0] = clut[1][1] = clut[1][2] = 63;
	clut[2][0] = 47; clut[2][1] = clut[2][2] = 63;
}

int plot_init(Plot *me, HINSTANCE instance, LPCSTR title)
{
    WNDCLASS  wc;
	int result;

    me->instance = instance;
	strcpy(me->title, title);

	result = GetClassInfo(me->instance, s_window_class, &wc);
	if (!result)
	{
		wc.style = 0;
		wc.lpfnWndProc = plot_proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = me->instance;
		wc.hIcon = NULL;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = GetStockObject(BLACK_BRUSH);
		wc.lpszMenuName =  me->title;
		wc.lpszClassName = s_window_class;

		result = RegisterClass(&wc);
	}

    return result;
}

void plot_terminate(Plot *p)
{
	if (p->pixels)
	{
		free(p->pixels);
		p->pixels = NULL;
	}
}

void plot_window(Plot *p, HWND parent)
{
	if (NULL == p->window)
	{
		init_pixels(p);
		s_plot = p;
		p->window = CreateWindow(s_window_class,
			p->title,
			parent ? WS_CHILD : WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,               /* default horizontal position */
			CW_USEDEFAULT,               /* default vertical position */
			p->width,
			p->height,
			parent, NULL, p->instance,
			NULL);

		{
			HDC dc = GetDC(p->window);
			g_got_real_dac = (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) ? TRUE : FALSE;
			ReleaseDC(p->window, dc);
		}
	}
}

void plot_write_pixel(Plot *p, int x, int y, int color)
{
	_ASSERTE(p->pixels);
	_ASSERTE(x >= 0 && x < p->width);
	_ASSERTE(y >= 0 && y < p->height);
	p->pixels[y*p->row_len + x] = (BYTE) (color & 0xFF);
	plot_set_dirty_region(p, x, y, x+1, y+1);
}

int plot_read_pixel(Plot *p, int x, int y)
{
	_ASSERTE(p->pixels);
	_ASSERTE(x >= 0 && x < p->width);
	_ASSERTE(y >= 0 && y < p->height);
	return (int) p->pixels[y*p->row_len + x];
}

void plot_write_span(Plot *p, int y, int x, int lastx, const BYTE *pixels)
{
	int i;
	int width = lastx-x+1;

	for (i = 0; i < width; i++)
	{
		plot_write_pixel(p, x+i, y, pixels[i]);
	}
	plot_set_dirty_region(p, x, y, lastx+1, y+1);
}

void plot_flush(Plot *p)
{
	if (p->dirty)
	{
		RECT r = { -1, -1, -1, -1 };
#if 0
		InvalidateRect(p->window, &p->dirty_region, FALSE);
#else
		InvalidateRect(p->window, NULL, FALSE);
#endif
		p->dirty = FALSE;
		p->dirty_region = r;
	}
}

void plot_read_span(Plot *p, int y, int x, int lastx, BYTE *pixels)
{
	int i, width;

	plot_flush(p);
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
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
	BOOL status;

	if ((sxdots == p->width) && (sydots == p->height)) 
	{
		return 0;
	}

	init_pixels(p);
	status = SetWindowPos(p->window, NULL, 0, 0, p->width, p->height, SWP_NOZORDER | SWP_NOMOVE);
	_ASSERTE(status);

	return !0;
}

int plot_read_palette(Plot *p)
{
	int i;

	if (g_got_real_dac == 0)
	{
		return -1;
	}

	for (i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = p->clut[i][0];
		g_dac_box[i][1] = p->clut[i][1];
		g_dac_box[i][2] = p->clut[i][2];
	}
	return 0;
}

int plot_write_palette(Plot *p)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		p->clut[i][0] = g_dac_box[i][0];
		p->clut[i][1] = g_dac_box[i][1];
		p->clut[i][2] = g_dac_box[i][2];

		p->bmi.bmiColors[i].rgbRed = g_dac_box[i][0]*4;
		p->bmi.bmiColors[i].rgbGreen = g_dac_box[i][1]*4;
		p->bmi.bmiColors[i].rgbBlue = g_dac_box[i][2]*4;
	}

	return 0;
}

static VOID CALLBACK frame_timer_redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	InvalidateRect(window, NULL, FALSE);
	KillTimer(window, TIMER_ID);
}

void plot_schedule_alarm(Plot *me, int delay)
{
	UINT_PTR result = SetTimer(me->window, TIMER_ID, delay, frame_timer_redraw);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result);
	}
}
