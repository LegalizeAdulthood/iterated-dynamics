#pragma once

// TODO: make sure X Window System font can render these chars
#define UPARR "\x18"
#define DNARR "\x19"
#define RTARR "\x1A"
#define LTARR "\x1B"
#define UPARR1 "\x18"
#define DNARR1 "\x19"
#define RTARR1 "\x1A"
#define LTARR1 "\x1B"

// Values returned by driver_get_key()
enum
{
    ID_KEY_ALT_A           = 1030,
    ID_KEY_ALT_S           = 1031,
    ID_KEY_ALT_F1          = 1104,
    ID_KEY_ALT_F2          = 1105,
    ID_KEY_ALT_F3          = 1106,
    ID_KEY_ALT_F4          = 1107,
    ID_KEY_ALT_F5          = 1108,
    ID_KEY_ALT_F6          = 1109,
    ID_KEY_ALT_F7          = 1110,
    ID_KEY_ALT_F8          = 1111,
    ID_KEY_ALT_F9          = 1112,
    ID_KEY_ALT_F10         = 1113,
    ID_KEY_CTL_A           = 1,
    ID_KEY_CTL_B           = 2,
    ID_KEY_CTL_E           = 5,
    ID_KEY_CTL_F           = 6,
    ID_KEY_CTL_G           = 7,
    ID_KEY_CTL_H           = 8,
    ID_KEY_CTL_O           = 15,
    ID_KEY_CTL_P           = 16,
    ID_KEY_CTL_S           = 19,
    ID_KEY_CTL_U           = 21,
    ID_KEY_CTL_X           = 24,
    ID_KEY_CTL_Y           = 25,
    ID_KEY_CTL_Z           = 26,
    ID_KEY_CTL_BACKSLASH   = 28,
    ID_KEY_CTL_DEL         = 1147,
    ID_KEY_CTL_DOWN_ARROW  = 1145,
    ID_KEY_CTL_END         = 1117,
    ID_KEY_CTL_ENTER       = 10,
    ID_KEY_CTL_ENTER_2     = 1010,
    ID_KEY_CTL_F1          = 1094,
    ID_KEY_CTL_F2          = 1095,
    ID_KEY_CTL_F3          = 1096,
    ID_KEY_CTL_F4          = 1097,
    ID_KEY_CTL_F5          = 1098,
    ID_KEY_CTL_F6          = 1099,
    ID_KEY_CTL_F7          = 1100,
    ID_KEY_CTL_F8          = 1101,
    ID_KEY_CTL_F9          = 1102,
    ID_KEY_CTL_F10         = 1103,
    ID_KEY_CTL_HOME        = 1119,
    ID_KEY_CTL_INSERT      = 1146,
    ID_KEY_CTL_LEFT_ARROW  = 1115,
    ID_KEY_CTL_MINUS       = 1142,
    ID_KEY_CTL_PAGE_DOWN   = 1118,
    ID_KEY_CTL_PAGE_UP     = 1132,
    ID_KEY_CTL_PLUS        = 1144,
    ID_KEY_CTL_RIGHT_ARROW = 1116,
    ID_KEY_CTL_TAB         = 1148,
    ID_KEY_CTL_UP_ARROW    = 1141,
    ID_KEY_SHF_TAB         = 1015,  // shift tab aka BACKTAB
    ID_KEY_BACKSPACE       = 8,
    ID_KEY_DELETE          = 1083,
    ID_KEY_DOWN_ARROW      = 1080,
    ID_KEY_END             = 1079,
    ID_KEY_ENTER           = 13,
    ID_KEY_ENTER_2         = 1013,
    ID_KEY_ESC             = 27,
    ID_KEY_F1              = 1059,
    ID_KEY_F2              = 1060,
    ID_KEY_F3              = 1061,
    ID_KEY_F4              = 1062,
    ID_KEY_F5              = 1063,
    ID_KEY_F6              = 1064,
    ID_KEY_F7              = 1065,
    ID_KEY_F8              = 1066,
    ID_KEY_F9              = 1067,
    ID_KEY_F10             = 1068,
    ID_KEY_HOME            = 1071,
    ID_KEY_INSERT          = 1082,
    ID_KEY_LEFT_ARROW      = 1075,
    ID_KEY_PAGE_DOWN       = 1081,
    ID_KEY_PAGE_UP         = 1073,
    ID_KEY_RIGHT_ARROW     = 1077,
    ID_KEY_SPACE           = 32,
    ID_KEY_SF1             = 1084,
    ID_KEY_SF2             = 1085,
    ID_KEY_SF3             = 1086,
    ID_KEY_SF4             = 1087,
    ID_KEY_SF5             = 1088,
    ID_KEY_SF6             = 1089,
    ID_KEY_SF7             = 1090,
    ID_KEY_SF8             = 1091,
    ID_KEY_SF9             = 1092,
    ID_KEY_SF10            = 1093,
    ID_KEY_TAB             = 9,
    ID_KEY_UP_ARROW        = 1072,
    ID_KEY_ALT_1           = 1120,
    ID_KEY_ALT_2           = 1121,
    ID_KEY_ALT_3           = 1122,
    ID_KEY_ALT_4           = 1123,
    ID_KEY_ALT_5           = 1124,
    ID_KEY_ALT_6           = 1125,
    ID_KEY_ALT_7           = 1126,
    ID_KEY_CTL_KEYPAD_5    = 1143,
    ID_KEY_KEYPAD_5        = 1076
};

// nonalpha tests if we have a control character
inline bool nonalpha(int c)
{
    return c < 32 || c > 127;
}
