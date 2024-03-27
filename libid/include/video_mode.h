#pragma once

enum
{
    MAX_VIDEO_MODES = 300       // maximum entries in id.cfg
};

struct Driver;
struct VIDEOINFO
{                           // All we need to know about a Video Adapter
    char    name[26];       // Adapter name (IBM EGA, etc)
    char    comment[26];    // Comments (UNTESTED, etc)
    int     keynum;         // key number used to invoked this mode
    // 2-10 = F2-10, 11-40 = S,C,A{F1-F10}
    int     videomodeax;    // begin with INT 10H, AX=(this)
    int     videomodebx;    // ...and BX=(this)
    int     videomodecx;    // ...and CX=(this)
    int     videomodedx;    // ...and DX=(this)
    // NOTE:  IF AX==BX==CX==0, SEE BELOW
    int     dotmode;        // video access method used by asm code
    // 1 == BIOS 10H, AH=12,13 (SLOW)
    // 2 == access like EGA/VGA
    // 3 == access like MCGA
    // 4 == Tseng-like  SuperVGA*256
    // 5 == P'dise-like SuperVGA*256
    // 6 == Vega-like   SuperVGA*256
    // 7 == "Tweaked" IBM-VGA ...*256
    // 8 == "Tweaked" SuperVGA ...*256
    // 9 == Targa Format
    // 10 = Hercules
    // 11 = "disk video" (no screen)
    // 12 = 8514/A
    // 13 = CGA 320x200x4, 640x200x2
    // 14 = Tandy 1000
    // 15 = TRIDENT  SuperVGA*256
    // 16 = Chips&Tech SuperVGA*256
    int     xdots;          // number of dots across the screen
    int     ydots;          // number of dots down the screen
    int     colors;         // number of colors available
    Driver *driver;
};

extern VIDEOINFO             g_video_table[];
extern int                   g_video_table_len;

int check_vidmode_key(int, int);
int check_vidmode_keyname(char const *kname);
void vidmode_keyname(int k, char *buf);
