#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "id.h"
#include "prototyp.h"

#include "Browse.h"
#include "drivers.h"
#include "FileNameGetter.h"
#include "filesystem.h"
#include "fimain.h"
#include "FullScreenChooser.h"
#include "prompts2.h"
#include "realdos.h"
#include "stereo.h"
#include "TextColors.h"

struct CHOICE
{
	char name[13];
	char full_name[FILE_MAX_PATH];
	bool is_directory;
};

#define   MAXNUMFILES    2977L

char *g_masks[] = {"*.pot", "*.gif"};

FileNameGetter::SpeedStateType FileNameGetter::_speedState = SPEEDSTATE_MATCHING;

int FileNameGetter::SpeedPrompt(int row, int col, int vid,
	char *speedstring, int speed_match)
{
	std::string prompt;
	if (strchr(speedstring, ':')
		|| strchr(speedstring, '*') || strchr(speedstring, '*')
		|| strchr(speedstring, '?'))
	{
		_speedState = SPEEDSTATE_TEMPLATE;  /* template */
		prompt = "File Template";
	}
	else if (speed_match)
	{
		_speedState = SPEEDSTATE_SEARCH_PATH; /* does not match list */
		prompt = "Search Path for";
	}
	else
	{
		_speedState = SPEEDSTATE_MATCHING;
		prompt = "Speed key string";
	}
	driver_put_string(row, col, vid, prompt);
	return int(prompt.length());
}

int FileNameGetter::CheckSpecialKeys(int key, int)
{
	if ((key == FIK_F6) || (key == FIK_F4))
	{
		return -key;
	}
	return 0;
}

int FileNameGetter::Execute()
{
	/* if getting an RDS image map */
	/* used to locate next file in list */
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
	for (int i = 0; i < MAXNUMFILES; i++)
	{
		attributes[i] = 1;
		choices[i] = &storage[i];
	}
	/* save filename */
	std::string old_flname = _fileName;

restart:  /* return here if template or directory changes */
	char tmpmask[FILE_MAX_PATH];
	tmpmask[0] = 0;
	if (_fileName.length() == 0)
	{
		_fileName = DOTSLASH;
	}
	split_path(_fileName, drive, dir, fname, ext);
	std::string filename = boost::filesystem::path(_fileName).leaf();
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
	int masklen = int(strlen(tmpmask));
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
			choices[filecount]->is_directory = true;
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
	char speedstr[81];
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
					choices[filecount]->is_directory = false;
				}
				else
				{
					strncpy(choices[++filecount]->name, g_dta.filename.c_str(), 13);
					choices[filecount]->is_directory = false;
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
		choices[filecount]->is_directory = false;
		++filecount;
	}

	std::string instructions = "Press F6 for default directory, F4 to toggle sort ";
	if (sort_entries)
	{
		instructions += "off";
		shell_sort(&choices, filecount, sizeof(CHOICE *), lccompare); /* sort file list */
	}
	else
	{
		instructions += "on";
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
	strcpy(speedstr, filename.c_str());
	int current = 0;
	if (speedstr[0] == 0)
	{
		for (int i = 0; i < filecount; i++) /* find first file */
		{
			if (!choices[i]->is_directory)
			{
				current = i;
				break;
			}
		}
	}

	std::string heading = str(boost::format("%s\nTemplate: %s") % _heading % tmpmask);
	const int box_width = 5;
	const int box_depth = 99;
	const int column_width = 12;
	int i = full_screen_choice(CHOICE_INSTRUCTIONS | (sort_entries ? 0 : CHOICE_NOT_SORTED),
		heading, 0, instructions,
		filecount, (char **) choices, attributes, 
		box_width, box_depth, column_width, current, 0, speedstr, SpeedPrompt, CheckSpecialKeys);
	if (i == -FIK_F4)
	{
		sort_entries = !sort_entries;
		goto restart;
	}
	if (i == -FIK_F6)
	{
		static bool lastdir = false;
		if (!lastdir)
		{
			strcpy(dir, g_fract_dir1.c_str());
		}
		else
		{
			strcpy(dir, g_fract_dir2.c_str());
		}
		ensure_slash_on_directory(dir);
		_fileName = std::string(drive) +  dir;
		lastdir = !lastdir;
		goto restart;
	}
	if (i < 0)
	{
		/* restore filename */
		_fileName = old_flname;
		return -1;
	}
	if (speedstr[0] == 0 || _speedState == SPEEDSTATE_MATCHING)
	{
		if (choices[i]->is_directory)
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
		if (_speedState == SPEEDSTATE_SEARCH_PATH
			&& strchr(speedstr, '*') == 0 && strchr(speedstr, '?') == 0
			&& ((fr_find_first(speedstr) == 0
			&& (g_dta.attribute & SUBDIR))|| strcmp(speedstr, SLASH) == 0)) /* it is a directory */
		{
			_speedState = SPEEDSTATE_TEMPLATE;
		}

		if (_speedState == SPEEDSTATE_TEMPLATE)
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

