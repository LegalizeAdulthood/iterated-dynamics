#pragma once

/* text colors */
enum TextColorType
{
	TEXTCOLOR_BLACK = 0,
	TEXTCOLOR_BLUE = 1,
	TEXTCOLOR_GREEN = 2,
	TEXTCOLOR_CYAN = 3,
	TEXTCOLOR_RED = 4,
	TEXTCOLOR_MAGENTA = 5,
	/* dirty yellow on cga */
	TEXTCOLOR_BROWN = 6,
	TEXTCOLOR_WHITE = 7,
	/* use values below this for foreground only, they don't work background */
	/* don't use this much - is black on cga */
	TEXTCOLOR_GRAY = 8,
	TEXTCOLOR_LIGHT_BLUE = 9,
	TEXTCOLOR_LIGHT_GREEN = 10,
	TEXTCOLOR_LIGHT_CYAN = 11,
	TEXTCOLOR_LIGHT_RED = 12,
	TEXTCOLOR_LIGHT_MAGENTA = 13,
	TEXTCOLOR_YELLOW = 14,
	TEXTCOLOR_LIGHT_WHITE = 15,
	TEXTCOLOR_INVERSE = 0x8000,
	TEXTCOLOR_BRIGHT = 0X4000
};

extern int get_text_color(int idx);
extern void set_text_color(int idx, int value);
extern void set_mono_text_colors();
extern void set_custom_text_colors(char const *text);

inline int text_color_bright(int index)
{
	return get_text_color(index) | TEXTCOLOR_BRIGHT;
}

inline int text_color_inverse(int index)
{
	return get_text_color(index) | TEXTCOLOR_INVERSE;
}

#define C_TITLE           text_color_bright(0)
#define C_TITLE_DEV       get_text_color(1)
#define C_HELP_HDG        text_color_bright(2)
#define C_HELP_BODY       get_text_color(3)
#define C_HELP_INSTR      get_text_color(4)
#define C_HELP_LINK       text_color_bright(5)
#define C_HELP_CURLINK    text_color_inverse(6)
#define C_PROMPT_BKGRD    get_text_color(7)
#define C_PROMPT_TEXT     get_text_color(8)
#define C_PROMPT_LO       get_text_color(9)
#define C_PROMPT_MED      get_text_color(10)
#define C_PROMPT_HI       text_color_bright(11)
#define C_PROMPT_INPUT    text_color_inverse(12)
#define C_PROMPT_CHOOSE   text_color_inverse(13)
#define C_CHOICE_CURRENT  text_color_inverse(14)
#define C_CHOICE_SP_INSTR get_text_color(15)
#define C_CHOICE_SP_KEYIN text_color_bright(16)
#define C_GENERAL_HI      text_color_bright(17)
#define C_GENERAL_MED     get_text_color(18)
#define C_GENERAL_LO      get_text_color(19)
#define C_GENERAL_INPUT   text_color_inverse(20)
#define C_DVID_BKGRD      get_text_color(21)
#define C_DVID_HI         text_color_bright(22)
#define C_DVID_LO         get_text_color(23)
#define C_STOP_ERR        text_color_bright(24)
#define C_STOP_INFO       text_color_bright(25)
#define C_TITLE_LOW       get_text_color(26)
#define C_AUTHDIV1        text_color_inverse(27)
#define C_AUTHDIV2        text_color_inverse(28)
#define C_PRIMARY         get_text_color(29)
#define C_CONTRIB         get_text_color(30)
