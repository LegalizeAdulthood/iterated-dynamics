#include <string.h>
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

int merge_path_names(char *oldfullpath, char *newfilename, bool copy_directory)
{
	int isadir = 0;
	int isafile = 0;
	int len;
	char drive[FILE_MAX_DRIVE];
	char dir[FILE_MAX_DIR];
	char fname[FILE_MAX_FNAME];
	char ext[FILE_MAX_EXT];
	char temp_path[FILE_MAX_PATH];

	char drive1[FILE_MAX_DRIVE];
	char dir1[FILE_MAX_DIR];
	char fname1[FILE_MAX_FNAME];
	char ext1[FILE_MAX_EXT];

	/* no dot or slash so assume a file */
	if (strchr(newfilename, '.') == NULL && strchr(newfilename, SLASHC) == NULL)
	{
		isafile = 1;
	}
	isadir = is_a_directory(newfilename);
	if (isadir != 0)
	{
		fix_dir_name(newfilename);
	}
#if 0
	/* if slash by itself, it's a directory */
	if (strcmp(newfilename, SLASH) == 0)
	{
		isadir = 1;
	}
#endif
#ifndef XFRACT
	/* if drive, colon, slash, is a directory */
	if ((int) strlen(newfilename) == 3 &&
			newfilename[1] == ':' &&
			newfilename[2] == SLASHC)
		isadir = 1;
	/* if drive, colon, with no slash, is a directory */
	if ((int) strlen(newfilename) == 2 && newfilename[1] == ':')
	{
		newfilename[2] = SLASHC;
		newfilename[3] = 0;
		isadir = 1;
	}
	/* if dot, slash, '0', its the current directory, set up full path */
	if (newfilename[0] == '.' && newfilename[1] == SLASHC && newfilename[2] == 0)
	{
		temp_path[0] = (char)('a' + _getdrive() - 1);
		temp_path[1] = ':';
		temp_path[2] = 0;
		expand_dirname(newfilename, temp_path);
		strcat(temp_path, newfilename);
		strcpy(newfilename, temp_path);
		isadir = 1;
	}
	/* if dot, slash, its relative to the current directory, set up full path */
	if (newfilename[0] == '.' && newfilename[1] == SLASHC)
	{
		int len;
		int test_dir = 0;
		temp_path[0] = (char)('a' + _getdrive() - 1);
		temp_path[1] = ':';
		temp_path[2] = 0;
		if (strrchr(newfilename, '.') == newfilename)
		{
			test_dir = 1;  /* only one '.' assume its a directory */
		}
		expand_dirname(newfilename, temp_path);
		strcat(temp_path, newfilename);
		strcpy(newfilename, temp_path);
		if (!test_dir)
		{
			len = (int) strlen(newfilename);
			newfilename[len-1] = 0; /* get rid of slash added by expand_dirname */
		}
	}
#else
	findpath(newfilename, temp_path);
	strcpy(newfilename, temp_path);
#endif
	/* check existence */
	if (isadir == 0 || isafile == 1)
	{
		if (fr_find_first(newfilename) == 0)
		{
			if (g_dta.attribute & SUBDIR) /* exists and is dir */
			{
				fix_dir_name(newfilename);  /* add trailing slash */
				isadir = 1;
				isafile = 0;
			}
			else
			{
				isafile = 1;
			}
		}
	}

	split_path(newfilename, drive, dir, fname, ext);
	split_path(oldfullpath, drive1, dir1, fname1, ext1);
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
	if (isadir == 0 && isafile == 0 && copy_directory)
	{
		make_path(oldfullpath, drive1, dir1, NULL, NULL);
		len = (int) strlen(oldfullpath);
		if (len > 0)
		{
			char save;
			/* strip trailing slash */
			save = oldfullpath[len-1];
			if (save == SLASHC)
			{
				oldfullpath[len-1] = 0;
			}
			if (access(oldfullpath, 0))
			{
				isadir = -1;
			}
			oldfullpath[len-1] = save;
		}
	}
	make_path(oldfullpath, drive1, dir1, fname1, ext1);
	return isadir;
}

/* copies the proposed new filename to the fullpath variable */
/* does not copy directories for PAR files (modes 2 and 3)   */
/* attempts to extract directory and test for existence (modes 0 and 1) */
int merge_path_names(char *oldfullpath, char *newfilename, int mode)
{
	return merge_path_names(oldfullpath, newfilename, mode < 2);
}

/* fix up directory names */
void fix_dir_name(char *dirname)
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


int make_path(char *template_str, char *drive, char *dir, char *fname, char *ext)
{
	if (template_str)
	{
		*template_str = 0;
	}
	else
	{
		return -1;
	}
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
	return 0;
}

#ifndef XFRACT  /* This routine moved to unix.c so we can use it in hc.c */

int split_path(const char *file_template, char *drive, char *dir, char *fname, char *ext)
{
	int length;
	int len;
	int offset;
	const char *tmp;
	if (drive)
	{
		drive[0] = 0;
	}
	if (dir)
	{
		dir[0]   = 0;
	}
	if (fname)
	{
		fname[0] = 0;
	}
	if (ext)
	{
		ext[0]   = 0;
	}

	length = (int) strlen(file_template);
	if (length == 0)
	{
		return 0;
	}

	offset = 0;

	/* get drive */
	if (length >= 2)
	{
		if (file_template[1] == ':')
		{
			if (drive)
			{
				drive[0] = file_template[offset++];
				drive[1] = file_template[offset++];
				drive[2] = 0;
			}
			else
			{
				offset++;
				offset++;
			}
		}
	}

	/* get dir */
	if (offset < length)
	{
		tmp = strrchr(file_template, SLASHC);
		if (tmp)
		{
			tmp++;  /* first character after slash */
			len = (int) (tmp - (char *)&file_template[offset]);
			if (len >= 0 && len < FILE_MAX_DIR && dir)
			{
				::strncpy(dir, &file_template[offset], min(len, FILE_MAX_DIR));
			}
			if (len < FILE_MAX_DIR && dir)
			{
				dir[len] = 0;
			}
			offset += len;
		}
	}
	else
	{
		return 0;
	}

	/* get fname */
	if (offset < length)
	{
		tmp = strrchr(file_template, '.');
		if (tmp < strrchr(file_template, SLASHC) || tmp < strrchr(file_template, ':'))
		{
			tmp = 0; /* in this case the '.' must be a directory */
		}
		if (tmp)
		{
			/* tmp++; */ /* first character past "." */
			len = (int) (tmp - (char *)&file_template[offset]);
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
			if ((offset < length) && ext)
			{
				strncpy(ext, &file_template[offset], FILE_MAX_EXT);
				ext[FILE_MAX_EXT-1] = 0;
			}
		}
		else if ((offset < length) && fname)
		{
			strncpy(fname, &file_template[offset], FILE_MAX_FNAME);
			fname[FILE_MAX_FNAME-1] = 0;
		}
	}
	return 0;
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
void extract_filename(char *target, char *source)
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


