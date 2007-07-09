/*
		Various routines that prompt for things.
*/
#include <assert.h>
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

#include <string>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#include "calcfrac.h"
#include "drivers.h"
#include "fihelp.h"
#include "filesystem.h"
#include "fracsuba.h"
#include "fracsubr.h"
#include "fractalp.h"
#include "fractals.h"
#include "loadfile.h"
#include "loadmap.h"
#include "lsys.h"
#include "miscres.h"
#include "prompts1.h"
#include "prompts2.h"
#include "realdos.h"
#include "zoom.h"

#include "EscapeTime.h"
#include "ThreeDimensionalState.h"
#include "Formula.h"
#include "UIChoices.h"
#include "MathUtil.h"

#ifdef __hpux
#include <sys/param.h>
#define getwd(a) getcwd(a, MAXPATHLEN)
#endif

#ifdef __SVR4
#include <sys/param.h>
#define getwd(a) getcwd(a, MAXPATHLEN)
#endif

char g_glasses1_map[] = "glasses1.map";
char g_map_name[FILE_MAX_DIR] = "";
bool g_map_set = false;
bool g_julibrot;   /* flag for julibrot */

/* These need to be global because F6 exits full_screen_prompt() */
static int s_prompt_function_key_mask;
/* will be set to first line of extra info to be displayed (0 = top line) */
static int s_scroll_row_status;
/* will be set to first column of extra info to be displayed (0 = leftmost column)*/
static int s_scroll_column_status;
static FILE *s_gfe_file;

static char funnyglasses_map_name[16];
static char ifsmask[13]     = {"*.ifs"};
static char formmask[13]    = {"*.frm"};
static char lsysmask[13]    = {"*.l"};

/* Routines in this module      */

static bool select_type_params(int newfractype, int oldfractype);
static  int input_field_list(int attr, char *fld, int vlen, const char **list, int llen,
							int row, int col, int (*checkkey)(int));
static  int select_fracttype(int t);
static  int sel_fractype_help(int curkey, int choice);
static  long gfe_choose_entry(int type, const char *title, char *filename, char *entryname);
static  int check_gfe_key(int curkey, int choice);
static  void load_entry_text(FILE *entfile, char *buf, int maxlines, int startrow, int startcol);
static  void format_parmfile_line(int, char *);
static  int get_light_params();
static  int check_mapfile();
static  int get_funny_glasses_params();
static int prompt_checkkey(int curkey);
static int prompt_checkkey_scroll(int curkey);


/* --------------------------------------------------------------------- */

class FullScreenPrompter
{
public:
	FullScreenPrompter(const char *heading, int num_prompts, const char **prompts,
		full_screen_values *values, int function_key_mask, char *footer);
	int Prompt();

private:
	void PrepareHeader();
	void PrepareFooter();
	void PrepareFooterFile();
	void PrepareFooterLinesInEntry();
	void PrepareFooterSize();
	void WorkOutVerticalPositioning();
	void WorkOutHorizontalPositioning();
	void DisplayHeader();
	void DisplayFooter();
	void DisplayInitialScreen();
	void DisplayInitialValues();
	void DisplayEmptyBox();

	const char *m_heading;
	int m_num_prompts;
	const char **m_prompts;
	full_screen_values *m_values;
	int m_function_key_mask;
	char *m_footer;

	FILE *m_scroll_file;			/* file with extrainfo entry to scroll   */
	long m_scroll_file_start;		/* where entry starts in scroll_file     */
	bool m_in_scrolling_mode;		/* will be true if need to scroll footer */
	int m_lines_in_entry;			/* total lines in entry to be scrolled   */
	int m_title_lines;
	int m_title_width;
	int m_vertical_scroll_limit;	/* don't scroll down if this is top line */
	int m_footer_lines;
	int m_footer_width;
	int m_title_row;
	int m_box_row;
	int m_box_lines;
	int m_footer_row;
	int m_instructions_row;
	int m_prompt_row;
	int m_max_prompt_width;
	int m_max_field_width;
	int m_max_comment;
	int m_box_column;
	int m_box_width;
	int m_prompt_column;
	int m_value_column;
	bool m_any_input;
};

FullScreenPrompter::FullScreenPrompter(const char *heading,
		int num_prompts, const char **prompts,
		full_screen_values *values,
		int function_key_mask, char *footer)
	: m_heading(heading),
	m_num_prompts(num_prompts),
	m_prompts(prompts),
	m_values(values),
	m_function_key_mask(function_key_mask),
	m_footer(footer),
	m_scroll_file(NULL),
	m_scroll_file_start(0),
	m_in_scrolling_mode(false),
	m_lines_in_entry(0),
	m_title_lines(0),
	m_title_width(0),
	m_vertical_scroll_limit(0),
	m_footer_lines(0),
	m_footer_width(0),
	m_title_row(0),
	m_box_row(0),
	m_box_lines(0),
	m_footer_row(0),
	m_instructions_row(0),
	m_prompt_row(0),
	m_max_prompt_width(0),
	m_max_field_width(0),
	m_max_comment(0),
	m_box_column(0),
	m_box_width(0),
	m_prompt_column(0),
	m_value_column(0),
	m_any_input(false)
{
}

void FullScreenPrompter::PrepareFooterFile()
{
	/* If applicable, open file for scrolling extrainfo. The function
	find_file_item() opens the file and sets the file pointer to the
	beginning of the entry.
	*/
	if (m_footer && *m_footer)
	{
		if (fractal_type_formula(g_fractal_type))
		{
			g_formula_state.find_item(&m_scroll_file);
			m_in_scrolling_mode = true;
			m_scroll_file_start = ftell(m_scroll_file);
		}
		else if (g_fractal_type == FRACTYPE_L_SYSTEM)
		{
			find_file_item(g_l_system_filename, g_l_system_name, &m_scroll_file, ITEMTYPE_L_SYSTEM);
			m_in_scrolling_mode = true;
			m_scroll_file_start = ftell(m_scroll_file);
		}
		else if (fractal_type_ifs(g_fractal_type))
		{
			find_file_item(g_ifs_filename, g_ifs_name, &m_scroll_file, ITEMTYPE_IFS);
			m_in_scrolling_mode = true;
			m_scroll_file_start = ftell(m_scroll_file);
		}
	}
}

void FullScreenPrompter::PrepareFooterLinesInEntry()
{
	/* initialize m_lines_in_entry */
	if (m_in_scrolling_mode && m_scroll_file != NULL)
	{
		bool comment = false;
		int c = 0;
		while ((c = fgetc(m_scroll_file)) != EOF && c != '\032')
		{
			if (c == ';')
			{
				comment = true;
			}
			else if (c == '\n')
			{
				comment = false;
				m_lines_in_entry++;
			}
			else if (c == '\r')
			{
				continue;
			}
			if (c == '}' && !comment)
			{
				m_lines_in_entry++;
				break;
			}
		}
		if (c == EOF || c == '\032')  /* should never happen */
		{
			fclose(m_scroll_file);
			m_in_scrolling_mode = false;
		}
	}
}

void FullScreenPrompter::PrepareFooter()
{
	PrepareFooterFile();
	PrepareFooterLinesInEntry();
	PrepareFooterSize();

	/* if entry fits in available space, shut off scrolling */
	if (m_in_scrolling_mode && s_scroll_row_status == 0
		&& m_lines_in_entry == m_footer_lines - 2
		&& s_scroll_column_status == 0
		&& strchr(m_footer, '\021') == NULL)
	{
		m_in_scrolling_mode = false;
		fclose(m_scroll_file);
		m_scroll_file = NULL;
	}

	/*initialize vertical scroll limit. When the top line of the text
	box is the vertical scroll limit, the bottom line is the end of the
	entry, and no further down scrolling is necessary.
	*/
	if (m_in_scrolling_mode)
	{
		m_vertical_scroll_limit = m_lines_in_entry - (m_footer_lines - 2);
	}
}

void FullScreenPrompter::PrepareFooterSize()
{
	m_footer_lines = 0;
	m_footer_width = 0;
	if (!m_footer)
	{
		return;
	}
	if (!*m_footer)
	{
		m_footer = NULL;
		return;
	}

	/* count footer lines, find widest */
	m_footer_lines = 3;
	int i = 0;
	for (char *scan = m_footer; *scan; scan++)
	{
		if (*scan == '\n')
		{
			if (m_footer_lines + m_num_prompts + m_title_lines >= 20)
			{
				*scan = 0; /* full screen, cut off here */
				break;
			}
			++m_footer_lines;
			i = -1;
		}
		if (++i > m_footer_width)
		{
			m_footer_width = i;
		}
	}
}

void FullScreenPrompter::PrepareHeader()
{
	/* count title lines, find widest */
	m_title_lines = 1;
	m_title_width = 0;
	int i = 0;
	for (const char *heading_scan = m_heading; *heading_scan; heading_scan++)
	{
		if (*heading_scan == '\n')
		{
			++m_title_lines;
			i = -1;
		}
		if (++i > m_title_width)
		{
			m_title_width = i;
		}
	}
}

void FullScreenPrompter::WorkOutVerticalPositioning()
{
	int total_rows = m_num_prompts + m_title_lines + m_footer_lines + 3;
	int j = (25 - total_rows) / 2;                   /* top row of it all when centered */
	j -= j / 4;                         /* higher is better if lots extra */
	m_box_lines = m_num_prompts;
	m_title_row = 1 + j;
	m_box_row = m_title_row + m_title_lines;
	m_prompt_row = m_box_row;
		
	if (m_title_row > 2)  /* room for blank between title & box? */
	{
		--m_title_row;
		--m_box_row;
		++m_box_lines;
	}
	m_instructions_row = m_box_row + m_box_lines;
	if (m_instructions_row + 3 + m_footer_lines < 25)
	{
		++m_box_lines;    /* blank at bottom of box */
		++m_instructions_row;
		if (m_instructions_row + 3 + m_footer_lines < 25)
		{
			++m_instructions_row; /* blank before instructions */
		}
	}
	m_footer_row = m_instructions_row + 2;
	if (m_num_prompts > 1) /* 3 instructions lines */
	{
		++m_footer_row;
	}
	if (m_footer_row + m_footer_lines < 25)
	{
		++m_footer_row;
	}
}

