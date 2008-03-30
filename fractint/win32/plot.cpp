#define _CRTDBG_MAP_ALLOC
#include <string>

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
#include "id.h"
#include "plot3d.h"

#include "plot.h"
#include "ods.h"
#include "frame.h"

enum
{
	PLOT_TIMER_ID = 1
};

Plot *Plot::s_plot = 0;
LPCSTR Plot::s_window_class = "FractIntPlot";

void Plot::set_dirty_region(int x_min, int y_min, int x_max, int y_max)
{
	assert(x_min < x_max);
	assert(y_min < y_max);
	assert((m_dirty_region.left <= m_dirty_region.right) && (m_dirty_region.top <= m_dirty_region.bottom));
	if (m_dirty_region.left < 0)
	{
		m_dirty_region.left = x_min;
		m_dirty_region.right = x_max;
		m_dirty_region.top = y_min;
		m_dirty_region.bottom = y_max;
		m_dirty = true;
	}
	else
	{
		if (x_min < m_dirty_region.left)
		{
			m_dirty_region.left = x_min;
			m_dirty = true;
		}
		if (x_max > m_dirty_region.right)
		{
			m_dirty_region.right = x_max;
			m_dirty = true;
		}
		if (y_min < m_dirty_region.top)
		{
			m_dirty_region.top = y_min;
			m_dirty = true;
		}
		if (y_max > m_dirty_region.bottom)
		{
			m_dirty_region.bottom = y_max;
			m_dirty = true;
		}
	}
}

/* init_pixels
 *
 * Resize the pixel array to g_screen_width by g_screen_height and initialize it to zero.
 * Any existing pixel array is freed.
 */
void Plot::init_pixels()
{
	delete[] m_pixels;
	m_pixels = 0;
	delete[] m_saved_pixels;
	m_saved_pixels = 0;
	m_width = g_screen_width;
	m_height = g_screen_height;
	m_row_len = m_width*sizeof(m_pixels[0]);
	m_row_len = ((m_row_len + 3)/4)*4;
	m_pixels_len = m_row_len*m_height;
	assert(m_pixels_len > 0);
	m_pixels = new BYTE[m_pixels_len];
	memset(m_pixels, 0, m_pixels_len);
	m_dirty = false;
	{
		RECT dirty_rect = { -1, -1, -1, -1 };
		m_dirty_region = dirty_rect;
	}
	{
		BITMAPINFOHEADER *h = &m_bmi.bmiHeader;

		h->biSize = sizeof(m_bmi.bmiHeader);
		h->biWidth = m_width;
		h->biHeight = m_height;
		h->biPlanes = 1;
		h->biBitCount = 8;
		h->biCompression = BI_RGB;
		h->biSizeImage = 0;
		h->biClrUsed = 256;
	}
}

void Plot::OnPaint(HWND window)
{
	PAINTSTRUCT ps;
	HDC dc = ::BeginPaint(window, &ps);
	RECT *r = &ps.rcPaint;
	int width = r->right - r->left;
	int height = r->bottom - r->top;

	assert(width >= 0 && height >= 0);
	if (width > 0 && height > 0)
	{
		DWORD status;
#if 0
		status = ::StretchDIBits(dc,
			r->left, r->top, width, height,
			r->left, r->top, width, height,
			s_plot->pixels, &s_plot->bmi, DIB_RGB_COLORS, SRCCOPY);
#else
#if 0
		status = ::StretchBlt(dc, 0, 0, s_plot->width, s_plot->height,
			s_plot->memory_dc, 0, 0, s_plot->width, s_plot->height, SRCCOPY);
#else
		status = ::StretchDIBits(dc,
			0, 0, s_plot->m_width, s_plot->m_height,
			0, 0, s_plot->m_width, s_plot->m_height,
			s_plot->m_pixels, &s_plot->m_bmi, DIB_RGB_COLORS, SRCCOPY);
#endif
#endif
		assert(status != GDI_ERROR);
	}
	::EndPaint(window, &ps);
}

// forward all mouse events to the frame
void Plot::OnLeftButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags)
{
	FORWARD_WM_LBUTTONDOWN(hwnd, doubleClick, x, y, keyFlags, Frame::proc);
}
void Plot::OnLeftButtonUp(HWND hwnd, int x, int y, int keyFlags)
{
	FORWARD_WM_LBUTTONUP(hwnd, x, y, keyFlags, Frame::proc);
}
void Plot::OnMiddleButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags)
{
	FORWARD_WM_MBUTTONDOWN(hwnd, doubleClick, x, y, keyFlags, Frame::proc);
}
void Plot::OnMiddleButtonUp(HWND hwnd, int x, int y, int keyFlags)
{
	FORWARD_WM_MBUTTONUP(hwnd, x, y, keyFlags, Frame::proc);
}
void Plot::OnRightButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags)
{
	FORWARD_WM_RBUTTONDOWN(hwnd, doubleClick, x, y, keyFlags, Frame::proc);
}
void Plot::OnRightButtonUp(HWND hwnd, int x, int y, int keyFlags)
{
	FORWARD_WM_RBUTTONUP(hwnd, x, y, keyFlags, Frame::proc);
}
void Plot::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	FORWARD_WM_MOUSEMOVE(hwnd, x, y, keyFlags, Frame::proc);
}

