#pragma once

#include <string>

enum class cmd_file;

extern std::string const     g_gray_map_file;

int get_corners();
int get_toggles2();
int passes_options();
int get_view_params();
int get_commands();
void goodbye();
bool getafilename(char const *hdg, char const *file_template, char *flname);
bool getafilename(char const *hdg, char const *file_template, std::string &flname);
int get_cmd_string();