void FullScreenPrompter::WorkOutHorizontalPositioning()
{
	if (m_in_scrolling_mode)  /* set box to max width if in scrolling mode */
	{
		m_footer_width = 76;
	}

	/* work out horizontal positioning */
	m_max_field_width = 0;
	m_max_prompt_width = 0;
	m_max_comment = 0;
	m_any_input = false;
	for (int i = 0; i < m_num_prompts; i++)
	{
		if (m_values[i].type == 'y')
		{
			static const char *noyes[2] =
			{ 
				"no", "yes"
			};
			m_values[i].type = 'l';
			m_values[i].uval.ch.vlen = 3;
			m_values[i].uval.ch.list = noyes;
			m_values[i].uval.ch.llen = 2;
		}
		int j = (int) strlen(m_prompts[i]);
		if (m_values[i].type == '*')
		{
			if (j > m_max_comment)
			{
				m_max_comment = j;
			}
		}
		else
		{
			m_any_input = true;
			if (j > m_max_prompt_width)
			{
				m_max_prompt_width = j;
			}
			char buffer[81];
			j = prompt_value_string(buffer, &m_values[i]);
			if (j > m_max_field_width)
			{
				m_max_field_width = j;
			}
		}
	}
	m_box_width = m_max_prompt_width + m_max_field_width + 2;
	if (m_max_comment > m_box_width)
	{
		m_box_width = m_max_comment;
	}
	m_box_width += 4;
	if (m_box_width > 80)
	{
		m_box_width = 80;
	}
	m_box_column = (80 - m_box_width) / 2;       /* center the box */
	m_prompt_column = m_box_column + 2;
	m_value_column = m_box_column + m_box_width - m_max_field_width - 2;
	if (m_box_width <= 76)  /* make margin a bit wider if we can */
	{
		m_box_width += 2;
		--m_box_column;
	}
	int j = m_title_width;
	if (j < m_footer_width)
	{
		j = m_footer_width;
	}
	int i = j + 4 - m_box_width;
	if (i > 0)  /* expand box for title/extra */
	{
		if (m_box_width + i > 80)
		{
			i = 80 - m_box_width;
		}
		m_box_width += i;
		m_box_column -= i / 2;
	}
	i = (90 - m_box_width) / 20;
	m_box_column    -= i;
	m_prompt_column -= i;
	m_value_column  -= i;
}

void FullScreenPrompter::DisplayHeader()
{
	for (int i = m_title_row; i < m_box_row; ++i)
	{
		driver_set_attr(i, m_box_column, C_PROMPT_HI, m_box_width);
	}

	char buffer[256];
	char *heading_line = buffer;
	/* center each line of heading independently */
	int i;
	::strcpy(heading_line, m_heading);
	for (i = 0; i < m_title_lines-1; i++)
	{
		char *next = ::strchr(heading_line, '\n');
		if (next == NULL)
		{
			break; /* shouldn't happen */
		}
		*next = '\0';
		m_title_width = (int) ::strlen(heading_line);
		g_text_cbase = m_box_column + (m_box_width - m_title_width) / 2;
		driver_put_string(m_title_row + i, 0, C_PROMPT_HI, heading_line);
		*next = '\n';
		heading_line = next + 1;
	}
	/* add scrolling key message, if applicable */
	if (m_in_scrolling_mode)
	{
		*(heading_line + 31) = (char) 0;   /* replace the ')' */
		::strcat(heading_line, ". CTRL+(direction key) to scroll text.)");
	}

	m_title_width = (int) ::strlen(heading_line);
	g_text_cbase = m_box_column + (m_box_width - m_title_width) / 2;
	driver_put_string(m_title_row + i, 0, C_PROMPT_HI, heading_line);
}

void FullScreenPrompter::DisplayFooter()
{
	if (!m_footer)
	{
		return;
	}

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
	char buffer[81];
	memset(buffer, S1, 80);
	buffer[m_box_width-2] = 0;
	g_text_cbase = m_box_column + 1;
	driver_put_string(m_footer_row, 0, C_PROMPT_BKGRD, buffer);
	driver_put_string(m_footer_row + m_footer_lines-1, 0, C_PROMPT_BKGRD, buffer);
	--g_text_cbase;
	driver_put_string(m_footer_row, 0, C_PROMPT_BKGRD, S5);
	driver_put_string(m_footer_row + m_footer_lines-1, 0, C_PROMPT_BKGRD, S2);
	g_text_cbase += m_box_width - 1;
	driver_put_string(m_footer_row, 0, C_PROMPT_BKGRD, S6);
	driver_put_string(m_footer_row + m_footer_lines-1, 0, C_PROMPT_BKGRD, S3);

	g_text_cbase = m_box_column;

	for (int i = 1; i < m_footer_lines-1; ++i)
	{
		driver_put_string(m_footer_row + i, 0, C_PROMPT_BKGRD, S4);
		driver_put_string(m_footer_row + i, m_box_width-1, C_PROMPT_BKGRD, S4);
	}
	g_text_cbase += (m_box_width - m_footer_width) / 2;
	driver_put_string(m_footer_row + 1, 0, C_PROMPT_TEXT, m_footer);
}

void FullScreenPrompter::DisplayEmptyBox()
{
	g_text_cbase = 0;

	/* display empty box */
	for (int i = 0; i < m_box_lines; ++i)
	{
		driver_set_attr(m_box_row + i, m_box_column, C_PROMPT_LO, m_box_width);
	}
}

void FullScreenPrompter::DisplayInitialValues()
{
	/* display initial values */
	for (int i = 0; i < m_num_prompts; i++)
	{
		driver_put_string(m_prompt_row + i, m_prompt_column, C_PROMPT_LO, m_prompts[i]);
		char buffer[81];
		prompt_value_string(buffer, &m_values[i]);
		driver_put_string(m_prompt_row + i, m_value_column, C_PROMPT_LO, buffer);
	}
}

void FullScreenPrompter::DisplayInitialScreen()
{
	help_title();                        /* clear screen, display title line  */
	driver_set_attr(1, 0, C_PROMPT_BKGRD, 24*80);  /* init rest of screen to background */

	DisplayHeader();
	DisplayFooter();

	DisplayEmptyBox();
	DisplayInitialValues();
}

