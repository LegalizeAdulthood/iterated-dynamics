#pragma once
#if !defined(PROMPTS2_H)
#define PROMPTS2_H

#include <stdio.h>

#include <string>

enum class cmd_file;

#define   FILEATTR       0x37      // File attributes; select all but volume labels
#define   HIDDEN         2
#define   SYSTEM         4
#define   SUBDIR         16

struct DIR_SEARCH               // Allocate DTA and define structure
{
    char path[FILE_MAX_PATH];       // DOS path and filespec
    char attribute;             // File attributes wanted
    int  ftime;                 // File creation time
    int  fdate;                 // File creation date
    long size;                  // File size in bytes
    char filename[FILE_MAX_PATH];   // Filename and extension
};

extern DIR_SEARCH DTA;          // Disk Transfer Area

extern long                  g_concentration;
extern std::string const     g_gray_map_file;

extern int get_corners();
extern int get_toggles();
extern int get_toggles2();
extern int passes_options();
extern int get_view_params();
extern int get_starfield_params();
extern int get_commands();
extern void goodbye();
extern bool isadirectory(char const *s);
extern bool getafilename(char const *hdg, char const *file_template, char *flname);
extern bool getafilename(char const *hdg, char const *file_template, std::string &flname);
extern int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);
extern int makepath(char *template_str, char const *drive, char const *dir, char const *fname, char const *ext);
extern int fr_findfirst(char const *path);
extern int fr_findnext();
extern void shell_sort(void *, int n, unsigned, int (*fct)(void *, void *));
extern void fix_dirname(char *dirname);
extern void fix_dirname(std::string &dirname);
extern int merge_pathnames(char *oldfullpath, char const *newfilename, cmd_file mode);
extern int merge_pathnames(std::string &oldfullpath, char const *newfilename, cmd_file mode);
extern int get_browse_params();
extern int get_cmd_string();
extern int get_rds_params();
extern int starfield();
extern int get_a_number(double *, double *);
extern int lccompare(void *, void *);
extern int dir_remove(char const *dir, char const *filename);
extern FILE *dir_fopen(char const *dir, char const *filename, char const *mode);
extern void extract_filename(char *target, char const *source);
extern std::string extract_filename(char const *source);
extern const char *has_ext(char const *source);

#endif