LRESULT CALLBACK Plot::proc(HWND window, UINT message, WPARAM wp, LPARAM lp)
{
	assert(s_plot != 0);
	switch (message)
	{
	case WM_PAINT:			HANDLE_WM_PAINT(window, wp, lp, Plot::OnPaint);					break;
	case WM_MOUSEMOVE:		HANDLE_WM_MOUSEMOVE(window, wp, lp, Plot::OnMouseMove);			break;
	case WM_LBUTTONDOWN:	HANDLE_WM_LBUTTONDOWN(window, wp, lp, Plot::OnLeftButtonDown);	break;
	case WM_LBUTTONUP:		HANDLE_WM_LBUTTONUP(window, wp, lp, Plot::OnLeftButtonUp);		break;
	case WM_LBUTTONDBLCLK:	HANDLE_WM_LBUTTONDBLCLK(window, wp, lp, Plot::OnLeftButtonDown);	break;
	case WM_MBUTTONDOWN:	HANDLE_WM_MBUTTONDOWN(window, wp, lp, Plot::OnMiddleButtonDown);	break;
	case WM_MBUTTONUP:		HANDLE_WM_MBUTTONUP(window, wp, lp, Plot::OnMiddleButtonUp);		break;
	case WM_RBUTTONDOWN:	HANDLE_WM_RBUTTONDOWN(window, wp, lp, Plot::OnRightButtonDown);	break;
	case WM_RBUTTONUP:		HANDLE_WM_RBUTTONUP(window, wp, lp, Plot::OnRightButtonUp);		break;
	case WM_RBUTTONDBLCLK:	HANDLE_WM_RBUTTONDBLCLK(window, wp, lp, Plot::OnRightButtonDown); break;

	default: return ::DefWindowProc(window, message, wp, lp);
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
static void init_clut(BYTE clut[256][3])
{
	int i;
	for (i = 0; i < 256; i++)
	{
		clut[i][0] = (i >> 5)*8 + 7;
		clut[i][1] = (((i + 16) & 28) >> 2)*8 + 7;
		clut[i][2] = (((i + 2) & 3))*16 + 15;
	}
	clut[0][0] = 0;
	clut[0][1] = 0;
	clut[0][2] = 0;
	clut[1][0] = COLOR_CHANNEL_MAX;
	clut[1][1] = COLOR_CHANNEL_MAX;
	clut[1][2] = COLOR_CHANNEL_MAX;
	clut[2][0] = 3*COLOR_CHANNEL_MAX/4;
	clut[2][1] = COLOR_CHANNEL_MAX;
	clut[2][2] = COLOR_CHANNEL_MAX;
}

int Plot::initialize(HINSTANCE instance, LPCSTR title)
{
	WNDCLASS  wc;
	int result;

	m_instance = instance;
	::strcpy(m_title, title);

	result = ::GetClassInfo(m_instance, s_window_class, &wc);
	if (!result)
	{
		wc.style = CS_DBLCLKS;
		wc.lpfnWndProc = proc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_instance;
		wc.hIcon = 0;
		wc.hCursor = ::LoadCursor(0, IDC_ARROW);
		wc.hbrBackground = static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH));
		wc.lpszMenuName =  m_title;
		wc.lpszClassName = s_window_class;

		result = ::RegisterClass(&wc);
	}

	return result;
}

void Plot::terminate()
{
	delete[] m_pixels;
	m_pixels = 0;
	delete[] m_saved_pixels;
	m_saved_pixels = 0;

	{
		HBITMAP rendering = static_cast<HBITMAP>(::SelectObject(m_memory_dc, static_cast<HGDIOBJ>(m_backup)));
		assert(rendering == m_rendering);
	}
	::DeleteObject(m_rendering);
	::DeleteDC(m_memory_dc);
	::DestroyWindow(m_window);
}

void Plot::create_backing_store()
{
	{
		HDC dc = ::GetDC(m_window);
		m_memory_dc = ::CreateCompatibleDC(dc);
		assert(m_memory_dc);
		::ReleaseDC(m_window, dc);
	}

	m_rendering = ::CreateCompatibleBitmap(m_memory_dc, m_width, m_height);
	assert(m_rendering);
	m_backup = static_cast<HBITMAP>(::SelectObject(m_memory_dc, static_cast<HGDIOBJ>(m_rendering)));

	m_font = ::CreateFont(8, 8, 0, 0, 0, FALSE, FALSE, FALSE, ANSI_CHARSET,
		OUT_RASTER_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
		DEFAULT_PITCH | FF_MODERN, "Courier");
	assert(m_font);
	::SelectObject(m_memory_dc, static_cast<HGDIOBJ>(m_font));
	::SetBkMode(m_memory_dc, OPAQUE);
}

