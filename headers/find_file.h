#pragma once

#include <string>

#define   FILEATTR       0x37      // File attributes; select all but volume labels
#define   HIDDEN         2
#define   SYSTEM         4
#define   SUBDIR         16

struct DIR_SEARCH               // Allocate DTA and define structure
{
    std::string path;           // path and filespec
    char attribute;             // File attributes wanted
    int  ftime;                 // File creation time
    int  fdate;                 // File creation date
    long size;                  // File size in bytes
    std::string filename;       // Filename and extension
};

extern DIR_SEARCH DTA;

int fr_findfirst(char const *path);
int fr_findnext();
