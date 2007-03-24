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
#include "frame.h"

#define PLOT_TIMER_ID 1

static Plot *s_plot = NULL;
static LPCSTR s_window_class = "FractIntPlot";

static void
plot_set_dirty_region(Plot *me, int xmin, int ymin, int xmax, int ymax)
{
	RECT *r = &me->dirty_region;

	_ASSERTE(xmin < xmax);
	_ASSERTE(ymin < ymax);
	_ASSERTE((r->left <= r->right) && (r->top <= r->bottom));
	if (r->left < 0)
	{
		r->left = xmin;
		r->right = xmax;
		r->top = ymin;
		r->bottom = ymax;
		me->dirty = TRUE;
	}
	else
	{
		if (xmin < r->left)
		{
			r->left = xmin;
			me->dirty = TRUE;
		}
		if (xmax > r->right)
		{
			r->right = xmax;
			me->dirty = TRUE;
		}
		if (ymin < r->top)
		{
			r->top = ymin;
			me->dirty = TRUE;
		}
		if (ymax > r->bottom)
		{
			r->bottom = ymax;
			me->dirty = TRUE;
		}
	}
}

/* init_pixels
 *
 * Resize the pixel array to sxdots by sydots and initialize it to zero.
 * Any existing pixel array is freed.
 */
static void
init_pixels(Plot *me)
{
	if (me->pixels != NULL)
	{
		free(me->pixels);
		me->pixels = NULL;
	}
	if (me->saved_pixels != NULL)
	{
		free(me->saved_pixels);
		me->saved_pixels = NULL;
	}
	me->width = sxdots;
	me->height = sydots;
	me->row_len = me->width*sizeof(me->pixels[0]);
	me->row_len = ((me->row_len + 3)/4)*4;
	me->pixels_len = me->row_len*me->height;
	_ASSERTE(me->pixels_len > 0);
	me->pixels = (BYTE *) malloc(me->pixels_len);
	memset(me->pixels, 0, me->pixels_len);
	me->dirty = FALSE;
	{
		RECT dirty_rect = { -1, -1, -1, -1 };
		me->dirty_region = dirty_rect;
	}
	{
		BITMAPINFOHEADER *h = &me->bmi.bmiHeader;

		h->biSize = sizeof(me->bmi.bmiHeader);
		h->biWidth = me->width;
		h->biHeight = me->height;
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
#if 0
		status = StretchDIBits(dc,
			r->left, r->top, width, height,
			r->left, r->top, width, height,
			s_plot->pixels, &s_plot->bmi, DIB_RGB_COLORS, SRCCOPY);
#else
#if 0
		status = StretchBlt(dc, 0, 0, s_plot->width, s_plot->height,
			s_plot->memory_dc, 0, 0, s_plot->width, s_plot->height, SRCCOPY);
#else
		status = StretchDIBits(dc,
			0, 0, s_plot->width, s_plot->height,
			0, 0, s_plot->width, s_plot->height,
			s_plot->pixels, &s_plot->bmi, DIB_RGB_COLORS, SRCCOPY);
#endif
#endif
		_ASSERTE(status != GDI_ERROR);
	}
	EndPaint(window, &ps);
}

/* forward all mouse events to the frame */
static void plot_OnLeftButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags)
{
	FORWARD_WM_LBUTTONDOWN(hwnd, doubleClick, x, y, keyFlags, frame_proc);
}
static void plot_OnLeftButtonUp(HWND hwnd, int x, int y, int keyFlags)
{
	FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, frame_proc);
}
static void plot_OnMiddleButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags)
{
	FORWARD_WM_MBUTTONDOWN(hwnd, doubleClick, x, y, keyFlags, frame_proc);
}
static void plot_OnMiddleButtonUp(HWND hwnd, int x, int y, int keyFlags)
{
	FORWARD_WM_MBUTTONUP(hwnd, x, y, keyFlags, frame_proc);
}
static void plot_OnRightButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags)
{
	FORWARD_WM_RBUTTONDOWN(hwnd, doubleClick, x, y, keyFlags, frame_proc);
}
static void plot_OnRightButtonUp(HWND hwnd, int x, int y, int keyFlags)
{
	FORWARD_WM_RBUTTONUP(hwnd, x, y, keyFlags, frame_proc);
}
static void plot_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, frame_proc);
}