void Plot::create(HWND parent)
{
	if (0 == m_window)
	{
		init_pixels();
		s_plot = this;
		m_parent = parent;
		m_window = ::CreateWindow(s_window_class,
			m_title,
			parent ? WS_CHILD : WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,               // default horizontal position
			CW_USEDEFAULT,               // default vertical position
			m_width,
			m_height,
			parent, 0, m_instance,
			0);

		create_backing_store();
	}
}

void Plot::write_pixel(int x, int y, int color)
{
	assert(m_pixels);
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	m_pixels[(m_height - y - 1)*m_row_len + x] = BYTE(color & 0xFF);
	set_dirty_region(x, y, x + 1, y + 1);
}

int Plot::read_pixel(int x, int y)
{
	assert(m_pixels);
	assert(x >= 0 && x < m_width);
	assert(y >= 0 && y < m_height);
	return int(m_pixels[(m_height - 1 - y)*m_row_len + x]);
}

void Plot::write_span(int y, int x, int lastx, const BYTE *pixels)
{
	int i;
	int width = lastx-x + 1;

	for (i = 0; i < width; i++)
	{
		write_pixel(x + i, y, pixels[i]);
	}
	set_dirty_region(x, y, lastx + 1, y + 1);
}

void Plot::flush()
{
	if (m_dirty)
	{
		RECT r = { -1, -1, -1, -1 };
#if 0
		::InvalidateRect(m_window, &m_dirty_region, FALSE);
#else
#if 0
		DWORD status;
		status = ::StretchDIBits(m_memory_dc,
			0, 0, m_width, m_width,
			0, 0, m_width, m_height,
			m_pixels, &m_bmi, DIB_RGB_COLORS, SRCCOPY);
#else
		::InvalidateRect(m_window, 0, FALSE);
#endif
#endif
		m_dirty = false;
		m_dirty_region = r;
	}
}

void Plot::read_span(int y, int x, int lastx, BYTE *pixels)
{
	int i, width;

	flush();
	width = lastx - x + 1;
	for (i = 0; i < width; i++)
	{
		pixels[i] = read_pixel(x + i, y);
	}
}

void Plot::set_line_mode(int mode)
{
}

void Plot::draw_line(int x1, int y1, int x2, int y2, int color)
{
	::draw_line(x1, y1, x2, y2, color);
}

int Plot::resize()
{
	BOOL status;

	if ((g_screen_width == m_width) && (g_screen_height == m_height))
	{
		return 0;
	}

	init_pixels();
	status = ::SetWindowPos(m_window, 0, 0, 0, m_width, m_height, SWP_NOZORDER | SWP_NOMOVE);
	assert(status);

	return !0;
}

int Plot::read_palette()
{
	if (!g_.RealDAC())
	{
		return -1;
	}

	g_.DAC() = m_clut;

	return 0;
}

int Plot::write_palette()
{
	int i;

	m_clut = g_.DAC();
	for (i = 0; i < 256; i++)
	{
		// TODO: review case when COLOR_CHANNEL_MAX != 63
		m_bmi.bmiColors[i].rgbRed = m_clut.Red(i)*4;
		m_bmi.bmiColors[i].rgbGreen = m_clut.Green(i)*4;
		m_bmi.bmiColors[i].rgbBlue = m_clut.Blue(i)*4;
	}
	redraw();

	return 0;
}

static VOID CALLBACK redraw_window(HWND window, UINT msg, UINT_PTR idEvent, DWORD dwTime)
{
	::InvalidateRect(window, 0, FALSE);
	::KillTimer(window, PLOT_TIMER_ID);
}

void Plot::schedule_alarm(int delay)
{
	UINT_PTR result = ::SetTimer(m_window, PLOT_TIMER_ID, delay, redraw_window);
	if (!result)
	{
		DWORD error = ::GetLastError();
		assert(result);
	}
}

void Plot::clear()
{
	RECT r = { 0, 0, m_width, m_height };
	m_dirty_region = r;
	m_dirty = true;
	::memset(m_pixels, 0, m_pixels_len);
}

void Plot::redraw()
{
	::InvalidateRect(m_window, 0, FALSE);
}

void Plot::display_string(int start_x, int start_y, int fg, int bg, const char *text)
{
	while (*text)
	{
		int row;
		int char_x = start_x;
		for (row = 0; row < 8; row++)
		{
			int x = char_x;
			int col = 8;
			BYTE pixel = g_font_8x8[row][*text];
			while (col-- > 0)
			{
				int color = (pixel & (1 << col)) ? fg : bg;
				write_pixel(x++, start_y + row, color);
			}
		}
		char_x += 8;
		text++;
	}
}

void Plot::save_graphics()
{
	if (0 == m_saved_pixels)
	{
		m_saved_pixels = new BYTE[m_pixels_len];
		::memset(m_saved_pixels, 0, m_pixels_len);
	}
	::memcpy(m_saved_pixels, m_pixels, m_pixels_len);
}

void Plot::restore_graphics()
{
	assert(m_saved_pixels);
	::memcpy(m_pixels, m_saved_pixels, m_pixels_len);
	redraw();
}
