#include <string.h>
#include <ctype.h>
#ifndef XFRACT
#include <direct.h>
#endif
#ifndef XFRACT
#include <io.h>
#elif !defined(__386BSD__) && !defined(_WIN32)
#include <sys/types.h>
#include <sys/stat.h>

#ifdef DIRENT
#include <dirent.h>
#elif !defined(__SVR4)
#include <sys/dir.h>
#else
#include <dirent.h>
#ifndef DIRENT
#define DIRENT
#endif
#endif

#endif

#include "port.h"
#include "fractint.h"
#include "prototyp.h"

#include "filesystem.h"
#include "miscres.h"

int merge_path_names(char *old_full_path, char *new_filename, bool copy_directory)
{
	/* no dot or slash so assume a file */
	bool isafile = (strchr(new_filename, '.') == NULL && strchr(new_filename, SLASHC) == NULL);
	int isadir = is_a_directory(new_filename);
	if (isadir != 0)
	{
		ensure_slash_on_directory(new_filename);
	}
#ifndef XFRACT
	/* if drive, colon, slash, is a directory */
	if ((int) strlen(new_filename) == 3 &&
			new_filename[1] == ':' &&
			new_filename[2] == SLASHC)
	{
		isadir = 1;
	}
	/* if drive, colon, with no slash, is a directory */
	if ((int) strlen(new_filename) == 2 && new_filename[1] == ':')
	{
		new_filename[2] = SLASHC;
		new_filename[3] = 0;
		isadir = 1;
	}
	/* if dot, slash, '0', its the current directory, set up full path */
	if (new_filename[0] == '.' && new_filename[1] == SLASHC && new_filename[2] == 0)
	{
		char temp_path[FILE_MAX_PATH];
		temp_path[0] = (char)('a' + _getdrive() - 1);
		temp_path[1] = ':';
		temp_path[2] = 0;
		expand_dirname(new_filename, temp_path);
		strcat(temp_path, new_filename);
		strcpy(new_filename, temp_path);
		isadir = 1;
	}
	/* if dot, slash, its relative to the current directory, set up full path */
	if (new_filename[0] == '.' && new_filename[1] == SLASHC)
	{
		int len;
		int test_dir = 0;
		char temp_path[FILE_MAX_PATH];
		temp_path[0] = (char)('a' + _getdrive() - 1);
		temp_path[1] = ':';
		temp_path[2] = 0;
		if (strrchr(new_filename, '.') == new_filename)
		{
			test_dir = 1;  /* only one '.' assume its a directory */
		}
		expand_dirname(new_filename, temp_path);
		strcat(temp_path, new_filename);
		strcpy(new_filename, temp_path);
		if (!test_dir)
		{
			len = (int) strlen(new_filename);
			new_filename[len-1] = 0; /* get rid of slash added by expand_dirname */
		}
	}
#else
	{
		char temp_path[FILE_MAX_PATH];
		find_path(newfilename, temp_path);
		strcpy(newfilename, temp_path);
	}
#endif
	/* check existence */
	if (isadir == 0 || isafile)
	{
		if (fr_find_first(new_filename) == 0)
		{
			if (g_dta.attribute & SUBDIR) /* exists and is dir */
			{
				ensure_slash_on_directory(new_filename);  /* add trailing slash */
				isadir = 1;
				isafile = false;
			}
			else
			{
				isafile = true;
			}
		}
	}

	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	split_path(new_filename, drive, dir, fname, ext);

	char drive1[FILE_MAX_DRIVE];
	char dir1[FILE_MAX_DIR];
	char fname1[FILE_MAX_FNAME];
	char ext1[FILE_MAX_EXT];
	split_path(old_full_path, drive1, dir1, fname1, ext1);

	if ((int) strlen(drive) != 0 && copy_directory)
	{
		strcpy(drive1, drive);
	}
	if ((int) strlen(dir) != 0 && copy_directory)
	{
		strcpy(dir1, dir);
	}
	if ((int) strlen(fname) != 0)
	{
		strcpy(fname1, fname);
	}
	if ((int) strlen(ext) != 0)
	{
		strcpy(ext1, ext);
	}
	if (isadir == 0 && !isafile && copy_directory)
	{
		make_path(old_full_path, drive1, dir1, NULL, NULL);
		int len = (int) strlen(old_full_path);
		if (len > 0)
		{
			char save;
			/* strip trailing slash */
			save = old_full_path[len-1];
			if (save == SLASHC)
			{
				old_full_path[len-1] = 0;
			}
			if (access(old_full_path, 0))
			{
				isadir = -1;
			}
			old_full_path[len-1] = save;
		}
	}
	make_path(old_full_path, drive1, dir1, fname1, ext1);
	return isadir;
}

/* copies the proposed new filename to the fullpath variable */
/* does not copy directories for PAR files (modes 2 and 3)   */
/* attempts to extract directory and test for existence (modes 0 and 1) */
int merge_path_names(char *old_full_path, char *new_filename, int mode)
{
	return merge_path_names(old_full_path, new_filename, mode < 2);
}