static LRESULT CALLBACK plot_proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	_ASSERTE(s_plot != NULL);
	switch (message)
	{
	case WM_PAINT:			HANDLE_WM_PAINT(window, wp, lp, plot_OnPaint); break;
	case WM_MOUSEMOVE:		HANDLE_WM_MOUSEMOVE(window, wp, lp, plot_OnMouseMove);			break;
	case WM_LBUTTONDOWN:	HANDLE_WM_LBUTTONDOWN(window, wp, lp, plot_OnLeftButtonDown);	break;
	case WM_LBUTTONUP:		HANDLE_WM_LBUTTONUP(window, wp, lp, plot_OnLeftButtonUp);		break;
	case WM_LBUTTONDBLCLK:	HANDLE_WM_LBUTTONDBLCLK(window, wp, lp, plot_OnLeftButtonDown);	break;
	case WM_MBUTTONDOWN:	HANDLE_WM_MBUTTONDOWN(window, wp, lp, plot_OnMiddleButtonDown);	break;
	case WM_MBUTTONUP:		HANDLE_WM_MBUTTONUP(window, wp, lp, plot_OnMiddleButtonUp);		break;
	case WM_RBUTTONDOWN:	HANDLE_WM_RBUTTONDOWN(window, wp, lp, plot_OnRightButtonDown);	break;
	case WM_RBUTTONUP:		HANDLE_WM_RBUTTONUP(window, wp, lp, plot_OnRightButtonUp);		break;
	case WM_RBUTTONDBLCLK:	HANDLE_WM_RBUTTONDBLCLK(window, wp, lp, plot_OnRightButtonDown); break;

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
	for (i = 0; i < 256; i++)
	{
		clut[i][0] = (i >> 5)*8 + 7;
		clut[i][1] = (((i + 16) & 28) >> 2)*8 + 7;
		clut[i][2] = (((i + 2) & 3))*16 + 15;
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
		wc.style = CS_DBLCLKS;
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

void plot_terminate(Plot *me)
{
	if (me->pixels)
	{
		free(me->pixels);
		me->pixels = NULL;
	}
	if (me->saved_pixels)
	{
		free(me->saved_pixels);
		me->saved_pixels = NULL;
	}

	{
		HBITMAP rendering = (HBITMAP) SelectObject(me->memory_dc, (HGDIOBJ) me->backup);
		_ASSERTE(rendering == me->rendering);
	}
	DeleteObject(me->rendering);
	DeleteDC(me->memory_dc);
	DestroyWindow(me->window);
}

static void plot_create_backing_store(Plot *me)
{
	{
		HDC dc = GetDC(me->window);
		me->memory_dc = CreateCompatibleDC(dc);
		_ASSERTE(me->memory_dc);
		ReleaseDC(me->window, dc);
	}

	me->rendering = CreateCompatibleBitmap(me->memory_dc, me->width, me->height);
	_ASSERTE(me->rendering);
	me->backup = (HBITMAP) SelectObject(me->memory_dc, (HGDIOBJ) me->rendering);

	me->font = CreateFont(8, 8, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_MODERN, "Courier");
	_ASSERTE(me->font);
	SelectObject(me->memory_dc, (HGDIOBJ) me->font);
	SetBkMode(me->memory_dc, OPAQUE);
}
void plot_window(Plot *me, HWND parent)
{
	if (NULL == me->window)
	{
		init_pixels(me);
		s_plot = me;
		me->parent = parent;
		me->window = CreateWindow(s_window_class,
			me->title,
			parent ? WS_CHILD : WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,               /* default horizontal position */
			CW_USEDEFAULT,               /* default vertical position */
			me->width,
			me->height,
			parent, NULL, me->instance,
			NULL);

		plot_create_backing_store(me);
	}
}

void plot_write_pixel(Plot *me, int x, int y, int color)
{
	_ASSERTE(me->pixels);
	_ASSERTE(x >= 0 && x < me->width);
	_ASSERTE(y >= 0 && y < me->height);
	me->pixels[(me->height - y - 1)*me->row_len + x] = (BYTE) (color & 0xFF);
	plot_set_dirty_region(me, x, y, x + 1, y + 1);
}

int plot_read_pixel(Plot *me, int x, int y)
{
	_ASSERTE(me->pixels);
	_ASSERTE(x >= 0 && x < me->width);
	_ASSERTE(y >= 0 && y < me->height);
	return (int) me->pixels[(me->height - 1 - y)*me->row_len + x];
}

void plot_write_span(Plot *me, int y, int x, int lastx, const BYTE *pixels)
{
	int i;
	int width = lastx-x + 1;

	for (i = 0; i < width; i++)
	{
		plot_write_pixel(me, x + i, y, pixels[i]);
	}
	plot_set_dirty_region(me, x, y, lastx + 1, y + 1);
}

void plot_flush(Plot *me)
{
	if (me->dirty)
	{
		RECT r = { -1, -1, -1, -1 };
#if 0
		InvalidateRect(me->window, &me->dirty_region, FALSE);
#else
#if 0
		DWORD status;
		status = StretchDIBits(me->memory_dc,
			0, 0, me->width, me->width,
			0, 0, me->width, me->height,
			me->pixels, &me->bmi, DIB_RGB_COLORS, SRCCOPY);
#else
		InvalidateRect(me->window, NULL, FALSE);
#endif
#endif
		me->dirty = FALSE;
		me->dirty_region = r;
	}
}

void plot_read_span(Plot *me, int y, int x, int lastx, BYTE *pixels)
{
	int i, width;

	plot_flush(me);
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = plot_read_pixel(me, x + i, y);
	}
}

void plot_set_line_mode(Plot *me, int mode)
{
}

void plot_draw_line(Plot *me, int x1, int y1, int x2, int y2, int color)
{
	draw_line(x1, y1, x2, y2, color);
}

int plot_resize(Plot *me)
{
	BOOL status;

	if ((sxdots == me->width) && (sydots == me->height)) 
	{
		return 0;
	}

	init_pixels(me);
	status = SetWindowPos(me->window, NULL, 0, 0, me->width, me->height, SWP_NOZORDER | SWP_NOMOVE);
	_ASSERTE(status);

	return !0;
}

int plot_read_palette(Plot *me)
{
	int i;

	if (g_got_real_dac == 0)
	{
		return -1;
	}

	for (i = 0; i < 256; i++)
	{
		g_dac_box[i][0] = me->clut[i][0];
		g_dac_box[i][1] = me->clut[i][1];
		g_dac_box[i][2] = me->clut[i][2];
	}
	return 0;
}

int plot_write_palette(Plot *me)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		me->clut[i][0] = g_dac_box[i][0];
		me->clut[i][1] = g_dac_box[i][1];
		me->clut[i][2] = g_dac_box[i][2];

		me->bmi.bmiColors[i].rgbRed = g_dac_box[i][0]*4;
		me->bmi.bmiColors[i].rgbGreen = g_dac_box[i][1]*4;
		me->bmi.bmiColors[i].rgbBlue = g_dac_box[i][2]*4;
	}
	plot_redraw(me);

	return 0;
}

