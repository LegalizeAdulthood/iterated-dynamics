#pragma once

#include <cstdio>
#include <string>

enum class gfe_type
{
    FORMULA = 0,
    L_SYSTEM = 1,
    IFS = 2,
    PARM = 3,
};

bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, gfe_type itemtype);
bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, gfe_type itemtype);

long get_file_entry(gfe_type type, char const *title, char const *fmask, char *filename, char *entryname);
long get_file_entry(gfe_type type, char const *title, char const *fmask, std::string &filename, char *entryname);
long get_file_entry(gfe_type type, char const *title, char const *fmask, std::string &filename, std::string &entryname);

bool search_for_entry(std::FILE *infile, char const *itemname);
