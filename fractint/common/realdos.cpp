/*
		Miscellaneous C routines used only in DOS Fractint.
*/
#include <assert.h>
#include <string.h>
#ifndef XFRACT
#include <io.h>
#include <process.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"
#include "drivers.h"
#include "fihelp.h"


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
static int s_full_menu = 0;
static int menu_check_key(int curkey, int choice);

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
	int ret, toprow, color;
	static unsigned char batchmode = 0;
	if (g_debug_flag || g_initialize_batch >= INITBATCH_NORMAL)
	{
		FILE *fp = NULL;
		if (g_initialize_batch == INITBATCH_NONE)
		{
			fp = dir_fopen(g_work_dir, "stop_message.txt", "w");
		}
		else
		{
			fp = dir_fopen(g_work_dir, "stop_message.txt", "a");
		}
		if (fp != NULL)
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
	if (g_debug_flag != DEBUGFLAG_NO_HELP_F1_ESC)
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


/* text_temp_message(msg) displays a text message of up to 40 characters, waits
		for a key press, restores the prior display, and returns (without
		eating the key).
		It works in almost any video mode - does nothing in some very odd cases
		(HCGA hi-res with old bios), or when there isn't 10k of temp mem free. */
int text_temp_message(char *msgparm)
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
	if (s_temp_text_save != NULL)
	{
		free(s_temp_text_save);
		s_temp_text_save = NULL;
	}
}

