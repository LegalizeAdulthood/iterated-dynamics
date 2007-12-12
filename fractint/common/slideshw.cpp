/***********************************************************************/
/* These routines are called by driver_get_key to allow keystrokes to control */
/* Fractint to be read from a file.                                    */
/***********************************************************************/
#include <string>

#include <ctype.h>
#include <time.h>
#include <string.h>
#ifndef XFRACT
#include <conio.h>
#endif

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "realdos.h"
#include "slideshw.h"

bool g_busy = false;

SlideShow g_slideShow;

class SlideShowImpl
{
public:
	SlideShowImpl(int &calculationStatus, bool &busy)
		: _slide_file(0),
		_start_tick(0),
		_ticks(0),
		_slow_count(0),
		_quotes(0),
		_calc_wait(false),
		_repeats(0),
		_last1(0),
		_calculationStatus(calculationStatus),
		_busy(busy),
		_autoKeyFile("auto.key")
	{
	}

	int GetKeyStroke();
	int Start();
	void Stop();
	void Record(int keyStroke);
	const std::string &AutoKeyFile() const
	{
		return _autoKeyFile;
	}
	void SetAutoKeyFile(const std::string &value)
	{
		_autoKeyFile = value;
	}

private:
	FILE *_slide_file;
	long _start_tick;
	long _ticks;
	int _slow_count;
	unsigned int _quotes;
	bool _calc_wait;
	int _repeats;
	int _last1;
	const int &_calculationStatus;
	const bool &_busy;
	std::string _autoKeyFile;

	void Error(const std::string &msg);
	void SleepSeconds(int secs);
	void ShowTempMessageText(int row, int col, int attr, int secs, char *txt);
	void Message(int secs, char *buf);
};

class ScanCodes
{
public:
	static int ScanCodeFromMnemonic(const char *mnemonic);
	static const char *MnemonicFromScanCode(int scan_code);

private:
	struct ScanCodeMnemonicPair
	{
		int code;
		const char *mnemonic;
	};

	static const ScanCodeMnemonicPair _scanCodes[];
};

const ScanCodes::ScanCodeMnemonicPair ScanCodes::_scanCodes[] =
{
	{ FIK_ENTER,			"ENTER"     },
	{ FIK_INSERT,			"INSERT"    },
	{ FIK_DELETE,			"DELETE"    },
	{ FIK_ESC,				"ESC"       },
	{ FIK_TAB,				"TAB"       },
	{ FIK_PAGE_UP,			"PAGEUP"    },
	{ FIK_PAGE_DOWN,		"PAGEDOWN"  },
	{ FIK_HOME,				"HOME"      },
	{ FIK_END,				"END"       },
	{ FIK_LEFT_ARROW,		"LEFT"      },
	{ FIK_RIGHT_ARROW,		"RIGHT"     },
	{ FIK_UP_ARROW,			"UP"        },
	{ FIK_DOWN_ARROW,		"DOWN"      },
	{ FIK_F1,				"F1"        },
	{ FIK_CTL_RIGHT_ARROW,	"CTRL_RIGHT"},
	{ FIK_CTL_LEFT_ARROW,	"CTRL_LEFT" },
	{ FIK_CTL_DOWN_ARROW,	"CTRL_DOWN" },
	{ FIK_CTL_UP_ARROW,		"CTRL_UP"   },
	{ FIK_CTL_END,			"CTRL_END"  },
	{ FIK_CTL_HOME,			"CTRL_HOME" }
};

int ScanCodes::ScanCodeFromMnemonic(const char *mn)
{

	for (int i = 0; i < NUM_OF(_scanCodes); i++)
	{
		if (strcmp(mn, _scanCodes[i].mnemonic) == 0)
		{
			return _scanCodes[i].code;
		}
	}
	return -1;
}

const char *ScanCodes::MnemonicFromScanCode(int code)
{
	for (int i = 0; i < NUM_OF(_scanCodes); i++)
	{
		if (code == _scanCodes[i].code)
		{
			return _scanCodes[i].mnemonic;
		}
	}
	return 0;
}

/* places a temporary message on the screen in text mode */
void SlideShowImpl::ShowTempMessageText(int row, int col, int attr, int secs, char *txt)
{
	int savescrn[80];

	for (int i = 0; i < 80; i++)
	{
		driver_move_cursor(row, i);
		savescrn[i] = driver_get_char_attr();
	}
	driver_put_string(row, col, attr, txt);
	driver_hide_text_cursor();
	SleepSeconds(secs);
	for (int i = 0; i < 80; i++)
	{
		driver_move_cursor(row, i);
		driver_put_char_attr(savescrn[i]);
	}
}

