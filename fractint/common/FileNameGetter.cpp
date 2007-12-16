#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "prototyp.h"

#include "Browse.h"
#include "drivers.h"
#include "filesystem.h"
#include "fimain.h"
#include "prompts2.h"
#include "realdos.h"
#include "stereo.h"

#include "FileNameGetter.h"

struct CHOICE
{
	char name[13];
	char full_name[FILE_MAX_PATH];
	char type;
};

#define   MAXNUMFILES    2977L

static int filename_speedstr(int row, int col, int vid,
					char *speedstring, int speed_match)
{
	char *prompt;
	if (strchr(speedstring, ':')
		|| strchr(speedstring, '*') || strchr(speedstring, '*')
		|| strchr(speedstring, '?'))
	{
		g_speed_state = SPEEDSTATE_TEMPLATE;  /* template */
		prompt = "File Template";
	}
	else if (speed_match)
	{
		g_speed_state = SPEEDSTATE_SEARCH_PATH; /* does not match list */
		prompt = "Search Path for";
	}
	else
	{
		g_speed_state = SPEEDSTATE_MATCHING;
		prompt = "Speed key string";
	}
	driver_put_string(row, col, vid, prompt);
	return int(strlen(prompt));
}

static int check_f6_key(int curkey, int)
{
	if (curkey == FIK_F6)
	{
		return -FIK_F6;
	}
	else if (curkey == FIK_F4)
	{
		return -FIK_F4;
	}
	return 0;
}

