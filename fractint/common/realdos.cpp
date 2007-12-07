/*
		Miscellaneous C routines used only in DOS Fractint.
*/
#include <sstream>
#include <string>

#include <string.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "strcpy.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"

/* uncomment following for production version */
/*
#define PRODUCTION
*/
#define MENU_HDG 3
#define MENU_ITEM 1

#define SWAPBLKLEN 4096 /* must be a power of 2 */

BYTE g_suffix[10000];
int g_release = 2099;	/* this has 2 implied decimals; increment it every synch */
int g_patch_level = 9;	/* patchlevel for this version */
int g_video_table_len;                 /* number of entries in above           */
int g_cfg_line_nums[MAXVIDEOMODES] = { 0 };

static BYTE *s_temp_text_save = 0;
static int s_text_x_dots = 0;
static int s_text_y_dots = 0;
static bool s_full_menu = false;
static int menu_check_key(int curkey, int choice);

int stop_message(int flags, const std::string &msg)
{
	return stop_message(flags, msg.c_str());
}

/* int stop_message(flags, message) displays message and waits for a key:
	message should be a max of 9 lines with \n's separating them;
	no leading or trailing \n's in message;
	no line longer than 76 chars for best appearance;
	flag options:
		&1 if already in text display mode, stackscreen is not called
			and message is displayed at (12, 0) instead of (4, 0)
		&2 if continue/cancel indication is to be returned;
			when not set, "Any key to continue..." is displayed
			when set, "Escape to cancel, any other key to continue..."
			-1 is returned for cancel, 0 for continue
		&4 set to suppress buzzer
		&8 for Fractint for Windows & parser - use a fixed pitch font
		&16 for info only message (green box instead of red in DOS vsn)
*/
int stop_message(int flags, const char *msg)
{
	int ret;
	int toprow;
	int color;
	static unsigned char batchmode = 0;
	if (g_debug_mode || g_initialize_batch >= INITBATCH_NORMAL)
	{
		FILE *fp = 0;
		if (g_initialize_batch == INITBATCH_NONE)
		{
			fp = dir_fopen(g_work_dir, "stop_message.txt", "w");
		}
		else
		{
			fp = dir_fopen(g_work_dir, "stop_message.txt", "a");
		}
		if (fp != 0)
		{
			fprintf(fp, "%s\n", msg);
			fclose(fp);
		}
	}
	if (g_command_initialize)  /* & command_files hasn't finished 1st try */
	{
		init_failure(msg);
		goodbye();
	}
	if (g_initialize_batch >= INITBATCH_NORMAL || batchmode)  /* in batch mode */
	{
		g_initialize_batch = INITBATCH_BAILOUT_INTERRUPTED; /* used to set errorlevel */
		batchmode = 1; /* fixes *second* stop_message in batch mode bug */
		return -1;
	}
	ret = 0;
	MouseModeSaver saved_mouse(-FIK_ENTER);
	if ((flags & STOPMSG_NO_STACK))
	{
		blank_rows(toprow = 12, 10, 7);
	}
	else
	{
		driver_stack_screen();
		toprow = 4;
		driver_move_cursor(4, 0);
	}
	g_text_cbase = 2; /* left margin is 2 */
	driver_put_string(toprow, 0, 7, msg);
	if (flags & STOPMSG_CANCEL)
	{
		driver_put_string(g_text_row + 2, 0, 7, "Escape to cancel, any other key to continue...");
	}
	else
	{
		driver_put_string(g_text_row + 2, 0, 7, "Any key to continue...");
	}
	g_text_cbase = 0; /* back to full line */
	color = (flags & STOPMSG_INFO_ONLY) ? C_STOP_INFO : C_STOP_ERR;
	driver_set_attr(toprow, 0, color, (g_text_row + 1-toprow)*80);
	driver_hide_text_cursor();   /* cursor off */
	if ((flags & STOPMSG_NO_BUZZER) == 0)
	{
		driver_buzzer((flags & STOPMSG_INFO_ONLY) ? 0 : 2);
	}
	while (driver_key_pressed()) /* flush any keyahead */
	{
		driver_get_key();
	}
	if (g_debug_mode != DEBUGMODE_NO_HELP_F1_ESC)
	{
		if (getakeynohelp() == FIK_ESC)
		{
			ret = -1;
		}
	}
	if ((flags & STOPMSG_NO_STACK))
	{
		blank_rows(toprow, 10, 7);
	}
	else
	{
		driver_unstack_screen();
	}
	return ret;
}


/*	text_temp_message(msg)

	displays a text message of up to 40 characters, waits
	for a key press, restores the prior display, and returns (without
	eating the key).
*/
int text_temp_message(const std::string &message)
{
	return text_temp_message(message.c_str());
}
int text_temp_message(const char *msgparm)
{
	if (show_temp_message(msgparm))
	{
		return -1;
	}

	driver_wait_key_pressed(0); /* wait for a keystroke but don't eat it */
	clear_temp_message();
	return 0;
}

void free_temp_message()
{
	delete[] s_temp_text_save;
	s_temp_text_save = 0;
}

int show_temp_message(const std::string &message)
{
	return show_temp_message(message.c_str());
}

int show_temp_message(const char *msgparm)
{
	static long size = 0;
	char msg[41];
	BYTE *fontptr = 0;
	int i;
	int xrepeat = 0;
	int yrepeat = 0;
	int save_sxoffs;
	int save_syoffs;

	strncpy(msg, msgparm, 40);
	msg[40] = 0; /* ensure max message len of 40 chars */
	if (driver_diskp())  /* disk video, screen in text mode, easy */
	{
		disk_video_status(0, msg);
		return 0;
	}
	if (g_command_initialize)      /* & command_files hasn't finished 1st try */
	{
		// TODO : don't use printf!
#if 0
		printf("%s\n", msg);
#endif
		return 0;
	}

	xrepeat = (g_screen_width >= 640) ? 2 : 1;
	yrepeat = (g_screen_height >= 300) ? 2 : 1;
	s_text_x_dots = int(strlen(msg))*xrepeat*8;
	s_text_y_dots = yrepeat*8;

	/* worst case needs 10k */
	if (s_temp_text_save != 0)
	{
		if (size != long(s_text_x_dots)*s_text_y_dots)
		{
			free_temp_message();
		}
	}
	size = long(s_text_x_dots)*s_text_y_dots;
	save_sxoffs = g_sx_offset;
	save_syoffs = g_sy_offset;
	g_sx_offset = 0;
	g_sy_offset = 0;
	if (s_temp_text_save == 0) /* only save screen first time called */
	{
		s_temp_text_save = new BYTE[s_text_x_dots*s_text_y_dots];
		if (s_temp_text_save == 0)
		{
			return -1; /* sorry, message not displayed */
		}
		for (i = 0; i < s_text_y_dots; ++i)
		{
			get_line(i, 0, s_text_x_dots-1, &s_temp_text_save[i*s_text_x_dots]);
		}
	}

	find_special_colors(); /* get g_color_dark & g_color_medium set */
	driver_display_string(0, 0, g_color_medium, g_color_dark, msg);
	g_sx_offset = save_sxoffs;
	g_sy_offset = save_syoffs;

	return 0;
}

