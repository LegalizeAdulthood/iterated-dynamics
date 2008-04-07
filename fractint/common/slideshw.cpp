// These routines are called by driver_get_key to allow controlling
// keystrokes to be read from a file.

#include <cctype>
#include <ctime>
#include <fstream>
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "Externals.h"
#include "realdos.h"
#include "slideshw.h"

bool g_busy = false;

SlideShow g_slideShow;

class SlideShowImpl
{
public:
	SlideShowImpl(CalculationStatusType calculationStatus, bool &busy)
		: _slideFile(),
		_start_tick(0),
		_ticks(0),
		_slow_count(0),
		_quotes(0),
		_calc_wait(false),
		_repeats(0),
		_last1(0),
		_calculationStatus(calculationStatus),
		_busy(busy),
		_autoKeyFile("auto.key"),
		_mode(SLIDES_OFF)
	{
	}

	int GetKeyStroke();
	SlideType Start();
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
	SlideType Mode() const { return _mode; }
	void Mode(SlideType value) { _mode = value; }

private:
	std::fstream _slideFile;
	long _start_tick;
	long _ticks;
	int _slow_count;
	unsigned int _quotes;
	bool _calc_wait;
	int _repeats;
	int _last1;
	const CalculationStatusType _calculationStatus;
	const bool &_busy;
	std::string _autoKeyFile;
	SlideType _mode;