void SlideShowImpl::Message(int secs, char *buf)
{
	char nearbuf[41] = { 0 };
	strncpy(nearbuf, buf, NUM_OF(nearbuf)-1);
	ShowTempMessageText(0, 0, 7, secs, nearbuf);
	if (show_temp_message(nearbuf) == 0)
	{
		SleepSeconds(secs);
		clear_temp_message();
	}
}

/* this routine reads the file _autoKeyFile and returns keystrokes */
int SlideShowImpl::GetKeyStroke()
{
	if (_calc_wait)
	{
		if (_calculationStatus == CALCSTAT_IN_PROGRESS || _busy) /* restart timer - process not done */
		{
			return 0; /* wait for calc to finish before reading more keystrokes */
		}
		_calc_wait = false;
	}
	if (_slide_file == 0)   /* open files first time through */
	{
		if (start_slide_show() == 0)
		{
			stop_slide_show();
			return 0;
		}
	}

	if (_ticks) /* if waiting, see if waited long enough */
	{
		if (clock_ticks() - _start_tick < _ticks) /* haven't waited long enough */
		{
			return 0;
		}
		_ticks = 0;
	}
	if (++_slow_count <= 18)
	{
		_start_tick = clock_ticks();
		_ticks = CLK_TCK/5; /* a slight delay so keystrokes are visible */
		if (_slow_count > 10)
		{
			_ticks /= 2;
		}
	}
	if (_repeats > 0)
	{
		_repeats--;
		return _last1;
	}

start:
	int out;
	if (_quotes) /* reading a quoted string */
	{
		out = fgetc(_slide_file);
		if (out != '\"' && out != EOF)
		{
			return _last1 = out;
		}
		_quotes = 0;
	}
	/* skip white space: */
	do
	{
		out = fgetc(_slide_file);
	}
	while (out == ' ' || out == '\t' || out == '\n');
	switch (out)
	{
	case EOF:
		stop_slide_show();
		return 0;
	case '\"':        /* begin quoted string */
		_quotes = 1;
		goto start;
	case ';':         /* comment from here to end of line, skip it */
		do
		{
			out = fgetc(_slide_file);
		}
		while (out != '\n' && out != EOF);
		goto start;
	case '*':
		if (fscanf(_slide_file, "%d", &_repeats) != 1
			|| _repeats <= 1 || _repeats >= 256 || feof(_slide_file))
		{
			Error("error in * argument");
			_last1 = 0;
			_repeats = 0;
		}
		_repeats -= 2;
		return out = _last1;
	}

	char buffer[81];
	int i = 0;
	while (true) /* get a token */
	{
		if (i < 80)
		{
			buffer[i++] = (char) out;
		}
		out = fgetc(_slide_file);
		if (out == ' ' || out == '\t' || out == '\n' || out == EOF)
		{
			break;
		}
	}
	buffer[i] = 0;
	if (buffer[i - 1] == ':')
	{
		goto start;
	}
	out = -12345;
	if (isdigit(buffer[0]))       /* an arbitrary scan code number - use it */
	{
		out = atoi(buffer);
	}
	else if (strcmp((char *)buffer, "MESSAGE") == 0)
	{
		out = 0;
		int secs;
		if (fscanf(_slide_file, "%d", &secs) != 1)
		{
			Error("MESSAGE needs argument");
		}
		else
		{
			char buf[41];
			buf[40] = 0;
			fgets(buf, 40, _slide_file);
			int len = int(strlen(buf));
			buf[len - 1] = 0; /* zap newline */
			Message(secs, buf);
		}
		out = 0;
	}
	else if (strcmp((char *)buffer, "GOTO") == 0)
	{
		if (fscanf(_slide_file, "%s", buffer) != 1)
		{
			Error("GOTO needs target");
			out = 0;
		}
		else
		{
			rewind(_slide_file);
			strcat(buffer, ":");
			int err;
			char buffer1[80];
			do
			{
				err = fscanf(_slide_file, "%s", buffer1);
			}
			while (err == 1 && strcmp(buffer1, buffer) != 0);
			if (feof(_slide_file))
			{
				Error("GOTO target not found");
				return 0;
			}
			goto start;
		}
	}
	else if ((i = ScanCodes::ScanCodeFromMnemonic(buffer)) > 0)
	{
		out = i;
	}
	else if (strcmp("WAIT", (char *)buffer) == 0)
	{
		float fticks;
		int err = fscanf(_slide_file, "%f", &fticks); /* how many ticks to wait */
		driver_set_keyboard_timeout(int(fticks*1000.f));
		fticks *= CLK_TCK;             /* convert from seconds to ticks */
		if (err == 1)
		{
			_ticks = long(fticks);
			_start_tick = clock_ticks();  /* start timing */
		}
		else
		{
			Error("WAIT needs argument");
		}
		_slow_count = 0;
		out = 0;
	}
	else if (strcmp("CALCWAIT", (char *)buffer) == 0) /* wait for calc to finish */
	{
		_calc_wait = true;
		_slow_count = 0;
		out = 0;
	}
	else
	{
		i = check_vidmode_keyname(buffer);
		if (i != 0)
		{
			out = i;
		}
	}
	if (out == -12345)
	{
		Error("Can't understand " + std::string(buffer));
		out = 0;
	}
	_last1 = out;
	return out;
}