/* ensure directory names end in a slash character */
void ensure_slash_on_directory(char *dirname)
{
	int length = (int) strlen(dirname); /* index of last character */

	/* make sure dirname ends with a slash */
	if (length > 0)
	{
		if (dirname[length-1] == SLASHC)
		{
			return;
		}
	}
	strcat(dirname, SLASH);
}

static void dir_name(char *target, const char *dir, const char *name)
{
	*target = 0;
	if (*dir != 0)
	{
		strcpy(target, dir);
	}
	strcat(target, name);
}

/* removes file in dir directory */
int dir_remove(char *dir, char *filename)
{
	char tmp[FILE_MAX_PATH];
	dir_name(tmp, dir, filename);
	return remove(tmp);
}

/* fopens file in dir directory */
FILE *dir_fopen(const char *dir, const char *filename, const char *mode)
{
	char tmp[FILE_MAX_PATH];
	dir_name(tmp, dir, filename);
	return fopen(tmp, mode);
}


void make_path(char *template_str, const char *drive, const char *dir, const char *fname, const char *ext)
{
	if (!template_str)
	{
		return;
	}

	*template_str = 0;
#ifndef XFRACT
	if (drive)
	{
		strcpy(template_str, drive);
	}
#endif
	if (dir)
	{
		strcat(template_str, dir);
	}
	if (fname)
	{
		strcat(template_str, fname);
	}
	if (ext)
	{
		strcat(template_str, ext);
	}
}

#ifndef XFRACT  /* This routine moved to unix.c so we can use it in hc.c */

static void get_drive(char const *file_template, char *drive, int length, int &offset)
{
	/* get drive */
	if (drive)
	{
		drive[0] = 0;
	}
	if (length >= 2)
	{
		if (file_template[1] == ':')
		{
			if (drive)
			{
				drive[0] = file_template[offset];
				drive[1] = file_template[offset + 1];
				drive[2] = 0;
			}
			offset += 2;
		}
	}
}

static void get_dir(char const *file_template, char *dir, int &offset)
{
	if (!dir)
	{
		return;
	}

	dir[0] = 0;
	const char *tmp = strrchr(file_template, SLASHC);
	if (tmp)
	{
		tmp++;  /* first character after slash */
		int len = (int) (tmp - (char *) &file_template[offset]);
		if (len >= 0 && len < FILE_MAX_DIR)
		{
			::strncpy(dir, &file_template[offset], min(len, FILE_MAX_DIR));
		}
		if (len < FILE_MAX_DIR)
		{
			dir[len] = 0;
		}
		offset += len;
	}
}

static void get_filename_ext(char const *file_template, char *fname, char *ext, int length, int offset)
{
	if (fname)
	{
		fname[0] = 0;
	}
	const char *tmp = strrchr(file_template, '.');
	if (tmp < strrchr(file_template, SLASHC) || tmp < strrchr(file_template, ':'))
	{
		tmp = 0; /* in this case the '.' must be a directory */
	}
	if (tmp)
	{
		/* first character past "." */
		int len = (int) (tmp - (char *)&file_template[offset]);
		if ((len > 0) && (offset + len < length) && fname)
		{
			strncpy(fname, &file_template[offset], min(len, FILE_MAX_FNAME));
			if (len < FILE_MAX_FNAME)
			{
				fname[len] = 0;
			}
			else
			{
				fname[FILE_MAX_FNAME-1] = 0;
			}
		}
		offset += len;
		if (ext)
		{
			ext[0] = 0;
			if (offset < length)
			{
				strncpy(ext, &file_template[offset], FILE_MAX_EXT);
				ext[FILE_MAX_EXT-1] = 0;
			}
		}
	}
	else if ((offset < length) && fname)
	{
		strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
		fname[FILE_MAX_FNAME-1] = 0;
	}
}

void empty_string(char *text)
{
	if (text)
	{
		text[0] = 0;
	}
}

void split_path(const char *file_template, char *drive, char *dir, char *filename, char *extension)
{
	empty_string(drive);
	empty_string(dir);
	empty_string(filename);
	empty_string(extension);
	int length = (int) strlen(file_template);
	if (length == 0)
	{
		return;
	}

	int offset = 0;
	get_drive(file_template, drive, length, offset);
	if (offset >= length)
	{
		return;
	}

	get_dir(file_template, dir, offset);
	if (offset >= length)
	{
		return;
	}

	get_filename_ext(file_template, filename, extension, length, offset);
}
#endif