void clear_temp_message()
{
	int i;
	int save_sxoffs;
	int save_syoffs;
	if (driver_diskp()) /* disk video, easy */
	{
		disk_video_status(0, "");
	}
	else if (s_temp_text_save != 0)
	{
		save_sxoffs = g_sx_offset;
		save_syoffs = g_sy_offset;
		g_sx_offset = 0;
		g_sy_offset = 0;
		for (i = 0; i < s_text_y_dots; ++i)
		{
			put_line(i, 0, s_text_x_dots-1, &s_temp_text_save[i*s_text_x_dots]);
		}
		if (!g_using_jiim)  /* jiim frees memory with free_temp_message() */
		{
			delete[] s_temp_text_save;
			s_temp_text_save = 0;
		}
		g_sx_offset = save_sxoffs;
		g_sy_offset = save_syoffs;
	}
}

void blank_rows(int row, int rows, int attr)
{
	char buf[81];
	memset(buf, ' ', 80);
	buf[80] = 0;
	while (--rows >= 0)
	{
		driver_put_string(row++, 0, attr, buf);
	}
}

void help_title()
{
	driver_set_clear(); /* clear the screen */
	std::ostringstream message;
	message << boost::format("Iterated Dynamics Version %d.%01d") % (g_release/100) % ((g_release % 100)/10);
	if (g_release % 10)
	{
		message << boost::format("%01d") % (g_release % 10);
	}
	if (g_patch_level)
	{
		message << "." << g_patch_level;
	}
	put_string_center(0, 0, 80, C_TITLE, message.str().c_str());

/* uncomment next for production executable: */
#if defined(PRODUCTION) || defined(XFRACT)
	return;
	/*NOTREACHED*/
#else
	if (DEBUGMODE_NO_DEV_HEADING == g_debug_mode)
	{
		return;
	}
#define DEVELOPMENT
#ifdef DEVELOPMENT
	driver_put_string(0, 2, C_TITLE_DEV, "Development Version");
#else
	driver_put_string(0, 3, C_TITLE_DEV, "Customized Version");
#endif
	driver_put_string(0, 55, C_TITLE_DEV, "Not for Public Release");
#endif
}


void footer_msg(int *i, int options, char *speedstring)
{
	put_string_center((*i)++, 0, 80, C_PROMPT_BKGRD,
		(speedstring) ? "Use the cursor keys or type a value to make a selection"
		: "Use the cursor keys to highlight your selection");
	put_string_center(*(i++), 0, 80, C_PROMPT_BKGRD,
			(options & CHOICE_MENU) ? "Press ENTER for highlighted choice, or "FK_F1" for help"
		: ((options & CHOICE_HELP) ? "Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help"
		: "Press ENTER for highlighted choice, or ESCAPE to back out"));
}

int put_string_center(int row, int col, int width, int attr, const std::string &msg)
{
	return put_string_center(row, col, width, attr, msg.c_str());
}

int put_string_center(int row, int col, int width, int attr, const char *msg)
{
	char buf[81];
	int i;
	int j;
	int k;
	i = 0;
#ifdef XFRACT
	if (width >= 80) /* Some systems choke in column 80  */
	{
		width = 79;
	}
#endif
	while (msg[i])
	{
		++i; /* strlen for a */
	}
	if (i == 0)
	{
		return -1;
	}
	if (i >= width) /* sanity check  */
	{
		i = width - 1;
	}
	j = (width - i)/2;
	j -= (width + 10 - i)/20; /* when wide a bit left of center looks better */
	memset(buf, ' ', width);
	buf[width] = 0;
	i = 0;
	k = j;
	while (msg[i])
	{
		buf[k++] = msg[i++]; /* strcpy for a */
	}
	driver_put_string(row, col, attr, buf);
	return j;
}

/* ------------------------------------------------------------------------ */

/* For file list purposes only, it's a directory name if first
	char is a dot or last char is a slash */