int SlideShowImpl::Start()
{
	_slide_file = fopen(_autoKeyFile.c_str(), "r");
	if (_slide_file == 0)
	{
		g_slides = SLIDES_OFF;
	}
	_ticks = 0;
	_quotes = 0;
	_calc_wait = false;
	_slow_count = 0;
	return g_slides;
}

void SlideShowImpl::Stop()
{
	if (_slide_file)
	{
		fclose(_slide_file);
	}
	_slide_file = 0;
	g_slides = SLIDES_OFF;
}

void SlideShowImpl::Record(int key)
{
	float dt = float(_ticks);      /* save time of last call */
	_ticks = clock_ticks();  /* current time */
	if (_slide_file == 0)
	{
		_slide_file = fopen(_autoKeyFile.c_str(), "w");
		if (_slide_file == 0)
		{
			return;
		}
	}
	dt = _ticks-dt;
	dt /= CLK_TCK;  /* dt now in seconds */
	if (dt > 0.5) /* don't bother with less than half a second */
	{
		if (_quotes) /* close quotes first */
		{
			_quotes = 0;
			fprintf(_slide_file, "\"\n");
		}
		fprintf(_slide_file, "WAIT %4.1f\n", dt);
	}
	if (key >= 32 && key < 128)
	{
		if (!_quotes)
		{
			_quotes = 1;
			fputc('\"', _slide_file);
		}
		fputc(key, _slide_file);
	}
	else
	{
		if (_quotes) /* not an ASCII character - turn off quotes */
		{
			fprintf(_slide_file, "\"\n");
			_quotes = 0;
		}
		const char *mn = ScanCodes::MnemonicFromScanCode(key);
		if (mn)
		{
			fprintf(_slide_file, "%s", mn);
		}
		else if (check_video_mode_key(0, key) >= 0)
		{
			char buf[10];
			video_mode_key_name(key, buf);
			fprintf(_slide_file, buf);
		}
		else /* not ASCII and not FN key */
		{
			fprintf(_slide_file, "%4d", key);
		}
		fputc('\n', _slide_file);
	}
}

/* suspend process # of seconds */
void SlideShowImpl::SleepSeconds(int secs)
{
	long stop;
	stop = clock_ticks() + long(secs)*CLK_TCK;
	while (clock_ticks() < stop && kbhit() == 0)
	{
	} /* bailout if key hit */
}

void SlideShowImpl::Error(const std::string &msg)
{
	stop_slide_show();
	stop_message(STOPMSG_NORMAL, "Slideshow error:\n" + msg);
}

SlideShow::SlideShow()
	: _impl(new SlideShowImpl(g_calculation_status, g_busy))
{
}

SlideShow::~SlideShow()
{
	delete _impl;
}

int SlideShow::GetKeyStroke()
{
	return _impl->GetKeyStroke();
}

int SlideShow::Start()
{
	return _impl->Start();
}

void SlideShow::Stop()
{
	_impl->Stop();
}

void SlideShow::Record(int keyStroke)
{
	_impl->Record(keyStroke);
}

const std::string &SlideShow::AutoKeyFile() const
{
	return _impl->AutoKeyFile();
}

void SlideShow::SetAutoKeyFile(const std::string &value)
{
	_impl->SetAutoKeyFile(value);
}