static VOID CALLBACK redraw(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	InvalidateRect(window, NULL, FALSE);
	KillTimer(window, PLOT_TIMER_ID);
}

void plot_schedule_alarm(Plot *me, int delay)
{
	UINT_PTR result = SetTimer(me->window, PLOT_TIMER_ID, delay, redraw);
	if (!result)
	{
		DWORD error = GetLastError();
		_ASSERTE(result);
	}
}

void plot_clear(Plot *me)
{
	RECT r = { 0, 0, me->width, me->height };
	me->dirty_region = r;
	me->dirty = TRUE;
	memset(me->pixels, 0, me->pixels_len);
}

void plot_redraw(Plot *me)
{
	InvalidateRect(me->window, NULL, FALSE);
}

void plot_display_string(Plot *me, int x, int y, int fg, int bg, const char *text)
{
	while (*text)
	{
		int row;
		for (row = 0; row < 8; row++)
		{
			int x1 = x;
			int col = 8;
			BYTE pixel = g_font_8x8[row][*text];
			while (col-- > 0)
			{
				int color = (pixel & (1 << col)) ? fg : bg;
				plot_write_pixel(me, x1++, y + row, color);
			}
		}
		x += 8;
		text++;
	}
}

void plot_save_graphics(Plot *me)
{
	if (NULL == me->saved_pixels)
	{
		me->saved_pixels = malloc(me->pixels_len);
		memset(me->saved_pixels, 0, me->pixels_len);
	}
	memcpy(me->saved_pixels, me->pixels, me->pixels_len);
}

void plot_restore_graphics(Plot *me)
{
	_ASSERTE(me->saved_pixels);
	memcpy(me->pixels, me->saved_pixels, me->pixels_len);
	plot_redraw(me);
}