#if !defined(_WIN32)
bool is_a_directory(char *s)
{
	int len;
	char sv;
#ifdef _MSC_VER
	unsigned attrib = 0;
#endif
	if (strchr(s, '*') || strchr(s, '?'))
	{
		return false; /* for my purposes, not a directory */
	}

	len = (int) strlen(s);
	if (len > 0)
	{
		sv = s[len-1];   /* last char */
	}
	else
	{
		sv = 0;
	}

#ifdef _MSC_VER
	if (_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
	{
		return true;  /* not a directory or doesn't exist */
	}
	else if (sv == SLASHC)
	{
		/* strip trailing slash and try again */
		s[len-1] = 0;
		if (_dos_getfileattr(s, &attrib) == 0 && ((attrib&_A_SUBDIR) != 0))
		{
			s[len-1] = sv;
			return true;
		}
		s[len-1] = sv;
	}
	return false;
#else
	if (fr_find_first(s) != 0) /* couldn't find it */
	{
		/* any better ideas?? */
		if (sv == SLASHC) /* we'll guess it is a directory */
		{
			return true;
		}
		else
		{
			return false;  /* no slashes - we'll guess it's a file */
		}
	}
	else if ((g_dta.attribute & SUBDIR) != 0)
	{
		if (sv == SLASHC)
		{
			/* strip trailing slash and try again */
			s[len-1] = 0;
			if (fr_find_first(s) != 0) /* couldn't find it */
			{
				return false;
			}
			else if ((g_dta.attribute & SUBDIR) != 0)
			{
				return true;   /* we're SURE it's a directory */
			}
			else
			{
				return false;
			}
		}
		else
		{
			return true;   /* we're SURE it's a directory */
		}
	}
	return false;
#endif
}
#endif

/* extract just the filename/extension portion of a path */
void extract_filename(char *target, const char *source)
{
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	split_path(source, NULL, NULL, fname, ext);
	make_path(target, "", "", fname, ext);
}

/* tells if filename has extension */
/* returns pointer to period or NULL */
char *has_extension(char *source)
{
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char *ret = NULL;
	split_path(source, NULL, NULL, fname, ext);
	if (ext != NULL)
	{
		if (*ext != 0)
		{
			ret = strrchr(source, '.');
		}
	}
	return ret;
}

#ifndef XFRACT
/* return full pathnames */
void find_path(const char *filename, char *fullpathname)
{
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char temp_path[FILE_MAX_PATH];

	split_path(filename , NULL, NULL, fname, ext);
	make_path(temp_path, ""   , "" , fname, ext);

	if (g_check_current_dir && access(temp_path, 0) == 0)   /* file exists */
	{
		strcpy(fullpathname, temp_path);
		return;
	}

	strcpy(temp_path, filename);   /* avoid side effect changes to filename */

	if (temp_path[0] == SLASHC || (temp_path[0] && temp_path[1] == ':'))
	{
		if (access(temp_path, 0) == 0)   /* file exists */
		{
			strcpy(fullpathname, temp_path);
			return;
		}
		else
		{
			split_path(temp_path , NULL, NULL, fname, ext);
			make_path(temp_path, ""   , "" , fname, ext);
		}
	}
	fullpathname[0] = 0;                         /* indicate none found */
	_searchenv(temp_path, "PATH", fullpathname);
	if (!fullpathname[0])
	{
		_searchenv(temp_path, "FRACTDIR", fullpathname);
	}
	if (fullpathname[0] != 0)                    /* found it! */
	{
		if (strncmp(&fullpathname[2], SLASHSLASH, 2) == 0) /* stupid klooge! */
		{
			strcpy(&fullpathname[3], temp_path);
		}
	}
}
#endif

int check_write_file(char *name, const char *ext)
{
	/* TODO: change encoder.cpp to also use this routine */
nextname:
	char openfile[FILE_MAX_DIR];
	strcpy(openfile, name);
	char opentype[20];
	strcpy(opentype, ext);
	char *period = has_extension(openfile);
	if (period != NULL)
	{
		strcpy(opentype, period);
		*period = 0;
	}
	strcat(openfile, opentype);
	if (access(openfile, 0) != 0) /* file doesn't exist */
	{
		strcpy(name, openfile);
		return 0;
	}
	/* file already exists */
	if (!g_fractal_overwrite)
	{
		update_save_name(name);
		goto nextname;
	}
	return 1;
}

void update_save_name(char *filename) /* go to the next file name */
{
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];

	split_path(filename, drive, dir, fname, ext);

	char *hold = fname + strlen(fname) - 1; /* start at the end */
	while (hold >= fname && (*hold == ' ' || isdigit(*hold))) /* skip backwards */
	{
		hold--;
	}
	hold++;                      /* recover first digit */
	while (*hold == '0')         /* skip leading zeros */
	{
		hold++;
	}
	char *save = hold;
	while (*save)  /* check for all nines */
	{
		if (*save != '9')
		{
			break;
		}
		save++;
	}
	if (!*save)                  /* if the whole thing is nines then back */
	{
		save = hold - 1;          /* up one place. Note that this will eat */
	}
								/* your last letter if you go to far.    */
	else
	{
		save = hold;
	}
	sprintf(save, "%ld", atol(hold) + 1); /* increment the number */
	make_path(filename, drive, dir, fname, ext);
}
