#if !defined(PLOT_H)
#define PLOT_H

class Plot
{
public:
	int initialize(HINSTANCE instance, LPCSTR title);
	void terminate();
	void create(HWND parent);
	void write_pixel(int x, int y, int color);
	int read_pixel(int x, int y);
	void write_span(int x, int y, int lastx, const BYTE *pixels);
	void read_span(int x, int y, int lastx, BYTE *pixels);
	void set_line_mode(int mode);
	void draw_line(int x1, int y1, int x2, int y2, int color);
	int resize();
	int read_palette();
	int write_palette();
	void flush();
	void schedule_alarm(int delay);
	void clear();
	void redraw();
	void display_string(int x, int y, int fg, int bg, const char *text);
	void save_graphics();
	void restore_graphics();
	int width() const { return m_width; }
	int height() const { return m_height; }
	HWND window() const { return m_window; }

private:
	void set_dirty_region(int x_min, int y_min, int x_max, int y_max);
	void init_pixels();
	void create_backing_store();

	static LRESULT CALLBACK proc(HWND window, UINT message, WPARAM wp, LPARAM lp);
	static void OnPaint(HWND window);
	static void OnLeftButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags);
	static void OnLeftButtonUp(HWND hwnd, int x, int y, int keyFlags);
	static void OnMiddleButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags);
	static void OnMiddleButtonUp(HWND hwnd, int x, int y, int keyFlags);
	static void OnRightButtonDown(HWND hwnd, BOOL doubleClick, int x, int y, int keyFlags);
	static void OnRightButtonUp(HWND hwnd, int x, int y, int keyFlags);
	static void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);

	static Plot *s_plot;
	static LPCTSTR s_window_class;

	HINSTANCE m_instance;
	char m_title[120];
	HWND m_parent;

	HWND m_window;
	HDC m_memory_dc;
	HBITMAP m_rendering;
	HBITMAP m_backup;
	HFONT m_font;

	bool m_dirty;
	RECT m_dirty_region;
	BITMAPINFO m_bmi;						/* contains first clut entry too */
	RGBQUAD m_bmiColors[255];				/* color look up table */

	BYTE *m_pixels;
	BYTE *m_saved_pixels;
	size_t m_pixels_len;
	size_t m_row_len;
	int m_width;
	int m_height;
	unsigned char m_clut[256][3];
};

#endif
