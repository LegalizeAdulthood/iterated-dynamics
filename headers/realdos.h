#pragma once
#if !defined(REALDOS_H)
#define REALDOS_H

enum input_field_flags
{
    INPUTFIELD_NUMERIC  = 1,
    INPUTFIELD_INTEGER  = 2,
    INPUTFIELD_DOUBLE   = 4
};

extern int                   g_cfg_line_nums[];
extern int                   g_patch_level;
extern int                   g_release;
extern std::string const     g_speed_prompt;
extern int                   g_video_table_len;

extern void blankrows(int, int, int);
extern int texttempmsg(char const *);
extern int fullscreen_choice(
    int options,
    char const *hdg,
    char const *hdg2,
    char const *instr,
    int numchoices,
    char const **choices,
    int *attributes,
    int boxwidth,
    int boxdepth,
    int colwidth,
    int current,
    void (*formatitem)(int, char*),
    char *speedstring,
    int (*speedprompt)(int row, int col, int vid, char const *speedstring, int speed_match),
    int (*checkkey)(int, int)
);
extern bool showtempmsg(char const *);
extern void cleartempmsg();
extern void helptitle();
extern int putstringcenter(int row, int col, int width, int attr, char const *msg);
extern int main_menu(int);
extern int input_field(int options, int attr, char *fld, int len, int row, int col,
    int (*checkkey)(int curkey));
extern int field_prompt(char const *hdg, char const *instr, char *fld, int len,
    int (*checkkey)(int curkey));
extern bool thinking(int options, char const *msg);
extern void discardgraphics();
extern void load_fractint_config();
extern int check_vidmode_key(int, int);
extern int check_vidmode_keyname(char const *kname);
extern void vidmode_keyname(int k, char *buf);
extern void freetempmsg();
extern void load_videotable(int);
extern void bad_fractint_cfg_msg();
extern int showvidlength();
extern bool stopmsg(int flags, char const* msg);

#endif
