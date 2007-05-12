/***********************************************************************/
/* These routines are called by driver_get_key to allow keystrokes to control */
/* Fractint to be read from a file.                                    */
/***********************************************************************/

#include <ctype.h>
#include <time.h>
#include <string.h>
#ifndef XFRACT
#include <conio.h>
#endif

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

bool g_busy = false;
static FILE *s_slide_file = NULL;
static long s_start_tick = 0;
static long s_ticks = 0;
static int s_slow_count = 0;
static unsigned int s_quotes = 0;
static bool s_calc_wait = false;
static int s_repeats = 0;
static int s_last1 = 0;

static void sleep_secs(int);
static void showtempmsg_txt(int, int, int, int, char *);
static void message(int secs, char *buf);
static void slideshowerr(char *msg);
static int  get_scancode(char *mn);
static void get_mnemonic(int code, char *mnemonic);

#define MAX_MNEMONIC    20   /* max size of any mnemonic string */

struct scancodes
{
	int code;
	char *mnemonic;
};

static struct scancodes scancodes[] =
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

static int get_scancode(char *mn)
{
	for (int i = 0; i < NUM_OF(scancodes); i++)
	{
		if (strcmp((char *) mn, scancodes[i].mnemonic) == 0)
		{
			return scancodes[i].code;
		}
	}
	return -1;
}

static void get_mnemonic(int code, char *mnemonic)
{
	*mnemonic = 0;
	for (int i = 0; i < NUM_OF(scancodes); i++)
	{
		if (code == scancodes[i].code)
		{
			strcpy(mnemonic, scancodes[i].mnemonic);
			return;
		}
	}
}

/* places a temporary message on the screen in text mode */
static void showtempmsg_txt(int row, int col, int attr, int secs, char *txt)
{
	int savescrn[80];

	for (int i = 0; i < 80; i++)
	{
		driver_move_cursor(row, i);
		savescrn[i] = driver_get_char_attr();
	}
	driver_put_string(row, col, attr, txt);
	driver_hide_text_cursor();
	sleep_secs(secs);
	for (int i = 0; i < 80; i++)
	{
		driver_move_cursor(row, i);
		driver_put_char_attr(savescrn[i]);
	}
}

static void message(int secs, char *buf)
{
	char nearbuf[41] = { 0 };
	strncpy(nearbuf, buf, NUM_OF(nearbuf)-1);
	showtempmsg_txt(0, 0, 7, secs, nearbuf);
	if (show_temp_message(nearbuf) == 0)
	{
		sleep_secs(secs);
		clear_temp_message();
	}
}

/* this routine reads the file g_autokey_name and returns keystrokes */
int slide_show()
{
	if (s_calc_wait)
	{
		if (g_calculation_status == CALCSTAT_IN_PROGRESS || g_busy) /* restart timer - process not done */
		{
			return 0; /* wait for calc to finish before reading more keystrokes */
		}
		s_calc_wait = false;
	}
	if (s_slide_file == NULL)   /* open files first time through */
	{
		if (start_slide_show() == 0)
		{
			stop_slide_show();
			return 0;
		}
	}

	if (s_ticks) /* if waiting, see if waited long enough */
	{
		if (clock_ticks() - s_start_tick < s_ticks) /* haven't waited long enough */
		{
			return 0;
		}
		s_ticks = 0;
	}
	if (++s_slow_count <= 18)
	{
		s_start_tick = clock_ticks();
		s_ticks = CLK_TCK/5; /* a slight delay so keystrokes are visible */
		if (s_slow_count > 10)
		{
			s_ticks /= 2;
		}
	}
	if (s_repeats > 0)
	{
		s_repeats--;
		return s_last1;
	}

start:
	int out;
	if (s_quotes) /* reading a quoted string */
	{
		out = fgetc(s_slide_file);
		if (out != '\"' && out != EOF)
		{
			return s_last1 = out;
		}
		s_quotes = 0;
	}
	/* skip white space: */
	do
	{
		out = fgetc(s_slide_file);
	}
	while (out == ' ' || out == '\t' || out == '\n');
	switch (out)
	{
	case EOF:
		stop_slide_show();
		return 0;
	case '\"':        /* begin quoted string */
		s_quotes = 1;
		goto start;
	case ';':         /* comment from here to end of line, skip it */
		do
		{
			out = fgetc(s_slide_file);
		}
		while (out != '\n' && out != EOF);
		goto start;
	case '*':
		if (fscanf(s_slide_file, "%d", &s_repeats) != 1
			|| s_repeats <= 1 || s_repeats >= 256 || feof(s_slide_file))
		{
			slideshowerr("error in * argument");
			s_last1 = s_repeats = 0;
		}
		s_repeats -= 2;
		return out = s_last1;
	}

	char buffer[81];
	int i = 0;
	while (1) /* get a token */
	{
		if (i < 80)
		{
			buffer[i++] = (char) out;
		}
		out = fgetc(s_slide_file);
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
		if (fscanf(s_slide_file, "%d", &secs) != 1)
		{
			slideshowerr("MESSAGE needs argument");
		}
		else
		{
			char buf[41];
			buf[40] = 0;
			fgets(buf, 40, s_slide_file);
			int len = (int) strlen(buf);
			buf[len - 1] = 0; /* zap newline */
			message(secs, (char *) buf);
		}
		out = 0;
	}
	else if (strcmp((char *)buffer, "GOTO") == 0)
	{
		if (fscanf(s_slide_file, "%s", buffer) != 1)
		{
			slideshowerr("GOTO needs target");
			out = 0;
		}
		else
		{
			rewind(s_slide_file);
			strcat(buffer, ":");
			int err;
			char buffer1[80];
			do
			{
				err = fscanf(s_slide_file, "%s", buffer1);
			}
			while (err == 1 && strcmp(buffer1, buffer) != 0);
			if (feof(s_slide_file))
			{
				slideshowerr("GOTO target not found");
				return 0;
			}
			goto start;
		}
	}
	else if ((i = get_scancode(buffer)) > 0)
	{
		out = i;
	}
	else if (strcmp("WAIT", (char *)buffer) == 0)
	{
		float fticks;
		int err = fscanf(s_slide_file, "%f", &fticks); /* how many ticks to wait */
		driver_set_keyboard_timeout((int) (fticks*1000.f));
		fticks *= CLK_TCK;             /* convert from seconds to ticks */
		if (err == 1)
		{
			s_ticks = (long) fticks;
			s_start_tick = clock_ticks();  /* start timing */
		}
		else
		{
			slideshowerr("WAIT needs argument");
		}
		s_slow_count = 0;
		out = 0;
	}
	else if (strcmp("CALCWAIT", (char *)buffer) == 0) /* wait for calc to finish */
	{
		s_calc_wait = true;
		s_slow_count = 0;
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
		char msg[MESSAGE_LEN];
		sprintf(msg, "Can't understand %s", buffer);
		slideshowerr(msg);
		out = 0;
	}
	s_last1 = out;
	return out;
}