int FileNameGetter::Execute()
{
	/* if getting an RDS image map */
	char instr[80];
	int masklen;
	char speedstr[81];
	char tmpmask[FILE_MAX_PATH];   /* used to locate next file in list */
	int i;
	int j;
	/* Only the first 13 characters of file names are displayed... */
	CHOICE storage[MAXNUMFILES];
	CHOICE *choices[MAXNUMFILES];
	int attributes[MAXNUMFILES];
	/* how many files */
	/* not the root directory */
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	static int num_templates = 1;
	static bool sort_entries = true;

	bool rds = (g_stereo_map_name == _fileName);
	for (i = 0; i < MAXNUMFILES; i++)
	{
		attributes[i] = 1;
		choices[i] = &storage[i];
	}
	/* save filename */
	std::string old_flname = _fileName;

restart:  /* return here if template or directory changes */
	tmpmask[0] = 0;
	if (_fileName.length() == 0)
	{
		_fileName = DOTSLASH;
	}
	split_path(_fileName, drive, dir, fname, ext);
	char filename[FILE_MAX_PATH];
	make_path(filename, ""   , "" , fname, ext);
	bool retried = false;

retry_dir:
	if (dir[0] == 0)
	{
		strcpy(dir, ".");
	}
	expand_dirname(dir, drive);
	make_path(tmpmask, drive, dir, "", "");
	ensure_slash_on_directory(tmpmask);
	if (!retried && strcmp(dir, SLASH) && strcmp(dir, DOTSLASH))
	{
		j = int(strlen(tmpmask)) - 1;
		tmpmask[j] = 0; /* strip trailing \ */
		if (strchr(tmpmask, '*') || strchr(tmpmask, '?')
			|| fr_find_first(tmpmask) != 0
			|| (g_dta.attribute & SUBDIR) == 0)
		{
			strcpy(dir, DOTSLASH);
			retried = true;
			goto retry_dir;
		}
		tmpmask[j] = SLASHC;
	}
	if (_fileTemplate.length() > 0)
	{
		num_templates = 1;
		split_path(_fileTemplate, 0, 0, fname, ext);
	}
	else
	{
		num_templates = sizeof(g_masks)/sizeof(g_masks[0]);
	}
	int filecount = -1;
	bool root_directory = true;
	j = 0;
	masklen = int(strlen(tmpmask));
	strcat(tmpmask, "*.*");
	int out = fr_find_first(tmpmask);
	while (out == 0 && filecount < MAXNUMFILES)
	{
		if ((g_dta.attribute & SUBDIR) && strcmp(g_dta.filename.c_str(), "."))
		{
			if (strcmp(g_dta.filename.c_str(), ".."))
			{
				ensure_slash_on_directory(g_dta.filename);
			}
			strncpy(choices[++filecount]->name, g_dta.filename.c_str(), 13);
			choices[filecount]->name[12] = 0;
			choices[filecount]->type = 1;
			strcpy(choices[filecount]->full_name, g_dta.filename.c_str());
			if (strcmp(g_dta.filename.c_str(), "..") == 0)
			{
				root_directory = false;
			}
		}
		out = fr_find_next();
	}
	tmpmask[masklen] = 0;
	if (_fileTemplate.length())
	{
		make_path(tmpmask, drive, dir, fname, ext);
	}
	do
	{
		if (num_templates > 1)
		{
			strcpy(&(tmpmask[masklen]), g_masks[j]);
		}
		out = fr_find_first(tmpmask);
		while (out == 0 && filecount < MAXNUMFILES)
		{
			if (!(g_dta.attribute & SUBDIR))
			{
				if (rds)
				{
					put_string_center(2, 0, 80, C_GENERAL_INPUT, g_dta.filename.c_str());

					split_path(g_dta.filename, 0, 0, fname, ext);
					/* just using speedstr as a handy buffer */
					make_path(speedstr, drive, dir, fname, ext);
					strncpy(choices[++filecount]->name, g_dta.filename.c_str(), 13);
					choices[filecount]->type = 0;
				}
				else
				{
					strncpy(choices[++filecount]->name, g_dta.filename.c_str(), 13);
					choices[filecount]->type = 0;
					strcpy(choices[filecount]->full_name, g_dta.filename.c_str());
				}
			}
			out = fr_find_next();
		}
	}
	while (++j < num_templates);
	if (++filecount == 0)
	{
		strcpy(choices[filecount]->name, "*nofiles*");
		choices[filecount]->type = 0;
		++filecount;
	}

	strcpy(instr, "Press " FK_F6 " for default directory, " FK_F4 " to toggle sort ");
	if (sort_entries)
	{
		strcat(instr, "off");
		shell_sort(&choices, filecount, sizeof(CHOICE *), lccompare); /* sort file list */
	}
	else
	{
		strcat(instr, "on");
	}
	if (root_directory && dir[0] && dir[0] != SLASHC) /* must be in root directory */
	{
		split_path(tmpmask, drive, dir, fname, ext);
		strcpy(dir, SLASH);
		make_path(tmpmask, drive, dir, fname, ext);
	}
	if (num_templates > 1)
	{
		strcat(tmpmask, " ");
		strcat(tmpmask, g_masks[0]);
	}
	strcpy(speedstr, filename);
	if (speedstr[0] == 0)
	{
		for (i = 0; i < filecount; i++) /* find first file */
		{
			if (choices[i]->type == 0)
			{
				break;
			}
		}
		if (i >= filecount)
		{
			i = 0;
		}
	}

	std::string heading = str(boost::format("%s\nTemplate: %s") % _heading % tmpmask);
	i = full_screen_choice(CHOICE_INSTRUCTIONS | (sort_entries ? 0 : CHOICE_NOT_SORTED),
		heading.c_str(), 0, instr, filecount, (char **) choices,
		attributes, 5, 99, 12, i, 0, speedstr, filename_speedstr, check_f6_key);
	if (i == -FIK_F4)
	{
		sort_entries = !sort_entries;
		goto restart;
	}
	if (i == -FIK_F6)
	{
		static int lastdir = 0;
		if (lastdir == 0)
		{
			strcpy(dir, g_fract_dir1.c_str());
		}
		else
		{
			strcpy(dir, g_fract_dir2.c_str());
		}
		ensure_slash_on_directory(dir);
		_fileName = std::string(drive) +  dir;
		lastdir = 1 - lastdir;
		goto restart;
	}
	if (i < 0)
	{
		/* restore filename */
		_fileName = old_flname;
		return -1;
	}
	if (speedstr[0] == 0 || g_speed_state == SPEEDSTATE_MATCHING)
	{
		if (choices[i]->type)
		{
			if (strcmp(choices[i]->name, "..") == 0) /* go up a directory */
			{
				if (strcmp(dir, DOTSLASH) == 0)
				{
					strcpy(dir, DOTDOTSLASH);
				}
				else
				{
					char *s = strrchr(dir, SLASHC);
					if (s != 0) /* trailing slash */
					{
						*s = 0;
						s = strrchr(dir, SLASHC);
						if (s != 0)
						{
							*(s + 1) = 0;
						}
					}
				}
			}
			else  /* go down a directory */
			{
				strcat(dir, choices[i]->full_name);
			}
			ensure_slash_on_directory(dir);
			_fileName = std::string(drive) + dir;
			goto restart;
		}
		split_path(choices[i]->full_name, 0, 0, fname, ext);
		_fileName = std::string(drive) + dir + fname + ext;
	}
	else
	{
		if (g_speed_state == SPEEDSTATE_SEARCH_PATH
			&& strchr(speedstr, '*') == 0 && strchr(speedstr, '?') == 0
			&& ((fr_find_first(speedstr) == 0
			&& (g_dta.attribute & SUBDIR))|| strcmp(speedstr, SLASH) == 0)) /* it is a directory */
		{
			g_speed_state = SPEEDSTATE_TEMPLATE;
		}

		if (g_speed_state == SPEEDSTATE_TEMPLATE)
		{
			/* extract from tempstr the pathname and template information,
				being careful not to overwrite drive and directory if not
				newly specified */
			char drive1[FILE_MAX_DRIVE];
			char dir1[FILE_MAX_DIR];
			char fname1[FILE_MAX_FNAME];
			char ext1[FILE_MAX_EXT];
			split_path(speedstr, drive1, dir1, fname1, ext1);
			if (drive1[0])
			{
				strcpy(drive, drive1);
			}
			if (dir1[0])
			{
				strcpy(dir, dir1);
			}
			_fileName = std::string(drive) + dir + fname1 + ext1;
			if (strchr(fname1, '*') || strchr(fname1, '?') ||
				strchr(ext1,   '*') || strchr(ext1,   '?'))
			{
				// TODO: can't do this when file_template is constant string
				// like "*.frm"!!!
				_fileTemplate = std::string(fname1) + ext1;
			}
			else if (is_a_directory(_fileName.c_str()))
			{
				ensure_slash_on_directory(_fileName);
			}
			goto restart;
		}
		else /* speedstate == SPEEDSTATE_SEARCH_PATH */
		{
			char fullpath[FILE_MAX_DIR];
			find_path(speedstr, fullpath);
			if (fullpath[0])
			{
				_fileName = fullpath;
			}
			else
			{  /* failed, make diagnostic useful: */
				_fileName = speedstr;
				if (strchr(speedstr, SLASHC) == 0)
				{
					split_path(speedstr, 0, 0, fname, ext);
					_fileName = std::string(drive) + dir + fname + ext;
				}
			}
		}
	}
	g_browse_state.make_path(fname, ext);
	return 0;
}

