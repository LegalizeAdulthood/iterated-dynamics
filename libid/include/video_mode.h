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
    int     keynum;         // key number used to invoke this mode
    // 2-10 = F2-10, 11-40 = Shift,Ctrl,Alt{F1-F10}
    int     videomodeax;    // begin with INT 10H, AX=(this)
    int     videomodebx;    // ...and BX=(this)
    int     videomodecx;    // ...and CX=(this)
    int     videomodedx;    // ...and DX=(this)
    int     dotmode;        // video access method
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