int start_slide_show()
{
	s_slide_file = fopen(g_autokey_name, "r");
	if (s_slide_file == NULL)
	{
		g_slides = SLIDES_OFF;
	}
	s_ticks = 0;
	s_quotes = 0;
	s_calc_wait = false;
	s_slow_count = 0;
	return g_slides;
}

void stop_slide_show()
{
	if (s_slide_file)
	{
		fclose(s_slide_file);
	}
	s_slide_file = NULL;
	g_slides = SLIDES_OFF;
}

void record_show(int key)
{
	float dt = (float) s_ticks;      /* save time of last call */
	s_ticks = clock_ticks();  /* current time */
	if (s_slide_file == NULL)
	{
		s_slide_file = fopen(g_autokey_name, "w");
		if (s_slide_file == NULL)
		{
			return;
		}
	}
	dt = s_ticks-dt;
	dt /= CLK_TCK;  /* dt now in seconds */
	if (dt > 0.5) /* don't bother with less than half a second */
	{
		if (s_quotes) /* close quotes first */
		{
			s_quotes = 0;
			fprintf(s_slide_file, "\"\n");
		}
		fprintf(s_slide_file, "WAIT %4.1f\n", dt);
	}
	if (key >= 32 && key < 128)
	{
		if (!s_quotes)
		{
			s_quotes = 1;
			fputc('\"', s_slide_file);
		}
		fputc(key, s_slide_file);
	}
	else
	{
		if (s_quotes) /* not an ASCII character - turn off quotes */
		{
			fprintf(s_slide_file, "\"\n");
			s_quotes = 0;
		}
		char mn[MAX_MNEMONIC];
		get_mnemonic(key, mn);
		if (*mn)
		{
			fprintf(s_slide_file, "%s", mn);
		}
		else if (check_video_mode_key(0, key) >= 0)
		{
			char buf[10];
			video_mode_key_name(key, buf);
			fprintf(s_slide_file, buf);
		}
		else /* not ASCII and not FN key */
		{
			fprintf(s_slide_file, "%4d", key);
		}
		fputc('\n', s_slide_file);
	}
}

/* suspend process # of seconds */
static void sleep_secs(int secs)
{
	long stop;
	stop = clock_ticks() + (long)secs*CLK_TCK;
	while (clock_ticks() < stop && kbhit() == 0)
	{
	} /* bailout if key hit */
}

static void slideshowerr(char *msg)
{
	char msgbuf[300] = { "Slideshow error:\n" };
	stop_slide_show();
	strcat(msgbuf, msg);
	stop_message(0, msgbuf);
}
