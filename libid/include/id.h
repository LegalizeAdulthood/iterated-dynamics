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

// bitmask values for fractalspecific flags
enum
{
    NOZOOM = 1,      // zoombox not allowed at all
    NOGUESS = 2,     // solid guessing not allowed
    NOTRACE = 4,     // boundary tracing not allowed
    NOROTATE = 8,    // zoombox rotate/stretch not allowed
    NORESUME = 16,   // can't interrupt and resume
    INFCALC = 32,    // this type calculates forever
    TRIG1 = 64,      // number of trig functions in formula
    TRIG2 = 128,     //
    TRIG3 = 192,     //
    TRIG4 = 256,     //
    PARMS3D = 1024,  // uses 3d parameters
    OKJB = 2048,     // works with Julibrot
    MORE = 4096,     // more than 4 parms
    BAILTEST = 8192, // can use different bailout tests
    BF_MATH = 16384, // supports arbitrary precision
    LD_MATH = 32768  // supports long double
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
#ifndef XFRACT
// TODO: make sure X Window System font can render these chars
#define UPARR "\x18"
#define DNARR "\x19"
#define RTARR "\x1A"
#define LTARR "\x1B"
#define UPARR1 "\x18"
#define DNARR1 "\x19"
#define RTARR1 "\x1A"
#define LTARR1 "\x1B"
#define FK_F1  "F1"
#define FK_F2  "F2"
#define FK_F3  "F3"
#define FK_F4  "F4"
#define FK_F5  "F5"
#define FK_F6  "F6"
#define FK_F7  "F7"
#define FK_F8  "F8"
#define FK_F9  "F9"
#else
#define UPARR "K"
#define DNARR "J"
#define RTARR "L"
#define LTARR "H"
#define UPARR1 "up(K)"
#define DNARR1 "down(J)"
#define RTARR1 "left(L)"
#define LTARR1 "right(H)"
#define FK_F1  "Shift-1"
#define FK_F2  "Shift-2"
#define FK_F3  "Shift-3"
#define FK_F4  "Shift-4"
#define FK_F5  "Shift-5"
#define FK_F6  "Shift-6"
#define FK_F7  "Shift-7"
#define FK_F8  "Shift-8"
#define FK_F9  "Shift-9"
#endif

// for overlay return stack
#define BIG 100000.0

// nonalpha tests if we have a control character
inline bool nonalpha(int c)
{
    return c < 32 || c > 127;
}

/* keys; FIK = "FractInt Key"
 * Use this prefix to disambiguate key name symbols used in the fractint source
 * from symbols defined by the external environment, i.e. "DELETE" on Win32
 */
enum
{
    FIK_ALT_A           = 1030,
    FIK_ALT_S           = 1031,
    FIK_ALT_F1          = 1104,
    FIK_ALT_F2          = 1105,
    FIK_ALT_F3          = 1106,
    FIK_ALT_F4          = 1107,
    FIK_ALT_F5          = 1108,
    FIK_ALT_F6          = 1109,
    FIK_ALT_F7          = 1110,
    FIK_ALT_F8          = 1111,
    FIK_ALT_F9          = 1112,
    FIK_ALT_F10         = 1113,
    FIK_CTL_A           = 1,
    FIK_CTL_B           = 2,
    FIK_CTL_E           = 5,
    FIK_CTL_F           = 6,
    FIK_CTL_G           = 7,
    FIK_CTL_H           = 8,
    FIK_CTL_O           = 15,
    FIK_CTL_P           = 16,
    FIK_CTL_S           = 19,
    FIK_CTL_U           = 21,
    FIK_CTL_X           = 24,
    FIK_CTL_Y           = 25,
    FIK_CTL_Z           = 26,
    FIK_CTL_BACKSLASH   = 28,
    FIK_CTL_DEL         = 1147,
    FIK_CTL_DOWN_ARROW  = 1145,
    FIK_CTL_END         = 1117,
    FIK_CTL_ENTER       = 10,
    FIK_CTL_ENTER_2     = 1010,
    FIK_CTL_F1          = 1094,
    FIK_CTL_F2          = 1095,
    FIK_CTL_F3          = 1096,
    FIK_CTL_F4          = 1097,
    FIK_CTL_F5          = 1098,
    FIK_CTL_F6          = 1099,
    FIK_CTL_F7          = 1100,
    FIK_CTL_F8          = 1101,
    FIK_CTL_F9          = 1102,
    FIK_CTL_F10         = 1103,
    FIK_CTL_HOME        = 1119,
    FIK_CTL_INSERT      = 1146,
    FIK_CTL_LEFT_ARROW  = 1115,
    FIK_CTL_MINUS       = 1142,
    FIK_CTL_PAGE_DOWN   = 1118,
    FIK_CTL_PAGE_UP     = 1132,
    FIK_CTL_PLUS        = 1144,
    FIK_CTL_RIGHT_ARROW = 1116,
    FIK_CTL_TAB         = 1148,
    FIK_CTL_UP_ARROW    = 1141,
    FIK_SHF_TAB         = 1015,  // shift tab aka BACKTAB
    FIK_BACKSPACE       = 8,
    FIK_DELETE          = 1083,
    FIK_DOWN_ARROW      = 1080,
    FIK_END             = 1079,
    FIK_ENTER           = 13,
    FIK_ENTER_2         = 1013,
    FIK_ESC             = 27,
    FIK_F1              = 1059,
    FIK_F2              = 1060,
    FIK_F3              = 1061,
    FIK_F4              = 1062,
    FIK_F5              = 1063,
    FIK_F6              = 1064,
    FIK_F7              = 1065,
    FIK_F8              = 1066,
    FIK_F9              = 1067,
    FIK_F10             = 1068,
    FIK_HOME            = 1071,
    FIK_INSERT          = 1082,
    FIK_LEFT_ARROW      = 1075,
    FIK_PAGE_DOWN       = 1081,
    FIK_PAGE_UP         = 1073,
    FIK_RIGHT_ARROW     = 1077,
    FIK_SPACE           = 32,
    FIK_SF1             = 1084,
    FIK_SF2             = 1085,
    FIK_SF3             = 1086,
    FIK_SF4             = 1087,
    FIK_SF5             = 1088,
    FIK_SF6             = 1089,
    FIK_SF7             = 1090,
    FIK_SF8             = 1091,
    FIK_SF9             = 1092,
    FIK_SF10            = 1093,
    FIK_TAB             = 9,
    FIK_UP_ARROW        = 1072,
    FIK_ALT_1           = 1120,
    FIK_ALT_2           = 1121,
    FIK_ALT_3           = 1122,
    FIK_ALT_4           = 1123,
    FIK_ALT_5           = 1124,
    FIK_ALT_6           = 1125,
    FIK_ALT_7           = 1126,
    FIK_CTL_KEYPAD_5    = 1143,
    FIK_KEYPAD_5        = 1076
};

template <typename T>
int sign(T x)
{
    return (T{} < x) - (x < T{});
}
