#pragma once

#define WIN32_MAXSCREENS 10

class Win32BaseDriver : public AbstractDriver
{
public:
	Win32BaseDriver(const char *name, const char *description);

	/* name of driver */				virtual const char *name() const		{ return m_name; }
	/* driver description */			virtual const char *description() const { return m_description; }
	/* initialize the driver */			virtual int initialize(int *argc, char **argv);
	/* shutdown the driver */			virtual void terminate();

	/* pause this driver */				virtual void pause() = 0;
	/* resume this driver */			virtual void resume() = 0;

	/* validate a fractint.cfg mode */	virtual int validate_mode(const VIDEOINFO &mode) = 0;
										virtual void set_video_mode(const VIDEOINFO &mode);
	/* find max screen extents */		virtual void get_max_screen(int &x_max, int &y_max) const = 0;

	/* creates a window */				virtual void window() = 0;
	/* handles window resize.  */		virtual int resize() = 0;
	/* redraws the screen */			virtual void redraw() = 0;

	/* read palette into g_dac_box */	virtual int read_palette() = 0;
	/* write g_dac_box into palette */	virtual int write_palette() = 0;

	/* reads a single pixel */			virtual int read_pixel(int x, int y) = 0;
	/* writes a single pixel */			virtual void write_pixel(int x, int y, int color) = 0;
	/* reads a span of pixel */			virtual void read_span(int y, int x, int lastx, BYTE *pixels) = 0;
	/* writes a span of pixels */		virtual void write_span(int y, int x, int lastx, const BYTE *pixels) = 0;
										virtual void get_truecolor(int x, int y, int &r, int &g, int &b, int &a) = 0;
										virtual void put_truecolor(int x, int y, int r, int g, int b, int a) = 0;
	/* set copy/xor line */				virtual void set_line_mode(int mode) = 0;
	/* draw line */						virtual void draw_line(int x1, int y1, int x2, int y2, int color) = 0;
	/* draw string in graphics mode */	virtual void display_string(int x, int y, int fg, int bg, const char *text) = 0;
	/* save graphics */					virtual void save_graphics() = 0;
	/* restore graphics */				virtual void restore_graphics() = 0;
	/* poll or block for a key */		virtual int get_key();
										virtual void unget_key(int key);
										virtual int key_cursor(int row, int col);
										virtual int key_pressed();
										virtual int wait_key_pressed(int timeout);
										virtual void set_keyboard_timeout(int ms);
	/* invoke a command shell */		virtual void shell();
	/* set for text mode & save gfx */	virtual void set_for_text() = 0;
	/* restores graphics and data */	virtual void set_for_graphics() = 0;
	/* clears text screen */			virtual void set_clear() = 0;
	/* text screen functions */			virtual void move_cursor(int row, int col);
										virtual void hide_text_cursor();
										virtual void put_string(int row, int col, int attr, const char *text);
										virtual void set_attr(int row, int col, int attr, int count);
										virtual void scroll_up(int top, int bottom);
										virtual void stack_screen();
										virtual void unstack_screen();
										virtual void discard_screen();
										virtual int get_char_attr();
										virtual void put_char_attr(int char_attr);
										virtual int get_char_attr_rowcol(int row, int col);
										virtual void put_char_attr_rowcol(int row, int col, int char_attr);

	/* sound routines */				virtual int init_fm();
										virtual void buzzer(int kind);
										virtual int sound_on(int frequency);
										virtual void sound_off();
										virtual void mute();

	/* returns true if disk video */	virtual int diskp();

										virtual void delay(int ms);
										virtual void flush() = 0;
	/* refresh alarm */					virtual void schedule_alarm(int secs) = 0;

protected:
	Frame m_frame;
	WinText m_wintext;

	/* key_buffer
	*
	* When we peeked ahead and saw a keypress, stash it here for later
	* feeding to our caller.
	*/
	int m_key_buffer;

	int m_screen_count;
	BYTE *m_saved_screens[WIN32_MAXSCREENS];
	int m_saved_cursor[WIN32_MAXSCREENS+1];
	bool m_cursor_shown;
	int m_cursor_row;
	int m_cursor_col;

private:
	void flush_output();
	int handle_timed_save(int ch);
	int handle_special_keys(int ch);

	const char *m_name;
	const char *m_description;

	bool m_inside_help;
	int m_save_check_time;				/* time of last autosave check */

	time_t m_start;
	long m_ticks_per_second;
	long m_last;
	static const long m_frames_per_second = 10;
};
