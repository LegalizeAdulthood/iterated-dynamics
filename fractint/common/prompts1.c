/*
		Various routines that prompt for things.
*/

#include <string.h>
#include <ctype.h>
#ifdef   XFRACT
#ifndef  __386BSD__
#include <sys/types.h>
#include <sys/stat.h>
#endif
#endif
#if !defined(__386BSD__)
#if !defined(_WIN32)
#include <malloc.h>
#endif
#endif

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#ifdef __hpux
#include <sys/param.h>
#define getwd(a) getcwd(a, MAXPATHLEN)
#endif

#ifdef __SVR4
#include <sys/param.h>
#define getwd(a) getcwd(a, MAXPATHLEN)
#endif
#include "drivers.h"

/* Routines used in prompts2.c */

	int prompt_checkkey(int curkey);
	int prompt_checkkey_scroll(int curkey);
	long get_file_entry(int, char *, char *, char *, char *);

/* Routines in this module      */

int prompt_value_string(char *buf, struct full_screen_values *val);
static  int input_field_list(int attr, char *fld, int vlen, char **list, int llen,
							int row, int col, int (*checkkey)(int));
static  int select_fracttype(int t);
static  int sel_fractype_help(int curkey, int choice);
		int select_type_params(int newfractype, int oldfractype);
		void set_default_parms(void);
static  long gfe_choose_entry(int, char *, char *, char *);
static  int check_gfe_key(int curkey, int choice);
static  void load_entry_text(FILE *entfile, char *buf, int maxlines, int startrow, int startcol);
static  void format_parmfile_line(int, char *);
static  int get_light_params(void);
static  int check_mapfile(void);
static  int get_funny_glasses_params(void);

static char funnyglasses_map_name[16];
char ifsmask[13]     = {"*.ifs"};
char formmask[13]    = {"*.frm"};
char lsysmask[13]    = {"*.l"};
char g_glasses1_map[] = "glasses1.map";
char g_map_name[FILE_MAX_DIR] = "";
int  g_map_set = FALSE;
int g_julibrot;   /* flag for julibrot */

/* --------------------------------------------------------------------- */

int promptfkeys;

	/* These need to be global because F6 exits full_screen_prompt() */
int scroll_row_status;    /* will be set to first line of extra info to
                             be displayed (0 = top line) */
int scroll_column_status; /* will be set to first column of extra info to
                             be displayed (0 = leftmost column)*/