	void Error(const std::string &msg);
	void SleepSeconds(int secs);
	void ShowTempMessageText(int row, int col, int attr, int secs, const char *txt);
	void Message(int secs, const char *message);
	void Message(int secs, const std::string &message)
	{
		Message(secs, message.c_str());
	}
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
	{ IDK_ENTER,			"ENTER"     },
	{ IDK_INSERT,			"INSERT"    },
	{ IDK_DELETE,			"DELETE"    },
	{ IDK_ESC,				"ESC"       },
	{ IDK_TAB,				"TAB"       },
	{ IDK_PAGE_UP,			"PAGEUP"    },
	{ IDK_PAGE_DOWN,		"PAGEDOWN"  },
	{ IDK_HOME,				"HOME"      },
	{ IDK_END,				"END"       },
	{ IDK_LEFT_ARROW,		"LEFT"      },
	{ IDK_RIGHT_ARROW,		"RIGHT"     },
	{ IDK_UP_ARROW,			"UP"        },
	{ IDK_DOWN_ARROW,		"DOWN"      },
	{ IDK_F1,				"F1"        },
	{ IDK_CTL_RIGHT_ARROW,	"CTRL_RIGHT"},
	{ IDK_CTL_LEFT_ARROW,	"CTRL_LEFT" },
	{ IDK_CTL_DOWN_ARROW,	"CTRL_DOWN" },
	{ IDK_CTL_UP_ARROW,		"CTRL_UP"   },
	{ IDK_CTL_END,			"CTRL_END"  },
	{ IDK_CTL_HOME,			"CTRL_HOME" }
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

// places a temporary message on the screen in text mode
void SlideShowImpl::ShowTempMessageText(int row, int col, int attr, int secs, const char *txt)
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

void SlideShowImpl::Message(int secs, const char *buf)
{
	ShowTempMessageText(0, 0, 7, secs, buf);
	if (show_temp_message(buf) == 0)
	{
		SleepSeconds(secs);
		clear_temp_message();
	}
}

// this routine reads the file _autoKeyFile and returns keystrokes
int SlideShowImpl::GetKeyStroke()
{
	if (_calc_wait)
	{
		if (_calculationStatus == CALCSTAT_IN_PROGRESS || _busy) // restart timer - process not done
		{
			return 0; // wait for calc to finish before reading more keystrokes
		}
		_calc_wait = false;
	}
	if (!_slideFile)   // open files first time through
	{
		if (Start() == SLIDES_OFF)
		{
			Stop();
			return 0;
		}
	}

	if (_ticks) // if waiting, see if waited long enough
	{
		if (clock_ticks() - _start_tick < _ticks) // haven't waited long enough
		{
			return 0;
		}
		_ticks = 0;
	}
	if (++_slow_count <= 18)
	{
		_start_tick = clock_ticks();
		_ticks = CLK_TCK/5; // a slight delay so keystrokes are visible
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
	if (_quotes) // reading a quoted string
	{
		out = _slideFile.get();
		if (out != '\"' && out != EOF)
		{
			return _last1 = out;
		}
		_quotes = 0;
	}
	// skip white space:
	do
	{
		out = _slideFile.get();
	}
	while (out == ' ' || out == '\t' || out == '\n');
	switch (out)
	{
	case EOF:
		g_slideShow.Stop();
		return 0;
	case '\"':        // begin quoted string
		_quotes = 1;
		goto start;
	case ';':         // comment from here to end of line, skip it
		do
		{
			out = _slideFile.get();
		}
		while (out != '\n' && out != EOF);
		goto start;
	case '*':
		_slideFile >> _repeats;
		if (!_slideFile
			|| _repeats <= 1 || _repeats >= 256 || _slideFile.eof())
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
	while (true) // get a token
	{
		if (i < 80)
		{
			buffer[i++] = (char) out;
		}
		out = _slideFile.get();
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
	if (isdigit(buffer[0]))       // an arbitrary scan code number - use it
	{
		out = atoi(buffer);
	}
	else if (strcmp((char *)buffer, "MESSAGE") == 0)
	{
		out = 0;
		int secs;
		_slideFile >> secs;
		if (!_slideFile)
		{
			Error("MESSAGE needs argument");
		}
		else
		{
			std::string buf;
			getline(_slideFile, buf);
			Message(secs, buf);
		}
		out = 0;
	}
	else if (strcmp((char *)buffer, "GOTO") == 0)
	{
		std::string line;
		if (!getline(_slideFile, line))
		{
			Error("GOTO needs target");
			out = 0;
		}
		else
		{
			_slideFile.seekg(0);
			line += ":";
			bool err;
			std::string buffer1;
			do
			{
				err = !getline(_slideFile, buffer1);
			}
			while (!err && line != buffer);
			if (!_slideFile)
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
		_slideFile >> fticks;
		if (_slideFile)
		{
			driver_set_keyboard_timeout(int(fticks*1000.f));
			fticks *= CLK_TCK;             // convert from seconds to ticks
			_ticks = long(fticks);
			_start_tick = clock_ticks();  // start timing
		}
		else
		{
			Error("WAIT needs argument");
		}
		_slow_count = 0;
		out = 0;
	}
	else if (strcmp("CALCWAIT", (char *)buffer) == 0) // wait for calc to finish
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

SlideType SlideShowImpl::Start()
{
	_slideFile.open(_autoKeyFile.c_str(), std::ios::in);
	if (!_slideFile.is_open())
	{
		_mode = SLIDES_OFF;
	}
	_ticks = 0;
	_quotes = 0;
	_calc_wait = false;
	_slow_count = 0;
	return _mode;
}

void SlideShowImpl::Stop()
{
	_slideFile.close();
	_mode = SLIDES_OFF;
}

void SlideShowImpl::Record(int key)
{
	float dt = float(_ticks);      // save time of last call
	_ticks = clock_ticks();  // current time
	if (_slideFile == 0)
	{
		_slideFile.open(_autoKeyFile.c_str(), std::ios::out);
		if (!_slideFile.is_open())
		{
			return;
		}
	}
	dt = _ticks-dt;
	dt /= CLK_TCK;  // dt now in seconds
	if (dt > 0.5) // don't bother with less than half a second
	{
		if (_quotes) // close quotes first
		{
			_quotes = 0;
			_slideFile << "\"\n";
		}
		_slideFile << boost::format("WAIT %4.1f\n") % dt;
	}
	if (key >= 32 && key < 128)
	{
		if (!_quotes)
		{
			_quotes = 1;
			_slideFile << '\"';
		}
		_slideFile << char(key);
	}
	else
	{
		if (_quotes) // not an ASCII character - turn off quotes
		{
			_slideFile << "\"\n";
			_quotes = 0;
		}
		const char *mn = ScanCodes::MnemonicFromScanCode(key);
		if (mn)
		{
			_slideFile << mn;
		}
		else if (check_video_mode_key(key) >= 0)
		{
			_slideFile << video_mode_key_name(key);
		}
		else // not ASCII and not FN key
		{
			_slideFile << boost::format("%4d") % key;
		}
		_slideFile << '\n';
	}
}

// suspend process # of seconds
void SlideShowImpl::SleepSeconds(int secs)
{
	long stop = clock_ticks() + long(secs)*CLK_TCK;
	while (clock_ticks() < stop && driver_key_pressed() == 0)
	{
	} // bailout if key hit
}

void SlideShowImpl::Error(const std::string &msg)
{
	Stop();
	stop_message(STOPMSG_NORMAL, "Slideshow error:\n" + msg);
}

SlideShow::SlideShow()
	: _impl(new SlideShowImpl(g_externs.CalculationStatus(), g_busy))
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

SlideType SlideShow::Start()
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

SlideType SlideShow::Mode() const
{
	return _impl->Mode();
}

void SlideShow::Mode(SlideType value)
{
	_impl->Mode(value);
}
