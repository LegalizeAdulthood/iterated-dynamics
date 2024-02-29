#pragma once

#include <cstdio>
#include <string>

enum class cmd_file;

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
extern bool getafilename(char const *hdg, char const *file_template, char *flname);
extern bool getafilename(char const *hdg, char const *file_template, std::string &flname);
extern int splitpath(char const *file_template, char *drive, char *dir, char *fname, char *ext);
inline int split_fname_ext(const char *file_template, char *fname, char *ext)
{
    return splitpath(file_template, nullptr, nullptr, fname, ext);
}
inline int splitpath(const std::string &file_template, char *drive, char *dir, char *fname, char *ext)
{
    return splitpath(file_template.c_str(), drive, dir, fname, ext);
}
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
inline int dir_remove(const std::string &dir, const std::string &filename)
{
    return dir_remove(dir.c_str(), filename.c_str());
}
extern std::FILE *dir_fopen(char const *dir, char const *filename, char const *mode);
std::string extract_filename(char const *source);
extern const char *has_ext(char const *source);
