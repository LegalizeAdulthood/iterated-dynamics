#pragma once

#define WIN32_MAXSCREENS 10

class Win32BaseDriver : public NamedDriver
{
public:
	Win32BaseDriver(const char *name, const char *description);

	/* initialize the driver */			virtual int initialize(int *argc, char **argv);
	/* shutdown the driver */			virtual void terminate();

										virtual void set_video_mode(const VIDEOINFO &mode);

	/* poll or block for a key */		virtual int get_key();
										virtual void unget_key(int key);
										virtual int key_cursor(int row, int col);
										virtual int key_pressed();
										virtual int wait_key_pressed(int timeout);
										virtual void set_keyboard_timeout(int ms);
	/* invoke a command shell */		virtual void shell();
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
										virtual void set_clear();

	/* sound routines */				virtual int init_fm();
										virtual void buzzer(int kind);
										virtual int sound_on(int frequency);
										virtual void sound_off();
										virtual void mute();

	/* returns true if disk video */	virtual int diskp();

										virtual void delay(int ms);

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

	bool m_inside_help;
	int m_save_check_time;				/* time of last autosave check */

	time_t m_start;
	long m_ticks_per_second;
	long m_last;
	static const long m_frames_per_second = 10;
};
