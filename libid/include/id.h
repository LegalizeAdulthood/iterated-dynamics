#pragma once

// these are used to declare arrays for file names
#if defined(_WIN32)
#define FILE_MAX_PATH _MAX_PATH
#define FILE_MAX_DIR _MAX_DIR
#else
#ifdef XFRACT
#define FILE_MAX_PATH  256       // max length of path+filename
#define FILE_MAX_DIR   256       // max length of directory name
#else
#define FILE_MAX_PATH  80       // max length of path+filename
#define FILE_MAX_DIR   80       // max length of directory name
#endif
#endif

//
// The filename limits were increased in Xfract 3.02. But alas,
// in this poor program that was originally developed on the
// nearly-brain-dead DOS operating system, quite a few things
// in the UI would break if file names were bigger than DOS 8-3
// names. So for now humor us and let's keep the names short.
//
enum
{
    FILE_MAX_DRIVE = 3,  // max length of drive letter
    FILE_MAX_FNAME = 64, // max length of filename
    FILE_MAX_EXT = 64,   // max length of extension
    MSG_LEN = 80,        // handy buffer size for messages
    MAX_PARAMS = 10      // maximum number of parameters
};

#define DEFAULT_ASPECT 1.0F         // Assumed overall screen dimensions, y/x

enum
{
    ITEM_NAME_LEN = 18, // max length of names in .frm/.l/.ifs/.fc
};

#define DEFAULT_FRACTAL_TYPE      ".gif"

#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
