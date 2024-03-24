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

long get_file_entry(gfe_type type, char const *title, char const *fmask, char *filename, char *entryname);
long get_file_entry(gfe_type type, char const *title, char const *fmask, std::string &filename, char *entryname);
long get_file_entry(gfe_type type, char const *title, char const *fmask, std::string &filename, std::string &entryname);

bool search_for_entry(std::FILE *infile, char const *itemname);