static int is_a_dir_name(char *name)
{
	if (*name == '.' || ends_with_slash(name))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void show_speed_string(int speedrow,
					char *speedstring,
					int (*speed_prompt)(int, int, int, char *, int))
{
	int speed_match = 0;
	int i;
	int j;
	char buf[81];
	memset(buf, ' ', 80);
	buf[80] = 0;
	driver_put_string(speedrow, 0, C_PROMPT_BKGRD, buf);
	if (*speedstring)  /* got a speedstring on the go */
	{
		driver_put_string(speedrow, 15, C_CHOICE_SP_INSTR, " ");
		if (speed_prompt)
		{
			j = speed_prompt(speedrow, 16, C_CHOICE_SP_INSTR, speedstring, speed_match);
		}
		else
		{
			driver_put_string(speedrow, 16, C_CHOICE_SP_INSTR, "Speed key string");
			j = sizeof("Speed key string")-1;
		}
		strcpy(buf, speedstring);
		i = int(strlen(buf));
		while (i < 30)
		{
			buf[i++] = ' ';
		}
		buf[i] = 0;
		driver_put_string(speedrow, 16 + j, C_CHOICE_SP_INSTR, " ");
		driver_put_string(speedrow, 17 + j, C_CHOICE_SP_KEYIN, buf);
		driver_move_cursor(speedrow, 17 + j + int(strlen(speedstring)));
	}
	else
	{
		driver_hide_text_cursor();
	}
}

static void process_speed_string(char    *speedstring,
								char **choices,         /* array of choice strings                */
								int       curkey,
								int      *pcurrent,
								int       numchoices,
								int       is_unsorted)
{
	int i;
	int comp_result;

	i = int(strlen(speedstring));
	if (curkey == 8 && i > 0) /* backspace */
		speedstring[--i] = 0;
	if (33 <= curkey && curkey <= 126 && i < 30)
	{
#ifndef XFRACT
		curkey = tolower(curkey);
#endif
		speedstring[i] = (char)curkey;
		speedstring[++i] = 0;
	}
	if (i > 0)   /* locate matching type */
	{
		*pcurrent = 0;
		while (*pcurrent < numchoices
			&& (comp_result = strncasecmp(speedstring, choices[*pcurrent], i)) != 0)
		{
			if (comp_result < 0 && !is_unsorted)
			{
				*pcurrent -= *pcurrent ? 1 : 0;
				break;
			}
			else
			{
				++*pcurrent;
			}
		}
		if (*pcurrent >= numchoices) /* bumped end of list */
		{
			*pcurrent = numchoices - 1;
				/*if the list is unsorted, and the entry found is not the exact
					entry, then go looking for the exact entry.
				*/
		}
		else if (is_unsorted && choices[*pcurrent][i])
		{
			int temp = *pcurrent;
			while (++temp < numchoices)
			{
				if (!choices[temp][i] && !strncasecmp(speedstring, choices[temp], i))
				{
					*pcurrent = temp;
					break;
				}
			}
		}
	}
}

static int prompt_color(int attributes)
{
	switch (attributes & 3)
	{
	case 1:		return C_PROMPT_LO;
	case 3:		return C_PROMPT_HI;
	default:	return C_PROMPT_MED;
	}
}

int initialize_current(int options, int num_choices, char **choices, const char *speed_string)
{
	int i = int(strlen(speed_string));
	int current;
	current = 0;
	if (options & CHOICE_NOT_SORTED)
	{
		int k = strncasecmp(speed_string, choices[current], i);
		while (current < num_choices && k != 0)
		{
			++current;
			k = strncasecmp(speed_string, choices[current], i);
		}
		if (k != 0)
		{
			current = 0;
		}
	}
	else
	{
		int k = strncasecmp(speed_string, choices[current], i);
		while (current < num_choices && k > 0)
		{
			++current;
			k = strncasecmp(speed_string, choices[current], i);
		}
		if (k < 0 && current > 0)  /* oops - overshot */
		{
			--current;
		}
	}
	if (current >= num_choices) /* bumped end of list */
	{
		current = num_choices - 1;
	}
	return current;
}


/*
return is:
	n >= 0 for choice n selected,
	-1 for escape
	k for check_key routine return value k (if not 0 nor -1)
	speedstring[0] != 0 on return if string is present
*/
int full_screen_choice(
					   int options,					/* &2 use menu coloring scheme */
					   /* &4 include F1 for help in instructions */
					   /* &8 add caller's instr after normal set */
					   /* &16 menu items up one line */
					   const char *heading,			/* heading info, \n delimited */
					   const char *heading2,			/* column heading or 0 */
					   const char *instructions,		/* instructions, \n delimited, or 0 */
					   int num_choices,				/* How many choices in list */
					   char **choices,					/* array of choice strings */
					   const int *attributes,			/* &3: 0 normal color, 1, 3 highlight */
					   /* &256 marks a dummy entry */
					   int box_width,					/* box width, 0 for calc (in items) */
					   int box_depth,					/* box depth, 0 for calc, 99 for max */
					   int column_width,				/* data width of a column, 0 for calc */
					   int current,					/* start with this item */
					   void (*format_item)(int, char*), /* routine to display an item or 0 */
					   char *speed_string,				/* returned speed key value, or 0 */
					   int (*speed_prompt)(int, int, int, char *, int), /* routine to display prompt or 0 */
					   int (*check_key)(int, int)		/* routine to check keystroke or 0 */
					   )
{
	int title_lines;
	int title_width;
	int required_rows;
	int top_left_row;
	int top_left_col;
	int top_left_choice;
	int speed_row = 0;  /* speed key prompt */
	int box_items;      /* boxwidth*boxdepth */
	int current_key;
	int increment;
	int rev_increment = 0;
	const char *charptr;
	char buf[81];
	char current_item[81];
	char *item_ptr;
	int scrunch = (options & CHOICE_CRUNCH) ? 1 : 0;		/* scrunch up a line */

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	int ret = -1;
	/* preset current to passed string */
	if (speed_string && speed_string[0])
	{
		current = initialize_current(options, num_choices, choices, speed_string);
	}

	while (true)
	{
		if (current >= num_choices)  /* no real choice in the list? */
		{
			return ret;
		}
		if ((attributes[current] & 256) == 0)
		{
			break;
		}
		++current;                  /* scan for a real choice */
	}

	title_lines = 0;
	title_width = 0;
	if (heading)
	{
		charptr = heading;              /* count title lines, find widest */
		int i = 0;
		title_lines = 1;
		while (*charptr)
		{
			if (*(charptr++) == '\n')
			{
				++title_lines;
				i = -1;
			}
			if (++i > title_width)
			{
				title_width = i;
			}
		}
	}

	if (column_width == 0)             /* find widest column */
	{
		for (int i = 0; i < num_choices; ++i)
		{
			int len = int(strlen(choices[i]));
			if (len > column_width)
			{
				column_width = len;
			}
		}
	}
	/* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
	required_rows = 3 - scrunch;                /* calc rows available */
	if (heading)
	{
		required_rows += title_lines + 1;
	}
	if (instructions)                   /* count instructions lines */
	{
		charptr = instructions;
		++required_rows;
		while (*charptr)
		{
			if (*(charptr++) == '\n')
			{
				++required_rows;
			}
		}
		if ((options & CHOICE_INSTRUCTIONS))          /* show std instr too */
		{
			required_rows += 2;
		}
	}
	else
	{
		required_rows += 2;              /* standard instructions */
	}
	if (speed_string)
	{
		++required_rows;   /* a row for speedkey prompt */
	}
	int i = 25 - required_rows;
	if (box_depth > i) /* limit the depth to max */
	{
		box_depth = i;
	}
	if (box_width == 0)           /* pick box width and depth */
	{
		if (num_choices <= i - 2)  /* single column is 1st choice if we can */
		{
			box_depth = num_choices;
			box_width = 1;
		}
		else
		{                      /* sort-of-wide is 2nd choice */
			box_width = 60/(column_width + 1);
			if (box_width == 0
				|| (box_depth = (num_choices + box_width - 1)/box_width) > i - 2)
			{
				box_width = 80/(column_width + 1); /* last gasp, full width */
				box_depth = (num_choices + box_width - 1)/box_width;
				if (box_depth > i)
				{
					box_depth = i;
				}
			}
		}
	}
	int i2 = (80 / box_width - column_width) / 2 - 1;
	if (i2 == 0) /* to allow wider prompts */
	{
		i2 = 1;
	}
	if (i2 < 0)
	{
		i2 = 0;
	}
	if (i2 > 3)
	{
		i2 = 3;
	}
	int j = box_width*(column_width += i2) + i2;     /* overall width of box */
	if (j < title_width + 2)
	{
		j = title_width + 2;
	}
	if (j > 80)
	{
		j = 80;
	}
	if (j <= 70 && box_width == 2)         /* special case makes menus nicer */
	{
		++j;
		++column_width;
	}
	int k = (80 - j)/2;                       /* center the box */
	k -= (90 - j)/20;
	top_left_col = k + i2;                     /* column of topleft choice */
	int i3 = (25 - required_rows - box_depth) / 2;
	i3 -= i3/4;                             /* higher is better if lots extra */
	top_left_row = 3 + title_lines + i3;        /* row of topleft choice */

	/* now set up the overall display */
	help_title();                            /* clear, display title line */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);      /* init rest to background */
	int i4;
	for (i4 = top_left_row - 1 - title_lines; i4 < top_left_row + box_depth + 1; ++i4)
	{
		driver_set_attr(i4, k, C_PROMPT_LO, j);          /* draw empty box */
	}
	if (heading)
	{
		g_text_cbase = (80 - title_width)/2;   /* set left margin for driver_put_string */
		g_text_cbase -= (90 - title_width)/20; /* put heading into box */
		driver_put_string(top_left_row - title_lines - 1, 0, C_PROMPT_HI, heading);
		g_text_cbase = 0;
	}
	if (heading2)                               /* display 2nd heading */
	{
		driver_put_string(top_left_row - 1, top_left_col, C_PROMPT_MED, heading2);
	}
	int i5 = top_left_row + box_depth + 1;
	if (instructions == 0 || (options & CHOICE_INSTRUCTIONS))   /* display default instructions */
	{
		if (i5 < 20)
		{
			++i5;
		}
		if (speed_string)
		{
			speed_row = i5;
			speed_string[0] = 0;
			if (++i5 < 22)
			{
				++i5;
			}
		}
		i5 -= scrunch;
		footer_msg(&i5, options, speed_string);
	}
	if (instructions)                            /* display caller's instructions */
	{
		charptr = instructions;
		int j2 = -1;
		while ((buf[++j2] = *(charptr++)) != 0)
		{
			if (buf[j2] == '\n')
			{
				buf[j2] = 0;
				put_string_center(i5++, 0, 80, C_PROMPT_BKGRD, buf);
				j2 = -1;
			}
		}
		put_string_center(i5, 0, 80, C_PROMPT_BKGRD, buf);
	}

	box_items = box_width*box_depth;
	top_left_choice = 0;                      /* pick topleft for init display */
	while (current - top_left_choice >= box_items
		|| (current - top_left_choice > box_items/2
		&& top_left_choice + box_items < num_choices))
	{
		top_left_choice += box_width;
	}
	bool redisplay = true;
	top_left_row -= scrunch;
	while (true) /* main loop */
	{
		if (redisplay)                       /* display the current choices */
		{
			memset(buf, ' ', 80);
			buf[box_width*column_width] = 0;
			for (int i = (heading2) ? 0 : -1; i <= box_depth; ++i)  /* blank the box */
			{
				driver_put_string(top_left_row + i, top_left_col, C_PROMPT_LO, buf);
			}
			for (int i = 0; i + top_left_choice < num_choices && i < box_items; ++i)
			{
				/* display the choices */
				int j = i + top_left_choice;
				if (format_item)
				{
					(*format_item)(j, buf);
					charptr = buf;
				}
				else
				{
					charptr = choices[j];
				}
				driver_put_string(top_left_row + i/box_width, top_left_col + (i % box_width)*column_width,
					prompt_color(attributes[j]), charptr);
			}
			/***
			... format differs for summary/detail, whups, force box width to
			...  be 72 when detail toggle available?  (2 grey margin each
			...  side, 1 blue margin each side)
			***/
			if (top_left_choice > 0 && heading2 == 0)
			{
				driver_put_string(top_left_row - 1, top_left_col, C_PROMPT_LO, "(more)");
			}
			if (top_left_choice + box_items < num_choices)
			{
				driver_put_string(top_left_row + box_depth, top_left_col, C_PROMPT_LO, "(more)");
			}
			redisplay = false;
		}

		int i = current - top_left_choice;           /* highlight the current choice */
		if (format_item)
		{
			(*format_item)(current, current_item);
			item_ptr = current_item;
		}
		else
		{
			item_ptr = choices[current];
		}
		driver_put_string(top_left_row + i/box_width, top_left_col + (i % box_width)*column_width,
			C_CHOICE_CURRENT, item_ptr);

		if (speed_string)                     /* show speedstring if any */
		{
			show_speed_string(speed_row, speed_string, speed_prompt);
		}
		else
		{
			driver_hide_text_cursor();
		}

		driver_wait_key_pressed(0); /* enables help */
		current_key = driver_get_key();
#ifdef XFRACT
		if (curkey == FIK_F10)
		{
			curkey = ')';
		}
		if (curkey == FIK_F9)
		{
			curkey = '(';
		}
		if (curkey == FIK_F8)
		{
			curkey = '*';
		}
#endif

		i = current - top_left_choice;           /* unhighlight current choice */
		driver_put_string(top_left_row + i/box_width, top_left_col + (i % box_width)*column_width,
			prompt_color(attributes[current]), item_ptr);

		increment = 0;
		switch (current_key)
		{                      /* deal with input key */
		case FIK_ENTER:
		case FIK_ENTER_2:
			return current;
		case FIK_ESC:
			return ret;
		case FIK_DOWN_ARROW:
			increment = box_width;
			rev_increment = -increment;
			break;
		case FIK_CTL_DOWN_ARROW:
			increment = box_width;
			rev_increment = -increment;
			{
				int newcurrent = current;
				while ((newcurrent += box_width) != current)
				{
					if (newcurrent >= num_choices)
					{
						newcurrent = (newcurrent % box_width) - box_width;
					}
					else if (!is_a_dir_name(choices[newcurrent]))
					{
						if (current != newcurrent)
						{
							current = newcurrent - box_width;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_UP_ARROW:
			rev_increment = box_width;
			increment = -rev_increment;
			break;
		case FIK_CTL_UP_ARROW:
			rev_increment = box_width;
			increment = -rev_increment;
			{
				int newcurrent = current;
				while ((newcurrent -= box_width) != current)
				{
					if (newcurrent < 0)
					{
						newcurrent = (num_choices - current) % box_width;
						newcurrent =  num_choices + (newcurrent ? box_width - newcurrent: 0);
					}
					else if (!is_a_dir_name(choices[newcurrent]))
					{
						if (current != newcurrent)
						{
							current = newcurrent + box_width;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_RIGHT_ARROW:
			increment = 1;
			rev_increment = -1;
			break;
		case FIK_CTL_RIGHT_ARROW:  /* move to next file; if at last file, go to first file */
			increment = 1;
			rev_increment = -1;
			{
				int newcurrent = current;
				while (++newcurrent != current)
				{
					if (newcurrent >= num_choices)
					{
						newcurrent = -1;
					}
					else if (!is_a_dir_name(choices[newcurrent]))
					{
						if (current != newcurrent)
						{
							current = newcurrent - 1;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_LEFT_ARROW:
			increment = -1;
			rev_increment = 1;
			break;
		case FIK_CTL_LEFT_ARROW: /* move to previous file; if at first file, go to last file */
			increment = -1;
			rev_increment = 1;
			{
				int newcurrent = current;
				while (--newcurrent != current)
				{
					if (newcurrent < 0)
					{
						newcurrent = num_choices;
					}
					else if (!is_a_dir_name(choices[newcurrent]))
					{
						if (current != newcurrent)
						{
							current = newcurrent + 1;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_PAGE_UP:
			if (num_choices > box_items)
			{
				top_left_choice -= box_items;
				increment = -box_items;
				rev_increment = box_width;
				redisplay = true;
			}
			break;
		case FIK_PAGE_DOWN:
			if (num_choices > box_items)
			{
				top_left_choice += box_items;
				increment = box_items;
				rev_increment = -box_width;
				redisplay = true;
			}
			break;
		case FIK_HOME:
			current = -1;
			increment = 1;
			rev_increment = 1;
			break;
		case FIK_CTL_HOME:
			current = -1;
			increment = 1;
			rev_increment = 1;
			{
				int newcurrent;
				for (newcurrent = 0; newcurrent < num_choices; ++newcurrent)
				{
					if (!is_a_dir_name(choices[newcurrent]))
					{
						current = newcurrent - 1;
						break;  /* breaks the for loop */
					}
				}
			}
			break;
		case FIK_END:
			current = num_choices;
			increment = -1;
			rev_increment = -1;
			break;
		case FIK_CTL_END:
			current = num_choices;
			increment = -1;
			rev_increment = -1;
			{
				int newcurrent;
				for (newcurrent = num_choices - 1; newcurrent >= 0; --newcurrent)
				{
					if (!is_a_dir_name(choices[newcurrent]))
					{
						current = newcurrent + 1;
						break;  /* breaks the for loop */
					}
				}
			}
			break;
		default:
			if (check_key)
			{
				ret = (*check_key)(current_key, current);
				if (ret < -1 || ret > 0)
				{
					return ret;
				}
				if (ret == -1)
				{
					redisplay = true;
				}
			}
			ret = -1;
			if (speed_string)
			{
				process_speed_string(speed_string, choices, current_key, &current,
					num_choices, options & CHOICE_NOT_SORTED);
			}
			break;
		}
		if (increment)                  /* apply cursor movement */
		{
			current += increment;
			if (speed_string)               /* zap speedstring */
			{
				speed_string[0] = 0;
			}
		}
		while (true)
		{                 /* adjust to a non-comment choice */
			if (current < 0 || current >= num_choices)
			{
				increment = rev_increment;
			}
			else if ((attributes[current] & 256) == 0)
			{
				break;
			}
			current += increment;
		}
		if (top_left_choice > num_choices - box_items)
		{
			top_left_choice = ((num_choices + box_width - 1)/box_width)*box_width - box_items;
		}
		if (top_left_choice < 0)
		{
			top_left_choice = 0;
		}
		while (current < top_left_choice)
		{
			top_left_choice -= box_width;
			redisplay = true;
		}
		while (current >= top_left_choice + box_items)
		{
			top_left_choice += box_width;
			redisplay = true;
		}
	}

	return ret;
}

int full_screen_choice_help(int help_mode, int options, const char *heading,
	const char *heading2, const char *instr, int num_choices, char **choices, const int *attributes,
	int box_width, int box_depth, int column_width, int current,
	void (*format_item)(int, char*), char *speed_string,
	int (*speed_prompt)(int, int, int, char *, int), int (*check_key)(int, int))
{
	int result;
	HelpModeSaver saved_help(help_mode);
	result = full_screen_choice(options, heading, heading2, instr,
		num_choices, choices, attributes, box_width, box_depth, column_width,
		current, format_item, speed_string, speed_prompt, check_key);
	return result;
}

#ifndef XFRACT
/* case independent version of strncmp */
int strncasecmp(const char *s, const char *t, int ct)
{
	for (; (tolower(*s) == tolower(*t)) && --ct; s++, t++)
	{
		if (*s == '\0')
		{
			return 0;
		}
	}
	return tolower(*s) - tolower(*t);
}
#endif

int main_menu(bool full_menu)
{
	const char *choices[44]; /* 2 columns*22 rows */
	int attributes[44];
	int choicekey[44];
	int i;
	int nextleft;
	int nextright;
	int showjuliatoggle;
	bool save_tab_display_enabled = g_tab_display_enabled;

top:
	s_full_menu = full_menu;
	g_tab_display_enabled = false;
	showjuliatoggle = 0;
	for (i = 0; i < 44; ++i)
	{
		attributes[i] = 256;
		choices[i] = "";
		choicekey[i] = -1;
	}
	nextleft = -2;
	nextright = -1;

	if (full_menu)
	{
		nextleft += 2;
		choices[nextleft] = "      CURRENT IMAGE         ";
		attributes[nextleft] = 256 + MENU_HDG;

		nextleft += 2;
		choicekey[nextleft] = 13; /* enter */
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = (g_calculation_status == CALCSTAT_RESUMABLE) ?
			"continue calculation        " :
			"return to image             ";

		nextleft += 2;
		choicekey[nextleft] = 9; /* tab */
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "info about image      <tab> ";

		nextleft += 2;
		choicekey[nextleft] = 'o';
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "orbits window          <o>  ";
		if (!fractal_type_julia_or_inverse(g_fractal_type))
		{
			nextleft += 2;
		}
	}

	nextleft += 2;
	choices[nextleft] = "      NEW IMAGE             ";
	attributes[nextleft] = 256 + MENU_HDG;

	nextleft += 2;
	choicekey[nextleft] = FIK_DELETE;
	attributes[nextleft] = MENU_ITEM;
#ifdef XFRACT
	choices[nextleft] = "draw fractal           <D>  ";
#else
	choices[nextleft] = "select video mode...  <del> ";
#endif

	nextleft += 2;
	choicekey[nextleft] = 't';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "select fractal type    <t>  ";

	if (full_menu)
	{
		if ((!fractal_type_none(g_current_fractal_specific->tojulia)
			&& g_parameters[0] == 0.0 && g_parameters[1] == 0.0)
			|| !fractal_type_none(g_current_fractal_specific->tomandel))
		{
			nextleft += 2;
			choicekey[nextleft] = FIK_SPACE;
			attributes[nextleft] = MENU_ITEM;
			choices[nextleft] = "toggle to/from julia <space>";
			showjuliatoggle = 1;
		}
		if (fractal_type_julia_or_inverse(g_fractal_type))
		{
			nextleft += 2;
			choicekey[nextleft] = 'j';
			attributes[nextleft] = MENU_ITEM;
			choices[nextleft] = "toggle to/from inverse <j>  ";
			showjuliatoggle = 1;
		}

		nextleft += 2;
		choicekey[nextleft] = 'h';
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "return to prior image  <h>   ";

		nextleft += 2;
		choicekey[nextleft] = FIK_BACKSPACE;
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "reverse thru history <ctl-h> ";
	}
	else
	{
		nextleft += 2;
	}

	nextleft += 2;
	choices[nextleft] = "      OPTIONS                ";
	attributes[nextleft] = 256 + MENU_HDG;

	nextleft += 2;
	choicekey[nextleft] = 'x';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "basic options...       <x>  ";

	nextleft += 2;
	choicekey[nextleft] = 'y';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "extended options...    <y>  ";

	nextleft += 2;
	choicekey[nextleft] = 'z';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "type-specific parms... <z>  ";

	nextleft += 2;
	choicekey[nextleft] = 'p';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "passes options...      <p>  ";

	nextleft += 2;
	choicekey[nextleft] = 'v';
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "view window options... <v>  ";

	if (showjuliatoggle == 0)
	{
		nextleft += 2;
		choicekey[nextleft] = 'i';
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "fractal 3D parms...    <i>  ";
	}

	nextleft += 2;
	choicekey[nextleft] = FIK_CTL_B;
	attributes[nextleft] = MENU_ITEM;
	choices[nextleft] = "browse parms...      <ctl-b>";

	if (full_menu)
	{
		nextleft += 2;
		choicekey[nextleft] = FIK_CTL_E;
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "evolver parms...     <ctl-e>";

		// TODO: sound support for unix/X11
#if !defined(XFRACT)
		nextleft += 2;
		choicekey[nextleft] = FIK_CTL_F;
		attributes[nextleft] = MENU_ITEM;
		choices[nextleft] = "sound parms...       <ctl-f>";
#endif
	}

	nextright += 2;
	attributes[nextright] = 256 + MENU_HDG;
	choices[nextright] = "        FILE                  ";

	nextright += 2;
	choicekey[nextright] = '@';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "run saved command set... <@>  ";

	if (full_menu)
	{
		nextright += 2;
		choicekey[nextright] = 's';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "save image to file       <s>  ";
	}

	nextright += 2;
	choicekey[nextright] = 'r';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "load image from file...  <r>  ";

	nextright += 2;
	choicekey[nextright] = '3';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "3d transform from file...<3>  ";

	if (full_menu)
	{
		nextright += 2;
		choicekey[nextright] = '#';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "3d overlay from file.....<#>  ";

		nextright += 2;
		choicekey[nextright] = 'b';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "save current parameters..<b>  ";

		nextright += 2;
		choicekey[nextright] = 16;
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "print image          <ctl-p>  ";
	}

	nextright += 2;
	choicekey[nextright] = 'd';
	attributes[nextright] = MENU_ITEM;
#ifdef XFRACT
	choices[nextright] = "shell to Linux/Unix      <d>  ";
#else
	choices[nextright] = "shell to dos             <d>  ";
#endif

	nextright += 2;
	choicekey[nextright] = 'g';
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "give command string      <g>  ";

	nextright += 2;
	choicekey[nextright] = FIK_ESC;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "quit Iterated Dynamics  <esc> ";

	nextright += 2;
	choicekey[nextright] = FIK_INSERT;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "restart Iterated Dynamics<ins>";

#ifdef XFRACT
	if (full_menu && (g_got_real_dac || g_fake_lut))
#else
	if (full_menu && g_got_real_dac)
#endif
	{
		nextright += 2;
		choices[nextright] = "       COLORS                 ";
		attributes[nextright] = 256 + MENU_HDG;

		nextright += 2;
		choicekey[nextright] = 'c';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "color cycling mode       <c>  ";

		nextright += 2;
		choicekey[nextright] = '+';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "rotate palette      <+>, <->  ";

		nextright += 2;
		choicekey[nextright] = 'e';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "palette editing mode     <e>  ";

		nextright += 2;
		choicekey[nextright] = 'a';
		attributes[nextright] = MENU_ITEM;
		choices[nextright] = "make starfield           <a>  ";
	}

	nextright += 2;
	choicekey[nextright] = FIK_CTL_A;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "ant automaton          <ctl-a>";

	nextright += 2;
	choicekey[nextright] = FIK_CTL_S;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "stereogram             <ctl-s>";

	i = driver_key_pressed() ? driver_get_key() : 0;
	if (menu_check_key(i, 0) == 0)
	{
		nextleft += 2;
		if (nextleft < nextright)
		{
			nextleft = nextright + 1;
		}
		set_help_mode(HELPMAIN);
		i = full_screen_choice(CHOICE_MENU | CHOICE_CRUNCH,
			"MAIN MENU",
			0, 0, nextleft, (char **) choices, attributes,
			2, nextleft/2, 29, 0, 0, 0, 0, menu_check_key);
		if (i == -1)     /* escape */
		{
			i = FIK_ESC;
		}
		else if (i < 0)
		{
			i = -i;
		}
		else                      /* user selected a choice */
		{
			i = choicekey[i];
			if (-10 == i)
			{
				set_help_mode(HELPZOOM);
				help(ACTION_CALL);
				i = 0;
			}
		}
	}
	if (i == FIK_ESC)             /* escape from menu exits Fractint */
	{
		help_title();
		driver_set_attr(1, 0, C_GENERAL_MED, 24*80);
		for (i = 9; i <= 11; ++i)
		{
			driver_set_attr(i, 18, C_GENERAL_INPUT, 40);
		}
		put_string_center(10, 18, 40, C_GENERAL_INPUT,
#ifdef XFRACT
			"Exit from Xfractint (y/n)? y"
#else
			"Exit from Fractint (y/n)? y"
#endif
);
		driver_hide_text_cursor();
		while ((i = driver_get_key()) != 'y' && i != 'Y' && i != 13)
		{
			if (i == 'n' || i == 'N')
			{
				goto top;
			}
		}
		goodbye();
	}
	if (i == FIK_TAB)
	{
		tab_display();
		i = 0;
	}
	if (i == FIK_ENTER || i == FIK_ENTER_2)
	{
		i = 0;                 /* don't trigger new calc */
	}
	g_tab_display_enabled = save_tab_display_enabled;
	return i;
}

static int menu_check_key(int curkey, int choice)
{ /* choice is dummy used by other routines called by full_screen_choice() */
	int testkey;
	testkey = choice; /* for warning only */
	testkey = (curkey >= 'A' && curkey <= 'Z') ? curkey + ('a'-'A') : curkey;
#ifdef XFRACT
	/* We use F2 for shift-@, annoyingly enough */
	if (testkey == FIK_F2)
	{
		return -testkey;
	}
#endif
	if (testkey == '2')
	{
		testkey = '@';
	}
	if (strchr("#@2txyzgvir3dj", testkey) || testkey == FIK_INSERT || testkey == FIK_CTL_B
			|| testkey == FIK_ESC || testkey == FIK_DELETE || testkey == FIK_CTL_F)
	{
		return -testkey;
	}
	if (s_full_menu)
	{
		if (strchr("\\sobpkrh", testkey) || testkey == FIK_TAB
		|| testkey == FIK_CTL_A || testkey == FIK_CTL_E || testkey == FIK_BACKSPACE
		|| testkey == FIK_CTL_S || testkey == FIK_CTL_U) /* ctrl-A, E, H, P, S, U */
			return -testkey;
		if (testkey == ' ')
		{
			if ((!fractal_type_none(g_current_fractal_specific->tojulia)
					&& g_parameters[0] == 0.0
					&& g_parameters[1] == 0.0)
				|| !fractal_type_none(g_current_fractal_specific->tomandel))
			{
				return -testkey;
			}
		}
		if (g_got_real_dac)
		{
			if (strchr("c+-", testkey))
			{
				return -testkey;
			}
			if (testkey == 'a' || (testkey == 'e'))
			{
				return -testkey;
			}
		}
		/* Alt-A and Alt-S */
		if (testkey == FIK_ALT_A || testkey == FIK_ALT_S )
		{
			return -testkey;
		}
	}
	if (check_video_mode_key(0, testkey) >= 0)
	{
		return -testkey;
	}
	return 0;
}

int input_field(
		int options,          /* &1 numeric, &2 integer, &4 double */
		int attr,             /* display attribute */
		char *fld,            /* the field itself */
		int len,              /* field length (declare as 1 larger for \0) */
		int row,              /* display row */
		int col,              /* display column */
		int (*check_key)(int key)  /* routine to check non data keys, or 0 */
		)
{
	char savefld[81];
	char buf[81];
	int offset;
	int curkey;
	int display;
	int i;
	int j;
	int ret;

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	ret = -1;
	strcpy(savefld, fld);
	int insert = 0;
	bool started = false;
	offset = 0;
	display = 1;
	while (true)
	{
		strcpy(buf, fld);
		i = int(strlen(buf));
		while (i < len)
		{
			buf[i++] = ' ';
		}
		buf[len] = 0;
		if (display)  /* display current value */
		{
			driver_put_string(row, col, attr, buf);
			display = 0;
		}
		curkey = driver_key_cursor(row + insert, col + offset);  /* get a keystroke */
		if (curkey == 1047) /* numeric slash  */
		{
			curkey = 47;
		}
		switch (curkey)
		{
		case FIK_ENTER:
		case FIK_ENTER_2:
			ret = 0;
			goto inpfld_end;
		case FIK_ESC:
			goto inpfld_end;
		case FIK_RIGHT_ARROW:
			if (offset < len-1)
			{
				++offset;
			}
			started = true;
			break;
		case FIK_LEFT_ARROW:
			if (offset > 0)
			{
				--offset;
			}
			started = true;
			break;
		case FIK_HOME:
			offset = 0;
			started = true;
			break;
		case FIK_END:
			offset = int(strlen(fld));
			started = true;
			break;
		case FIK_BACKSPACE:
		case 127:                              /* backspace */
#if defined(_WIN32)
			_ASSERTE(127 != curkey);
#endif
			if (offset > 0)
			{
				j = int(strlen(fld));
				for (i = offset-1; i < j; ++i)
				{
					fld[i] = fld[i + 1];
				}
				--offset;
			}
			started = true;
			display = 1;
			break;
		case FIK_DELETE:                           /* delete */
			j = int(strlen(fld));
			for (i = offset; i < j; ++i)
			{
				fld[i] = fld[i + 1];
			}
			started = true;
			display = 1;
			break;
		case FIK_INSERT:                           /* insert */
			insert ^= 0x8000;
			started = true;
			break;
		case FIK_F5:
			strcpy(fld, savefld);
			insert = 0;
			started = false;
			offset = 0;
			display = 1;
			break;
		default:
			if (nonalpha(curkey))
			{
				if (check_key)
				{
					ret = (*check_key)(curkey);
					if (ret != 0)
					{
						goto inpfld_end;
					}
				}
				break;                                /* non alphanum char */
			}
			if (offset >= len)                /* at end of field */
			{
				break;
			}
			if (insert && started && strlen(fld) >= (size_t)len)
			{
				break;                                /* insert & full */
			}
			if ((options & INPUTFIELD_NUMERIC)
					&& (curkey < '0' || curkey > '9')
					&& curkey != '+' && curkey != '-')
			{
				if (options & INPUTFIELD_INTEGER)
				{
					break;
				}
				/* allow scientific notation, and specials "e" and "p" */
				if (((curkey != 'e' && curkey != 'E') || offset >= 18)
						&& ((curkey != 'p' && curkey != 'P') || offset != 0 )
						&& curkey != '.')
				{
					break;
				}
			}
			if (!started) /* first char is data, zap field */
			{
				fld[0] = 0;
			}
			if (insert)
			{
				j = int(strlen(fld));
				while (j >= offset)
				{
					fld[j + 1] = fld[j];
					--j;
				}
			}
			if ((size_t)offset >= strlen(fld))
			{
				fld[offset + 1] = 0;
			}
			fld[offset++] = (char)curkey;
			/* if "e" or "p" in first col make number e or pi */
			if ((options & (INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER)) == INPUTFIELD_NUMERIC)  /* floating point */
			{
				double tmpd;
				bool specialv = false;
				if (fld[0] == 'e' || fld[0] == 'E')
				{
					tmpd = exp(1.0);
					specialv = true;
				}
				if (fld[0] == 'p' || fld[0] == 'P')
				{
					tmpd = atan(1.0)*4;
					specialv = true;
				}
				if (specialv)
				{
					if ((options & INPUTFIELD_DOUBLE) == 0)
					{
						round_float_d(&tmpd);
					}
					strcpy(fld, boost::format("%.15g") % tmpd);
					offset = 0;
				}
			}
			started = true;
			display = 1;
		}
	}

inpfld_end:
	return ret;
}

int field_prompt(
		char *hdg,      /* heading, \n delimited lines */
		char *instr,    /* additional instructions or 0 */
		char *fld,          /* the field itself */
		int len,            /* field length (declare as 1 larger for \0) */
		int (*check_key)(int key)   /* routine to check non data keys, or 0 */
		)
{
	char *charptr;
	int boxwidth;
	int titlelines;
	int titlecol;
	int titlerow;
	int promptcol;
	int i;
	int j;
	char buf[81];
	help_title();                           /* clear screen, display title */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);     /* init rest to background */
	charptr = hdg;                         /* count title lines, find widest */
	i = 0;
	boxwidth = 0;
	titlelines = 1;
	while (*charptr)
	{
		if (*(charptr++) == '\n')
		{
			++titlelines;
			i = -1;
		}
		if (++i > boxwidth)
		{
			boxwidth = i;
		}
	}
	if (len > boxwidth)
	{
		boxwidth = len;
	}
	i = titlelines + 4;                    /* total rows in box */
	titlerow = (25 - i)/2;               /* top row of it all when centered */
	titlerow -= titlerow/4;              /* higher is better if lots extra */
	titlecol = (80 - boxwidth)/2;        /* center the box */
	titlecol -= (90 - boxwidth)/20;
	promptcol = titlecol - (boxwidth-len)/2;
	j = titlecol;                          /* add margin at each side of box */
	i = (82-boxwidth)/4;
	if (i > 3)
	{
		i = 3;
	}
	j -= i;
	boxwidth += i*2;
	for (i = -1; i < titlelines + 3; ++i)    /* draw empty box */
	{
		driver_set_attr(titlerow + i, j, C_PROMPT_LO, boxwidth);
	}
	g_text_cbase = titlecol;                  /* set left margin for driver_put_string */
	driver_put_string(titlerow, 0, C_PROMPT_HI, hdg); /* display heading */
	g_text_cbase = 0;
	i = titlerow + titlelines + 4;
	if (instr)  /* display caller's instructions */
	{
		charptr = instr;
		j = -1;
		while ((buf[++j] = *(charptr++)) != 0)
		{
			if (buf[j] == '\n')
			{
				buf[j] = 0;
				put_string_center(i++, 0, 80, C_PROMPT_BKGRD, buf);
				j = -1;
			}
		}
		put_string_center(i, 0, 80, C_PROMPT_BKGRD, buf);
	}
	else                                   /* default instructions */
	{
		put_string_center(i, 0, 80, C_PROMPT_BKGRD, "Press ENTER when finished (or ESCAPE to back out)");
	}
	return input_field(0, C_PROMPT_INPUT, fld, len,
				titlerow + titlelines + 1, promptcol, check_key);
}


/* thinking(1, message):
		if thinking message not yet on display, it is displayed;
		otherwise the wheel is updated
		returns 0 to keep going, -1 if keystroke pending
	thinking(0, 0):
		call this when thinking phase is done
	*/

int thinking(int options, char *msg)
{
	static int thinkstate = -1;
	char *wheel[] = {"-", "\\", "|", "/"};
	static int thinkcol;
	static int count = 0;
	char buf[81];
	if (options == 0)
	{
		if (thinkstate >= 0)
		{
			thinkstate = -1;
			driver_unstack_screen();
		}
		return 0;
	}
	if (thinkstate < 0)
	{
		driver_stack_screen();
		thinkstate = 0;
		help_title();
		strcpy(buf, "  ");
		strcat(buf, msg);
		strcat(buf, "    ");
		driver_put_string(4, 10, C_GENERAL_HI, buf);
		thinkcol = g_text_col - 3;
		count = 0;
	}
	if ((count++) < 100)
	{
		return 0;
	}
	count = 0;
	driver_put_string(4, thinkcol, C_GENERAL_HI, wheel[thinkstate]);
	driver_hide_text_cursor(); /* turn off cursor */
	thinkstate = (thinkstate + 1) & 3;
	return driver_key_pressed();
}

int show_vid_length()
{
	return (sizeof(VIDEOINFO) + sizeof(int))*MAXVIDEOMODES;
}

int check_video_mode_key(int option, int k)
{
	int i;
	/* returns g_video_table entry number if the passed keystroke is a  */
	/* function key currently assigned to a video mode, -1 otherwise */
	if (k == 1400)              /* special value from select_vid_mode  */
	{
		return MAXVIDEOMODES-1; /* for last entry with no key assigned */
	}
	if (k != 0)
	{
		if (option == 0)  /* check resident video mode table */
		{
			for (i = 0; i < MAXVIDEOMODES; ++i)
			{
				if (g_video_table[i].keynum == k)
				{
					return i;
				}
			}
		}
		else  /* check full g_video_table */
		{
			for (i = 0; i < g_video_table_len; ++i)
			{
				if (g_video_table[i].keynum == k)
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int check_vidmode_keyname(char *kname)
{
	/* returns key number for the passed keyname, 0 if not a keyname */
	int i;
	int keyset;
	keyset = 1058;
	if (*kname == 'S' || *kname == 's')
	{
		keyset = FIK_SF1 - 1;
		++kname;
	}
	else if (*kname == 'C' || *kname == 'c')
	{
		keyset = FIK_CTL_F1 - 1;
		++kname;
	}
	else if (*kname == 'A' || *kname == 'a')
	{
		keyset = FIK_ALT_F1 - 1;
		++kname;
	}
	if (*kname != 'F' && *kname != 'f')
	{
		return 0;
	}
	if (*++kname < '1' || *kname > '9')
	{
		return 0;
	}
	i = *kname - '0';
	if (*++kname != 0 && *kname != ' ')
	{
		if (*kname != '0' || i != 1)
		{
			return 0;
		}
		i = 10;
		++kname;
	}
	while (*kname)
	{
		if (*(kname++) != ' ')
		{
			return 0;
		}
	}
	i += keyset;
	if (i < 2)
	{
		i = 0;
	}
	return i;
}

static std::string video_mode_key_name(int key, int base, const char *prefix)
{
	return str(boost::format("%s%d") % prefix % (key - base + 1));
}

std::string video_mode_key_name(int k)
{
	if (k >= FIK_ALT_F1 && k <= FIK_ALT_F10)
	{
		return video_mode_key_name(k, FIK_ALT_F1, "AF");
	}
	if (k >= FIK_CTL_F1 && k <= FIK_CTL_F10)
	{
		return video_mode_key_name(k, FIK_CTL_F1, "CF");
	}
	if (k >= FIK_SF1 && k <= FIK_SF10)
	{
		return video_mode_key_name(k, FIK_SF1, "SF");
	}
	if (k >= FIK_F1 && k <= FIK_F10)
	{
		return video_mode_key_name(k, FIK_F1, "F");
	}
	return "";
}

/* set buffer to name of passed key number */
void video_mode_key_name(int k, char *buf)
{
	strcpy(buf, video_mode_key_name(k).c_str());
}
