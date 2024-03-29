#pragma once

#include <cstdio>
#include <string>

enum gfe_type
{
    GETFORMULA = 0,
    GETLSYS = 1,
    GETIFS = 2,
    GETPARM = 3,
};

bool find_file_item(char *filename, char const *itemname, std::FILE **fileptr, int itemtype);
bool find_file_item(std::string &filename, char const *itemname, std::FILE **fileptr, int itemtype);

long get_file_entry(gfe_type type, char const *title, char const *fmask, char *filename, char *entryname);
long get_file_entry(gfe_type type, char const *title, char const *fmask, std::string &filename, char *entryname);
long get_file_entry(gfe_type type, char const *title, char const *fmask, std::string &filename, std::string &entryname);

bool search_for_entry(std::FILE *infile, char const *itemname);
