#pragma once

#include <cstdio>
#include <string>

enum class cmd_file;

extern long                  g_concentration;
extern std::string const     g_gray_map_file;

int get_corners();
int get_toggles2();
int passes_options();
int get_view_params();
int get_starfield_params();
int get_commands();
void goodbye();
bool getafilename(char const *hdg, char const *file_template, char *flname);
bool getafilename(char const *hdg, char const *file_template, std::string &flname);
void shell_sort(void *, int n, unsigned, int (*fct)(void *, void *));
int get_cmd_string();
int get_rds_params();
int starfield();
int get_a_number(double *, double *);
int lccompare(void *, void *);
int dir_remove(char const *dir, char const *filename);
inline int dir_remove(const std::string &dir, const std::string &filename)
{
    return dir_remove(dir.c_str(), filename.c_str());
}
std::FILE *dir_fopen(char const *dir, char const *filename, char const *mode);
std::string extract_filename(char const *source);
const char *has_ext(char const *source);
