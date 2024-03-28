// id.h - common structures and values for the FRACTINT routines
#pragma once

#include "big.h"

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
#define FILE_MAX_DRIVE  3       // max length of drive letter
/*
The filename limits were increased in Xfract 3.02. But alas,
in this poor program that was originally developed on the
nearly-brain-dead DOS operating system, quite a few things
in the UI would break if file names were bigger than DOS 8-3
names. So for now humor us and let's keep the names short.
*/
#define FILE_MAX_FNAME  64          // max length of filename
#define FILE_MAX_EXT    64          // max length of extension
#define MSG_LEN 80                  // handy buffer size for messages
#define MAX_PARAMS 10               // maximum number of parameters
#define MAX_PIXELS   32767          // Maximum pixel count across/down the screen
#define OLD_MAX_PIXELS 2048         // Limit of some old fixed arrays
#define MIN_PIXELS 10               // Minimum pixel count across/down the screen
#define DEFAULT_ASPECT 1.0F         // Assumed overall screen dimensions, y/x

enum
{
    ITEM_NAME_LEN = 18, // max length of names in .frm/.l/.ifs/.fc
};

// values for inside/outside
enum
{
    COLOR_BLACK = 0,
    ITER = -1,
    REAL = -2,
    IMAG = -3,
    MULT = -4,
    SUM = -5,
    ATAN = -6,
    FMOD = -7,
    TDIS = -8,
    ZMAG = -59,
    BOF60 = -60,
    BOF61 = -61,
    EPSCROSS = -100,
    STARTRAIL = -101,
    PERIOD = -102,
    FMODI = -103,
    ATANI = -104
};

#define DEFAULT_FRACTAL_TYPE      ".gif"

inline int sqr(int x)
{
    return x*x;
}

inline float sqr(float x)
{
    return x*x;
}

inline double sqr(double x)
{
    return x*x;
}

inline long lsqr(long x)
{
    extern int g_bit_shift;
    long multiply(long x, long y, int n);
    return multiply(x, x, g_bit_shift);
}

#ifdef PI
#undef PI
#endif
#define PI 3.14159265358979323846
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

// for overlay return stack
#define BIG 100000.0