int full_screen_prompt(/* full-screen prompting routine */
		char *hdg,          /* heading, lines separated by \n */
		int numprompts,         /* there are this many prompts (max) */
		char **prompts,     /* array of prompting pointers */
		struct full_screen_values *values, /* array of values */
		int fkeymask,           /* bit n on if Fn to cause return */
		char *extrainfo     /* extra info box to display, \n separated */
)
{
	char *hdgscan;
	int titlelines, titlewidth, titlerow;
	int maxpromptwidth, maxfldwidth, maxcomment;
	int boxrow, boxlines;
	int boxcol, boxwidth;
	int extralines, extrawidth, extrarow;
	int instrrow;
	int promptrow, promptcol, valuecol;
	int curchoice = 0;
	int done, i, j;
	int anyinput;
	int savelookatmouse;
	int curtype, curlen;
	char buf[81];

		/* scrolling related variables */
	FILE *scroll_file = NULL;     /* file with extrainfo entry to scroll   */
	long scroll_file_start = 0;    /* where entry starts in scroll_file     */
	int in_scrolling_mode = 0;     /* will be 1 if need to scroll extrainfo */
	int lines_in_entry = 0;        /* total lines in entry to be scrolled   */
	int vertical_scroll_limit = 0; /* don't scroll down if this is top line */
	int widest_entry_line = 0;     /* length of longest line in entry       */
	int rewrite_extrainfo = 0;     /* if 1: rewrite extrainfo to text box   */
	char blanks[78];               /* used to clear text box                */

	savelookatmouse = g_look_at_mouse;
	g_look_at_mouse = LOOK_MOUSE_NONE;
	promptfkeys = fkeymask;
	memset(blanks, ' ', 77);   /* initialize string of blanks */
	blanks[77] = (char) 0;

		/* If applicable, open file for scrolling extrainfo. The function
			find_file_item() opens the file and sets the file pointer to the
			beginning of the entry.
		*/
	if (extrainfo && *extrainfo)
	{
		if (g_fractal_type == FORMULA || g_fractal_type == FFORMULA)
		{
			find_file_item(g_formula_filename, g_formula_name, &scroll_file, ITEMTYPE_FORMULA);
			in_scrolling_mode = 1;
			scroll_file_start = ftell(scroll_file);
		}
		else if (g_fractal_type == LSYSTEM)
		{
			find_file_item(g_l_system_filename, g_l_system_name, &scroll_file, 2);
			in_scrolling_mode = 1;
			scroll_file_start = ftell(scroll_file);
		}
		else if (g_fractal_type == IFS || g_fractal_type == IFS3D)
		{
			find_file_item(g_ifs_filename, g_ifs_name, &scroll_file, 3);
			in_scrolling_mode = 1;
			scroll_file_start = ftell(scroll_file);
		}
	}

		/* initialize widest_entry_line and lines_in_entry */
	if (in_scrolling_mode && scroll_file != NULL)
	{
		int comment = 0;
		int c = 0;
		int widthct = 0;
		while ((c = fgetc(scroll_file)) != EOF && c != '\032')
		{
			if (c == ';')
			{
				comment = 1;
			}
			else if (c == '\n')
			{
				comment = 0;
				lines_in_entry++;
				widthct =  -1;
			}
			else if (c == '\t')
			{
				widthct += 7 - widthct % 8;
			}
			else if (c == '\r')
			{
				continue;
			}
			if (++widthct > widest_entry_line)
			{
				widest_entry_line = widthct;
			}
			if (c == '}' && !comment)
			{
				lines_in_entry++;
				break;
			}
		}
		if (c == EOF || c == '\032')  /* should never happen */
		{
			fclose(scroll_file);
			in_scrolling_mode = 0;
		}
	}



	help_title();                        /* clear screen, display title line  */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);  /* init rest of screen to background */


	hdgscan = hdg;                      /* count title lines, find widest */
	i = titlewidth = 0;
	titlelines = 1;
	while (*hdgscan)
	{
		if (*(hdgscan++) == '\n')
		{
			++titlelines;
			i = -1;
		}
		if (++i > titlewidth)
		{
			titlewidth = i;
		}
	}
	extralines = extrawidth = i = 0;
	hdgscan = extrainfo;
	if (hdgscan != 0)
	{
		if (*hdgscan == 0)
		{
			extrainfo = NULL;
		}
		else  /* count extra lines, find widest */
		{
			extralines = 3;
			while (*hdgscan)
			{
				if (*(hdgscan++) == '\n')
				{
					if (extralines + numprompts + titlelines >= 20)
					{
						*hdgscan = 0; /* full screen, cut off here */
						break;
					}
					++extralines;
					i = -1;
				}
				if (++i > extrawidth)
				{
					extrawidth = i;
				}
			}
		}
	}

		/* if entry fits in available space, shut off scrolling */
	if (in_scrolling_mode && scroll_row_status == 0
		&& lines_in_entry == extralines - 2
		&& scroll_column_status == 0
		&& strchr(extrainfo, '\021') == NULL)
	{
		in_scrolling_mode = 0;
		fclose(scroll_file);
		scroll_file = NULL;
	}

		/*initialize vertical scroll limit. When the top line of the text
		box is the vertical scroll limit, the bottom line is the end of the
		entry, and no further down scrolling is necessary.
		*/
	if (in_scrolling_mode)
	{
		vertical_scroll_limit = lines_in_entry - (extralines - 2);
	}

	/* work out vertical positioning */
	i = numprompts + titlelines + extralines + 3; /* total rows required */
	j = (25 - i) / 2;                   /* top row of it all when centered */
	j -= j / 4;                         /* higher is better if lots extra */
	boxlines = numprompts;
	titlerow = 1 + j;
	promptrow = boxrow = titlerow + titlelines;
	if (titlerow > 2)  /* room for blank between title & box? */
	{
		--titlerow;
		--boxrow;
		++boxlines;
		}
	instrrow = boxrow + boxlines;
	if (instrrow + 3 + extralines < 25)
	{
		++boxlines;    /* blank at bottom of box */
		++instrrow;
		if (instrrow + 3 + extralines < 25)
		{
			++instrrow; /* blank before instructions */
		}
		}
	extrarow = instrrow + 2;
	if (numprompts > 1) /* 3 instructions lines */
	{
		++extrarow;
	}
	if (extrarow + extralines < 25)
	{
		++extrarow;
	}

	if (in_scrolling_mode)  /* set box to max width if in scrolling mode */
	{
		extrawidth = 76;
	}

	/* work out horizontal positioning */
	maxfldwidth = maxpromptwidth = maxcomment = anyinput = 0;
	for (i = 0; i < numprompts; i++)
	{
		if (values[i].type == 'y')
		{
			static char *noyes[2] =
			{"no", "yes"};
			values[i].type = 'l';
			values[i].uval.ch.vlen = 3;
			values[i].uval.ch.list = noyes;
			values[i].uval.ch.llen = 2;
		}
		j = (int) strlen(prompts[i]);
		if (values[i].type == '*')
		{
			if (j > maxcomment)
			{
				maxcomment = j;
			}
		}
		else
		{
			anyinput = 1;
			if (j > maxpromptwidth)
			{
				maxpromptwidth = j;
			}
			j = prompt_value_string(buf, &values[i]);
			if (j > maxfldwidth)
			{
				maxfldwidth = j;
			}
		}
	}
	boxwidth = maxpromptwidth + maxfldwidth + 2;
	if (maxcomment > boxwidth)
	{
		boxwidth = maxcomment;
	}
	if ((boxwidth += 4) > 80)
	{
		boxwidth = 80;
	}
	boxcol = (80 - boxwidth) / 2;       /* center the box */
	promptcol = boxcol + 2;
	valuecol = boxcol + boxwidth - maxfldwidth - 2;
	if (boxwidth <= 76)  /* make margin a bit wider if we can */
	{
		boxwidth += 2;
		--boxcol;
	}
	j = titlewidth;
	if (j < extrawidth)
	{
		j = extrawidth;
	}
	i = j + 4 - boxwidth;
	if (i > 0)  /* expand box for title/extra */
	{
		if (boxwidth + i > 80)
		{
			i = 80 - boxwidth;
		}
		boxwidth += i;
		boxcol -= i / 2;
		}
	i = (90 - boxwidth) / 20;
	boxcol    -= i;
	promptcol -= i;
	valuecol  -= i;

	/* display box heading */
	for (i = titlerow; i < boxrow; ++i)
	{
		driver_set_attr(i, boxcol, C_PROMPT_HI, boxwidth);
	}

	{
		char buffer[256], *hdgline = buffer;
		/* center each line of heading independently */
		int i;
		strcpy(hdgline, hdg);
		for (i = 0; i < titlelines-1; i++)
		{
			char *next = strchr(hdgline, '\n');
			if (next == NULL)
			{
				break; /* shouldn't happen */
			}
			*next = '\0';
			titlewidth = (int) strlen(hdgline);
			g_text_cbase = boxcol + (boxwidth - titlewidth) / 2;
			driver_put_string(titlerow + i, 0, C_PROMPT_HI, hdgline);
			*next = '\n';
			hdgline = next + 1;
		}
		/* add scrolling key message, if applicable */
		if (in_scrolling_mode)
		{
			*(hdgline + 31) = (char) 0;   /* replace the ')' */
			strcat(hdgline, ". CTRL+(direction key) to scroll text.)");
		}

		titlewidth = (int) strlen(hdgline);
		g_text_cbase = boxcol + (boxwidth - titlewidth) / 2;
		driver_put_string(titlerow + i, 0, C_PROMPT_HI, hdgline);
	}

	/* display extra info */
	if (extrainfo)
	{
#ifndef XFRACT
#define S1 '\xC4'
#define S2 "\xC0"
#define S3 "\xD9"
#define S4 "\xB3"
#define S5 "\xDA"
#define S6 "\xBF"
#else
#define S1 '-'
#define S2 "+" /* ll corner */
#define S3 "+" /* lr corner */
#define S4 "|"
#define S5 "+" /* ul corner */
#define S6 "+" /* ur corner */
#endif
		memset(buf, S1, 80);
		buf[boxwidth-2] = 0;
		g_text_cbase = boxcol + 1;
		driver_put_string(extrarow, 0, C_PROMPT_BKGRD, buf);
		driver_put_string(extrarow + extralines-1, 0, C_PROMPT_BKGRD, buf);
		--g_text_cbase;
		driver_put_string(extrarow, 0, C_PROMPT_BKGRD, S5);
		driver_put_string(extrarow + extralines-1, 0, C_PROMPT_BKGRD, S2);
		g_text_cbase += boxwidth - 1;
		driver_put_string(extrarow, 0, C_PROMPT_BKGRD, S6);
		driver_put_string(extrarow + extralines-1, 0, C_PROMPT_BKGRD, S3);

		g_text_cbase = boxcol;

		for (i = 1; i < extralines-1; ++i)
		{
			driver_put_string(extrarow + i, 0, C_PROMPT_BKGRD, S4);
			driver_put_string(extrarow + i, boxwidth-1, C_PROMPT_BKGRD, S4);
		}
		g_text_cbase += (boxwidth - extrawidth) / 2;
		driver_put_string(extrarow + 1, 0, C_PROMPT_TEXT, extrainfo);
	}

	g_text_cbase = 0;

	/* display empty box */
	for (i = 0; i < boxlines; ++i)
	{
		driver_set_attr(boxrow + i, boxcol, C_PROMPT_LO, boxwidth);
	}

	/* display initial values */
	for (i = 0; i < numprompts; i++)
	{
		driver_put_string(promptrow + i, promptcol, C_PROMPT_LO, prompts[i]);
		prompt_value_string(buf, &values[i]);
		driver_put_string(promptrow + i, valuecol, C_PROMPT_LO, buf);
	}


	if (!anyinput)
	{
		put_string_center(instrrow++, 0, 80, C_PROMPT_BKGRD,
		"No changeable parameters;");
		put_string_center(instrrow, 0, 80, C_PROMPT_BKGRD,
		(g_help_mode > 0) ? "Press ENTER to exit, ESC to back out, "FK_F1" for help" : "Press ENTER to exit");
		driver_hide_text_cursor();
		g_text_cbase = 2;
		while (1)
		{
			if (rewrite_extrainfo)
			{
				rewrite_extrainfo = 0;
				fseek(scroll_file, scroll_file_start, SEEK_SET);
				load_entry_text(scroll_file, extrainfo, extralines - 2,
								scroll_row_status, scroll_column_status);
				for (i = 1; i <= extralines - 2; i++)
				{
					driver_put_string(extrarow + i, 0, C_PROMPT_TEXT, blanks);
				}
				driver_put_string(extrarow + 1, 0, C_PROMPT_TEXT, extrainfo);
			}
			/* TODO: rework key interaction to blocking wait */
			while (!driver_key_pressed())
			{
			}
			done = driver_get_key();
			switch (done)
			{
			case FIK_ESC:
				done = -1;
			case FIK_ENTER:
			case FIK_ENTER_2:
				goto fullscreen_exit;
			case FIK_CTL_DOWN_ARROW:    /* scrolling key - down one row */
				if (in_scrolling_mode && scroll_row_status < vertical_scroll_limit)
				{
					scroll_row_status++;
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_UP_ARROW:      /* scrolling key - up one row */
				if (in_scrolling_mode && scroll_row_status > 0)
				{
					scroll_row_status--;
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_LEFT_ARROW:    /* scrolling key - left one column */
				if (in_scrolling_mode && scroll_column_status > 0)
				{
					scroll_column_status--;
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_RIGHT_ARROW:   /* scrolling key - right one column */
				if (in_scrolling_mode && strchr(extrainfo, '\021') != NULL)
				{
					scroll_column_status++;
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_PAGE_DOWN:   /* scrolling key - down one screen */
				if (in_scrolling_mode && scroll_row_status < vertical_scroll_limit)
				{
					scroll_row_status += extralines - 2;
					if (scroll_row_status > vertical_scroll_limit)
					{
						scroll_row_status = vertical_scroll_limit;
					}
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_PAGE_UP:     /* scrolling key - up one screen */
				if (in_scrolling_mode && scroll_row_status > 0)
				{
					scroll_row_status -= extralines - 2;
					if (scroll_row_status < 0)
					{
						scroll_row_status = 0;
					}
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_END:         /* scrolling key - to end of entry */
				if (in_scrolling_mode)
				{
					scroll_row_status = vertical_scroll_limit;
					scroll_column_status = 0;
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_CTL_HOME:        /* scrolling key - to beginning of entry */
				if (in_scrolling_mode)
				{
					scroll_row_status = scroll_column_status = 0;
					rewrite_extrainfo = 1;
				}
				break;
			case FIK_F2:
			case FIK_F3:
			case FIK_F4:
			case FIK_F5:
			case FIK_F6:
			case FIK_F7:
			case FIK_F8:
			case FIK_F9:
			case FIK_F10:
				if (promptfkeys & (1 << (done + 1-FIK_F1)))
				{
					goto fullscreen_exit;
				}
			}
		}
	}


	/* display footing */
	if (numprompts > 1)
	{
		put_string_center(instrrow++, 0, 80, C_PROMPT_BKGRD,
			"Use " UPARR1 " and " DNARR1 " to select values to change");
	}
	put_string_center(instrrow + 1, 0, 80, C_PROMPT_BKGRD,
		(g_help_mode > 0) ? "Press ENTER when finished, ESCAPE to back out, or "FK_F1" for help" : "Press ENTER when finished (or ESCAPE to back out)");

	done = 0;
	while (values[curchoice].type == '*')
	{
		++curchoice;
	}

	while (!done)
	{
		if (rewrite_extrainfo)
		{
			j = g_text_cbase;
			g_text_cbase = 2;
			fseek(scroll_file, scroll_file_start, SEEK_SET);
			load_entry_text(scroll_file, extrainfo, extralines - 2,
							scroll_row_status, scroll_column_status);
			for (i = 1; i <= extralines - 2; i++)
			{
				driver_put_string(extrarow + i, 0, C_PROMPT_TEXT, blanks);
			}
			driver_put_string(extrarow + 1, 0, C_PROMPT_TEXT, extrainfo);
			g_text_cbase = j;
		}

		curtype = values[curchoice].type;
		curlen = prompt_value_string(buf, &values[curchoice]);
		if (!rewrite_extrainfo)
		{
			put_string_center(instrrow, 0, 80, C_PROMPT_BKGRD,
				(curtype == 'l') ? "Use " LTARR1 " or " RTARR1 " to change value of selected field" : "Type in replacement value for selected field");
		}
		else
		{
			rewrite_extrainfo = 0;
		}
		driver_put_string(promptrow + curchoice, promptcol, C_PROMPT_HI, prompts[curchoice]);

		if (curtype == 'l')
		{
			i = input_field_list(
				C_PROMPT_CHOOSE, buf, curlen,
				values[curchoice].uval.ch.list, values[curchoice].uval.ch.llen,
				promptrow + curchoice, valuecol, in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
			for (j = 0; j < values[curchoice].uval.ch.llen; ++j)
			{
				if (strcmp(buf, values[curchoice].uval.ch.list[j]) == 0)
				{
					break;
				}
			}
			values[curchoice].uval.ch.val = j;
			}
		else
		{
			j = 0;
			if (curtype == 'i')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER;
			}
			if (curtype == 'L')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER;
			}
			if (curtype == 'd')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_DOUBLE;
			}
			if (curtype == 'D')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_DOUBLE | INPUTFIELD_INTEGER;
			}
			if (curtype == 'f')
			{
				j = INPUTFIELD_NUMERIC;
			}
			i = input_field(j, C_PROMPT_INPUT, buf, curlen,
				promptrow + curchoice, valuecol, in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
			switch (values[curchoice].type)
			{
			case 'd':
			case 'D':
				values[curchoice].uval.dval = atof(buf);
				break;
			case 'f':
				values[curchoice].uval.dval = atof(buf);
				round_float_d(&values[curchoice].uval.dval);
				break;
			case 'i':
				values[curchoice].uval.ival = atoi(buf);
				break;
			case 'L':
				values[curchoice].uval.Lval = atol(buf);
				break;
			case 's':
				strncpy(values[curchoice].uval.sval, buf, 16);
				break;
			default: /* assume 0x100 + n */
				strcpy(values[curchoice].uval.sbuf, buf);
			}
		}

		driver_put_string(promptrow + curchoice, promptcol, C_PROMPT_LO, prompts[curchoice]);
		j = (int) strlen(buf);
		memset(&buf[j], ' ', 80-j);
		buf[curlen] = 0;
		driver_put_string(promptrow + curchoice, valuecol, C_PROMPT_LO,  buf);

		switch (i)
		{
		case 0:  /* enter  */
			done = FIK_ENTER;
			break;
		case -1: /* escape */
		case FIK_F2:
		case FIK_F3:
		case FIK_F4:
		case FIK_F5:
		case FIK_F6:
		case FIK_F7:
		case FIK_F8:
		case FIK_F9:
		case FIK_F10:
			done = i;
			break;
		case FIK_PAGE_UP:
			curchoice = -1;
		case FIK_DOWN_ARROW:
			do
			{
				if (++curchoice >= numprompts)
				{
					curchoice = 0;
				}
			}
			while (values[curchoice].type == '*');
			break;
		case FIK_PAGE_DOWN:
			curchoice = numprompts;
		case FIK_UP_ARROW:
			do
			{
				if (--curchoice < 0)
				{
					curchoice = numprompts - 1;
				}
			}
			while (values[curchoice].type == '*');
			break;
		case FIK_CTL_DOWN_ARROW:     /* scrolling key - down one row */
			if (in_scrolling_mode && scroll_row_status < vertical_scroll_limit)
			{
				scroll_row_status++;
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_UP_ARROW:       /* scrolling key - up one row */
			if (in_scrolling_mode && scroll_row_status > 0)
			{
				scroll_row_status--;
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_LEFT_ARROW:     /*scrolling key - left one column */
			if (in_scrolling_mode && scroll_column_status > 0)
			{
				scroll_column_status--;
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_RIGHT_ARROW:    /* scrolling key - right one column */
			if (in_scrolling_mode && strchr(extrainfo, '\021') != NULL)
			{
				scroll_column_status++;
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_PAGE_DOWN:    /* scrolling key - down on screen */
			if (in_scrolling_mode && scroll_row_status < vertical_scroll_limit)
			{
				scroll_row_status += extralines - 2;
				if (scroll_row_status > vertical_scroll_limit)
				{
					scroll_row_status = vertical_scroll_limit;
				}
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_PAGE_UP:      /* scrolling key - up one screen */
			if (in_scrolling_mode && scroll_row_status > 0)
			{
				scroll_row_status -= extralines - 2;
				if (scroll_row_status < 0)
				{
					scroll_row_status = 0;
				}
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_END:          /* scrolling key - go to end of entry */
			if (in_scrolling_mode)
			{
				scroll_row_status = vertical_scroll_limit;
				scroll_column_status = 0;
				rewrite_extrainfo = 1;
			}
			break;
		case FIK_CTL_HOME:         /* scrolling key - go to beginning of entry */
			if (in_scrolling_mode)
			{
				scroll_row_status = scroll_column_status = 0;
				rewrite_extrainfo = 1;
			}
			break;
		}
	}

fullscreen_exit:
	driver_hide_text_cursor();
	g_look_at_mouse = savelookatmouse;
	if (scroll_file)
	{
		fclose(scroll_file);
		scroll_file = NULL;
	}
	return done;
}

int prompt_value_string(char *buf, struct full_screen_values *val)
{  /* format value into buf, return field width */
	int i, ret;
	switch (val->type)
	{
	case 'd':
		ret = 20;
		i = 16;    /* cellular needs 16 (was 15)*/
		while (1)
		{
			sprintf(buf, "%.*g", i, val->uval.dval);
			if ((int)strlen(buf) <= ret)
			{
				break;
			}
			--i;
		}
		break;
	case 'D':
		if (val->uval.dval < 0)  /* We have to round the right way */
		{
			sprintf(buf, "%ld", (long)(val->uval.dval-.5));
		}
		else
		{
			sprintf(buf, "%ld", (long)(val->uval.dval + .5));
		}
		ret = 20;
		break;
	case 'f':
		sprintf(buf, "%.7g", val->uval.dval);
		ret = 14;
		break;
	case 'i':
		sprintf(buf, "%d", val->uval.ival);
		ret = 6;
		break;
	case 'L':
		sprintf(buf, "%ld", val->uval.Lval);
		ret = 10;
		break;
	case '*':
		ret = 0;
		*buf = 0;
		break;
	case 's':
		strncpy(buf, val->uval.sval, 16);
		buf[15] = 0;
		ret = 15;
		break;
	case 'l':
		strcpy(buf, val->uval.ch.list[val->uval.ch.val]);
		ret = val->uval.ch.vlen;
		break;
	default: /* assume 0x100 + n */
		strcpy(buf, val->uval.sbuf);
		ret = val->type & 0xff;
	}
	return ret;
}

int prompt_checkkey(int curkey)
{
	switch (curkey)
	{
	case FIK_PAGE_UP:
	case FIK_DOWN_ARROW:
	case FIK_PAGE_DOWN:
	case FIK_UP_ARROW:
		return curkey;
	case FIK_F2:
	case FIK_F3:
	case FIK_F4:
	case FIK_F5:
	case FIK_F6:
	case FIK_F7:
	case FIK_F8:
	case FIK_F9:
	case FIK_F10:
		if (promptfkeys & (1 << (curkey + 1-FIK_F1)))
		{
			return curkey;
		}
	}
	return 0;
}

int prompt_checkkey_scroll(int curkey)
{
	switch (curkey)
	{
	case FIK_PAGE_UP:
	case FIK_DOWN_ARROW:
	case FIK_CTL_DOWN_ARROW:
	case FIK_PAGE_DOWN:
	case FIK_UP_ARROW:
	case FIK_CTL_UP_ARROW:
	case FIK_CTL_LEFT_ARROW:
	case FIK_CTL_RIGHT_ARROW:
	case FIK_CTL_PAGE_DOWN:
	case FIK_CTL_PAGE_UP:
	case FIK_CTL_END:
	case FIK_CTL_HOME:
		return curkey;
	case FIK_F2:
	case FIK_F3:
	case FIK_F4:
	case FIK_F5:
	case FIK_F6:
	case FIK_F7:
	case FIK_F8:
	case FIK_F9:
	case FIK_F10:
		if (promptfkeys & (1 << (curkey + 1-FIK_F1)))
		{
			return curkey;
		}
	}
	return 0;
}

static int input_field_list(
		int attr,             /* display attribute */
		char *fld,            /* display form field value */
		int vlen,             /* field length */
		char **list,          /* list of values */
		int llen,             /* number of entries in list */
		int row,              /* display row */
		int col,              /* display column */
		int (*checkkey)(int)  /* routine to check non data keys, or NULL */
)
{
	int initval, curval;
	char buf[81];
	int curkey;
	int i, j;
	int ret, savelookatmouse;
	savelookatmouse = g_look_at_mouse;
	g_look_at_mouse = LOOK_MOUSE_NONE;
	for (initval = 0; initval < llen; ++initval)
	{
		if (strcmp(fld, list[initval]) == 0)
		{
			break;
		}
	}
	if (initval >= llen)
	{
		initval = 0;
	}
	curval = initval;
	ret = -1;
	while (1)
	{
		strcpy(buf, list[curval]);
		i = (int) strlen(buf);
		while (i < vlen)
		{
			buf[i++] = ' ';
		}
		buf[vlen] = 0;
		driver_put_string(row, col, attr, buf);
		curkey = driver_key_cursor(row, col); /* get a keystroke */
		switch (curkey)
		{
		case FIK_ENTER:
		case FIK_ENTER_2:
			ret = 0;
			goto inpfldl_end;
		case FIK_ESC:
			goto inpfldl_end;
		case FIK_RIGHT_ARROW:
			if (++curval >= llen)
			{
				curval = 0;
			}
			break;
		case FIK_LEFT_ARROW:
			if (--curval < 0)
			{
				curval = llen - 1;
			}
			break;
		case FIK_F5:
			curval = initval;
			break;
		default:
			if (nonalpha(curkey))
			{
				if (checkkey)
				{
					ret = (*checkkey)(curkey);
					if (ret != 0)
					{
						goto inpfldl_end;
					}
				}
				break;                                /* non alphanum char */
			}
			j = curval;
			for (i = 0; i < llen; ++i)
			{
				if (++j >= llen)
				{
					j = 0;
				}
				if ((*list[j] & 0xdf) == (curkey & 0xdf))
				{
					curval = j;
					break;
				}
			}
		}
	}

inpfldl_end:
	strcpy(fld, list[curval]);
	g_look_at_mouse = savelookatmouse;
	return ret;
}


/* --------------------------------------------------------------------- */

/* MCP 7-7-91, This is static code, but not called anywhere */
#ifdef DELETE_UNUSED_CODE

/* compare for sort of type table */
static int compare(const VOIDPTR i, const VOIDPTR j)
{
	return strcmp(g_fractal_specific[(int) *((BYTE *) i)].name,
					g_fractal_specific[(int) *((BYTE *) j)].name);
}

/* --------------------------------------------------------------------- */

static void clear_line(int row, int start, int stop, int color) /* clear part of a line */
{
	int col;
	for (col = start; col <= stop; col++)
	{
		driver_put_string(row, col, color, " ");
	}
}

#endif

/* --------------------------------------------------------------------- */

int get_fractal_type()             /* prompt for and select fractal type */
{
	int done, i, oldfractype, t;
	done = -1;
	oldfractype = g_fractal_type;
	while (1)
	{
		t = select_fracttype(g_fractal_type);
		if (t < 0)
		{
			break;
		}
		i = select_type_params(t, g_fractal_type);
		if (i == 0)  /* ok, all done */
		{
			done = 0;
			break;
			}
		if (i > 0) /* can't return to prior image anymore */
		{
			done = 1;
		}
		}
	if (done < 0)
	{
		g_fractal_type = oldfractype;
	}
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	return done;
}

struct FT_CHOICE
{
		char name[15];
		int  num;
		};
static struct FT_CHOICE **ft_choices; /* for sel_fractype_help subrtn */

static int select_fracttype(int t) /* subrtn of get_fractal_type, separated */
                                   /* so that storage gets freed up      */
{
	int oldhelpmode;
	int numtypes, done;
	int i, j;
#define MAXFTYPES 200
	char tname[40];
	struct FT_CHOICE storage[MAXFTYPES] = { 0 };
	struct FT_CHOICE *choices[MAXFTYPES];
	int attributes[MAXFTYPES];

	/* steal existing array for "choices" */
	choices[0] = &storage[0];
	attributes[0] = 1;
	for (i = 1; i < MAXFTYPES; ++i)
	{
		choices[i] = &storage[i];
		attributes[i] = 1;
	}
	ft_choices = &choices[0];

	/* setup context sensitive help */
	oldhelpmode = g_help_mode;
	g_help_mode = HELPFRACTALS;
	if (t == IFS3D)
	{
		t = IFS;
	}
	i = j = -1;
	while (g_fractal_specific[++i].name)
	{
		if (g_julibrot)
		{
			if (!((g_fractal_specific[i].flags & OKJB) && *g_fractal_specific[i].name != '*'))
			{
				continue;
			}
		}
		if (g_fractal_specific[i].name[0] == '*')
		{
			continue;
		}
		strcpy(choices[++j]->name, g_fractal_specific[i].name);
		choices[j]->name[14] = 0; /* safety */
		choices[j]->num = i;      /* remember where the real item is */
		}
	numtypes = j + 1;
	shell_sort(&choices, numtypes, sizeof(struct FT_CHOICE *), lccompare); /* sort list */
	j = 0;
	for (i = 0; i < numtypes; ++i) /* find starting choice in sorted list */
	{
		if (choices[i]->num == t || choices[i]->num == g_fractal_specific[t].tofloat)
		{
			j = i;
		}
	}

	tname[0] = 0;
	done = full_screen_choice(CHOICE_HELP | CHOICE_INSTRUCTIONS,
		g_julibrot ? "Select Orbit Algorithm for Julibrot" : "Select a Fractal Type",
		NULL, "Press "FK_F2" for a description of the highlighted type", numtypes,
		(char **)choices, attributes, 0, 0, 0, j, NULL, tname, NULL, sel_fractype_help);
	if (done >= 0)
	{
		done = choices[done]->num;
		if ((done == FORMULA || done == FFORMULA) && !strcmp(g_formula_filename, g_command_file))
		{
			strcpy(g_formula_filename, g_search_for.frm);
		}
		if (done == LSYSTEM && !strcmp(g_l_system_filename, g_command_file))
		{
			strcpy(g_l_system_filename, g_search_for.lsys);
		}
		if ((done == IFS || done == IFS3D) && !strcmp(g_ifs_filename, g_command_file))
		{
			strcpy(g_ifs_filename, g_search_for.ifs);
		}
	}


	g_help_mode = oldhelpmode;
	return done;
}

static int sel_fractype_help(int curkey, int choice)
{
	int oldhelpmode;
	if (curkey == FIK_F2)
	{
		oldhelpmode = g_help_mode;
		g_help_mode = g_fractal_specific[(*(ft_choices + choice))->num].helptext;
		help(0);
		g_help_mode = oldhelpmode;
		}
	return 0;
}

int select_type_params(/* prompt for new fractal type parameters */
		int newfractype,        /* new fractal type */
		int oldfractype         /* previous fractal type */
)
{
	int ret, oldhelpmode;

	oldhelpmode = g_help_mode;
sel_type_restart:
	ret = 0;
	g_fractal_type = newfractype;
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];

	if (g_fractal_type == LSYSTEM)
	{
		g_help_mode = HT_LSYS;
		if (get_file_entry(GETFILE_L_SYSTEM, "L-System", lsysmask, g_l_system_filename, g_l_system_name) < 0)
		{
			ret = 1;
			goto sel_type_exit;
			}
		}
	if (g_fractal_type == FORMULA || g_fractal_type == FFORMULA)
	{
		g_help_mode = HT_FORMULA;
		if (get_file_entry(GETFILE_FORMULA, "Formula", formmask, g_formula_filename, g_formula_name) < 0)
		{
			ret = 1;
			goto sel_type_exit;
			}
		}
	if (g_fractal_type == IFS || g_fractal_type == IFS3D)
	{
		g_help_mode = HT_IFS;
		if (get_file_entry(GETFILE_IFS, "IFS", ifsmask, g_ifs_filename, g_ifs_name) < 0)
		{
		ret = 1;
		goto sel_type_exit;
		}
		}

/* Added the following to accommodate fn bifurcations.  JCO 7/2/92 */
	if (((g_fractal_type == BIFURCATION) || (g_fractal_type == LBIFURCATION)) &&
			!((oldfractype == BIFURCATION) || (oldfractype == LBIFURCATION)))
		set_trig_array(0, "ident");
	if (((g_fractal_type == BIFSTEWART) || (g_fractal_type == LBIFSTEWART)) &&
			!((oldfractype == BIFSTEWART) || (oldfractype == LBIFSTEWART)))
		set_trig_array(0, "ident");
	if (((g_fractal_type == BIFLAMBDA) || (g_fractal_type == LBIFLAMBDA)) &&
			!((oldfractype == BIFLAMBDA) || (oldfractype == LBIFLAMBDA)))
		set_trig_array(0, "ident");
	if (((g_fractal_type == BIFEQSINPI) || (g_fractal_type == LBIFEQSINPI)) &&
			!((oldfractype == BIFEQSINPI) || (oldfractype == LBIFEQSINPI)))
		set_trig_array(0, "sin");
	if (((g_fractal_type == BIFADSINPI) || (g_fractal_type == LBIFADSINPI)) &&
			!((oldfractype == BIFADSINPI) || (oldfractype == LBIFADSINPI)))
		set_trig_array(0, "sin");

	/*
	* Next assumes that user going between popcorn and popcornjul
	* might not want to change function variables
	*/
	if (((g_fractal_type    == FPPOPCORN) || (g_fractal_type    == LPOPCORN) ||
			(g_fractal_type    == FPPOPCORNJUL) || (g_fractal_type    == LPOPCORNJUL)) &&
			!((oldfractype == FPPOPCORN) || (oldfractype == LPOPCORN) ||
			(oldfractype == FPPOPCORNJUL) || (oldfractype == LPOPCORNJUL)))
		set_function_parm_defaults();

	/* set LATOO function defaults */
	if (g_fractal_type == LATOO && oldfractype != LATOO)
	{
		set_function_parm_defaults();
	}
	set_default_parms();

	if (get_fractal_parameters(0) < 0)
	{
		if (g_fractal_type == FORMULA || g_fractal_type == FFORMULA ||
				g_fractal_type == IFS || g_fractal_type == IFS3D ||
				g_fractal_type == LSYSTEM)
		{
			goto sel_type_restart;
		}
		else
		{
			ret = 1;
		}
	}
	else
	{
		if (newfractype != oldfractype)
		{
			g_invert = 0;
			g_inversion[0] = g_inversion[1] = g_inversion[2] = 0;
		}
	}

sel_type_exit:
	g_help_mode = oldhelpmode;
	return ret;
}

void set_default_parms()
{
	int i, extra;
	g_xx_min = g_current_fractal_specific->x_min;
	g_xx_max = g_current_fractal_specific->x_max;
	g_yy_min = g_current_fractal_specific->y_min;
	g_yy_max = g_current_fractal_specific->y_max;
	g_xx_3rd = g_xx_min;
	g_yy_3rd = g_yy_min;

	if (g_view_crop && g_final_aspect_ratio != g_screen_aspect_ratio)
	{
		aspect_ratio_crop(g_screen_aspect_ratio, g_final_aspect_ratio);
	}
	for (i = 0; i < 4; i++)
	{
		g_parameters[i] = g_current_fractal_specific->paramvalue[i];
		if (g_fractal_type != CELLULAR && g_fractal_type != FROTH && g_fractal_type != FROTHFP &&
				g_fractal_type != ANT)
			round_float_d(&g_parameters[i]); /* don't round cellular, frothybasin or ant */
	}
	extra = find_extra_parameter(g_fractal_type);
	if (extra > -1)
	{
		for (i = 0; i < MAX_PARAMETERS-4; i++)
		{
			g_parameters[i + 4] = g_more_parameters[extra].paramvalue[i];
		}
	}
	if (g_debug_flag != DEBUGFLAG_NO_BIG_TO_FLOAT)
	{
		bf_math = 0;
	}
	else if (bf_math)
	{
		fractal_float_to_bf();
	}
}

#define MAXFRACTALS 25

int build_fractal_list(int fractals[], int *last_val, char *nameptr[])
{
	int numfractals, i;

	numfractals = 0;
	for (i = 0; i < g_num_fractal_types; i++)
	{
		if ((g_fractal_specific[i].flags & OKJB) && *g_fractal_specific[i].name != '*')
		{
			fractals[numfractals] = i;
			if (i == g_new_orbit_type || i == g_fractal_specific[g_new_orbit_type].tofloat)
			{
				*last_val = numfractals;
			}
			nameptr[numfractals] = g_fractal_specific[i].name;
			numfractals++;
			if (numfractals >= MAXFRACTALS)
			{
				break;
			}
		}
	}
	return numfractals;
}

char *g_juli_3d_options[] = {"monocular", "lefteye", "righteye", "red-blue"};

/* JIIM */
#ifdef RANDOM_RUN
static char JIIMstr1[] = "Breadth first, Depth first, Random Walk, Random Run?";
char *g_jiim_method[] = {"breadth", "depth", "walk", "run"};
#else
static char JIIMstr1[] = "Breadth first, Depth first, Random Walk";
char *g_jiim_method[] = {"breadth", "depth", "walk"};
#endif
static char JIIMstr2[] = "Left first or Right first?";
char *g_jiim_left_right[] = {"left", "right"};

/* moved from miscres.c so sizeof structure can be accessed here */
struct trig_funct_lst trigfn[] =
/* changing the order of these alters meaning of *.fra file */
/* maximum 6 characters in function names or recheck all related code */
{
#if !defined(XFRACT) && !defined(_WIN32)
	{"sin",   lStkSin,   dStkSin,   mStkSin   },
	{"cosxx", lStkCosXX, dStkCosXX, mStkCosXX },
	{"sinh",  lStkSinh,  dStkSinh,  mStkSinh  },
	{"cosh",  lStkCosh,  dStkCosh,  mStkCosh  },
	{"exp",   lStkExp,   dStkExp,   mStkExp   },
	{"log",   lStkLog,   dStkLog,   mStkLog   },
	{"sqr",   lStkSqr,   dStkSqr,   mStkSqr   },
	{"recip", lStkRecip, dStkRecip, mStkRecip }, /* from recip on new in v16 */
	{"ident", StkIdent,  StkIdent,  StkIdent  },
	{"cos",   lStkCos,   dStkCos,   mStkCos   },
	{"tan",   lStkTan,   dStkTan,   mStkTan   },
	{"tanh",  lStkTanh,  dStkTanh,  mStkTanh  },
	{"cotan", lStkCoTan, dStkCoTan, mStkCoTan },
	{"cotanh", lStkCoTanh, dStkCoTanh, mStkCoTanh},
	{"flip",  lStkFlip,  dStkFlip,  mStkFlip  },
	{"conj",  lStkConj,  dStkConj,  mStkConj  },
	{"zero",  lStkZero,  dStkZero,  mStkZero  },
	{"asin",  lStkASin,  dStkASin,  mStkASin  },
	{"asinh", lStkASinh, dStkASinh, mStkASinh },
	{"acos",  lStkACos,  dStkACos,  mStkACos  },
	{"acosh", lStkACosh, dStkACosh, mStkACosh },
	{"atan",  lStkATan,  dStkATan,  mStkATan  },
	{"atanh", lStkATanh, dStkATanh, mStkATanh },
	{"cabs",  lStkCAbs,  dStkCAbs,  mStkCAbs  },
	{"abs",   lStkAbs,   dStkAbs,   mStkAbs   },
	{"sqrt",  lStkSqrt,  dStkSqrt,  mStkSqrt  },
	{"floor", lStkFloor, dStkFloor, mStkFloor },
	{"ceil",  lStkCeil,  dStkCeil,  mStkCeil  },
	{"trunc", lStkTrunc, dStkTrunc, mStkTrunc },
	{"round", lStkRound, dStkRound, mStkRound },
	{"one",   lStkOne,   dStkOne,   mStkOne   },
#else
	{"sin",   dStkSin,   dStkSin,   dStkSin   },
	{"cosxx", dStkCosXX, dStkCosXX, dStkCosXX },
	{"sinh",  dStkSinh,  dStkSinh,  dStkSinh  },
	{"cosh",  dStkCosh,  dStkCosh,  dStkCosh  },
	{"exp",   dStkExp,   dStkExp,   dStkExp   },
	{"log",   dStkLog,   dStkLog,   dStkLog   },
	{"sqr",   dStkSqr,   dStkSqr,   dStkSqr   },
	{"recip", dStkRecip, dStkRecip, dStkRecip }, /* from recip on new in v16 */
	{"ident", StkIdent,  StkIdent,  StkIdent  },
	{"cos",   dStkCos,   dStkCos,   dStkCos   },
	{"tan",   dStkTan,   dStkTan,   dStkTan   },
	{"tanh",  dStkTanh,  dStkTanh,  dStkTanh  },
	{"cotan", dStkCoTan, dStkCoTan, dStkCoTan },
	{"cotanh", dStkCoTanh, dStkCoTanh, dStkCoTanh},
	{"flip",  dStkFlip,  dStkFlip,  dStkFlip  },
	{"conj",  dStkConj,  dStkConj,  dStkConj  },
	{"zero",  dStkZero,  dStkZero,  dStkZero  },
	{"asin",  dStkASin,  dStkASin,  dStkASin  },
	{"asinh", dStkASinh, dStkASinh, dStkASinh },
	{"acos",  dStkACos,  dStkACos,  dStkACos  },
	{"acosh", dStkACosh, dStkACosh, dStkACosh },
	{"atan",  dStkATan,  dStkATan,  dStkATan  },
	{"atanh", dStkATanh, dStkATanh, dStkATanh },
	{"cabs",  dStkCAbs,  dStkCAbs,  dStkCAbs  },
	{"abs",   dStkAbs,   dStkAbs,   dStkAbs   },
	{"sqrt",  dStkSqrt,  dStkSqrt,  dStkSqrt  },
	{"floor", dStkFloor, dStkFloor, dStkFloor },
	{"ceil",  dStkCeil,  dStkCeil,  dStkCeil  },
	{"trunc", dStkTrunc, dStkTrunc, dStkTrunc },
	{"round", dStkRound, dStkRound, dStkRound },
	{"one",   dStkOne,   dStkOne,   dStkOne   },
#endif
};

#define NUMTRIGFN  sizeof(trigfn)/sizeof(struct trig_funct_lst)

const int g_num_trig_fn = NUMTRIGFN;

/* --------------------------------------------------------------------- */
int get_fractal_parameters(int caller)        /* prompt for type-specific parms */
{
	char *v0 = "From cx (real part)";
	char *v1 = "From cy (imaginary part)";
	char *v2 = "To   cx (real part)";
	char *v3 = "To   cy (imaginary part)";
	char *juliorbitname = NULL;
	int i, j, k;
	int curtype, numparams, numtrig;
	struct full_screen_values paramvalues[30];
	char *choices[30];
	long oldbailout = 0L;
	int promptnum;
	char msg[120];
	char *type_name, *tmpptr;
	char bailoutmsg[50];
	int ret = 0;
	int oldhelpmode;
	char parmprompt[MAX_PARAMETERS][55];
	static char *trg[] =
	{
		"First Function", "Second Function", "Third Function", "Fourth Function"
	};
	char *filename, *entryname;
	FILE *entryfile;
	char *trignameptr[NUMTRIGFN];
#ifdef XFRACT
	static /* Can't initialize aggregates on the stack */
#endif
	char *bailnameptr[] = {"mod", "real", "imag", "or", "and", "manh", "manr"};
	struct fractal_specific_stuff *jborbit = NULL;
	struct fractal_specific_stuff *savespecific;
	int firstparm = 0;
	int lastparm  = MAX_PARAMETERS;
	double oldparam[MAX_PARAMETERS];
	int fkeymask = 0x40;
	oldbailout = g_bail_out;
	if (g_fractal_type == JULIBROT || g_fractal_type == JULIBROTFP)
	{
		g_julibrot = TRUE;
	}
	else
	{
		g_julibrot = FALSE;
	}
	curtype = g_fractal_type;
	if (g_current_fractal_specific->name[0] == '*'
			&& (i = g_current_fractal_specific->tofloat) != NOFRACTAL  /* FIXED BUG HERE!! */
			&& g_fractal_specific[i].name[0] != '*')
		curtype = i;
	g_current_fractal_specific = &g_fractal_specific[curtype];
	g_text_stack[0] = 0;
	i = g_current_fractal_specific->helpformula;
	if (i < -1)
	{
		int itemtype = ITEMTYPE_PARAMETER;
		if (i == -2)  /* special for formula */
		{
			filename = g_formula_filename;
			entryname = g_formula_name;
			itemtype = ITEMTYPE_FORMULA;
		}
		else if (i == -3)   /* special for lsystem */
		{
			filename = g_l_system_filename;
			entryname = g_l_system_name;
			itemtype = ITEMTYPE_L_SYSTEM;
		}
		else if (i == -4)   /* special for ifs */
		{
			filename = g_ifs_filename;
			entryname = g_ifs_name;
			itemtype = ITEMTYPE_IFS;
		}
		else  /* this shouldn't happen */
		{
#if defined(_WIN32)
			_ASSERTE(FALSE);
#endif
			filename = NULL;
			entryname = NULL;
		}
		if (find_file_item(filename, entryname, &entryfile, itemtype) == 0)
		{
			load_entry_text(entryfile, g_text_stack, 17, 0, 0);
			fclose(entryfile);
			if (g_fractal_type == FORMULA || g_fractal_type == FFORMULA)
			{
				frm_get_param_stuff(entryname); /* no error check, should be okay, from above */
			}
		}
	}
	else if (i >= 0)
	{
		int c, lines;
		read_help_topic(i, 0, 2000, g_text_stack); /* need error handling here ?? */
		g_text_stack[2000-i] = 0;
		i = j = lines = 0;
		k = 1;
		while ((c = g_text_stack[i++]) != 0)
		{
			/* stop at ctl, blank, or line with col 1 nonblank, max 16 lines */
			if (k && c == ' ' && ++k <= 5)
			{
			} /* skip 4 blanks at start of line */
			else
			{
				if (c == '\n')
				{
					if (k) /* blank line  */
					{
						break;
					}
					if (++lines >= 16)
					{
						break;
					}
					k = 1;
				}
				else if (c < 16) /* a special help format control char */
				{
					break;
				}
				else
				{
					if (k == 1) /* line starts in column 1 */
					{
						break;
					}
					k = 0;
				}
				g_text_stack[j++] = (char)c;
			}
		}
		while (--j >= 0 && g_text_stack[j] == '\n')
		{
		}
		g_text_stack[j + 1] = 0;
	}

gfp_top:
	promptnum = 0;
	if (g_julibrot)
	{
		i = select_fracttype(g_new_orbit_type);
		if (i < 0)
		{
			if (ret == 0)
			{
				ret = -1;
			}
			g_julibrot = FALSE;
			goto gfp_exit;
		}
		else
		{
			g_new_orbit_type = i;
		}
		jborbit = &g_fractal_specific[g_new_orbit_type];
		juliorbitname = jborbit->name;
	}

	if (g_fractal_type == FORMULA || g_fractal_type == FFORMULA)
	{
		if (g_uses_p1)  /* set first parameter */
		{
			firstparm = 0;
		}
		else if (g_uses_p2)
		{
			firstparm = 2;
		}
		else if (g_uses_p3)
		{
			firstparm = 4;
		}
		else if (g_uses_p4)
		{
			firstparm = 6;
		}
		else
		{
			firstparm = 8; /* g_uses_p5 or no parameter */
		}

		if (g_uses_p5)  /* set last parameter */
		{
			lastparm = 10;
		}
		else if (g_uses_p4)
		{
			lastparm = 8;
		}
		else if (g_uses_p3)
		{
			lastparm = 6;
		}
		else if (g_uses_p2)
		{
			lastparm = 4;
		}
		else
		{
			lastparm = 2; /* g_uses_p1 or no parameter */
		}
	}

	savespecific = g_current_fractal_specific;
	if (g_julibrot)
	{
		g_current_fractal_specific = jborbit;
		firstparm = 2; /* in most case Julibrot does not need first two parms */
		if (g_new_orbit_type == QUATJULFP     ||   /* all parameters needed */
			g_new_orbit_type == HYPERCMPLXJFP)
		{
			firstparm = 0;
			lastparm = 4;
		}
		if (g_new_orbit_type == QUATFP        ||   /* no parameters needed */
			g_new_orbit_type == HYPERCMPLXFP)
		{
			firstparm = 4;
		}
	}
	numparams = 0;
	j = 0;
	for (i = firstparm; i < lastparm; i++)
	{
		char tmpbuf[30];
		if (!type_has_parameter(g_julibrot ? g_new_orbit_type : g_fractal_type, i, parmprompt[j]))
		{
			if (curtype == FORMULA || curtype == FFORMULA)
			{
				if (parameter_not_used(i))
				{
					continue;
				}
			}
			break;
		}
		numparams++;
		choices[promptnum] = parmprompt[j++];
		paramvalues[promptnum].type = 'd';

		if (choices[promptnum][0] == '+')
		{
			choices[promptnum]++;
			paramvalues[promptnum].type = 'D';
		}
		else if (choices[promptnum][0] == '#')
		{
			choices[promptnum]++;
		}
		sprintf(tmpbuf, "%.17g", g_parameters[i]);
		paramvalues[promptnum].uval.dval = atof(tmpbuf);
		oldparam[i] = paramvalues[promptnum++].uval.dval;
	}

	/* The following is a goofy kludge to make reading in the formula
	* parameters work.
	*/
	if (curtype == FORMULA || curtype == FFORMULA)
	{
		numparams = lastparm - firstparm;
	}

	numtrig = (g_current_fractal_specific->flags >> 6) & 7;
	if (curtype == FORMULA || curtype == FFORMULA)
	{
		numtrig = g_max_fn;
	}

	i = NUMTRIGFN;
	while (--i >= 0)
	{
		trignameptr[i] = trigfn[i].name;
	}
	for (i = 0; i < numtrig; i++)
	{
		paramvalues[promptnum].type = 'l';
		paramvalues[promptnum].uval.ch.val  = g_trig_index[i];
		paramvalues[promptnum].uval.ch.llen = NUMTRIGFN;
		paramvalues[promptnum].uval.ch.vlen = 6;
		paramvalues[promptnum].uval.ch.list = trignameptr;
		choices[promptnum++] = (char *)trg[i];
	}
	type_name = g_current_fractal_specific->name;
	if (*type_name == '*')
	{
		++type_name;
	}

	i = g_current_fractal_specific->orbit_bailout;

	if (i != 0 && g_current_fractal_specific->calculate_type == standard_fractal &&
		(g_current_fractal_specific->flags & BAILTEST))
	{
		paramvalues[promptnum].type = 'l';
		paramvalues[promptnum].uval.ch.val  = (int)g_bail_out_test;
		paramvalues[promptnum].uval.ch.llen = 7;
		paramvalues[promptnum].uval.ch.vlen = 6;
		paramvalues[promptnum].uval.ch.list = bailnameptr;
		choices[promptnum++] = "Bailout Test (mod, real, imag, or, and, manh, manr)";
	}

	if (i)
	{
		if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != 0.0)
		{
			paramvalues[promptnum].type = '*';
			choices[promptnum++] = "Bailout: continuous potential (Y screen) value in use";
		}
		else
		{
			choices[promptnum] = "Bailout value (0 means use default)";
			paramvalues[promptnum].type = 'L';
			oldbailout = g_bail_out;
			paramvalues[promptnum++].uval.Lval = oldbailout;
			paramvalues[promptnum].type = '*';
			tmpptr = type_name;
			if (g_user_biomorph != -1)
			{
				i = 100;
				tmpptr = "biomorph";
			}
			sprintf(bailoutmsg, "    (%s default is %d)", tmpptr, i);
			choices[promptnum++] = bailoutmsg;
		}
	}
	if (g_julibrot)
	{
		switch (g_new_orbit_type)
		{
		case QUATFP:
		case HYPERCMPLXFP:
			v0 = "From cj (3rd dim)";
			v1 = "From ck (4th dim)";
			v2 = "To   cj (3rd dim)";
			v3 = "To   ck (4th dim)";
			break;
		case QUATJULFP:
		case HYPERCMPLXJFP:
			v0 = "From zj (3rd dim)";
			v1 = "From zk (4th dim)";
			v2 = "To   zj (3rd dim)";
			v3 = "To   zk (4th dim)";
			break;
		default:
			v0 = "From cx (real part)";
			v1 = "From cy (imaginary part)";
			v2 = "To   cx (real part)";
			v3 = "To   cy (imaginary part)";
			break;
		}

		g_current_fractal_specific = savespecific;
		paramvalues[promptnum].uval.dval = g_m_x_max_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = v0;
		paramvalues[promptnum].uval.dval = g_m_y_max_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = v1;
		paramvalues[promptnum].uval.dval = g_m_x_min_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = v2;
		paramvalues[promptnum].uval.dval = g_m_y_min_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = v3;
		paramvalues[promptnum].uval.ival = g_z_dots;
		paramvalues[promptnum].type = 'i';
		choices[promptnum++] = "Number of z pixels";

		paramvalues[promptnum].type = 'l';
		paramvalues[promptnum].uval.ch.val  = g_juli_3d_mode;
		paramvalues[promptnum].uval.ch.llen = 4;
		paramvalues[promptnum].uval.ch.vlen = 9;
		paramvalues[promptnum].uval.ch.list = g_juli_3d_options;
		choices[promptnum++] = "3D Mode";

		paramvalues[promptnum].uval.dval = g_eyes_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = "Distance between eyes";
		paramvalues[promptnum].uval.dval = g_origin_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = "Location of z origin";
		paramvalues[promptnum].uval.dval = g_depth_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = "Depth of z";
		paramvalues[promptnum].uval.dval = g_height_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = "Screen height";
		paramvalues[promptnum].uval.dval = g_width_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = "Screen width";
		paramvalues[promptnum].uval.dval = g_screen_distance_fp;
		paramvalues[promptnum].type = 'f';
		choices[promptnum++] = "Distance to Screen";
	}

	if (curtype == INVERSEJULIA || curtype == INVERSEJULIAFP)
	{
		choices[promptnum] = JIIMstr1;
		paramvalues[promptnum].type = 'l';
		paramvalues[promptnum].uval.ch.list = g_jiim_method;
		paramvalues[promptnum].uval.ch.vlen = 7;
#ifdef RANDOM_RUN
		paramvalues[promptnum].uval.ch.llen = 4;
#else
		paramvalues[promptnum].uval.ch.llen = 3; /* disable random run */
#endif
		paramvalues[promptnum++].uval.ch.val  = g_major_method;

		choices[promptnum] = JIIMstr2;
		paramvalues[promptnum].type = 'l';
		paramvalues[promptnum].uval.ch.list = g_jiim_left_right;
		paramvalues[promptnum].uval.ch.vlen = 5;
		paramvalues[promptnum].uval.ch.llen = 2;
		paramvalues[promptnum++].uval.ch.val  = g_minor_method;
	}

	if ((curtype == FORMULA || curtype == FFORMULA) && g_uses_is_mand)
	{
		choices[promptnum] = "ismand";
		paramvalues[promptnum].type = 'y';
		paramvalues[promptnum++].uval.ch.val = g_is_mand?1:0;
	}

	if (caller                           /* <z> command ? */
/*      && (g_display_3d > 0 || promptnum == 0)) */
		&& (g_display_3d > 0))
	{
		stop_message(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Current type has no type-specific parameters");
		goto gfp_exit;
	}
	if (g_julibrot)
	{
		sprintf(msg, "Julibrot Parameters (orbit= %s)", juliorbitname);
	}
	else
	{
		sprintf(msg, "Parameters for fractal type %s", type_name);
	}
	if (bf_math == 0)
	{
		strcat(msg, "\n(Press "FK_F6" for corner parameters)");
	}
	else
	{
		fkeymask = 0;
	}
	scroll_row_status = 0; /* make sure we start at beginning of entry */
	scroll_column_status = 0;
	while (1)
	{
		oldhelpmode = g_help_mode;
		g_help_mode = g_current_fractal_specific->helptext;
		i = full_screen_prompt(msg, promptnum, choices, paramvalues, fkeymask, g_text_stack);
		g_help_mode = oldhelpmode;
		if (i < 0)
		{
			if (g_julibrot)
			{
				goto gfp_top;
			}
			if (ret == 0)
			{
				ret = -1;
			}
			goto gfp_exit;
		}
		if (i != FIK_F6)
		{
			break;
		}
		if (bf_math == 0)
		{
			if (get_corners() > 0)
			{
				ret = 1;
			}
		}
	}
	promptnum = 0;
	for (i = firstparm; i < numparams + firstparm; i++)
	{
		if (curtype == FORMULA || curtype == FFORMULA)
		{
			if (parameter_not_used(i))
			{
				continue;
			}
		}
		if (oldparam[i] != paramvalues[promptnum].uval.dval)
		{
			g_parameters[i] = paramvalues[promptnum].uval.dval;
			ret = 1;
		}
		++promptnum;
	}

	for (i = 0; i < numtrig; i++)
	{
		if (paramvalues[promptnum].uval.ch.val != (int)g_trig_index[i])
		{
			set_trig_array(i, trigfn[paramvalues[promptnum].uval.ch.val].name);
			ret = 1;
		}
		++promptnum;
	}

	if (g_julibrot)
	{
		savespecific = g_current_fractal_specific;
		g_current_fractal_specific = jborbit;
	}

	i = g_current_fractal_specific->orbit_bailout;

	if (i != 0 && g_current_fractal_specific->calculate_type == standard_fractal &&
		(g_current_fractal_specific->flags & BAILTEST))
	{
		if (paramvalues[promptnum].uval.ch.val != (int)g_bail_out_test)
		{
			g_bail_out_test = (enum bailouts)paramvalues[promptnum].uval.ch.val;
			ret = 1;
		}
		promptnum++;
	}
	else
	{
		g_bail_out_test = Mod;
	}
	set_bail_out_formula(g_bail_out_test);

	if (i)
	{
		if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != 0.0)
		{
			promptnum++;
		}
		else
		{
			g_bail_out = paramvalues[promptnum++].uval.Lval;
			if (g_bail_out != 0 && (g_bail_out < 1 || g_bail_out > 2100000000L))
			{
				g_bail_out = oldbailout;
			}
			if (g_bail_out != oldbailout)
			{
				ret = 1;
			}
			promptnum++;
		}
	}
	if (g_julibrot)
	{
		g_m_x_max_fp    = paramvalues[promptnum++].uval.dval;
		g_m_y_max_fp    = paramvalues[promptnum++].uval.dval;
		g_m_x_min_fp    = paramvalues[promptnum++].uval.dval;
		g_m_y_min_fp    = paramvalues[promptnum++].uval.dval;
		g_z_dots      = paramvalues[promptnum++].uval.ival;
		g_juli_3d_mode = paramvalues[promptnum++].uval.ch.val;
		g_eyes_fp     = (float)paramvalues[promptnum++].uval.dval;
		g_origin_fp   = (float)paramvalues[promptnum++].uval.dval;
		g_depth_fp    = (float)paramvalues[promptnum++].uval.dval;
		g_height_fp   = (float)paramvalues[promptnum++].uval.dval;
		g_width_fp    = (float)paramvalues[promptnum++].uval.dval;
		g_screen_distance_fp     = (float)paramvalues[promptnum++].uval.dval;
		ret = 1;  /* force new calc since not resumable anyway */
	}
	if (curtype == INVERSEJULIA || curtype == INVERSEJULIAFP)
	{
		if (paramvalues[promptnum].uval.ch.val != g_major_method ||
				paramvalues[promptnum + 1].uval.ch.val != g_minor_method)
			ret = 1;
		g_major_method = (enum Major)paramvalues[promptnum++].uval.ch.val;
		g_minor_method = (enum Minor)paramvalues[promptnum++].uval.ch.val;
	}
	if ((curtype == FORMULA || curtype == FFORMULA) && g_uses_is_mand)
	{
		if (g_is_mand != (short int)paramvalues[promptnum].uval.ch.val)
		{
			g_is_mand = (short int)paramvalues[promptnum].uval.ch.val;
			ret = 1;
		}
		++promptnum;
	}

gfp_exit:
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	return ret;
}

int find_extra_parameter(int type)
{
	int i, ret, curtyp;
	ret = -1;
	i = -1;

	if (g_fractal_specific[type].flags&MORE)
	{
		do
		{
			curtyp = g_more_parameters[++i].type;
		}
		while (curtyp != type && curtyp != -1);
		if (curtyp == type)
		{
			ret = i;
		}
	}
	return ret;
}

void load_parameters(int g_fractal_type)
{
	int i, extra;
	for (i = 0; i < 4; ++i)
	{
		g_parameters[i] = g_fractal_specific[g_fractal_type].paramvalue[i];
		if (g_fractal_type != CELLULAR && g_fractal_type != ANT)
		{
			round_float_d(&g_parameters[i]); /* don't round cellular or ant */
		}
	}
	extra = find_extra_parameter(g_fractal_type);
	if (extra > -1)
	{
		for (i = 0; i < MAX_PARAMETERS-4; i++)
		{
			g_parameters[i + 4] = g_more_parameters[extra].paramvalue[i];
		}
	}
}

int check_orbit_name(char *orbitname)
{
	int i, numtypes, bad;
	char *nameptr[MAXFRACTALS];
	int fractals[MAXFRACTALS];
	int last_val;

	numtypes = build_fractal_list(fractals, &last_val, nameptr);
	bad = 1;
	for (i = 0; i < numtypes; i++)
	{
		if (strcmp(orbitname, nameptr[i]) == 0)
		{
			g_new_orbit_type = fractals[i];
			bad = 0;
			break;
		}
	}
	return bad;
}

/* --------------------------------------------------------------------- */

static FILE *gfe_file;

long get_file_entry(int type, char *title, char *fmask,
					char *filename, char *entryname)
{
	/* Formula, LSystem, etc type structure, select from file */
	/* containing definitions in the form    name { ... }     */
	int newfile, firsttry;
	long entry_pointer;
	newfile = 0;
	while (1)
	{
		firsttry = 0;
		/* pb: binary mode used here - it is more work, but much faster, */
		/*     especially when ftell or fgetpos is used                  */
		while (newfile || (gfe_file = fopen(filename, "rb")) == NULL)
		{
			char buf[60];
			newfile = 0;
			if (firsttry)
			{
				char message[256];
				sprintf(message, "Can't find %s", filename);
				stop_message(0, message);
			}
			sprintf(buf, "Select %s File", title);
			if (get_a_filename(buf, fmask, filename) < 0)
			{
				return -1;
			}

			firsttry = 1; /* if around open loop again it is an error */
		}
		setvbuf(gfe_file, g_text_stack, _IOFBF, 4096); /* improves speed when file is big */
		newfile = 0;
		entry_pointer = gfe_choose_entry(type, title, filename, entryname);
		if (entry_pointer == -2)
		{
			newfile = 1; /* go to file list, */
			continue;    /* back to get_a_filename */
		}
		if (entry_pointer == -1)
		{
			return -1;
		}
		switch (type)
		{
		case GETFILE_FORMULA:
			if (RunForm(entryname, 1) == 0)
			{
				return 0;
			}
			break;
		case GETFILE_L_SYSTEM:
			if (l_load() == 0)
			{
				return 0;
			}
			break;
		case GETFILE_IFS:
			if (ifs_load() == 0)
			{
				g_fractal_type = (g_ifs_type == IFSTYPE_2D) ? IFS : IFS3D;
				g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
				set_default_parms(); /* to correct them if 3d */
				return 0;
			}
			break;
		case GETFILE_PARAMETER:
			return entry_pointer;
		}
	}
}

static struct entryinfo **gfe_choices; /* for format_getparm_line */
static char *gfe_title;

/* skip to next non-white space character and return it */
static int skip_white_space(FILE *infile, long *file_offset)
{
	int c;
	do
	{
		c = getc(infile);
		(*file_offset)++;
	}
	while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
	return c;
}

/* skip to end of line */
int skip_comment(FILE *infile, long *file_offset)
{
	int c;
	do
	{
		c = getc(infile);
		(*file_offset)++;
	}
	while (c != '\n' && c != '\r' && c != EOF && c != '\032');
	return c;
}

#define MAXENTRIES 2000L

int scan_entries(FILE *infile, struct entryinfo *choices, char *itemname)
{
		/*
		function returns the number of entries found; if a
		specific entry is being looked for, returns -1 if
		the entry is found, 0 otherwise.
		*/
	char buf[101];
	int exclude_entry;
	long name_offset, temp_offset;   /*rev 5/23/96 to add temp_offset,
                                      used below to skip any '{' that
                                      does not have a corresponding
                                      '}' - GGM */
	long file_offset = -1;
	int numentries = 0;

	while (1)
	{                            /* scan the file for entry names */
		int c, len;
top:
		c = skip_white_space(infile, &file_offset);
		if (c == ';')
		{
			c = skip_comment(infile, &file_offset);
			if (c == EOF || c == '\032')
			{
				break;
			}
			continue;
		}
		name_offset = temp_offset = file_offset;
		/* next equiv roughly to fscanf(.., "%40[^* \n\r\t({\032]", buf) */
		len = 0;
		/* allow spaces in entry names in next JCO 9/2/2003 */
		while (c != ' ' && c != '\t' && c != '(' && c != ';'
			&& c != '{' && c != '\n' && c != '\r' && c != EOF && c != '\032')
		{
			if (len < 40)
			{
				buf[len++] = (char) c;
			}
			c = getc(infile);
			++file_offset;
			if (c == '\n' || c == '\r')
			{
				goto top;
			}
		}
		buf[len] = 0;
		while (c != '{' &&  c != EOF && c != '\032')
		{
			if (c == ';')
			{
				c = skip_comment(infile, &file_offset);
			}
			else
			{
				c = getc(infile);
				++file_offset;
				if (c == '\n' || c == '\r')
				{
					goto top;
				}
			}
		}
		if (c == '{')
		{
			while (c != '}' && c != EOF && c != '\032')
			{
				if (c == ';')
				{
					c = skip_comment(infile, &file_offset);
				}
				else
				{
					if (c == '\n' || c == '\r')     /* reset temp_offset to  */
					{
						temp_offset = file_offset;  /* beginning of new line */
					}
					c = getc(infile);
					++file_offset;
				}
				if (c == '{') /*second '{' found*/
				{
					if (temp_offset == name_offset) /*if on same line, skip line*/
					{
						c = skip_comment(infile, &file_offset);
						goto top;
					}
					else
					{
						fseek(infile, temp_offset, SEEK_SET); /*else, go back to */
						file_offset = temp_offset - 1;        /*beginning of line*/
						goto top;
					}
				}
			}
			if (c != '}')   /* i.e. is EOF or '\032'*/
			{
				break;
			}

			if (strnicmp(buf, "frm:", 4) == 0 ||
					strnicmp(buf, "ifs:", 4) == 0 ||
					strnicmp(buf, "par:", 4) == 0)
				exclude_entry = 4;
			else if (strnicmp(buf, "lsys:", 5) == 0)
			{
				exclude_entry = 5;
			}
			else
			{
				exclude_entry = 0;
			}

			buf[ITEMNAMELEN + exclude_entry] = 0;
			if (itemname != NULL)  /* looking for one entry */
			{
				if (stricmp(buf, itemname) == 0)
				{
					fseek(infile, name_offset + (long) exclude_entry, SEEK_SET);
					return -1;
				}
			}
			else /* make a whole list of entries */
			{
				if (buf[0] != 0 && stricmp(buf, "comment") != 0 && !exclude_entry)
				{
					strcpy(choices[numentries].name, buf);
					choices[numentries].point = name_offset;
					if (++numentries >= MAXENTRIES)
					{
						sprintf(buf, "Too many entries in file, first %ld used", MAXENTRIES);
						stop_message(0, buf);
						break;
					}
				}
			}
		}
		else if (c == EOF || c == '\032')
		{
			break;
		}
	}
	return numentries;
}

/* subrtn of get_file_entry, separated so that storage gets freed up */
static long gfe_choose_entry(int type, char *title, char *filename, char *entryname)
{
#ifdef XFRACT
	char *o_instr = "Press "FK_F6" to select file, "FK_F2" for details, "FK_F4" to toggle sort ";
	/* keep the above line length < 80 characters */
#else
	char *o_instr = "Press "FK_F6" to select different file, "FK_F2" for details, "FK_F4" to toggle sort ";
#endif
	int numentries, i;
	char buf[101];
	struct entryinfo storage[MAXENTRIES + 1];
	struct entryinfo *choices[MAXENTRIES + 1] = { NULL };
	int attributes[MAXENTRIES + 1] = { 0 };
	void (*formatitem)(int, char *);
	int boxwidth, boxdepth, colwidth;
	char instr[80];
	char temp1[256];

	static int dosort = 1;

	gfe_choices = &choices[0];
	gfe_title = title;

retry:
	for (i = 0; i < MAXENTRIES + 1; i++)
	{
		choices[i] = &storage[i];
		attributes[i] = 1;
	}

	numentries = 0;
	help_title(); /* to display a clue when file big and next is slow */

	numentries = scan_entries(gfe_file, &storage[0], NULL);
	if (numentries == 0)
	{
		stop_message(0, "File doesn't contain any valid entries");
		fclose(gfe_file);
		return -2; /* back to file list */
	}
	strcpy(instr, o_instr);
	if (dosort)
	{
		strcat(instr, "off");
		shell_sort((char *) &choices, numentries, sizeof(struct entryinfo *), lccompare);
	}
	else
	{
		strcat(instr, "on");
	}

	strcpy(buf, entryname); /* preset to last choice made */
	sprintf(temp1, "%s Selection\nFile: %s", title, filename);
	formatitem = NULL;
	boxwidth = colwidth = boxdepth = 0;
	if (type == GETFILE_PARAMETER)
	{
		formatitem = format_parmfile_line;
		boxwidth = 1;
		boxdepth = 16;
		colwidth = 76;
	}

	i = full_screen_choice(CHOICE_INSTRUCTIONS | (dosort ? 0 : CHOICE_NOT_SORTED),
		temp1, NULL, instr, numentries, (char **) choices,
		attributes, boxwidth, boxdepth, colwidth, 0,
		formatitem, buf, NULL, check_gfe_key);
	if (i == -FIK_F4)
	{
		rewind(gfe_file);
		dosort = 1-dosort;
		goto retry;
	}
	fclose(gfe_file);
	if (i < 0)
	{
		/* go back to file list or cancel */
		return (i == -FIK_F6) ? -2 : -1;
	}
	strcpy(entryname, choices[i]->name);
	return choices[i]->point;
}


static int check_gfe_key(int curkey, int choice)
{
	char infhdg[60];
	char infbuf[25*80];
	int in_scrolling_mode = 0; /* 1 if entry doesn't fit available space */
	int top_line = 0;
	int left_column = 0;
	int i;
	int done = 0;
	int rewrite_infbuf = 0;  /* if 1: rewrite the entry portion of screen */
	char blanks[79];         /* used to clear the entry portion of screen */
	memset(blanks, ' ', 78);
	blanks[78] = (char) 0;

	if (curkey == FIK_F6)
	{
		return -FIK_F6;
	}
	if (curkey == FIK_F4)
	{
		return -FIK_F4;
	}
	if (curkey == FIK_F2)
	{
		int widest_entry_line = 0;
		int lines_in_entry = 0;
		int comment = 0;
		int c = 0;
		int widthct = 0;
		fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
		while ((c = fgetc(gfe_file)) != EOF && c != '\032')
		{
			if (c == ';')
			{
				comment = 1;
			}
			else if (c == '\n')
			{
				comment = 0;
				lines_in_entry++;
				widthct =  -1;
			}
			else if (c == '\t')
			{
				widthct += 7 - widthct % 8;
			}
			else if (c == '\r')
			{
				continue;
			}
			if (++widthct > widest_entry_line)
			{
				widest_entry_line = widthct;
			}
			if (c == '}' && !comment)
			{
				lines_in_entry++;
				break;
			}
		}
		if (c == EOF || c == '\032')  /* should never happen */
		{
			fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
			in_scrolling_mode = 0;
		}
		fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
		load_entry_text(gfe_file, infbuf, 17, 0, 0);
		if (lines_in_entry > 17 || widest_entry_line > 74)
		{
			in_scrolling_mode = 1;
		}
		strcpy(infhdg, gfe_title);
		strcat(infhdg, " file entry:\n\n");
		/* ... instead, call help with buffer?  heading added */
		driver_stack_screen();
		help_title();
		driver_set_attr(1, 0, C_GENERAL_MED, 24*80);

		g_text_cbase = 0;
		driver_put_string(2, 1, C_GENERAL_HI, infhdg);
		g_text_cbase = 2; /* left margin is 2 */
		driver_put_string(4, 0, C_GENERAL_MED, infbuf);

		{
		driver_put_string(-1, 0, C_GENERAL_LO,
			"\n\n Use "UPARR1", "DNARR1", "RTARR1", "LTARR1", PgUp, PgDown, Home, and End to scroll text\nAny other key to return to selection list");
		}

		while (!done)
		{
			if (rewrite_infbuf)
			{
				rewrite_infbuf = 0;
				fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
				load_entry_text(gfe_file, infbuf, 17, top_line, left_column);
				for (i = 4; i < (lines_in_entry < 17 ? lines_in_entry + 4 : 21); i++)
				{
					driver_put_string(i, 0, C_GENERAL_MED, blanks);
				}
				driver_put_string(4, 0, C_GENERAL_MED, infbuf);
			}
			i = getakeynohelp();
			if (i == FIK_DOWN_ARROW		|| i == FIK_CTL_DOWN_ARROW
					|| i == FIK_UP_ARROW		|| i == FIK_CTL_UP_ARROW
					|| i == FIK_LEFT_ARROW		|| i == FIK_CTL_LEFT_ARROW
					|| i == FIK_RIGHT_ARROW	|| i == FIK_CTL_RIGHT_ARROW
					|| i == FIK_HOME			|| i == FIK_CTL_HOME
					|| i == FIK_END			|| i == FIK_CTL_END
					|| i == FIK_PAGE_UP		|| i == FIK_CTL_PAGE_UP
					|| i == FIK_PAGE_DOWN		|| i == FIK_CTL_PAGE_DOWN)
			{
				switch (i)
				{
				case FIK_DOWN_ARROW: case FIK_CTL_DOWN_ARROW: /* down one line */
					if (in_scrolling_mode && top_line < lines_in_entry - 17)
					{
						top_line++;
						rewrite_infbuf = 1;
					}
					break;
				case FIK_UP_ARROW: case FIK_CTL_UP_ARROW:  /* up one line */
					if (in_scrolling_mode && top_line > 0)
					{
						top_line--;
						rewrite_infbuf = 1;
					}
					break;
				case FIK_LEFT_ARROW: case FIK_CTL_LEFT_ARROW:  /* left one column */
					if (in_scrolling_mode && left_column > 0)
					{
						left_column--;
						rewrite_infbuf = 1;
					}
					break;
				case FIK_RIGHT_ARROW: case FIK_CTL_RIGHT_ARROW: /* right one column */
					if (in_scrolling_mode && strchr(infbuf, '\021') != NULL)
					{
						left_column++;
						rewrite_infbuf = 1;
					}
					break;
				case FIK_PAGE_DOWN: case FIK_CTL_PAGE_DOWN: /* down 17 lines */
					if (in_scrolling_mode && top_line < lines_in_entry - 17)
					{
						top_line += 17;
						if (top_line > lines_in_entry - 17)
						{
							top_line = lines_in_entry - 17;
						}
						rewrite_infbuf = 1;
					}
					break;
				case FIK_PAGE_UP: case FIK_CTL_PAGE_UP: /* up 17 lines */
					if (in_scrolling_mode && top_line > 0)
					{
						top_line -= 17;
						if (top_line < 0)
						{
							top_line = 0;
						}
						rewrite_infbuf = 1;
					}
					break;
				case FIK_END: case FIK_CTL_END:       /* to end of entry */
					if (in_scrolling_mode)
					{
						top_line = lines_in_entry - 17;
						left_column = 0;
						rewrite_infbuf = 1;
					}
					break;
				case FIK_HOME: case FIK_CTL_HOME:     /* to beginning of entry */
					if (in_scrolling_mode)
					{
						top_line = left_column = 0;
						rewrite_infbuf = 1;
					}
					break;
				default:
					break;
				}
			}
			else
			{
				done = 1;  /* a key other than scrolling key was pressed */
			}
		}
		g_text_cbase = 0;
		driver_hide_text_cursor();
		driver_unstack_screen();
	}
	return 0;
}

static void load_entry_text(
		FILE *entfile,
		char *buf,
		int maxlines,
		int startrow,
		int startcol)
{
    /* Revised 12/14/96 by George Martin. Up to maxlines of an entry
        is copied to *buf starting from row "startrow", and skipping
        characters in each line up to "startcol". The terminating '\n'
        is deleted if maxlines is reached before the end of the entry.
	*/

	int linelen, i;
	int comment = 0;
	int c = 0;
	int tabpos = 7 - (startcol % 8);

	if (maxlines <= 0)  /* no lines to get! */
	{
		*buf = (char) 0;
		return;
	}

		/*move down to starting row*/
	for (i = 0; i < startrow; i++)
	{
		while ((c = fgetc(entfile)) != '\n' && c != EOF && c != '\032')
		{
			if (c == ';')
			{
				comment = 1;
			}
			if (c == '}' && !comment)  /* end of entry before start line */
			{
				break;                 /* this should never happen       */
			}
		}
		if (c == '\n')
		{
			comment = 0;
		}
		else  /* reached end of file or end of entry */
		{
			*buf = (char) 0;
			return;
		}
	}

		/* write maxlines of entry */
	while (maxlines-- > 0)
	{
		comment = linelen = i = c = 0;

		/* skip line up to startcol */
		while (i++ < startcol && (c = fgetc(entfile)) != EOF && c != '\032')
		{
			if (c == ';')
			{
				comment = 1;
			}
			if (c == '}' && !comment)  /*reached end of entry*/
			{
				*buf = (char) 0;
				return;
			}
			if (c == '\r')
			{
				i--;
				continue;
			}
			if (c == '\t')
			{
				i += 7 - (i % 8);
			}
			if (c == '\n')  /*need to insert '\n', even for short lines*/
			{
				*(buf++) = (char)c;
				break;
			}
		}
		if (c == EOF || c == '\032')  /* unexpected end of file */
		{
			*buf = (char) 0;
			return;
		}
		if (c == '\n')       /* line is already completed */
		{
			continue;
		}

		if (i > startcol)  /* can happen because of <tab> character */
		{
			while (i-- > startcol)
			{
				*(buf++) = ' ';
				linelen++;
			}
		}

		/*process rest of line into buf */
		while ((c = fgetc(entfile)) != EOF && c != '\032')
		{
			if (c == ';')
			{
				comment = 1;
			}
			else if (c == '\n' || c == '\r')
			{
				comment = 0;
			}
			if (c != '\r')
			{
				if (c == '\t')
				{
					while ((linelen % 8) != tabpos && linelen < 75)  /* 76 wide max */
					{
						*(buf++) = ' ';
						++linelen;
					}
					c = ' ';
				}
				if (c == '\n')
				{
					*(buf++) = '\n';
					break;
				}
				if (++linelen > 75)
				{
					if (linelen == 76)
					{
						*(buf++) = '\021';
					}
				}
				else
				{
					*(buf++) = (char)c;
				}
				if (c == '}' && !comment)  /*reached end of entry*/
				{
					*(buf) = (char) 0;
					return;
				}
			}
		}
		if (c == EOF || c == '\032')  /* unexpected end of file */
		{
			*buf = (char) 0;
			return;
		}
	}
	if (*(buf-1) == '\n') /* specified that buf will not end with a '\n' */
	{
		buf--;
	}
	*buf = (char) 0;
}

static void format_parmfile_line(int choice, char *buf)
{
	int c, i;
	char line[80];
	fseek(gfe_file, gfe_choices[choice]->point, SEEK_SET);
	do
	{
		c = getc(gfe_file);
	}
	while (c != '{');
	do
	{
		c = getc(gfe_file);
	}
	while (c == ' ' || c == '\t' || c == ';');
	i = 0;
	while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\032')
	{
		line[i++] = (char)((c == '\t') ? ' ' : c);
		c = getc(gfe_file);
	}
	line[i] = 0;
#ifndef XFRACT
	sprintf(buf, "%-20Fs%-56s", gfe_choices[choice]->name, line);
#else
	sprintf(buf, "%-20s%-56s", gfe_choices[choice]->name, line);
#endif
}

/* --------------------------------------------------------------------- */

int get_fractal_3d_parameters() /* prompt for 3D fractal parameters */
{
	int i, k, ret, oldhelpmode;
	struct full_screen_values uvalues[20];
	char *ifs3d_prompts[7] =
	{
		"X-axis rotation in degrees",
		"Y-axis rotation in degrees",
		"Z-axis rotation in degrees",
		"Perspective distance [1 - 999, 0 for no persp]",
		"X shift with perspective (positive = right)",
		"Y shift with perspective (positive = up)",
		"Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)"
	};

	driver_stack_screen();
	k = 0;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = XROT;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = YROT;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = ZROT;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = ZVIEWER;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = XSHIFT;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = YSHIFT;
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = g_glasses_type;

	oldhelpmode = g_help_mode;
	g_help_mode = HELP3DFRACT;
	i = full_screen_prompt("3D Parameters", k, ifs3d_prompts, uvalues, 0, NULL);
	g_help_mode = oldhelpmode;
	if (i < 0)
	{
		ret = -1;
		goto get_f3d_exit;
		}

	ret = k = 0;
	XROT    =  uvalues[k++].uval.ival;
	YROT    =  uvalues[k++].uval.ival;
	ZROT    =  uvalues[k++].uval.ival;
	ZVIEWER =  uvalues[k++].uval.ival;
	XSHIFT  =  uvalues[k++].uval.ival;
	YSHIFT  =  uvalues[k++].uval.ival;
	g_glasses_type = uvalues[k++].uval.ival;
	if (g_glasses_type < 0 || g_glasses_type > STEREO_PAIR)
	{
		g_glasses_type = 0;
	}
	if (g_glasses_type)
	{
		if (get_funny_glasses_params() || check_mapfile())
		{
			ret = -1;
		}
	}

get_f3d_exit:
	driver_unstack_screen();
	return ret;
}

/* --------------------------------------------------------------------- */
/* These macros streamline the "save near space" campaign */

int get_3d_parameters()     /* prompt for 3D parameters */
{
	char *choices[11];
	int attributes[21];
	int sphere;
	char *s;
	char *prompts3d[21];
	struct full_screen_values uvalues[21];
	int i, k;
	int oldhelpmode;

#ifdef WINFRACT
	{
		extern int wintext_textmode;
		if (wintext_textmode != 2)  /* are we in textmode? */
		{
			return 0;              /* no - prompts are already handled */
		}
	}
#endif
restart_1:
	if (g_targa_output && g_overlay_3d)
	{
		g_targa_overlay = 1;
	}

	k = -1;

	prompts3d[++k] = "Preview Mode?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_preview;

	prompts3d[++k] = "    Show Box?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_show_box;

	prompts3d[++k] = "Coarseness, preview/grid/ray (in y dir)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_preview_factor;

	prompts3d[++k] = "Spherical Projection?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = sphere = SPHERE;

	prompts3d[++k] = "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_glasses_type;

	prompts3d[++k] = "                  3=photo,4=stereo pair)";
	uvalues[k].type = '*';

	prompts3d[++k] = "Ray trace out? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_raytrace_output;

	prompts3d[++k] = "                4=MTV, 5=RAYSHADE, 6=ACROSPIN, 7=DXF)";
	uvalues[k].type = '*';

	prompts3d[++k] = "    Brief output?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_raytrace_brief;

	check_write_file(g_ray_name, ".ray");
	prompts3d[++k] = "    Output File Name";
	uvalues[k].type = 's';
	strcpy(uvalues[k].uval.sval, g_ray_name);

	prompts3d[++k] = "Targa output?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_targa_output;

	prompts3d[++k] = "Use grayscale value for depth? (if \"no\" uses color number)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_grayscale_depth;

	oldhelpmode = g_help_mode;
	g_help_mode = HELP3DMODE;

	k = full_screen_prompt("3D Mode Selection", k + 1, prompts3d, uvalues, 0, NULL);
	g_help_mode = oldhelpmode;
	if (k < 0)
	{
		return -1;
		}

	k = 0;

	g_preview = uvalues[k++].uval.ch.val;
	g_show_box = uvalues[k++].uval.ch.val;
	g_preview_factor  = uvalues[k++].uval.ival;
	sphere = uvalues[k++].uval.ch.val;
	g_glasses_type = uvalues[k++].uval.ival;
	k++;

	g_raytrace_output = uvalues[k++].uval.ival;
	k++;
	if (g_raytrace_output == RAYTRACE_POVRAY)
	{
		stop_message(0, "DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
		"the online documentation.");
	}
	g_raytrace_brief = uvalues[k++].uval.ch.val;

	strcpy(g_ray_name, uvalues[k++].uval.sval);

	g_targa_output = uvalues[k++].uval.ch.val;
	g_grayscale_depth  = uvalues[k++].uval.ch.val;

	/* check ranges */
	if (g_preview_factor < 2)
	{
		g_preview_factor = 2;
	}
	if (g_preview_factor > 2000)
	{
		g_preview_factor = 2000;
	}

	if (sphere && !SPHERE)
	{
		SPHERE = TRUE;
		set_3d_defaults();
	}
	else if (!sphere && SPHERE)
	{
		SPHERE = FALSE;
		set_3d_defaults();
	}

	if (g_glasses_type < 0)
	{
		g_glasses_type = 0;
	}
	if (g_glasses_type > STEREO_PAIR)
	{
		g_glasses_type = STEREO_PAIR;
	}
	if (g_glasses_type)
	{
		g_which_image = WHICHIMAGE_RED;
	}

	if (g_raytrace_output < RAYTRACE_NONE)
	{
		g_raytrace_output = RAYTRACE_NONE;
	}
	if (g_raytrace_output > RAYTRACE_DXF)
	{
		g_raytrace_output = RAYTRACE_DXF;
	}

	if (!g_raytrace_output)
	{
		k = 0;
		choices[k++] = "make a surface grid";
		choices[k++] = "just draw the points";
		choices[k++] = "connect the dots (wire frame)";
		choices[k++] = "surface fill (g_colors interpolated)";
		choices[k++] = "surface fill (g_colors not interpolated)";
		choices[k++] = "solid fill (bars up from \"ground\")";
		if (SPHERE)
		{
			choices[k++] = "light source";
		}
		else
		{
			choices[k++] = "light source before transformation";
			choices[k++] = "light source after transformation";
		}
		for (i = 0; i < k; ++i)
		{
			attributes[i] = 1;
		}
		g_help_mode = HELP3DFILL;
		i = full_screen_choice(CHOICE_HELP, "Select 3D Fill Type", NULL, NULL, k, (char **) choices, attributes,
										0, 0, 0, FILLTYPE + 1, NULL, NULL, NULL, NULL);
		g_help_mode = oldhelpmode;
		if (i < 0)
		{
			goto restart_1;
		}
		FILLTYPE = i-1;

		if (g_glasses_type)
		{
			if (get_funny_glasses_params())
			{
				goto restart_1;
			}
			}
			if (check_mapfile())
			{
				goto restart_1;
			}
		}
	restart_3:

	if (SPHERE)
	{
		k = -1;
		prompts3d[++k] = "Longitude start (degrees)";
		prompts3d[++k] = "Longitude stop  (degrees)";
		prompts3d[++k] = "Latitude start  (degrees)";
		prompts3d[++k] = "Latitude stop   (degrees)";
		prompts3d[++k] = "Radius scaling factor in pct";
	}
	else
	{
		k = -1;
		if (!g_raytrace_output)
		{
			prompts3d[++k] = "X-axis rotation in degrees";
			prompts3d[++k] = "Y-axis rotation in degrees";
			prompts3d[++k] = "Z-axis rotation in degrees";
		}
		prompts3d[++k] = "X-axis scaling factor in pct";
		prompts3d[++k] = "Y-axis scaling factor in pct";
	}
	k = -1;
	if (!(g_raytrace_output && !SPHERE))
	{
		uvalues[++k].uval.ival   = XROT    ;
		uvalues[k].type = 'i';
		uvalues[++k].uval.ival   = YROT    ;
		uvalues[k].type = 'i';
		uvalues[++k].uval.ival   = ZROT    ;
		uvalues[k].type = 'i';
	}
	uvalues[++k].uval.ival   = XSCALE    ;
	uvalues[k].type = 'i';

	uvalues[++k].uval.ival   = YSCALE    ;
	uvalues[k].type = 'i';

	prompts3d[++k] = "Surface Roughness scaling factor in pct";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = ROUGH     ;

	prompts3d[++k] = "'Water Level' (minimum color value)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = WATERLINE ;

	if (!g_raytrace_output)
	{
		prompts3d[++k] = "Perspective distance [1 - 999, 0 for no persp])";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = ZVIEWER     ;

		prompts3d[++k] = "X shift with perspective (positive = right)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = XSHIFT    ;

		prompts3d[++k] = "Y shift with perspective (positive = up   )";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = YSHIFT    ;

		prompts3d[++k] = "Image non-perspective X adjust (positive = right)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_x_trans    ;

		prompts3d[++k] = "Image non-perspective Y adjust (positive = up)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_y_trans    ;

		prompts3d[++k] = "First transparent color";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_transparent[0];

		prompts3d[++k] = "Last transparent color";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_transparent[1];
	}

	prompts3d[++k] = "Randomize Colors      (0 - 7, '0' disables)";
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = g_randomize;

	if (SPHERE)
	{
		s = "Sphere 3D Parameters\n"
			"Sphere is on its side; North pole to right\n"
			"Long. 180 is top, 0 is bottom; Lat. -90 is left, 90 is right";
	}
	else
	{
		s = "Planar 3D Parameters\n"
			"Pre-rotation X axis is screen top; Y axis is left side\n"
			"Pre-rotation Z axis is coming at you out of the screen!";
	}

	g_help_mode = HELP3DPARMS;
	k = full_screen_prompt(s, k, prompts3d, uvalues, 0, NULL);
	g_help_mode = oldhelpmode;
	if (k < 0)
	{
		goto restart_1;
	}

	k = 0;
	if (!(g_raytrace_output && !SPHERE))
	{
		XROT    = uvalues[k++].uval.ival;
		YROT    = uvalues[k++].uval.ival;
		ZROT    = uvalues[k++].uval.ival;
	}
	XSCALE     = uvalues[k++].uval.ival;
	YSCALE     = uvalues[k++].uval.ival;
	ROUGH      = uvalues[k++].uval.ival;
	WATERLINE  = uvalues[k++].uval.ival;
	if (!g_raytrace_output)
	{
		ZVIEWER = uvalues[k++].uval.ival;
		XSHIFT     = uvalues[k++].uval.ival;
		YSHIFT     = uvalues[k++].uval.ival;
		g_x_trans     = uvalues[k++].uval.ival;
		g_y_trans     = uvalues[k++].uval.ival;
		g_transparent[0] = uvalues[k++].uval.ival;
		g_transparent[1] = uvalues[k++].uval.ival;
	}
	g_randomize  = uvalues[k++].uval.ival;
	if (g_randomize >= 7)
	{
		g_randomize = 7;
	}
	if (g_randomize <= 0)
	{
		g_randomize = 0;
	}

	if ((g_targa_output || ILLUMINE || g_raytrace_output))
	{
		if (get_light_params())
		{
			goto restart_3;
		}
	}
return 0;
}

/* --------------------------------------------------------------------- */
static int get_light_params()
{
	char *prompts3d[13];
	struct full_screen_values uvalues[13];

	int k;
	int oldhelpmode;

	/* defaults go here */

	k = -1;

	if (ILLUMINE || g_raytrace_output)
	{
		prompts3d[++k] = "X value light vector";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = XLIGHT    ;

		prompts3d[++k] = "Y value light vector";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = YLIGHT    ;

		prompts3d[++k] = "Z value light vector";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = ZLIGHT    ;

		if (!g_raytrace_output)
		{
			prompts3d[++k] = "Light Source Smoothing Factor";
			uvalues[k].type = 'i';
			uvalues[k].uval.ival = LIGHTAVG  ;

			prompts3d[++k] = "Ambient";
			uvalues[k].type = 'i';
			uvalues[k].uval.ival = g_ambient;
		}
	}

	if (g_targa_output && !g_raytrace_output)
	{
		prompts3d[++k] = "Haze Factor        (0 - 100, '0' disables)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_haze;

		if (!g_targa_overlay)
		{
			check_write_file(g_light_name, ".tga");
		}
		prompts3d[++k] = "Targa File Name  (Assume .tga)";
		uvalues[k].type = 's';
		strcpy(uvalues[k].uval.sval, g_light_name);

		prompts3d[++k] = "Back Ground Color (0 - 255)";
		uvalues[k].type = '*';

		prompts3d[++k] = "   Red";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = (int) g_back_color[0];

		prompts3d[++k] = "   Green";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = (int) g_back_color[1];

		prompts3d[++k] = "   Blue";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = (int) g_back_color[2];

		prompts3d[++k] = "Overlay Targa File? (Y/N)";
		uvalues[k].type = 'y';
		uvalues[k].uval.ch.val = g_targa_overlay;

	}

	prompts3d[++k] = "";

	oldhelpmode = g_help_mode;
	g_help_mode = HELP3DLIGHT;
	k = full_screen_prompt("Light Source Parameters", k, prompts3d, uvalues, 0, NULL);
	g_help_mode = oldhelpmode;
	if (k < 0)
	{
		return -1;
	}

	k = 0;
	if (ILLUMINE)
	{
		XLIGHT   = uvalues[k++].uval.ival;
		YLIGHT   = uvalues[k++].uval.ival;
		ZLIGHT   = uvalues[k++].uval.ival;
		if (!g_raytrace_output)
		{
			LIGHTAVG = uvalues[k++].uval.ival;
			g_ambient  = uvalues[k++].uval.ival;
			if (g_ambient >= 100)
			{
				g_ambient = 100;
			}
			if (g_ambient <= 0)
			{
				g_ambient = 0;
			}
		}
	}

	if (g_targa_output && !g_raytrace_output)
	{
		g_haze  =  uvalues[k++].uval.ival;
		if (g_haze >= 100)
		{
			g_haze = 100;
		}
		if (g_haze <= 0)
		{
			g_haze = 0;
		}
		strcpy(g_light_name, uvalues[k++].uval.sval);
        /* In case g_light_name conflicts with an existing name it is checked
						again in line3d */
		k++;
		g_back_color[0] = (BYTE) (uvalues[k++].uval.ival % 255);
		g_back_color[1] = (BYTE) (uvalues[k++].uval.ival % 255);
		g_back_color[2] = (BYTE) (uvalues[k++].uval.ival % 255);
		g_targa_overlay = uvalues[k].uval.ch.val;
	}
	return 0;
}

/* --------------------------------------------------------------------- */


static int check_mapfile()
{
	int askflag = 0;
	int i, oldhelpmode;
	char temp1[256];

	if (g_dont_read_color)
	{
		return 0;
	}
	strcpy(temp1, "*");
	if (g_map_set)
	{
		strcpy(temp1, g_map_name);
	}
	if (!(g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE))
	{
		askflag = 1;
	}
	else
	{
		merge_path_names(temp1, funnyglasses_map_name, 0);
	}

	while (1)
	{
		if (askflag)
		{
			oldhelpmode = g_help_mode;
			g_help_mode = -1;
			i = field_prompt("Enter name of .MAP file to use,\n"
				"or '*' to use palette from the image to be loaded.",
				NULL, temp1, 60, NULL);
			g_help_mode = oldhelpmode;
			if (i < 0)
			{
				return -1;
			}
			if (temp1[0] == '*')
			{
				g_map_set = FALSE;
				break;
			}
		}
		memcpy(g_old_dac_box, g_dac_box, 256*3); /* save the DAC */
		i = validate_luts(temp1);
		memcpy(g_dac_box, g_old_dac_box, 256*3); /* restore the DAC */
		if (i != 0)  /* Oops, somethings wrong */
		{
			askflag = 1;
			continue;
		}
		g_map_set = TRUE;
		merge_path_names(g_map_name, temp1, 0);
		break;
	}
	return 0;
}

static int get_funny_glasses_params()
{
	char *prompts3d[10];

	struct full_screen_values uvalues[10];

	int k;
	int oldhelpmode;

	/* defaults */
	if (ZVIEWER == 0)
	{
		ZVIEWER = 150;
	}
	if (g_eye_separation == 0)
	{
		if (g_fractal_type == IFS3D || g_fractal_type == LLORENZ3D || g_fractal_type == FPLORENZ3D)
		{
			g_eye_separation =  2;
			g_x_adjust       = -2;
		}
		else
		{
			g_eye_separation =  3;
			g_x_adjust       =  0;
		}
	}

	if (g_glasses_type == STEREO_ALTERNATE)
	{
		strcpy(funnyglasses_map_name, g_glasses1_map);
	}
	else if (g_glasses_type == STEREO_SUPERIMPOSE)
	{
		if (FILLTYPE == FILLTYPE_SURFACE_GRID)
		{
			strcpy(funnyglasses_map_name, "grid.map");
		}
		else
		{
			strcpy(funnyglasses_map_name, g_glasses1_map);
			funnyglasses_map_name[7] = '2';
		}
	}

	k = -1;
	prompts3d[++k] = "Interocular distance (as % of screen)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_eye_separation;

	prompts3d[++k] = "Convergence adjust (positive = spread greater)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_x_adjust;

	prompts3d[++k] = "Left  red image crop (% of screen)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_red_crop_left;

	prompts3d[++k] = "Right red image crop (% of screen)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_red_crop_right;

	prompts3d[++k] = "Left  blue image crop (% of screen)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_blue_crop_left;

	prompts3d[++k] = "Right blue image crop (% of screen)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_blue_crop_right;

	prompts3d[++k] = "Red brightness factor (%)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_red_bright;

	prompts3d[++k] = "Blue brightness factor (%)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_blue_bright;

	if (g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE)
	{
		prompts3d[++k] = "Map File name";
		uvalues[k].type = 's';
		strcpy(uvalues[k].uval.sval, funnyglasses_map_name);
	}

	oldhelpmode = g_help_mode;
	g_help_mode = HELP3DGLASSES;
	k = full_screen_prompt("Funny Glasses Parameters", k + 1, prompts3d, uvalues, 0, NULL);
	g_help_mode = oldhelpmode;
	if (k < 0)
	{
		return -1;
	}

	k = 0;
	g_eye_separation   =  uvalues[k++].uval.ival;
	g_x_adjust         =  uvalues[k++].uval.ival;
	g_red_crop_left   =  uvalues[k++].uval.ival;
	g_red_crop_right  =  uvalues[k++].uval.ival;
	g_blue_crop_left  =  uvalues[k++].uval.ival;
	g_blue_crop_right =  uvalues[k++].uval.ival;
	g_red_bright      =  uvalues[k++].uval.ival;
	g_blue_bright     =  uvalues[k++].uval.ival;

	if (g_glasses_type == STEREO_ALTERNATE || g_glasses_type == STEREO_SUPERIMPOSE)
	{
		strcpy(funnyglasses_map_name, uvalues[k].uval.sval);
	}
	return 0;
}

void set_bail_out_formula(enum bailouts test)
{
	switch (test)
	{
	case Mod:
	default:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_mod_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_mod_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lMODbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_mod_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_mod_bn;
		g_bail_out_bf = (int (*)(void))bail_out_mod_bf;
		break;
	case Real:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_real_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_real_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lREALbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_real_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_real_bn;
		g_bail_out_bf = (int (*)(void))bail_out_real_bf;
		break;
	case Imag:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_imag_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_imag_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lIMAGbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_imag_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_imag_bn;
		g_bail_out_bf = (int (*)(void))bail_out_imag_bf;
		break;
	case Or:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_or_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_or_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lORbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_or_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_or_bn;
		g_bail_out_bf = (int (*)(void))bail_out_or_bf;
		break;
	case And:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_and_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_and_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lANDbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_and_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_and_bn;
		g_bail_out_bf = (int (*)(void))bail_out_and_bf;
		break;
	case Manh:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_manhattan_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_manhattan_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lMANHbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_manhattan_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_manhattan_bn;
		g_bail_out_bf = (int (*)(void))bail_out_manhattan_bf;
		break;
	case Manr:
		if (g_fpu >= 287 && g_debug_flag != DEBUGFLAG_FAST_287_MATH)     /* Fast 287 math */
		{
			g_bail_out_fp = (int (*)(void))bail_out_manhattan_r_fp_asm;
		}
		else
		{
			g_bail_out_fp = (int (*)(void))bail_out_manhattan_r_fp;
		}
		if (g_cpu >= 386 && g_debug_flag != DEBUGFLAG_CPU_8088)    /* Fast 386 math */
		{
			g_bail_out_l = (int (*)(void))asm386lMANRbailout;
		}
		else
		{
			g_bail_out_l = (int (*)(void))bail_out_manhattan_r_l_asm;
		}
		g_bail_out_bn = (int (*)(void))bail_out_manhattan_r_bn;
		g_bail_out_bf = (int (*)(void))bail_out_manhattan_r_bf;
		break;
	}
}
