#pragma once

#include <filesystem>
#include <functional>
#include <string>

enum
{
    SUBDIR = 1
};

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