int FullScreenPrompter::Prompt()
{
	int current_choice = 0;
	int done;
	int i;
	int j;
	int current_type;
	int current_length;

	/* scrolling related variables */
	bool rewrite_footer = false;	/* if true: rewrite footer to text box   */

	char blanks[78];				/* used to clear text box                */
	memset(blanks, ' ', NUM_OF(blanks) - 1);   /* initialize string of blanks */
	blanks[NUM_OF(blanks) - 1] = 0;

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
	s_prompt_function_key_mask = m_function_key_mask;

	PrepareHeader();
	PrepareFooter();

	WorkOutVerticalPositioning();
	WorkOutHorizontalPositioning();

	DisplayInitialScreen();

	if (!m_any_input)
	{
		put_string_center(m_instructions_row++, 0, 80, C_PROMPT_BKGRD,
			"No changeable parameters;");
		put_string_center(m_instructions_row, 0, 80, C_PROMPT_BKGRD,
			(get_help_mode() > 0)
			? "Press ENTER to exit, ESC to back out, "FK_F1" for help"
			: "Press ENTER to exit");
		driver_hide_text_cursor();
		g_text_cbase = 2;
		while (true)
		{
			if (rewrite_footer)
			{
				rewrite_footer = false;
				fseek(m_scroll_file, m_scroll_file_start, SEEK_SET);
				load_entry_text(m_scroll_file, m_footer, m_footer_lines - 2,
					s_scroll_row_status, s_scroll_column_status);
				for (i = 1; i <= m_footer_lines - 2; i++)
				{
					driver_put_string(m_footer_row + i, 0, C_PROMPT_TEXT, blanks);
				}
				driver_put_string(m_footer_row + 1, 0, C_PROMPT_TEXT, m_footer);
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
				if (m_in_scrolling_mode && s_scroll_row_status < m_vertical_scroll_limit)
				{
					s_scroll_row_status++;
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_UP_ARROW:      /* scrolling key - up one row */
				if (m_in_scrolling_mode && s_scroll_row_status > 0)
				{
					s_scroll_row_status--;
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_LEFT_ARROW:    /* scrolling key - left one column */
				if (m_in_scrolling_mode && s_scroll_column_status > 0)
				{
					s_scroll_column_status--;
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_RIGHT_ARROW:   /* scrolling key - right one column */
				if (m_in_scrolling_mode && strchr(m_footer, '\021') != NULL)
				{
					s_scroll_column_status++;
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_PAGE_DOWN:   /* scrolling key - down one screen */
				if (m_in_scrolling_mode && s_scroll_row_status < m_vertical_scroll_limit)
				{
					s_scroll_row_status += m_footer_lines - 2;
					if (s_scroll_row_status > m_vertical_scroll_limit)
					{
						s_scroll_row_status = m_vertical_scroll_limit;
					}
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_PAGE_UP:     /* scrolling key - up one screen */
				if (m_in_scrolling_mode && s_scroll_row_status > 0)
				{
					s_scroll_row_status -= m_footer_lines - 2;
					if (s_scroll_row_status < 0)
					{
						s_scroll_row_status = 0;
					}
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_END:         /* scrolling key - to end of entry */
				if (m_in_scrolling_mode)
				{
					s_scroll_row_status = m_vertical_scroll_limit;
					s_scroll_column_status = 0;
					rewrite_footer = true;
				}
				break;
			case FIK_CTL_HOME:        /* scrolling key - to beginning of entry */
				if (m_in_scrolling_mode)
				{
					s_scroll_row_status = s_scroll_column_status = 0;
					rewrite_footer = true;
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
				if (s_prompt_function_key_mask & (1 << (done + 1-FIK_F1)))
				{
					goto fullscreen_exit;
				}
			}
		}
	}


	/* display footing */
	if (m_num_prompts > 1)
	{
		put_string_center(m_instructions_row++, 0, 80, C_PROMPT_BKGRD,
			"Use " UPARR1 " and " DNARR1 " to select values to change");
	}
	put_string_center(m_instructions_row + 1, 0, 80, C_PROMPT_BKGRD,
		(get_help_mode() > 0) ? "Press ENTER when finished, ESCAPE to back out, or "FK_F1" for help" : "Press ENTER when finished (or ESCAPE to back out)");

	done = 0;
	while (m_values[current_choice].type == '*')
	{
		++current_choice;
	}

	while (!done)
	{
		if (rewrite_footer)
		{
			j = g_text_cbase;
			g_text_cbase = 2;
			fseek(m_scroll_file, m_scroll_file_start, SEEK_SET);
			load_entry_text(m_scroll_file, m_footer, m_footer_lines - 2,
				s_scroll_row_status, s_scroll_column_status);
			for (i = 1; i <= m_footer_lines - 2; i++)
			{
				driver_put_string(m_footer_row + i, 0, C_PROMPT_TEXT, blanks);
			}
			driver_put_string(m_footer_row + 1, 0, C_PROMPT_TEXT, m_footer);
			g_text_cbase = j;
		}

		current_type = m_values[current_choice].type;
		char buffer[81];
		current_length = prompt_value_string(buffer, &m_values[current_choice]);
		if (!rewrite_footer)
		{
			put_string_center(m_instructions_row, 0, 80, C_PROMPT_BKGRD,
				(current_type == 'l') ? "Use " LTARR1 " or " RTARR1 " to change value of selected field" : "Type in replacement value for selected field");
		}
		else
		{
			rewrite_footer = false;
		}
		driver_put_string(m_prompt_row + current_choice, m_prompt_column, C_PROMPT_HI, m_prompts[current_choice]);

		if (current_type == 'l')
		{
			i = input_field_list(
				C_PROMPT_CHOOSE, buffer, current_length,
				m_values[current_choice].uval.ch.list, m_values[current_choice].uval.ch.llen,
				m_prompt_row + current_choice, m_value_column, m_in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
			for (j = 0; j < m_values[current_choice].uval.ch.llen; ++j)
			{
				if (strcmp(buffer, m_values[current_choice].uval.ch.list[j]) == 0)
				{
					break;
				}
			}
			m_values[current_choice].uval.ch.val = j;
		}
		else
		{
			j = 0;
			if (current_type == 'i')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER;
			}
			if (current_type == 'L')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_INTEGER;
			}
			if (current_type == 'd')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_DOUBLE;
			}
			if (current_type == 'D')
			{
				j = INPUTFIELD_NUMERIC | INPUTFIELD_DOUBLE | INPUTFIELD_INTEGER;
			}
			if (current_type == 'f')
			{
				j = INPUTFIELD_NUMERIC;
			}
			i = input_field(j, C_PROMPT_INPUT, buffer, current_length,
				m_prompt_row + current_choice, m_value_column, m_in_scrolling_mode ? prompt_checkkey_scroll : prompt_checkkey);
			switch (m_values[current_choice].type)
			{
			case 'd':
			case 'D':
				m_values[current_choice].uval.dval = atof(buffer);
				break;
			case 'f':
				m_values[current_choice].uval.dval = atof(buffer);
				round_float_d(&m_values[current_choice].uval.dval);
				break;
			case 'i':
				m_values[current_choice].uval.ival = atoi(buffer);
				break;
			case 'L':
				m_values[current_choice].uval.Lval = atol(buffer);
				break;
			case 's':
				strncpy(m_values[current_choice].uval.sval, buffer, 16);
				break;
			default: /* assume 0x100 + n */
				strcpy(m_values[current_choice].uval.sbuf, buffer);
			}
		}

		driver_put_string(m_prompt_row + current_choice, m_prompt_column, C_PROMPT_LO, m_prompts[current_choice]);
		j = (int) strlen(buffer);
		memset(&buffer[j], ' ', 80-j);
		buffer[current_length] = 0;
		driver_put_string(m_prompt_row + current_choice, m_value_column, C_PROMPT_LO,  buffer);

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
			current_choice = -1;
		case FIK_DOWN_ARROW:
			do
			{
				if (++current_choice >= m_num_prompts)
				{
					current_choice = 0;
				}
			}
			while (m_values[current_choice].type == '*');
			break;
		case FIK_PAGE_DOWN:
			current_choice = m_num_prompts;
		case FIK_UP_ARROW:
			do
			{
				if (--current_choice < 0)
				{
					current_choice = m_num_prompts - 1;
				}
			}
			while (m_values[current_choice].type == '*');
			break;
		case FIK_CTL_DOWN_ARROW:     /* scrolling key - down one row */
			if (m_in_scrolling_mode && s_scroll_row_status < m_vertical_scroll_limit)
			{
				s_scroll_row_status++;
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_UP_ARROW:       /* scrolling key - up one row */
			if (m_in_scrolling_mode && s_scroll_row_status > 0)
			{
				s_scroll_row_status--;
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_LEFT_ARROW:     /*scrolling key - left one column */
			if (m_in_scrolling_mode && s_scroll_column_status > 0)
			{
				s_scroll_column_status--;
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_RIGHT_ARROW:    /* scrolling key - right one column */
			if (m_in_scrolling_mode && strchr(m_footer, '\021') != NULL)
			{
				s_scroll_column_status++;
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_PAGE_DOWN:    /* scrolling key - down on screen */
			if (m_in_scrolling_mode && s_scroll_row_status < m_vertical_scroll_limit)
			{
				s_scroll_row_status += m_footer_lines - 2;
				if (s_scroll_row_status > m_vertical_scroll_limit)
				{
					s_scroll_row_status = m_vertical_scroll_limit;
				}
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_PAGE_UP:      /* scrolling key - up one screen */
			if (m_in_scrolling_mode && s_scroll_row_status > 0)
			{
				s_scroll_row_status -= m_footer_lines - 2;
				if (s_scroll_row_status < 0)
				{
					s_scroll_row_status = 0;
				}
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_END:          /* scrolling key - go to end of entry */
			if (m_in_scrolling_mode)
			{
				s_scroll_row_status = m_vertical_scroll_limit;
				s_scroll_column_status = 0;
				rewrite_footer = true;
			}
			break;
		case FIK_CTL_HOME:         /* scrolling key - go to beginning of entry */
			if (m_in_scrolling_mode)
			{
				s_scroll_row_status = s_scroll_column_status = 0;
				rewrite_footer = true;
			}
			break;
		}
	}

fullscreen_exit:
	driver_hide_text_cursor();
	if (m_scroll_file)
	{
		fclose(m_scroll_file);
		m_scroll_file = NULL;
	}
	return done;
}

/*
	full_screen_prompt

	full-screen prompting routine arguments:
	heading				heading, lines separated by \n
	num_prompts			there are this many prompts (max)
	prompts				array of promp strings
	values				in/out values
	function_key_mask	bit n on if Fn to cause return
	footer				extra info box to display, \n separated

*/
// TODO: extrainfo should be const, but it is modified here!
int full_screen_prompt(const char *heading, int num_prompts, const char **prompts,
	struct full_screen_values *values, int function_key_mask, char *footer)
{
	FullScreenPrompter prompter(heading, num_prompts, prompts, values, function_key_mask, footer);
	return prompter.Prompt();
}

int prompt_value_string(char *buf, struct full_screen_values *val)
{  /* format value into buf, return field width */
	int i;
	int ret;
	switch (val->type)
	{
	case 'd':
		ret = 20;
		i = 16;    /* cellular needs 16 (was 15)*/
		while (true)
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
		if (s_prompt_function_key_mask & (1 << (curkey + 1-FIK_F1)))
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
		if (s_prompt_function_key_mask & (1 << (curkey + 1-FIK_F1)))
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
		const char **list,          /* list of values */
		int llen,             /* number of entries in list */
		int row,              /* display row */
		int col,              /* display column */
		int (*checkkey)(int)  /* routine to check non data keys, or NULL */
)
{
	int initval;
	int curval;
	char buf[81];
	int curkey;
	int i;
	int j;
	int ret;

	MouseModeSaver saved_mouse(LOOK_MOUSE_NONE);
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
	while (true)
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
	return ret;
}

int get_fractal_type()             /* prompt for and select fractal type */
{
	int done;
	int oldfractype;
	int t;
	done = -1;
	oldfractype = g_fractal_type;
	while (true)
	{
		t = select_fracttype(g_fractal_type);
		if (t < 0)
		{
			break;
		}
		bool i = select_type_params(t, g_fractal_type);
		if (!i)  /* ok, all done */
		{
			done = 0;
			break;
		}
		if (i) /* can't return to prior image anymore */
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
	int numtypes;
	int done;
	int i;
	int j;
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
	HelpModeSaver saved_help(HELPFRACTALS);
	if (t == FRACTYPE_IFS_3D)
	{
		t = FRACTYPE_IFS;
	}
	i = j = -1;
	while (g_fractal_specific[++i].name)
	{
		if (g_julibrot)
		{
			if (!((g_fractal_specific[i].flags & FRACTALFLAG_JULIBROT) && g_fractal_specific[i].name[0] != '*'))
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
		(g_julibrot ?
			"Select Orbit Algorithm for Julibrot" : "Select a Fractal Type"),
		NULL, "Press "FK_F2" for a description of the highlighted type", numtypes,
		(char **)choices, attributes, 0, 0, 0, j, NULL, tname, NULL, sel_fractype_help);
	if (done >= 0)
	{
		done = choices[done]->num;
		if (fractal_type_formula(done) && !strcmp(g_formula_state.get_filename(), g_command_file))
		{
			g_formula_state.set_filename(g_search_for.frm);
		}
		if (done == FRACTYPE_L_SYSTEM && !strcmp(g_l_system_filename, g_command_file))
		{
			strcpy(g_l_system_filename, g_search_for.lsys);
		}
		if (fractal_type_ifs(done) && !strcmp(g_ifs_filename, g_command_file))
		{
			strcpy(g_ifs_filename, g_search_for.ifs);
		}
	}

	return done;
}

static int sel_fractype_help(int curkey, int choice)
{
	if (curkey == FIK_F2)
	{
		HelpModeSaver saved_help(g_fractal_specific[(*(ft_choices + choice))->num].helptext);
		help(0);
	}
	return 0;
}

static void set_trig_array_bifurcation(int oldfractype, int old_float_type, int old_int_type, char *value)
{
	/* Added the following to accommodate fn bifurcations.  JCO 7/2/92 */
	if (!((oldfractype == old_float_type) || (oldfractype == old_int_type)))
	{
		set_function_array(0, value);
	}
}

static void set_trig_array_ident(int old_type, int old_float_type, int old_int_type)
{
	set_trig_array_bifurcation(old_type, old_float_type, old_int_type, "ident");
}

static void set_trig_array_sin(int old_type, int old_float_type, int old_int_type)
{
	set_trig_array_bifurcation(old_type, old_float_type, old_int_type, "sin");
}

void set_default_parms()
{
	int i;
	int extra;
	g_escape_time_state.m_grid_fp.x_min() = g_current_fractal_specific->x_min;
	g_escape_time_state.m_grid_fp.x_max() = g_current_fractal_specific->x_max;
	g_escape_time_state.m_grid_fp.y_min() = g_current_fractal_specific->y_min;
	g_escape_time_state.m_grid_fp.y_max() = g_current_fractal_specific->y_max;
	g_escape_time_state.m_grid_fp.x_3rd() = g_escape_time_state.m_grid_fp.x_min();
	g_escape_time_state.m_grid_fp.y_3rd() = g_escape_time_state.m_grid_fp.y_min();

	if (g_view_crop && g_final_aspect_ratio != g_screen_aspect_ratio)
	{
		aspect_ratio_crop(g_screen_aspect_ratio, g_final_aspect_ratio);
	}
	for (i = 0; i < 4; i++)
	{
		g_parameters[i] = g_current_fractal_specific->paramvalue[i];
		if (!fractal_type_ant_or_cellular(g_fractal_type)
			&& g_fractal_type != FRACTYPE_FROTHY_BASIN
			&& g_fractal_type != FRACTYPE_FROTHY_BASIN_FP)
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
	if (g_debug_mode != DEBUGMODE_NO_BIG_TO_FLOAT)
	{
		g_bf_math = 0;
	}
	else if (g_bf_math)
	{
		fractal_float_to_bf();
	}
}

/*
	prompt for new fractal type parameters

*/
static bool select_type_params(int newfractype,		/* new fractal type */
		int oldfractype)					/* previous fractal type */
{
sel_type_restart:
	g_fractal_type = newfractype;
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];

	switch (g_fractal_type)
	{
	case FRACTYPE_L_SYSTEM:
		if (get_file_entry_help(HT_LSYS, GETFILE_L_SYSTEM,
			"L-System", lsysmask, g_l_system_filename, g_l_system_name))
		{
			return true;
		}
		break;

	case FRACTYPE_FORMULA:
	case FRACTYPE_FORMULA_FP:
		if (g_formula_state.get_file_entry(formmask))
		{
			return true;
		}
		break;

	case FRACTYPE_IFS:
	case FRACTYPE_IFS_3D:
		if (get_file_entry_help(HT_IFS, GETFILE_IFS,
			"IFS", ifsmask, g_ifs_filename, g_ifs_name))
		{
			return true;
		}
		break;

	case FRACTYPE_BIFURCATION:
	case FRACTYPE_BIFURCATION_L:		set_trig_array_ident(oldfractype, FRACTYPE_BIFURCATION, FRACTYPE_BIFURCATION_L); break;
	case FRACTYPE_BIFURCATION_STEWART:
	case FRACTYPE_BIFURCATION_STEWART_L:		set_trig_array_ident(oldfractype, FRACTYPE_BIFURCATION_STEWART, FRACTYPE_BIFURCATION_STEWART_L); break;
	case FRACTYPE_BIFURCATION_LAMBDA:
	case FRACTYPE_BIFURCATION_LAMBDA_L:		set_trig_array_ident(oldfractype, FRACTYPE_BIFURCATION_LAMBDA, FRACTYPE_BIFURCATION_LAMBDA_L); break;
	case FRACTYPE_BIFURCATION_EQUAL_FUNC_PI:
	case FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L:		set_trig_array_sin(oldfractype, FRACTYPE_BIFURCATION_EQUAL_FUNC_PI, FRACTYPE_BIFURCATION_EQUAL_FUNC_PI_L); break;
	case FRACTYPE_BIFURCATION_PLUS_FUNC_PI:
	case FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L:		set_trig_array_sin(oldfractype, FRACTYPE_BIFURCATION_PLUS_FUNC_PI, FRACTYPE_BIFURCATION_PLUS_FUNC_PI_L); break;

	/*
	* Next assumes that user going between popcorn and popcornjul
	* might not want to change function variables
	*/
	case FRACTYPE_POPCORN_FP:
	case FRACTYPE_POPCORN_L:
	case FRACTYPE_POPCORN_JULIA_FP:
	case FRACTYPE_POPCORN_JULIA_L:
		if (!((oldfractype == FRACTYPE_POPCORN_FP) || (oldfractype == FRACTYPE_POPCORN_L) ||
			(oldfractype == FRACTYPE_POPCORN_JULIA_FP) || (oldfractype == FRACTYPE_POPCORN_JULIA_L)))
		{
			set_function_parm_defaults();
		}
		break;

	/* set LATOO function defaults */
	case FRACTYPE_LATOOCARFIAN:
		if (oldfractype != FRACTYPE_LATOOCARFIAN)
		{
			set_function_parm_defaults();
		}
		break;
	}

	set_default_parms();

	if (get_fractal_parameters(0) < 0)
	{
		if (fractal_type_formula(g_fractal_type)
			|| fractal_type_ifs(g_fractal_type)
			|| g_fractal_type == FRACTYPE_L_SYSTEM)
		{
			goto sel_type_restart;
		}
		else
		{
			return true;
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

	return false;
}

#define MAXFRACTALS 25

int build_fractal_list(int fractals[], int *last_val, char *nameptr[])
{
	int numfractals;
	int i;

	numfractals = 0;
	for (i = 0; i < g_num_fractal_types; i++)
	{
		if ((g_fractal_specific[i].flags & FRACTALFLAG_JULIBROT) && g_fractal_specific[i].name[0] != '*')
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

const char *g_juli_3d_options[] =
{
	"monocular", "lefteye", "righteye", "red-blue"
};

/* JIIM */
#ifdef RANDOM_RUN
static char JIIMstr1[] = "Breadth first, Depth first, Random Walk, Random Run?";
const char *g_jiim_method[] = {"breadth", "depth", "walk", "run"};
#else
static char JIIMstr1[] = "Breadth first, Depth first, Random Walk";
const char *g_jiim_method[] =
{
	"breadth", "depth", "walk"
};
#endif
static char JIIMstr2[] = "Left first or Right first?";
const char *g_jiim_left_right[] =
{
	"left", "right"
};

struct FunctionListItem g_function_list[] =
/* changing the order of these alters meaning of *.fra file */
/* maximum 6 characters in function names or recheck all related code */
{
#if !defined(XFRACT)
	{"sin",   lStkSin,   dStkSin   },
	{"cosxx", lStkCosXX, dStkCosXX },
	{"sinh",  lStkSinh,  dStkSinh  },
	{"cosh",  lStkCosh,  dStkCosh  },
	{"exp",   lStkExp,   dStkExp   },
	{"log",   lStkLog,   dStkLog   },
	{"sqr",   lStkSqr,   dStkSqr   },
	{"recip", lStkRecip, dStkRecip }, /* from recip on new in v16 */
	{"ident", StkIdent,  StkIdent  },
	{"cos",   lStkCos,   dStkCos   },
	{"tan",   lStkTan,   dStkTan   },
	{"tanh",  lStkTanh,  dStkTanh  },
	{"cotan", lStkCoTan, dStkCoTan },
	{"cotanh", lStkCoTanh, dStkCoTanh },
	{"flip",  lStkFlip,  dStkFlip  },
	{"conj",  lStkConj,  dStkConj  },
	{"zero",  lStkZero,  dStkZero  },
	{"asin",  lStkASin,  dStkASin  },
	{"asinh", lStkASinh, dStkASinh },
	{"acos",  lStkACos,  dStkACos  },
	{"acosh", lStkACosh, dStkACosh },
	{"atan",  lStkATan,  dStkATan  },
	{"atanh", lStkATanh, dStkATanh },
	{"cabs",  lStkCAbs,  dStkCAbs  },
	{"abs",   lStkAbs,   dStkAbs   },
	{"sqrt",  lStkSqrt,  dStkSqrt  },
	{"floor", lStkFloor, dStkFloor },
	{"ceil",  lStkCeil,  dStkCeil  },
	{"trunc", lStkTrunc, dStkTrunc },
	{"round", lStkRound, dStkRound },
	{"one",   lStkOne,   dStkOne   },
#else
	{"sin",   dStkSin,   dStkSin   },
	{"cosxx", dStkCosXX, dStkCosXX },
	{"sinh",  dStkSinh,  dStkSinh  },
	{"cosh",  dStkCosh,  dStkCosh  },
	{"exp",   dStkExp,   dStkExp   },
	{"log",   dStkLog,   dStkLog   },
	{"sqr",   dStkSqr,   dStkSqr   },
	{"recip", dStkRecip, dStkRecip }, /* from recip on new in v16 */
	{"ident", StkIdent,  StkIdent  },
	{"cos",   dStkCos,   dStkCos   },
	{"tan",   dStkTan,   dStkTan   },
	{"tanh",  dStkTanh,  dStkTanh  },
	{"cotan", dStkCoTan, dStkCoTan },
	{"cotanh", dStkCoTanh, dStkCoTanh },
	{"flip",  dStkFlip,  dStkFlip  },
	{"conj",  dStkConj,  dStkConj  },
	{"zero",  dStkZero,  dStkZero  },
	{"asin",  dStkASin,  dStkASin  },
	{"asinh", dStkASinh, dStkASinh },
	{"acos",  dStkACos,  dStkACos  },
	{"acosh", dStkACosh, dStkACosh },
	{"atan",  dStkATan,  dStkATan  },
	{"atanh", dStkATanh, dStkATanh },
	{"cabs",  dStkCAbs,  dStkCAbs  },
	{"abs",   dStkAbs,   dStkAbs   },
	{"sqrt",  dStkSqrt,  dStkSqrt  },
	{"floor", dStkFloor, dStkFloor },
	{"ceil",  dStkCeil,  dStkCeil  },
	{"trunc", dStkTrunc, dStkTrunc },
	{"round", dStkRound, dStkRound },
	{"one",   dStkOne,   dStkOne   },
#endif
};

const int g_num_function_list = NUM_OF(g_function_list);

/* --------------------------------------------------------------------- */
int get_fractal_parameters(int caller)        /* prompt for type-specific parms */
{
	char *v0 = "From cx (real part)";
	char *v1 = "From cy (imaginary part)";
	char *v2 = "To   cx (real part)";
	char *v3 = "To   cy (imaginary part)";
	char *juliorbitname = NULL;
	int i;
	int j;
	int k;
	int current_fractal_type;
	int num_parameters;
	int num_functions;
	struct full_screen_values parameter_values[30];
	const char *choices[30];
	long old_bail_out = 0L;
	char message[120];
	char *type_name;
	char *tmpptr;
	char bailoutmsg[50];
	int ret = 0;
	char parmprompt[MAX_PARAMETERS][55];
	static char *trg[] =
	{
		"First Function", "Second Function", "Third Function", "Fourth Function"
	};
	char *filename;
	char *entryname;
	FILE *entryfile;
#ifdef XFRACT
	static /* Can't initialize aggregates on the stack */
#endif
	const char *bailnameptr[] = {"mod", "real", "imag", "or", "and", "manh", "manr"};
	FractalTypeSpecificData *jborbit = NULL;
	FractalTypeSpecificData *savespecific;
	int firstparm = 0;
	int lastparm  = MAX_PARAMETERS;
	double oldparam[MAX_PARAMETERS];
	int f_key_mask = 0x40;
	old_bail_out = g_bail_out;
	if (fractal_type_julibrot(g_fractal_type))
	{
		g_julibrot = true;
	}
	else
	{
		g_julibrot = false;
	}
	current_fractal_type = g_fractal_type;
	i = g_current_fractal_specific->tofloat;
	if (g_current_fractal_specific->name[0] == '*'
		&& !fractal_type_none(i)
		&& g_fractal_specific[i].name[0] != '*')
	{
		current_fractal_type = i;
	}
	g_current_fractal_specific = &g_fractal_specific[current_fractal_type];
	g_text_stack[0] = 0;
	i = g_current_fractal_specific->helpformula;
	if (i < -1)
	{
		int itemtype = ITEMTYPE_PARAMETER;
		if (i == -2)  /* special for formula */
		{
			if (g_formula_state.find_item(&entryfile) == 0)
			{
				load_entry_text(entryfile, g_text_stack, 17, 0, 0);
				fclose(entryfile);
				if (fractal_type_formula(g_fractal_type))
				{
					g_formula_state.get_parameter(g_formula_state.get_formula()); /* no error check, should be okay, from above */
				}
			}
		}
		else
		{
			if (i == -3)   /* special for lsystem */
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
				_ASSERTE(false);
#endif
				filename = NULL;
				entryname = NULL;
			}
			if (find_file_item(filename, entryname, &entryfile, itemtype) == 0)
			{
				load_entry_text(entryfile, g_text_stack, 17, 0, 0);
				fclose(entryfile);
				if (fractal_type_formula(g_fractal_type))
				{
					g_formula_state.get_parameter(entryname); /* no error check, should be okay, from above */
				}
			}
		}
	}
	else if (i >= 0)
	{
		int c;
		int lines;
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
				g_text_stack[j++] = (char) c;
			}
		}
		while (--j >= 0 && g_text_stack[j] == '\n')
		{
		}
		g_text_stack[j + 1] = 0;
	}

gfp_top:
	int prompt = 0;
	if (g_julibrot)
	{
		i = select_fracttype(g_new_orbit_type);
		if (i < 0)
		{
			if (ret == 0)
			{
				ret = -1;
			}
			g_julibrot = false;
			goto gfp_exit;
		}
		else
		{
			g_new_orbit_type = i;
		}
		jborbit = &g_fractal_specific[g_new_orbit_type];
		juliorbitname = jborbit->name;
	}

	if (fractal_type_formula(g_fractal_type))
	{
		if (g_formula_state.uses_p1())  /* set first parameter */
		{
			firstparm = 0;
		}
		else if (g_formula_state.uses_p2())
		{
			firstparm = 2;
		}
		else if (g_formula_state.uses_p3())
		{
			firstparm = 4;
		}
		else if (g_formula_state.uses_p4())
		{
			firstparm = 6;
		}
		else
		{
			firstparm = 8; /* g_formula_state.uses_p5() or no parameter */
		}

		if (g_formula_state.uses_p5())  /* set last parameter */
		{
			lastparm = 10;
		}
		else if (g_formula_state.uses_p4())
		{
			lastparm = 8;
		}
		else if (g_formula_state.uses_p3())
		{
			lastparm = 6;
		}
		else if (g_formula_state.uses_p2())
		{
			lastparm = 4;
		}
		else
		{
			lastparm = 2; /* g_formula_state.uses_p1() or no parameter */
		}
	}

	savespecific = g_current_fractal_specific;
	if (g_julibrot)
	{
		g_current_fractal_specific = jborbit;
		firstparm = 2; /* in most case Julibrot does not need first two parms */
		if (g_new_orbit_type == FRACTYPE_QUATERNION_JULIA_FP     ||   /* all parameters needed */
			g_new_orbit_type == FRACTYPE_HYPERCOMPLEX_JULIA_FP)
		{
			firstparm = 0;
			lastparm = 4;
		}
		if (g_new_orbit_type == FRACTYPE_QUATERNION_FP        ||   /* no parameters needed */
			g_new_orbit_type == FRACTYPE_HYPERCOMPLEX_FP)
		{
			firstparm = 4;
		}
	}
	num_parameters = 0;
	j = 0;
	for (i = firstparm; i < lastparm; i++)
	{
		char tmpbuf[30];
		if (!type_has_parameter(g_julibrot ? g_new_orbit_type : g_fractal_type, i, parmprompt[j]))
		{
			if (fractal_type_formula(current_fractal_type))
			{
				if (parameter_not_used(i))
				{
					continue;
				}
			}
			break;
		}
		num_parameters++;
		choices[prompt] = parmprompt[j++];
		parameter_values[prompt].type = 'd';

		if (choices[prompt][0] == '+')
		{
			choices[prompt]++;
			parameter_values[prompt].type = 'D';
		}
		else if (choices[prompt][0] == '#')
		{
			choices[prompt]++;
		}
		sprintf(tmpbuf, "%.17g", g_parameters[i]);
		parameter_values[prompt].uval.dval = atof(tmpbuf);
		oldparam[i] = parameter_values[prompt++].uval.dval;
	}

	/* The following is a goofy kludge to make reading in the formula
	* parameters work.
	*/
	if (fractal_type_formula(current_fractal_type))
	{
		num_parameters = lastparm - firstparm;
	}

	num_functions = g_current_fractal_specific->num_functions();
	if (fractal_type_formula(current_fractal_type))
	{
		num_functions = g_formula_state.max_fn();
	}

	const char *function_names[NUM_OF(g_function_list)];
	for (i = 0; i < g_num_function_list; i++)
	{
		function_names[i] = g_function_list[i].name;
	}
	for (i = 0; i < num_functions; i++)
	{
		parameter_values[prompt].type = 'l';
		parameter_values[prompt].uval.ch.val  = g_function_index[i];
		parameter_values[prompt].uval.ch.llen = g_num_function_list;
		parameter_values[prompt].uval.ch.vlen = 6;
		parameter_values[prompt].uval.ch.list = function_names;
		choices[prompt++] = (char *)trg[i];
	}
	type_name = g_current_fractal_specific->name;
	if (type_name[0] == '*')
	{
		++type_name;
	}

	i = g_current_fractal_specific->orbit_bailout;

	if (i != 0 && g_current_fractal_specific->calculate_type == standard_fractal &&
		(g_current_fractal_specific->flags & FRACTALFLAG_BAIL_OUT_TESTS))
	{
		parameter_values[prompt].type = 'l';
		parameter_values[prompt].uval.ch.val  = (int)g_bail_out_test;
		parameter_values[prompt].uval.ch.llen = 7;
		parameter_values[prompt].uval.ch.vlen = 6;
		parameter_values[prompt].uval.ch.list = bailnameptr;
		choices[prompt++] = "Bailout Test (mod, real, imag, or, and, manh, manr)";
	}

	if (i)
	{
		if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != 0.0)
		{
			parameter_values[prompt].type = '*';
			choices[prompt++] = "Bailout: continuous potential (Y screen) value in use";
		}
		else
		{
			choices[prompt] = "Bailout value (0 means use default)";
			parameter_values[prompt].type = 'L';
			old_bail_out = g_bail_out;
			parameter_values[prompt++].uval.Lval = old_bail_out;
			parameter_values[prompt].type = '*';
			tmpptr = type_name;
			if (g_user_biomorph != -1)
			{
				i = 100;
				tmpptr = "biomorph";
			}
			sprintf(bailoutmsg, "    (%s default is %d)", tmpptr, i);
			choices[prompt++] = bailoutmsg;
		}
	}
	if (g_julibrot)
	{
		switch (g_new_orbit_type)
		{
		case FRACTYPE_QUATERNION_FP:
		case FRACTYPE_HYPERCOMPLEX_FP:
			v0 = "From cj (3rd dim)";
			v1 = "From ck (4th dim)";
			v2 = "To   cj (3rd dim)";
			v3 = "To   ck (4th dim)";
			break;
		case FRACTYPE_QUATERNION_JULIA_FP:
		case FRACTYPE_HYPERCOMPLEX_JULIA_FP:
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
		parameter_values[prompt].uval.dval = g_m_x_max_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = v0;
		parameter_values[prompt].uval.dval = g_m_y_max_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = v1;
		parameter_values[prompt].uval.dval = g_m_x_min_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = v2;
		parameter_values[prompt].uval.dval = g_m_y_min_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = v3;
		parameter_values[prompt].uval.ival = g_z_dots;
		parameter_values[prompt].type = 'i';
		choices[prompt++] = "Number of z pixels";

		parameter_values[prompt].type = 'l';
		parameter_values[prompt].uval.ch.val  = g_juli_3d_mode;
		parameter_values[prompt].uval.ch.llen = 4;
		parameter_values[prompt].uval.ch.vlen = 9;
		parameter_values[prompt].uval.ch.list = g_juli_3d_options;
		choices[prompt++] = "3D Mode";

		parameter_values[prompt].uval.dval = g_eyes_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = "Distance between eyes";
		parameter_values[prompt].uval.dval = g_origin_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = "Location of z origin";
		parameter_values[prompt].uval.dval = g_depth_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = "Depth of z";
		parameter_values[prompt].uval.dval = g_height_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = "Screen height";
		parameter_values[prompt].uval.dval = g_width_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = "Screen width";
		parameter_values[prompt].uval.dval = g_screen_distance_fp;
		parameter_values[prompt].type = 'f';
		choices[prompt++] = "Distance to Screen";
	}

	if (fractal_type_inverse_julia(current_fractal_type))
	{
		choices[prompt] = JIIMstr1;
		parameter_values[prompt].type = 'l';
		parameter_values[prompt].uval.ch.list = g_jiim_method;
		parameter_values[prompt].uval.ch.vlen = 7;
#ifdef RANDOM_RUN
		paramvalues[promptnum].uval.ch.llen = 4;
#else
		parameter_values[prompt].uval.ch.llen = 3; /* disable random run */
#endif
		parameter_values[prompt++].uval.ch.val  = g_major_method;

		choices[prompt] = JIIMstr2;
		parameter_values[prompt].type = 'l';
		parameter_values[prompt].uval.ch.list = g_jiim_left_right;
		parameter_values[prompt].uval.ch.vlen = 5;
		parameter_values[prompt].uval.ch.llen = 2;
		parameter_values[prompt++].uval.ch.val  = g_minor_method;
	}

	if (fractal_type_formula(current_fractal_type) && g_formula_state.uses_is_mand())
	{
		choices[prompt] = "ismand";
		parameter_values[prompt].type = 'y';
		parameter_values[prompt++].uval.ch.val = g_is_mand ? 1 : 0;
	}

	if (caller                           /* <z> command ? */
		&& (g_display_3d > DISPLAY3D_NONE))
	{
		stop_message(STOPMSG_INFO_ONLY | STOPMSG_NO_BUZZER, "Current type has no type-specific parameters");
		goto gfp_exit;
	}
	if (g_julibrot)
	{
		sprintf(message, "Julibrot Parameters (orbit= %s)", juliorbitname);
	}
	else
	{
		sprintf(message, "Parameters for fractal type %s", type_name);
	}
	if (g_bf_math == 0)
	{
		strcat(message, "\n(Press "FK_F6" for corner parameters)");
	}
	else
	{
		f_key_mask = 0;
	}
	s_scroll_row_status = 0; /* make sure we start at beginning of entry */
	s_scroll_column_status = 0;
	while (true)
	{
		i = full_screen_prompt_help(g_current_fractal_specific->helptext, message,
			prompt, choices, parameter_values, f_key_mask, g_text_stack);
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
		if (g_bf_math == 0)
		{
			if (get_corners() > 0)
			{
				ret = 1;
			}
		}
	}
	prompt = 0;
	for (i = firstparm; i < num_parameters + firstparm; i++)
	{
		if (fractal_type_formula(current_fractal_type))
		{
			if (parameter_not_used(i))
			{
				continue;
			}
		}
		if (oldparam[i] != parameter_values[prompt].uval.dval)
		{
			g_parameters[i] = parameter_values[prompt].uval.dval;
			ret = 1;
		}
		++prompt;
	}

	for (i = 0; i < num_functions; i++)
	{
		if (parameter_values[prompt].uval.ch.val != g_function_index[i])
		{
			set_function_array(i, g_function_list[parameter_values[prompt].uval.ch.val].name);
			ret = 1;
		}
		++prompt;
	}

	if (g_julibrot)
	{
		savespecific = g_current_fractal_specific;
		g_current_fractal_specific = jborbit;
	}

	i = g_current_fractal_specific->orbit_bailout;

	if (i != 0 && g_current_fractal_specific->calculate_type == standard_fractal &&
		(g_current_fractal_specific->flags & FRACTALFLAG_BAIL_OUT_TESTS))
	{
		if (parameter_values[prompt].uval.ch.val != (int)g_bail_out_test)
		{
			g_bail_out_test = (enum bailouts)parameter_values[prompt].uval.ch.val;
			ret = 1;
		}
		prompt++;
	}
	else
	{
		g_bail_out_test = BAILOUT_MODULUS;
	}
	set_bail_out_formula(g_bail_out_test);

	if (i)
	{
		if (g_potential_parameter[0] != 0.0 && g_potential_parameter[2] != 0.0)
		{
			prompt++;
		}
		else
		{
			g_bail_out = parameter_values[prompt++].uval.Lval;
			if (g_bail_out != 0 && (g_bail_out < 1 || g_bail_out > 2100000000L))
			{
				g_bail_out = old_bail_out;
			}
			if (g_bail_out != old_bail_out)
			{
				ret = 1;
			}
			prompt++;
		}
	}
	if (g_julibrot)
	{
		g_m_x_max_fp    = parameter_values[prompt++].uval.dval;
		g_m_y_max_fp    = parameter_values[prompt++].uval.dval;
		g_m_x_min_fp    = parameter_values[prompt++].uval.dval;
		g_m_y_min_fp    = parameter_values[prompt++].uval.dval;
		g_z_dots      = parameter_values[prompt++].uval.ival;
		g_juli_3d_mode = parameter_values[prompt++].uval.ch.val;
		g_eyes_fp     = (float)parameter_values[prompt++].uval.dval;
		g_origin_fp   = (float)parameter_values[prompt++].uval.dval;
		g_depth_fp    = (float)parameter_values[prompt++].uval.dval;
		g_height_fp   = (float)parameter_values[prompt++].uval.dval;
		g_width_fp    = (float)parameter_values[prompt++].uval.dval;
		g_screen_distance_fp     = (float)parameter_values[prompt++].uval.dval;
		ret = 1;  /* force new calc since not resumable anyway */
	}
	if (fractal_type_inverse_julia(current_fractal_type))
	{
		if (parameter_values[prompt].uval.ch.val != g_major_method
			|| parameter_values[prompt + 1].uval.ch.val != g_minor_method)
		{
			ret = 1;
		}
		g_major_method = static_cast<MajorMethodType>(parameter_values[prompt++].uval.ch.val);
		g_minor_method = static_cast<MinorMethodType>(parameter_values[prompt++].uval.ch.val);
	}
	if (fractal_type_formula(current_fractal_type) && g_formula_state.uses_is_mand())
	{
		if (g_is_mand != (parameter_values[prompt].uval.ch.val != 0))
		{
			g_is_mand = (parameter_values[prompt].uval.ch.val != 0);
			ret = 1;
		}
		++prompt;
	}

gfp_exit:
	g_current_fractal_specific = &g_fractal_specific[g_fractal_type];
	return ret;
}

int find_extra_parameter(int type)
{
	int i;
	int ret;
	int curtyp;
	ret = -1;
	i = -1;

	if (g_fractal_specific[type].flags&FRACTALFLAG_MORE_PARAMETERS)
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
	int i;
	int extra;
	for (i = 0; i < 4; ++i)
	{
		g_parameters[i] = g_fractal_specific[g_fractal_type].paramvalue[i];
		if (!fractal_type_ant_or_cellular(g_fractal_type))
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
	int i;
	int numtypes;
	int bad;
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

long get_file_entry(int type, const char *title, char *fmask,
					char *filename, char *entryname)
{
	/* Formula, LSystem, etc type structure, select from file */
	/* containing definitions in the form    name { ... }     */
	int newfile;
	int firsttry;
	long entry_pointer;
	newfile = 0;
	while (true)
	{
		firsttry = 0;
		/* pb: binary mode used here - it is more work, but much faster, */
		/*     especially when ftell or fgetpos is used                  */
		while (newfile || (s_gfe_file = fopen(filename, "rb")) == NULL)
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
		setvbuf(s_gfe_file, g_text_stack, _IOFBF, 4096); /* improves speed when file is big */
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
			if (g_formula_state.RunFormula(entryname, true) == 0)
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
				g_fractal_type = (g_ifs_type == IFSTYPE_2D) ? FRACTYPE_IFS : FRACTYPE_IFS_3D;
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
static const char *gfe_title;

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

int scan_entries(FILE *infile, struct entryinfo *choices, const char *itemname)
{
		/*
		function returns the number of entries found; if a
		specific entry is being looked for, returns -1 if
		the entry is found, 0 otherwise.
		*/
	char buf[101];
	int exclude_entry;
	long name_offset;
	long temp_offset;   /*rev 5/23/96 to add temp_offset,
                                      used below to skip any '{' that
                                      does not have a corresponding
                                      '}' - GGM */
	long file_offset = -1;
	int numentries = 0;

	while (true)
	{                            /* scan the file for entry names */
		int c;
		int len;
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
static long gfe_choose_entry(int type, const char *title, char *filename, char *entryname)
{
#ifdef XFRACT
	char *o_instr = "Press "FK_F6" to select file, "FK_F2" for details, "FK_F4" to toggle sort ";
	/* keep the above line length < 80 characters */
#else
	char *o_instr = "Press "FK_F6" to select different file, "FK_F2" for details, "FK_F4" to toggle sort ";
#endif
	int numentries;
	int i;
	char buf[101];
	struct entryinfo storage[MAXENTRIES + 1];
	struct entryinfo *choices[MAXENTRIES + 1] = { NULL };
	int attributes[MAXENTRIES + 1] = { 0 };
	void (*formatitem)(int, char *);
	int boxwidth;
	int boxdepth;
	int colwidth;
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

	numentries = scan_entries(s_gfe_file, &storage[0], NULL);
	if (numentries == 0)
	{
		stop_message(0, "File doesn't contain any valid entries");
		fclose(s_gfe_file);
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
		rewind(s_gfe_file);
		dosort = 1-dosort;
		goto retry;
	}
	fclose(s_gfe_file);
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
	bool in_scrolling_mode = false; /* true if entry doesn't fit available space */
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
		fseek(s_gfe_file, gfe_choices[choice]->point, SEEK_SET);
		while ((c = fgetc(s_gfe_file)) != EOF && c != '\032')
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
			fseek(s_gfe_file, gfe_choices[choice]->point, SEEK_SET);
			in_scrolling_mode = false;
		}
		fseek(s_gfe_file, gfe_choices[choice]->point, SEEK_SET);
		load_entry_text(s_gfe_file, infbuf, 17, 0, 0);
		if (lines_in_entry > 17 || widest_entry_line > 74)
		{
			in_scrolling_mode = true;
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
				fseek(s_gfe_file, gfe_choices[choice]->point, SEEK_SET);
				load_entry_text(s_gfe_file, infbuf, 17, top_line, left_column);
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

	int linelen;
	int i;
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
	int c;
	int i;
	char line[80];
	fseek(s_gfe_file, gfe_choices[choice]->point, SEEK_SET);
	do
	{
		c = getc(s_gfe_file);
	}
	while (c != '{');
	do
	{
		c = getc(s_gfe_file);
	}
	while (c == ' ' || c == '\t' || c == ';');
	i = 0;
	while (i < 56 && c != '\n' && c != '\r' && c != EOF && c != '\032')
	{
		line[i++] = (char)((c == '\t') ? ' ' : c);
		c = getc(s_gfe_file);
	}
	line[i] = 0;
#ifndef XFRACT
	sprintf(buf, "%-20Fs%-56s", gfe_choices[choice]->name, line);
#else
	sprintf(buf, "%-20s%-56s", gfe_choices[choice]->name, line);
#endif
}

/* --------------------------------------------------------------------- */

static int get_fractal_3d_parameters_aux()
{
	UIChoices dialog(HELP3DFRACT, "3D Parameters", 0);
	dialog.push("X-axis rotation in degrees", g_3d_state.x_rotation());
	dialog.push("Y-axis rotation in degrees", g_3d_state.y_rotation());
	dialog.push("Z-axis rotation in degrees", g_3d_state.z_rotation());
	dialog.push("Perspective distance [1 - 999, 0 for no persp]", g_3d_state.z_viewer());
	dialog.push("X shift with perspective (positive = right)", g_3d_state.x_shift());
	dialog.push("Y shift with perspective (positive = up)", g_3d_state.y_shift());
	dialog.push("Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,3=photo,4=stereo pair)",
		g_3d_state.glasses_type());
	int i = dialog.prompt();
	if (i < 0)
	{
		return -1;
	}

	int k = 0;
	g_3d_state.set_x_rotation(dialog.values(k++).uval.ival);
	g_3d_state.set_y_rotation(dialog.values(k++).uval.ival);
	g_3d_state.set_z_rotation(dialog.values(k++).uval.ival);
	g_3d_state.set_z_viewer(dialog.values(k++).uval.ival);
	g_3d_state.set_x_shift(dialog.values(k++).uval.ival);
	g_3d_state.set_y_shift(dialog.values(k++).uval.ival);
	{
		int glasses_type = dialog.values(k++).uval.ival;
		if (glasses_type < STEREO_NONE || glasses_type > STEREO_PAIR)
		{
			glasses_type = 0;
		}
		g_3d_state.set_glasses_type(GlassesType(glasses_type));
		if (g_3d_state.glasses_type())
		{
			if (get_funny_glasses_params() || check_mapfile())
			{
				return -1;
			}
		}
	}

	return 0;
}

int get_fractal_3d_parameters() /* prompt for 3D fractal parameters */
{
	ScreenStacker stacker;
	return get_fractal_3d_parameters_aux();
}

int get_3d_parameters()     /* prompt for 3D parameters */
{
	const char *choices[11];
	int attributes[21];
	const char *prompts3d[21];
	struct full_screen_values uvalues[21];

restart_1:
	if (g_targa_output && g_overlay_3d)
	{
		g_targa_overlay = true;
	}

	int k = -1;

	prompts3d[++k] = "Preview Mode?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_3d_state.preview();

	prompts3d[++k] = "    Show Box?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_3d_state.show_box() ? 1 : 0;

	prompts3d[++k] = "Coarseness, preview/grid/ray (in y dir)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_3d_state.preview_factor();

	prompts3d[++k] = "Spherical Projection?";
	uvalues[k].type = 'y';
	int sphere = g_3d_state.sphere();
	uvalues[k].uval.ch.val = sphere;

	prompts3d[++k] = "Stereo (R/B 3D)? (0=no,1=alternate,2=superimpose,";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_3d_state.glasses_type();

	prompts3d[++k] = "                  3=photo,4=stereo pair)";
	uvalues[k].type = '*';

	prompts3d[++k] = "Ray trace out? (0=No, 1=DKB/POVRay, 2=VIVID, 3=RAW,";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_3d_state.raytrace_output();

	prompts3d[++k] = "                4=MTV, 5=RAYSHADE, 6=ACROSPIN, 7=DXF)";
	uvalues[k].type = '*';

	prompts3d[++k] = "    Brief output?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_3d_state.raytrace_brief();

	g_3d_state.next_ray_name();
	prompts3d[++k] = "    Output File Name";
	uvalues[k].type = 's';
	strcpy(uvalues[k].uval.sval, g_3d_state.ray_name());

	prompts3d[++k] = "Targa output?";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_targa_output ? 1 : 0;

	prompts3d[++k] = "Use grayscale value for depth? (if \"no\" uses color number)";
	uvalues[k].type = 'y';
	uvalues[k].uval.ch.val = g_grayscale_depth ? 1 : 0;

	k = full_screen_prompt_help(HELP3DMODE, "3D Mode Selection", k + 1, prompts3d, uvalues, 0, NULL);
	if (k < 0)
	{
		return -1;
	}

	k = 0;

	g_3d_state.set_preview(uvalues[k++].uval.ch.val != 0);
	g_3d_state.set_show_box(uvalues[k++].uval.ch.val != 0);
	g_3d_state.set_preview_factor(uvalues[k++].uval.ival);
	g_3d_state.set_sphere(uvalues[k++].uval.ch.val != 0);
	g_3d_state.set_glasses_type(GlassesType(uvalues[k++].uval.ival));
	k++;

	g_3d_state.set_raytrace_output(uvalues[k++].uval.ival);
	k++;
	if (g_3d_state.raytrace_output() == RAYTRACE_POVRAY)
	{
		stop_message(0, "DKB/POV-Ray output is obsolete but still works. See \"Ray Tracing Output\" in\n"
		"the online documentation.");
	}
	g_3d_state.set_raytrace_brief(uvalues[k++].uval.ch.val != 0);

	g_3d_state.set_ray_name(uvalues[k++].uval.sval);

	g_targa_output = (uvalues[k++].uval.ch.val != 0);
	g_grayscale_depth  = (uvalues[k++].uval.ch.val != 0);

	/* check ranges */
	if (g_3d_state.preview_factor() < 2)
	{
		g_3d_state.set_preview_factor(2);
	}
	else if (g_3d_state.preview_factor() > 2000)
	{
		g_3d_state.set_preview_factor(2000);
	}

	if (sphere && !g_3d_state.sphere())
	{
		g_3d_state.set_defaults();
		g_3d_state.set_sphere(true);
	}
	else if (!sphere && g_3d_state.sphere())
	{
		g_3d_state.set_defaults();
	}

	if (g_3d_state.glasses_type() < STEREO_NONE)
	{
		g_3d_state.set_glasses_type(STEREO_NONE);
	}
	else if (g_3d_state.glasses_type() > STEREO_PAIR)
	{
		g_3d_state.set_glasses_type(STEREO_PAIR);
	}
	if (g_3d_state.glasses_type())
	{
		g_which_image = WHICHIMAGE_RED;
	}

	if (g_3d_state.raytrace_output() < RAYTRACE_NONE)
	{
		g_3d_state.set_raytrace_output(RAYTRACE_NONE);
	}
	if (g_3d_state.raytrace_output() > RAYTRACE_DXF)
	{
		g_3d_state.set_raytrace_output(RAYTRACE_DXF);
	}

	if (!g_3d_state.raytrace_output())
	{
		k = 0;
		choices[k++] = "make a surface grid";
		choices[k++] = "just draw the points";
		choices[k++] = "connect the dots (wire frame)";
		choices[k++] = "surface fill (colors interpolated)";
		choices[k++] = "surface fill (colors not interpolated)";
		choices[k++] = "solid fill (bars up from \"ground\")";
		if (g_3d_state.sphere())
		{
			choices[k++] = "light source";
		}
		else
		{
			choices[k++] = "light source before transformation";
			choices[k++] = "light source after transformation";
		}
		for (int i = 0; i < k; ++i)
		{
			attributes[i] = 1;
		}
		int i = full_screen_choice_help(HELP3DFILL, CHOICE_HELP,
			"Select 3D Fill Type", NULL, NULL, k, (char **) choices, attributes,
			0, 0, 0, g_3d_state.fill_type() + 1, NULL, NULL, NULL, NULL);
		if (i < 0)
		{
			goto restart_1;
		}
		g_3d_state.set_fill_type(i - 1);

		if (g_3d_state.glasses_type())
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

	if (g_3d_state.sphere())
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
		if (!g_3d_state.raytrace_output())
		{
			prompts3d[++k] = "X-axis rotation in degrees";
			prompts3d[++k] = "Y-axis rotation in degrees";
			prompts3d[++k] = "Z-axis rotation in degrees";
		}
		prompts3d[++k] = "X-axis scaling factor in pct";
		prompts3d[++k] = "Y-axis scaling factor in pct";
	}
	k = -1;
	if (!(g_3d_state.raytrace_output() && !g_3d_state.sphere()))
	{
		uvalues[++k].uval.ival   = g_3d_state.x_rotation();
		uvalues[k].type = 'i';
		uvalues[++k].uval.ival   = g_3d_state.y_rotation();
		uvalues[k].type = 'i';
		uvalues[++k].uval.ival   = g_3d_state.z_rotation();
		uvalues[k].type = 'i';
	}
	uvalues[++k].uval.ival   = g_3d_state.x_scale();
	uvalues[k].type = 'i';

	uvalues[++k].uval.ival   = g_3d_state.y_scale();
	uvalues[k].type = 'i';

	prompts3d[++k] = "Surface Roughness scaling factor in pct";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_3d_state.roughness();

	prompts3d[++k] = "'Water Level' (minimum color value)";
	uvalues[k].type = 'i';
	uvalues[k].uval.ival = g_3d_state.water_line();

	if (!g_3d_state.raytrace_output())
	{
		prompts3d[++k] = "Perspective distance [1 - 999, 0 for no persp])";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.z_viewer();

		prompts3d[++k] = "X shift with perspective (positive = right)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.x_shift();

		prompts3d[++k] = "Y shift with perspective (positive = up   )";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.y_shift();

		prompts3d[++k] = "Image non-perspective X adjust (positive = right)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.x_trans();

		prompts3d[++k] = "Image non-perspective Y adjust (positive = up)";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.y_trans();

		prompts3d[++k] = "First transparent color";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.transparent0();

		prompts3d[++k] = "Last transparent color";
		uvalues[k].type = 'i';
		uvalues[k].uval.ival = g_3d_state.transparent1();
	}

	prompts3d[++k] = "Randomize Colors      (0 - 7, '0' disables)";
	uvalues[k].type = 'i';
	uvalues[k++].uval.ival = g_3d_state.randomize_colors();

	const char *s;
	if (g_3d_state.sphere())
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

	k = full_screen_prompt_help(HELP3DPARMS, s, k, prompts3d, uvalues, 0, NULL);
	if (k < 0)
	{
		goto restart_1;
	}

	k = 0;
	if (!(g_3d_state.raytrace_output() && !g_3d_state.sphere()))
	{
		g_3d_state.set_x_rotation(uvalues[k++].uval.ival);
		g_3d_state.set_y_rotation(uvalues[k++].uval.ival);
		g_3d_state.set_z_rotation(uvalues[k++].uval.ival);
	}
	g_3d_state.set_x_scale(uvalues[k++].uval.ival);
	g_3d_state.set_y_scale(uvalues[k++].uval.ival);
	g_3d_state.set_roughness(uvalues[k++].uval.ival);
	g_3d_state.set_water_line(uvalues[k++].uval.ival);
	if (!g_3d_state.raytrace_output())
	{
		g_3d_state.set_z_viewer(uvalues[k++].uval.ival);
		g_3d_state.set_x_shift(uvalues[k++].uval.ival);
		g_3d_state.set_y_shift(uvalues[k++].uval.ival);
		g_3d_state.set_x_trans(uvalues[k++].uval.ival);
		g_3d_state.set_y_trans(uvalues[k++].uval.ival);
		g_3d_state.set_transparent0(uvalues[k++].uval.ival);
		g_3d_state.set_transparent1(uvalues[k++].uval.ival);
	}
	int randomize_colors  = uvalues[k++].uval.ival;
	if (randomize_colors >= 7)
	{
		randomize_colors = 7;
	}
	if (randomize_colors <= 0)
	{
		randomize_colors = 0;
	}
	g_3d_state.set_randomize_colors(randomize_colors);

	if ((g_targa_output || (g_3d_state.fill_type() > FillType::Bars) || g_3d_state.raytrace_output()))
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
	UIChoices dialog(HELP3DLIGHT, "Light Source Parameters", 0);
	if ((g_3d_state.fill_type() > FillType::Bars) || g_3d_state.raytrace_output())
	{
		dialog.push("X value light vector", g_3d_state.x_light());
		dialog.push("Y value light vector", g_3d_state.y_light());
		dialog.push("Z value light vector", g_3d_state.z_light());

		if (!g_3d_state.raytrace_output())
		{
			dialog.push("Light Source Smoothing Factor", g_3d_state.light_avg());
			dialog.push("Ambient", g_3d_state.ambient());
		}
	}

	if (g_targa_output && !g_3d_state.raytrace_output())
	{
		dialog.push("Haze Factor        (0 - 100, '0' disables)", g_3d_state.haze());
		if (!g_targa_overlay)
		{
			check_write_file(g_light_name, ".tga");
		}
		dialog.push("Targa File Name  (Assume .tga)", g_light_name);
		dialog.push("Back Ground Color (0 - 255)");
		dialog.push("   Red", (int) g_3d_state.background_red());
		dialog.push("   Green", (int) g_3d_state.background_green());
		dialog.push("   Blue", (int) g_3d_state.background_blue());
		dialog.push("Overlay Targa File? (Y/N)", g_targa_overlay);
	}
	dialog.push("");
	if (dialog.prompt() < 0)
	{
		return -1;
	}

	int k = 0;
	if ((g_3d_state.fill_type() > FillType::Bars))
	{
		g_3d_state.set_x_light(dialog.values(k++).uval.ival);
		g_3d_state.set_y_light(dialog.values(k++).uval.ival);
		g_3d_state.set_z_light(dialog.values(k++).uval.ival);
		if (!g_3d_state.raytrace_output())
		{
			g_3d_state.set_light_average(dialog.values(k++).uval.ival);
			g_3d_state.set_ambient(MathUtil::Clamp(dialog.values(k++).uval.ival, 0, 100));
		}
	}

	if (g_targa_output && !g_3d_state.raytrace_output())
	{
		g_3d_state.set_haze(MathUtil::Clamp(dialog.values(k++).uval.ival, 0, 100));
		strcpy(g_light_name, dialog.values(k++).uval.sval);
        /* In case g_light_name conflicts with an existing name it is checked
						again in line3d */
		k++;
		g_3d_state.set_background_color(
			(BYTE) (dialog.values(k + 0).uval.ival % 255),
			(BYTE) (dialog.values(k + 1).uval.ival % 255),
			(BYTE) (dialog.values(k + 2).uval.ival % 255));
		k += 3;
		g_targa_overlay = (dialog.values(k).uval.ch.val != 0);
	}
	return 0;
}

/* --------------------------------------------------------------------- */


static int check_mapfile()
{
	int askflag = 0;
	int i;
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
	if (!(g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE))
	{
		askflag = 1;
	}
	else
	{
		merge_path_names(temp1, funnyglasses_map_name, true);
	}

	while (true)
	{
		if (askflag)
		{
			i = field_prompt_help(-1, "Enter name of .MAP file to use,\n"
				"or '*' to use palette from the image to be loaded.",
				NULL, temp1, 60, NULL);
			if (i < 0)
			{
				return -1;
			}
			if (temp1[0] == '*')
			{
				g_map_set = false;
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
		g_map_set = true;
		merge_path_names(g_map_name, temp1, true);
		break;
	}
	return 0;
}

static int get_funny_glasses_params()
{
	/* defaults */
	if (g_3d_state.z_viewer() == 0)
	{
		g_3d_state.set_z_viewer(150);
	}
	if (g_3d_state.eye_separation() == 0)
	{
		if (g_fractal_type == FRACTYPE_IFS_3D
			|| g_fractal_type == FRACTYPE_LORENZ_3D_L
			|| g_fractal_type == FRACTYPE_LORENZ_3D_FP)
		{
			g_3d_state.set_eye_separation(2);
			g_3d_state.set_x_adjust(-2);
		}
		else
		{
			g_3d_state.set_eye_separation(3);
			g_3d_state.set_x_adjust(0);
		}
	}

	if (g_3d_state.glasses_type() == STEREO_ALTERNATE)
	{
		strcpy(funnyglasses_map_name, g_glasses1_map);
	}
	else if (g_3d_state.glasses_type() == STEREO_SUPERIMPOSE)
	{
		if (g_3d_state.fill_type() == FillType::SurfaceGrid)
		{
			strcpy(funnyglasses_map_name, "grid.map");
		}
		else
		{
			strcpy(funnyglasses_map_name, g_glasses1_map);
			funnyglasses_map_name[7] = '2';
		}
	}

	UIChoices dialog(HELP3DGLASSES, "Funny Glasses Parameters", 0);
	dialog.push("Interocular distance (as % of screen)", g_3d_state.eye_separation());
	dialog.push("Convergence adjust (positive = spread greater)", g_3d_state.x_adjust());
	dialog.push("Left  red image crop (% of screen)", g_3d_state.red().crop_left());
	dialog.push("Right red image crop (% of screen)", g_3d_state.red().crop_right());
	dialog.push("Left  blue image crop (% of screen)", g_3d_state.blue().crop_left());
	dialog.push("Right blue image crop (% of screen)", g_3d_state.blue().crop_right());
	dialog.push("Red brightness factor (%)", g_3d_state.red().bright());
	dialog.push("Blue brightness factor (%)", g_3d_state.blue().bright());

	if (g_3d_state.glasses_type() == STEREO_ALTERNATE
		|| g_3d_state.glasses_type() == STEREO_SUPERIMPOSE)
	{
		dialog.push("Map File name", funnyglasses_map_name);
	}

	if (dialog.prompt() < 0)
	{
		return -1;
	}

	int k = 0;
	g_3d_state.set_eye_separation(dialog.values(k++).uval.ival);
	g_3d_state.set_x_adjust(dialog.values(k++).uval.ival);
	g_3d_state.set_red().set_crop_left(dialog.values(k++).uval.ival);
	g_3d_state.set_red().set_crop_right(dialog.values(k++).uval.ival);
	g_3d_state.set_blue().set_crop_left(dialog.values(k++).uval.ival);
	g_3d_state.set_blue().set_crop_right(dialog.values(k++).uval.ival);
	g_3d_state.set_red().set_bright(dialog.values(k++).uval.ival);
	g_3d_state.set_blue().set_bright(dialog.values(k++).uval.ival);

	if (g_3d_state.glasses_type() == STEREO_ALTERNATE || g_3d_state.glasses_type() == STEREO_SUPERIMPOSE)
	{
		strcpy(funnyglasses_map_name, dialog.values(k).uval.sval);
	}
	return 0;
}

void set_bail_out_formula(enum bailouts test)
{
	switch (test)
	{
	case BAILOUT_MODULUS:
	default:
		g_bail_out_fp = bail_out_mod_fp;
		g_bail_out_l = bail_out_mod_l;
		g_bail_out_bn = bail_out_mod_bn;
		g_bail_out_bf = bail_out_mod_bf;
		break;
	case BAILOUT_REAL:
		g_bail_out_fp = bail_out_real_fp;
		g_bail_out_l = bail_out_real_l;
		g_bail_out_bn = bail_out_real_bn;
		g_bail_out_bf = bail_out_real_bf;
		break;
	case BAILOUT_IMAGINARY:
		g_bail_out_fp = bail_out_imag_fp;
		g_bail_out_l = bail_out_imag_l;
		g_bail_out_bn = bail_out_imag_bn;
		g_bail_out_bf = bail_out_imag_bf;
		break;
	case BAILOUT_OR:
		g_bail_out_fp = bail_out_or_fp;
		g_bail_out_l = bail_out_or_l;
		g_bail_out_bn = bail_out_or_bn;
		g_bail_out_bf = bail_out_or_bf;
		break;
	case BAILOUT_AND:
		g_bail_out_fp = bail_out_and_fp;
		g_bail_out_l = bail_out_and_l;
		g_bail_out_bn = bail_out_and_bn;
		g_bail_out_bf = bail_out_and_bf;
		break;
	case BAILOUT_MANHATTAN:
		g_bail_out_fp = bail_out_manhattan_fp;
		g_bail_out_l = bail_out_manhattan_l;
		g_bail_out_bn = bail_out_manhattan_bn;
		g_bail_out_bf = bail_out_manhattan_bf;
		break;
	case BAILOUT_MANHATTAN_R:
		g_bail_out_fp = bail_out_manhattan_r_fp;
		g_bail_out_l = bail_out_manhattan_r_l;
		g_bail_out_bn = bail_out_manhattan_r_bn;
		g_bail_out_bf = bail_out_manhattan_r_bf;
		break;
	}
}
