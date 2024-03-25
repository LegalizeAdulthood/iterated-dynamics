#pragma once

enum
{
    MAX_VIDEO_MODES = 300       // maximum entries in id.cfg
};

struct VIDEOINFO;

extern VIDEOINFO             g_video_table[];
extern int                   g_video_table_len;

int check_vidmode_key(int, int);
int check_vidmode_keyname(char const *kname);
void vidmode_keyname(int k, char *buf);