int show_temp_message(char *msgparm)
{
	static long size = 0;
	char msg[41];
	BYTE *fontptr = NULL;
	int i;
	int xrepeat = 0;
	int yrepeat = 0;
	int save_sxoffs, save_syoffs;

	strncpy(msg, msgparm, 40);
	msg[40] = 0; /* ensure max message len of 40 chars */
	if (driver_diskp())  /* disk video, screen in text mode, easy */
	{
		disk_video_status(0, msg);
		return 0;
	}
	if (g_command_initialize)      /* & command_files hasn't finished 1st try */
	{
		printf("%s\n", msg);
		return 0;
	}

	xrepeat = (g_screen_width >= 640) ? 2 : 1;
	yrepeat = (g_screen_height >= 300) ? 2 : 1;
	s_text_x_dots = (int) strlen(msg)*xrepeat*8;
	s_text_y_dots = yrepeat*8;

	/* worst case needs 10k */
	if (s_temp_text_save != NULL)
	{
		if (size != (long) s_text_x_dots*s_text_y_dots)
		{
			free_temp_message();
		}
	}
	size = (long) s_text_x_dots*s_text_y_dots;
	save_sxoffs = g_sx_offset;
	save_syoffs = g_sy_offset;
	g_sx_offset = g_sy_offset = 0;
	if (s_temp_text_save == NULL) /* only save screen first time called */
	{
		s_temp_text_save = (BYTE *) malloc(s_text_x_dots*s_text_y_dots);
		if (s_temp_text_save == NULL)
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
	int save_sxoffs, save_syoffs;
	if (driver_diskp()) /* disk video, easy */
	{
		disk_video_status(0, "");
	}
	else if (s_temp_text_save != NULL)
	{
		save_sxoffs = g_sx_offset;
		save_syoffs = g_sy_offset;
		g_sx_offset = g_sy_offset = 0;
		for (i = 0; i < s_text_y_dots; ++i)
		{
			put_line(i, 0, s_text_x_dots-1, &s_temp_text_save[i*s_text_x_dots]);
		}
		if (g_using_jiim == 0)  /* jiim frees memory with free_temp_message() */
		{
			free(s_temp_text_save);
			s_temp_text_save = NULL;
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
	char msg[MESSAGE_LEN], buf[MESSAGE_LEN];
	driver_set_clear(); /* clear the screen */
#ifdef XFRACT
	strcpy(msg,"X");
#else
	*msg = 0;
#endif
	sprintf(buf, "FRACTINT Version %d.%01d", g_release/100, (g_release%100)/10);
	strcat(msg, buf);
	if (g_release % 10)
	{
		sprintf(buf, "%01d", g_release % 10);
		strcat(msg, buf);
	}
	if (g_patch_level)
	{
		sprintf(buf, ".%d", g_patch_level);
		strcat(msg, buf);
	}
	put_string_center(0, 0, 80, C_TITLE, msg);

/* uncomment next for production executable: */
#if defined(PRODUCTION) || defined(XFRACT)
	return;
	/*NOTREACHED*/
#else
	if (DEBUGFLAG_NO_DEV_HEADING == g_debug_flag)
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

int put_string_center(int row, int col, int width, int attr, const char *msg)
{
	char buf[81];
	int i, j, k;
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
	j = (width - i) / 2;
	j -= (width + 10 - i) / 20; /* when wide a bit left of center looks better */
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
					int (*speedprompt)(int, int, int, char *, int))
{
	int speed_match = 0;
	int i, j;
	char buf[81];
	memset(buf, ' ', 80);
	buf[80] = 0;
	driver_put_string(speedrow, 0, C_PROMPT_BKGRD, buf);
	if (*speedstring)  /* got a speedstring on the go */
	{
		driver_put_string(speedrow, 15, C_CHOICE_SP_INSTR, " ");
		if (speedprompt)
		{
			j = speedprompt(speedrow, 16, C_CHOICE_SP_INSTR, speedstring, speed_match);
		}
		else
		{
			driver_put_string(speedrow, 16, C_CHOICE_SP_INSTR, "Speed key string");
			j = sizeof("Speed key string")-1;
		}
		strcpy(buf, speedstring);
		i = (int) strlen(buf);
		while (i < 30)
		{
			buf[i++] = ' ';
		}
		buf[i] = 0;
		driver_put_string(speedrow, 16 + j, C_CHOICE_SP_INSTR, " ");
		driver_put_string(speedrow, 17 + j, C_CHOICE_SP_KEYIN, buf);
		driver_move_cursor(speedrow, 17 + j + (int) strlen(speedstring));
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
	int i, comp_result;

	i = (int) strlen(speedstring);
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


int full_screen_choice(
	int options,					/* &2 use menu coloring scheme            */
									/* &4 include F1 for help in instructions */
									/* &8 add caller's instr after normal set */
									/* &16 menu items up one line             */
	const char *hdg,				/* heading info, \n delimited             */
	char *hdg2,						/* column heading or NULL                 */
	char *instr,					/* instructions, \n delimited, or NULL    */
	int numchoices,					/* How many choices in list               */
	char **choices,					/* array of choice strings                */
	int *attributes,				/* &3: 0 normal color, 1, 3 highlight      */
									/* &256 marks a dummy entry               */
	int boxwidth,					/* box width, 0 for calc (in items)       */
	int boxdepth,					/* box depth, 0 for calc, 99 for max      */
	int colwidth,					/* data width of a column, 0 for calc     */
	int current,					/* start with this item                   */
	void (*formatitem)(int, char*), /* routine to display an item or NULL     */
	char *speedstring,				/* returned speed key value, or NULL      */
	int (*speedprompt)(int, int, int, char *, int), /* routine to display prompt or NULL      */
	int (*checkkey)(int, int)		/* routine to check keystroke or NULL     */
)
	/* return is: n >= 0 for choice n selected,
						-1 for escape
						k for checkkey routine return value k (if not 0 nor -1)
						speedstring[0] != 0 on return if string is present
	*/
{
	int titlelines, titlewidth;
	int reqdrows;
	int topleftrow, topleftcol;
	int topleftchoice;
	int speedrow = 0;  /* speed key prompt */
	int boxitems;      /* boxwidth*boxdepth */
	int curkey, increment, rev_increment = 0;
	int redisplay;
	int i, j, k = 0;
	const char *charptr;
	char buf[81];
	char curitem[81];
	char *itemptr;
	int ret;
	int scrunch;  /* scrunch up a line */

	scrunch = (options & CHOICE_CRUNCH) ? 1 : 0;
	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	ret = -1;
	/* preset current to passed string */
	if (speedstring && *speedstring)
	{
		i = (int) strlen(speedstring);
		current = 0;
		if (options & CHOICE_NOT_SORTED)
		{
			k = strncasecmp(speedstring, choices[current], i);
			while (current < numchoices && k != 0)
			{
				++current;
				k = strncasecmp(speedstring, choices[current], i);
			}
			if (k != 0)
			{
				current = 0;
			}
		}
		else
		{
			k = strncasecmp(speedstring, choices[current], i);
			while (current < numchoices && k > 0)
			{
				++current;
				k = strncasecmp(speedstring, choices[current], i);
			}
			if (k < 0 && current > 0)  /* oops - overshot */
			{
				--current;
			}
		}
		if (current >= numchoices) /* bumped end of list */
		{
			current = numchoices - 1;
		}
	}

	while (1)
	{
		if (current >= numchoices)  /* no real choice in the list? */
		{
			goto fs_choice_end;
		}
		if ((attributes[current] & 256) == 0)
		{
			break;
		}
		++current;                  /* scan for a real choice */
	}

	titlelines = titlewidth = 0;
	if (hdg)
	{
		charptr = hdg;              /* count title lines, find widest */
		i = 0;
		titlelines = 1;
		while (*charptr)
		{
			if (*(charptr++) == '\n')
			{
				++titlelines;
				i = -1;
			}
			if (++i > titlewidth)
			{
				titlewidth = i;
			}
		}
	}

	if (colwidth == 0)             /* find widest column */
	{
		for (i = 0; i < numchoices; ++i)
		{
			int len = (int) strlen(choices[i]);
			if (len > colwidth)
			{
				colwidth = len;
			}
		}
	}
	/* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
	reqdrows = 3 - scrunch;                /* calc rows available */
	if (hdg)
	{
		reqdrows += titlelines + 1;
	}
	if (instr)                   /* count instructions lines */
	{
		charptr = instr;
		++reqdrows;
		while (*charptr)
		{
			if (*(charptr++) == '\n')
			{
				++reqdrows;
			}
		}
		if ((options & CHOICE_INSTRUCTIONS))          /* show std instr too */
		{
			reqdrows += 2;
		}
	}
	else
	{
		reqdrows += 2;              /* standard instructions */
	}
	if (speedstring)
	{
		++reqdrows;   /* a row for speedkey prompt */
	}
	i = 25 - reqdrows;
	if (boxdepth > i) /* limit the depth to max */
	{
		boxdepth = i;
	}
	if (boxwidth == 0)           /* pick box width and depth */
	{
		if (numchoices <= i - 2)  /* single column is 1st choice if we can */
		{
			boxdepth = numchoices;
			boxwidth = 1;
		}
		else
		{                      /* sort-of-wide is 2nd choice */
			boxwidth = 60 / (colwidth + 1);
			if (boxwidth == 0
			|| (boxdepth = (numchoices + boxwidth - 1)/boxwidth) > i - 2)
			{
				boxwidth = 80 / (colwidth + 1); /* last gasp, full width */
				boxdepth = (numchoices + boxwidth - 1)/boxwidth;
				if (boxdepth > i)
				{
					boxdepth = i;
				}
			}
		}
	}
	i = (80 / boxwidth - colwidth) / 2 - 1;
	if (i == 0) /* to allow wider prompts */
	{
		i = 1;
	}
	if (i < 0)
	{
		i = 0;
	}
	if (i > 3)
	{
		i = 3;
	}
	j = boxwidth*(colwidth += i) + i;     /* overall width of box */
	if (j < titlewidth + 2)
	{
		j = titlewidth + 2;
	}
	if (j > 80)
	{
		j = 80;
	}
	if (j <= 70 && boxwidth == 2)         /* special case makes menus nicer */
	{
		++j;
		++colwidth;
	}
	k = (80 - j) / 2;                       /* center the box */
	k -= (90 - j) / 20;
	topleftcol = k + i;                     /* column of topleft choice */
	i = (25 - reqdrows - boxdepth) / 2;
	i -= i / 4;                             /* higher is better if lots extra */
	topleftrow = 3 + titlelines + i;        /* row of topleft choice */

	/* now set up the overall display */
	help_title();                            /* clear, display title line */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);      /* init rest to background */
	for (i = topleftrow - 1 - titlelines; i < topleftrow + boxdepth + 1; ++i)
	{
		driver_set_attr(i, k, C_PROMPT_LO, j);          /* draw empty box */
	}
	if (hdg)
	{
		g_text_cbase = (80 - titlewidth) / 2;   /* set left margin for driver_put_string */
		g_text_cbase -= (90 - titlewidth) / 20; /* put heading into box */
		driver_put_string(topleftrow - titlelines - 1, 0, C_PROMPT_HI, hdg);
		g_text_cbase = 0;
	}
	if (hdg2)                               /* display 2nd heading */
	{
		driver_put_string(topleftrow - 1, topleftcol, C_PROMPT_MED, hdg2);
	}
	i = topleftrow + boxdepth + 1;
	if (instr == NULL || (options & CHOICE_INSTRUCTIONS))   /* display default instructions */
	{
		if (i < 20)
		{
			++i;
		}
		if (speedstring)
		{
			speedrow = i;
			*speedstring = 0;
			if (++i < 22)
			{
				++i;
			}
		}
		i -= scrunch;
		footer_msg(&i, options, speedstring);
	}
	if (instr)                            /* display caller's instructions */
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

	boxitems = boxwidth*boxdepth;
	topleftchoice = 0;                      /* pick topleft for init display */
	while (current - topleftchoice >= boxitems
		|| (current - topleftchoice > boxitems/2
			&& topleftchoice + boxitems < numchoices))
	{
		topleftchoice += boxwidth;
	}
	redisplay = 1;
	topleftrow -= scrunch;
	while (1) /* main loop */
	{
		if (redisplay)                       /* display the current choices */
		{
			memset(buf, ' ', 80);
			buf[boxwidth*colwidth] = 0;
			for (i = (hdg2) ? 0 : -1; i <= boxdepth; ++i)  /* blank the box */
			{
				driver_put_string(topleftrow + i, topleftcol, C_PROMPT_LO, buf);
			}
			for (i = 0; i + topleftchoice < numchoices && i < boxitems; ++i)
			{
				/* display the choices */
				j = i + topleftchoice;
				k = attributes[j] & 3;
				if (k == 1)
				{
					k = C_PROMPT_LO;
				}
				else if (k == 3)
				{
					k = C_PROMPT_HI;
				}
				else
				{
					k = C_PROMPT_MED;
				}
				if (formatitem)
				{
					(*formatitem)(j, buf);
					charptr = buf;
				}
				else
				{
					charptr = choices[j];
				}
				driver_put_string(topleftrow + i/boxwidth, topleftcol + (i % boxwidth)*colwidth,
						k, charptr);
			}
			/***
			... format differs for summary/detail, whups, force box width to
			...  be 72 when detail toggle available?  (2 grey margin each
			...  side, 1 blue margin each side)
			***/
			if (topleftchoice > 0 && hdg2 == NULL)
			{
				driver_put_string(topleftrow - 1, topleftcol, C_PROMPT_LO, "(more)");
			}
			if (topleftchoice + boxitems < numchoices)
			{
				driver_put_string(topleftrow + boxdepth, topleftcol, C_PROMPT_LO, "(more)");
			}
			redisplay = 0;
		}

		i = current - topleftchoice;           /* highlight the current choice */
		if (formatitem)
		{
			(*formatitem)(current, curitem);
			itemptr = curitem;
		}
		else
		{
			itemptr = choices[current];
		}
		driver_put_string(topleftrow + i/boxwidth, topleftcol + (i % boxwidth)*colwidth,
					C_CHOICE_CURRENT, itemptr);

		if (speedstring)                     /* show speedstring if any */
		{
			show_speed_string(speedrow, speedstring, speedprompt);
		}
		else
		{
			driver_hide_text_cursor();
		}

		driver_wait_key_pressed(0); /* enables help */
		curkey = driver_get_key();
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

		i = current - topleftchoice;           /* unhighlight current choice */
		k = attributes[current] & 3;
		if (k == 1)
		{
			k = C_PROMPT_LO;
		}
		else if (k == 3)
		{
			k = C_PROMPT_HI;
		}
		else
		{
			k = C_PROMPT_MED;
		}
		driver_put_string(topleftrow + i/boxwidth, topleftcol + (i % boxwidth)*colwidth,
					k, itemptr);

		increment = 0;
		switch (curkey)
		{                      /* deal with input key */
		case FIK_ENTER:
		case FIK_ENTER_2:
			ret = current;
			goto fs_choice_end;
		case FIK_ESC:
			goto fs_choice_end;
		case FIK_DOWN_ARROW:
			increment = boxwidth;
			rev_increment = -increment;
			break;
		case FIK_CTL_DOWN_ARROW:
			increment = boxwidth;
			rev_increment = -increment;
			{
				int newcurrent = current;
				while ((newcurrent += boxwidth) != current)
				{
					if (newcurrent >= numchoices)
					{
						newcurrent = (newcurrent % boxwidth) - boxwidth;
					}
					else if (!is_a_dir_name(choices[newcurrent]))
					{
						if (current != newcurrent)
						{
							current = newcurrent - boxwidth;
						}
						break;  /* breaks the while loop */
					}
				}
			}
			break;
		case FIK_UP_ARROW:
			rev_increment = boxwidth;
			increment = -rev_increment;
			break;
		case FIK_CTL_UP_ARROW:
			rev_increment = boxwidth;
			increment = -rev_increment;
			{
				int newcurrent = current;
				while ((newcurrent -= boxwidth) != current)
				{
					if (newcurrent < 0)
					{
						newcurrent = (numchoices - current) % boxwidth;
						newcurrent =  numchoices + (newcurrent ? boxwidth - newcurrent: 0);
					}
					else if (!is_a_dir_name(choices[newcurrent]))
					{
						if (current != newcurrent)
						{
							current = newcurrent + boxwidth;
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
					if (newcurrent >= numchoices)
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
						newcurrent = numchoices;
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
			if (numchoices > boxitems)
			{
				topleftchoice -= boxitems;
				increment = -boxitems;
				rev_increment = boxwidth;
				redisplay = 1;
			}
			break;
		case FIK_PAGE_DOWN:
			if (numchoices > boxitems)
			{
				topleftchoice += boxitems;
				increment = boxitems;
				rev_increment = -boxwidth;
				redisplay = 1;
			}
			break;
		case FIK_HOME:
			current = -1;
			increment = rev_increment = 1;
			break;
		case FIK_CTL_HOME:
			current = -1;
			increment = rev_increment = 1;
			{
				int newcurrent;
				for (newcurrent = 0; newcurrent < numchoices; ++newcurrent)
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
			current = numchoices;
			increment = rev_increment = -1;
			break;
		case FIK_CTL_END:
			current = numchoices;
			increment = rev_increment = -1;
			{
				int newcurrent;
				for (newcurrent = numchoices - 1; newcurrent >= 0; --newcurrent)
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
			if (checkkey)
			{
				ret = (*checkkey)(curkey, current);
				if (ret < -1 || ret > 0)
				{
					goto fs_choice_end;
				}
				if (ret == -1)
				{
					redisplay = -1;
				}
			}
			ret = -1;
			if (speedstring)
			{
				process_speed_string(speedstring, choices, curkey, &current,
						numchoices, options & CHOICE_NOT_SORTED);
			}
			break;
		}
		if (increment)                  /* apply cursor movement */
		{
			current += increment;
			if (speedstring)               /* zap speedstring */
			{
				speedstring[0] = 0;
			}
		}
		while (1)
		{                 /* adjust to a non-comment choice */
			if (current < 0 || current >= numchoices)
			{
				increment = rev_increment;
			}
			else if ((attributes[current] & 256) == 0)
			{
				break;
			}
			current += increment;
		}
		if (topleftchoice > numchoices - boxitems)
		{
			topleftchoice = ((numchoices + boxwidth - 1)/boxwidth)*boxwidth - boxitems;
		}
		if (topleftchoice < 0)
		{
			topleftchoice = 0;
		}
		while (current < topleftchoice)
		{
			topleftchoice -= boxwidth;
			redisplay = 1;
		}
		while (current >= topleftchoice + boxitems)
		{
			topleftchoice += boxwidth;
			redisplay = 1;
		}
	}

fs_choice_end:
	return ret;
}

int full_screen_choice_help(int help_mode, int options, const char *hdg,
	char *hdg2, char *instr, int numchoices, char **choices, int *attributes,
	int boxwidth, int boxdepth, int colwidth, int current,
	void (*formatitem)(int, char*), char *speedstring,
	int (*speedprompt)(int, int, int, char *, int), int (*checkkey)(int, int))
{
	int result;
	HelpModeSaver saved_help(help_mode);
	result = full_screen_choice(options, hdg, hdg2, instr,
		numchoices, choices, attributes, boxwidth, boxdepth, colwidth,
		current, formatitem, speedstring, speedprompt, checkkey);
	return result;
}

#ifndef XFRACT
/* case independent version of strncmp */
int strncasecmp(char *s, char *t, int ct)
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

int main_menu(int fullmenu)
{
	const char *choices[44]; /* 2 columns*22 rows */
	int attributes[44];
	int choicekey[44];
	int i;
	int nextleft, nextright;
	int oldtabmode;
	int showjuliatoggle;
	oldtabmode = g_tab_mode;

top:
	s_full_menu = fullmenu;
	g_tab_mode = 0;
	showjuliatoggle = 0;
	for (i = 0; i < 44; ++i)
	{
		attributes[i] = 256;
		choices[i] = "";
		choicekey[i] = -1;
	}
	nextleft = -2;
	nextright = -1;

	if (fullmenu)
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
		if (!(g_fractal_type == FRACTYPE_JULIA || g_fractal_type == FRACTYPE_JULIA_FP || g_fractal_type == FRACTYPE_INVERSE_JULIA))
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

	if (fullmenu)
	{
		if ((g_current_fractal_specific->tojulia != FRACTYPE_NO_FRACTAL
			&& g_parameters[0] == 0.0 && g_parameters[1] == 0.0)
			|| g_current_fractal_specific->tomandel != FRACTYPE_NO_FRACTAL)
		{
			nextleft += 2;
			choicekey[nextleft] = FIK_SPACE;
			attributes[nextleft] = MENU_ITEM;
			choices[nextleft] = "toggle to/from julia <space>";
			showjuliatoggle = 1;
		}
		if (g_fractal_type == FRACTYPE_JULIA || g_fractal_type == FRACTYPE_JULIA_FP || g_fractal_type == FRACTYPE_INVERSE_JULIA)
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

	if (fullmenu)
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

	if (fullmenu)
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

	if (fullmenu)
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
	choices[nextright] = "quit "FRACTINT"           <esc> ";

	nextright += 2;
	choicekey[nextright] = FIK_INSERT;
	attributes[nextright] = MENU_ITEM;
	choices[nextright] = "restart "FRACTINT"        <ins> ";

#ifdef XFRACT
	if (fullmenu && (g_got_real_dac || g_fake_lut) && g_colors >= 16)
#else
	if (fullmenu && g_got_real_dac && g_colors >= 16)
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

		if (g_colors > 16)
		{
			nextright += 2;
			choicekey[nextright] = 'e';
			attributes[nextright] = MENU_ITEM;
			choices[nextright] = "palette editing mode     <e>  ";

			nextright += 2;
			choicekey[nextright] = 'a';
			attributes[nextright] = MENU_ITEM;
			choices[nextright] = "make starfield           <a>  ";
		}
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
			NULL, NULL, nextleft, (char **) choices, attributes,
			2, nextleft/2, 29, 0, NULL, NULL, NULL, menu_check_key);
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
				help(0);
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
	g_tab_mode = oldtabmode;
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
		|| testkey == FIK_CTL_P
		|| testkey == FIK_CTL_S || testkey == FIK_CTL_U) /* ctrl-A, E, H, P, S, U */
			return -testkey;
		if (testkey == ' ')
		{
			if ((g_current_fractal_specific->tojulia != FRACTYPE_NO_FRACTAL
					&& g_parameters[0] == 0.0 && g_parameters[1] == 0.0)
				|| g_current_fractal_specific->tomandel != FRACTYPE_NO_FRACTAL)
			{
				return -testkey;
			}
		}
		if (g_got_real_dac && g_colors >= 16)
		{
			if (strchr("c+-", testkey))
			{
				return -testkey;
			}
			if (g_colors > 16
					&& (testkey == 'a' || (testkey == 'e')))
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
		int (*checkkey)(int)  /* routine to check non data keys, or NULL */
		)
{
	char savefld[81];
	char buf[81];
	int insert, started, offset, curkey, display;
	int i, j;
	int ret;

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	ret = -1;
	strcpy(savefld, fld);
	insert = started = offset = 0;
	display = 1;
	while (1)
	{
		strcpy(buf, fld);
		i = (int) strlen(buf);
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
			started = 1;
			break;
		case FIK_LEFT_ARROW:
			if (offset > 0)
			{
				--offset;
			}
			started = 1;
			break;
		case FIK_HOME:
				offset = 0;
			started = 1;
			break;
		case FIK_END:
			offset = (int) strlen(fld);
			started = 1;
			break;
		case FIK_BACKSPACE:
		case 127:                              /* backspace */
#if defined(_WIN32)
			_ASSERTE(127 != curkey);
#endif
			if (offset > 0)
			{
				j = (int) strlen(fld);
				for (i = offset-1; i < j; ++i)
				{
					fld[i] = fld[i + 1];
				}
				--offset;
			}
			started = display = 1;
			break;
		case FIK_DELETE:                           /* delete */
			j = (int) strlen(fld);
			for (i = offset; i < j; ++i)
			{
				fld[i] = fld[i + 1];
			}
			started = display = 1;
			break;
		case FIK_INSERT:                           /* insert */
			insert ^= 0x8000;
			started = 1;
			break;
		case FIK_F5:
			strcpy(fld, savefld);
			insert = started = offset = 0;
			display = 1;
			break;
		default:
			if (nonalpha(curkey))
			{
				if (checkkey)
				{
					ret = (*checkkey)(curkey);
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
			if (started == 0) /* first char is data, zap field */
			{
				fld[0] = 0;
			}
			if (insert)
			{
				j = (int) strlen(fld);
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
				int specialv;
				char tmpfld[30];
				specialv = 0;
				if (*fld == 'e' || *fld == 'E')
				{
					tmpd = exp(1.0);
					specialv = 1;
				}
				if (*fld == 'p' || *fld == 'P')
				{
					tmpd = atan(1.0)*4;
					specialv = 1;
				}
				if (specialv)
				{
					if ((options & INPUTFIELD_DOUBLE) == 0)
					{
						round_float_d(&tmpd);
					}
					sprintf(tmpfld, "%.15g", tmpd);
					tmpfld[len-1] = 0; /* safety, field should be long enough */
					strcpy(fld, tmpfld);
					offset = 0;
				}
			}
			started = display = 1;
		}
	}

inpfld_end:
	return ret;
}

int field_prompt(
		char *hdg,      /* heading, \n delimited lines */
		char *instr,    /* additional instructions or NULL */
		char *fld,          /* the field itself */
		int len,            /* field length (declare as 1 larger for \0) */
		int (*checkkey)(int)   /* routine to check non data keys, or NULL */
		)
{
	char *charptr;
	int boxwidth, titlelines, titlecol, titlerow;
	int promptcol;
	int i, j;
	char buf[81];
	help_title();                           /* clear screen, display title */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);     /* init rest to background */
	charptr = hdg;                         /* count title lines, find widest */
	i = boxwidth = 0;
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
	titlerow = (25 - i) / 2;               /* top row of it all when centered */
	titlerow -= titlerow / 4;              /* higher is better if lots extra */
	titlecol = (80 - boxwidth) / 2;        /* center the box */
	titlecol -= (90 - boxwidth) / 20;
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
				titlerow + titlelines + 1, promptcol, checkkey);
}


/* thinking(1, message):
		if thinking message not yet on display, it is displayed;
		otherwise the wheel is updated
		returns 0 to keep going, -1 if keystroke pending
	thinking(0, NULL):
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

/* load_fractint_config
 *
 * Reads fractint.cfg, loading video_info entries into g_video_table.
 * Sets the number of entries, sets g_video_table_len.
 * Past g_video_table, g_cfg_line_nums are stored for update_fractint_cfg.
 * If fractint.cfg is not found or invalid, issues a message
 * (first time the problem occurs only, and only if options is
 * zero) and uses the hard-coded table.
 */
void load_fractint_config()
{
	FILE *cfgfile;
	VIDEOINFO vident;
	int linenum;
	long g_x_dots, g_y_dots;
	int i, j, keynum, ax, bx, cx, dx, dotmode, g_colors;
	char *fields[11];
	int textsafe2;
	char tempstring[150];
	int truecolorbits;

	findpath("fractint.cfg", tempstring);
	if (tempstring[0] == 0                            /* can't find the file */
		|| (cfgfile = fopen(tempstring, "r")) == NULL)   /* can't open it */
	{
		goto bad_fractint_cfg;
	}

	linenum = 0;
	while (g_video_table_len < MAXVIDEOMODES
		&& fgets(tempstring, 120, cfgfile))
	{
		if (strchr(tempstring, '\n') == NULL)
		{
			/* finish reading the line */
			while (fgetc(cfgfile) != '\n' && !feof(cfgfile))
			{
			}
		}
		++linenum;
		if (tempstring[0] == ';')
		{
			continue;   /* comment line */
		}
		tempstring[120] = 0;
		tempstring[(int) strlen(tempstring)-1] = 0; /* zap trailing \n */
		i = j = -1;
		/* key, mode name, ax, bx, cx, dx, dotmode, x, y, g_colors, comments, driver */
		while (1)
		{
			if (tempstring[++i] < ' ')
			{
				if (tempstring[i] == 0)
				{
					break;
				}
				tempstring[i] = ' '; /* convert tab (or whatever) to blank */
			}
			else if (tempstring[i] == ',' && ++j < 11)
			{
				assert(j >= 0 && j < 11);
				fields[j] = &tempstring[i + 1]; /* remember start of next field */
				tempstring[i] = 0;   /* make field a separate string */
			}
		}
		keynum = check_vidmode_keyname(tempstring);
		sscanf(fields[1], "%x", &ax);
		sscanf(fields[2], "%x", &bx);
		sscanf(fields[3], "%x", &cx);
		sscanf(fields[4], "%x", &dx);
		dotmode     = atoi(fields[5]);
		g_x_dots       = atol(fields[6]);
		g_y_dots       = atol(fields[7]);
		g_colors      = atoi(fields[8]);
		if (g_colors == 4 && strchr(strlwr(fields[8]), 'g'))
		{
			g_colors = 256;
			truecolorbits = 4; /* 32 bits */
		}
		else if (g_colors == 16 && strchr(fields[8], 'm'))
		{
			g_colors = 256;
			truecolorbits = 3; /* 24 bits */
		}
		else if (g_colors == 64 && strchr(fields[8], 'k'))
		{
			g_colors = 256;
			truecolorbits = 2; /* 16 bits */
		}
		else if (g_colors == 32 && strchr(fields[8], 'k'))
		{
			g_colors = 256;
			truecolorbits = 1; /* 15 bits */
		}
		else
		{
			truecolorbits = 0;
		}

		textsafe2   = dotmode / 100;
		dotmode    %= 100;
		if (j < 9 ||
			keynum < 0 ||
			dotmode < 0 || dotmode > 30 ||
			textsafe2 < 0 || textsafe2 > 4 ||
			g_x_dots < MIN_PIXELS || g_x_dots > MAX_PIXELS ||
			g_y_dots < MIN_PIXELS || g_y_dots > MAX_PIXELS ||
			(g_colors != 0 && g_colors != 2 && g_colors != 4 && g_colors != 16 &&
				g_colors != 256))
		{
			goto bad_fractint_cfg;
		}
		g_cfg_line_nums[g_video_table_len] = linenum; /* for update_fractint_cfg */

		memset(&vident, 0, sizeof(vident));
		strncpy(&vident.name[0], fields[0], NUM_OF(vident.name));
		strncpy(&vident.comment[0], fields[9], NUM_OF(vident.comment));
		vident.name[25] = vident.comment[25] = 0;
		vident.keynum      = keynum;
		vident.videomodeax = ax;
		vident.videomodebx = bx;
		vident.videomodecx = cx;
		vident.videomodedx = dx;
		vident.dotmode     = truecolorbits*1000 + textsafe2*100 + dotmode;
		vident.x_dots       = (short)g_x_dots;
		vident.y_dots       = (short)g_y_dots;
		vident.colors      = g_colors;

		/* if valid, add to supported modes */
		vident.driver = DriverManager::find_by_name(fields[10]);
		if (vident.driver != NULL)
		{
			if (vident.driver->validate_mode(vident))
			{
				/* look for a synonym mode and if found, overwite its key */
				int m;
				int synonym_found = FALSE;
				for (m = 0; m < g_video_table_len; m++)
				{
					VIDEOINFO *mode = &g_video_table[m];
					if ((mode->driver == vident.driver) && (mode->colors == vident.colors) &&
						(mode->x_dots == vident.x_dots) && (mode->y_dots == vident.y_dots) &&
						(mode->dotmode == vident.dotmode))
					{
						if (0 == mode->keynum)
						{
							mode->keynum = vident.keynum;
						}
						synonym_found = TRUE;
						break;
					}
				}
				/* no synonym found, append it to current list of video modes */
				if (FALSE == synonym_found)
				{
					add_video_mode(vident.driver, vident);
				}
			}
		}
	}
	fclose(cfgfile);
	return;

bad_fractint_cfg:
	g_bad_config = -1; /* bad, no message issued yet */
}

void bad_fractint_cfg_msg()
{
	stop_message(0,
		"File FRACTINT.CFG is missing or invalid.\n"
		"See Hardware Support and Video Modes in the full documentation for help.\n"
		"I will continue with only the built-in video modes available.");
	g_bad_config = 1; /* bad, message issued */
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
	int i, keyset;
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

void video_mode_key_name(int k, char *buf)
{
	/* set buffer to name of passed key number */
	*buf = 0;
	if (k > 0)
	{
		if (k > 1103)
		{
			*(buf++) = 'A';
			k -= 1103;
		}
		else if (k > 1093)
		{
			*(buf++) = 'C';
			k -= 1093;
		}
		else if (k > 1083)
		{
			*(buf++) = 'S';
			k -= 1083;
		}
		else
		{
			k -= 1058;
		}
		sprintf(buf, "F%d", k);
	}
}
