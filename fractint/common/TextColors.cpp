#include <algorithm>

#include <stdio.h>
#include <string.h>

#include "port.h"
#include "id.h"
#include "TextColors.h"

class TextColor
{
public:
	TextColor(TextColorType color = TEXTCOLOR_BLACK, int attributes = 0)
		: _color(color | attributes)
	{
	}
	TextColor(TextColorType background, TextColorType foreground, int attributes = 0)
		: _color((background << 4) | foreground | attributes)
	{
	}
	~TextColor()
	{
	}

	int Foreground() const { return _color & 0x0F; }
	int Background() const { return _color & 0xF0; }
	int Attributes() const { return _color & ~0xFF; }
	operator int() const { return _color; }

protected:
	int _color;
};

class TextColorMap
{
public:
	TextColorMap();
	TextColor GetColor(int indx) const;
	void SetColor(int indx, int value);
	void SetMonoColors();
	void SetCustomColors(const char *text);

private:
	enum
	{
		NUM_TEXT_COLORS = 32
	};
	TextColor _colors[NUM_TEXT_COLORS];
};

TextColorMap::TextColorMap()
{
	static TextColor initial_colors[NUM_TEXT_COLORS] =
	{
		TextColor(TEXTCOLOR_BLUE, TEXTCOLOR_LIGHT_WHITE),		// C_TITLE           title background 
		TextColor(TEXTCOLOR_BLUE, TEXTCOLOR_LIGHT_GREEN),		// C_TITLE_DEV       development vsn foreground 
		TextColor(TEXTCOLOR_GREEN, TEXTCOLOR_YELLOW),			// C_HELP_HDG        help page title line 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK),			// C_HELP_BODY       help page body 
		TextColor(TEXTCOLOR_GREEN, TEXTCOLOR_GRAY),				// C_HELP_INSTR      help page instr at bottom 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLUE),				// C_HELP_LINK       help page links 
		TextColor(TEXTCOLOR_CYAN, TEXTCOLOR_BLUE),				// C_HELP_CURLINK    help page current link 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_GRAY),				// C_PROMPT_BKGRD    prompt/choice background 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK),			// C_PROMPT_TEXT     prompt/choice extra info 
		TextColor(TEXTCOLOR_BLUE, TEXTCOLOR_WHITE),				// C_PROMPT_LO       prompt/choice text 
		TextColor(TEXTCOLOR_BLUE, TEXTCOLOR_LIGHT_WHITE),		// C_PROMPT_MED      prompt/choice hdg2/... 
		TextColor(TEXTCOLOR_BLUE, TEXTCOLOR_YELLOW),			// C_PROMPT_HI       prompt/choice hdg/cur/... 
		TextColor(TEXTCOLOR_GREEN, TEXTCOLOR_LIGHT_WHITE),		// C_PROMPT_INPUT    full_screen_prompt input 
		TextColor(TEXTCOLOR_CYAN, TEXTCOLOR_LIGHT_WHITE),		// C_PROMPT_CHOOSE   full_screen_prompt choice 
		TextColor(TEXTCOLOR_MAGENTA, TEXTCOLOR_LIGHT_WHITE),	// C_CHOICE_CURRENT  full_screen_choice input 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_WHITE),			// C_CHOICE_SP_INSTR speed key bar & instr 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_LIGHT_MAGENTA),	// C_CHOICE_SP_KEYIN speed key value 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLUE),				// C_GENERAL_HI      tab, thinking, IFS 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK),			// C_GENERAL_MED 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_GRAY),				// C_GENERAL_LO 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_LIGHT_WHITE),		// C_GENERAL_INPUT 
		TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK),			// C_DVID_BKGRD      disk video 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_YELLOW),			// C_DVID_HI 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_LIGHT_WHITE),		// C_DVID_LO 
		TextColor(TEXTCOLOR_RED, TEXTCOLOR_LIGHT_WHITE),		// C_STOP_ERR        stop message, error 
		TextColor(TEXTCOLOR_GREEN, TEXTCOLOR_BLACK),			// C_STOP_INFO       stop message, info 
		TextColor(TEXTCOLOR_BLUE, TEXTCOLOR_WHITE),				// C_TITLE_LOW       bottom lines of title screen 
		TextColor(TEXTCOLOR_GREEN, TEXTCOLOR_BLACK),			// C_AUTHDIV1        title screen dividers 
		TextColor(TEXTCOLOR_GREEN, TEXTCOLOR_GRAY),				// C_AUTHDIV2        title screen dividers 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_LIGHT_WHITE),		// C_PRIMARY         primary authors 
		TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_WHITE)				// C_CONTRIB         contributing authors 
	};

	std::copy(&initial_colors[0], &initial_colors[NUM_TEXT_COLORS], &_colors[0]);
}

TextColor TextColorMap::GetColor(int idx) const
{
	return _colors[idx];
}

void TextColorMap::SetColor(int idx, int value)
{
	_colors[idx] = TextColor(TextColorType(value));
}

void TextColorMap::SetMonoColors()
{
	for (int k = 0; k < NUM_TEXT_COLORS; ++k)
	{
		_colors[k] = TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_WHITE);
	}
	/* C_HELP_CURLINK =
		C_PROMPT_INPUT =
		C_CHOICE_CURRENT =
		C_GENERAL_INPUT =
		C_AUTHDIV1 =
		C_AUTHDIV2 = TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK); */
	_colors[6] =
		_colors[12] =
		_colors[13] =
		_colors[14] =
		_colors[20] =
		_colors[27] =
		_colors[28] = TextColor(TEXTCOLOR_WHITE, TEXTCOLOR_BLACK);
	/* C_TITLE =
		C_HELP_HDG =
		C_HELP_LINK =
		C_PROMPT_HI =
		C_CHOICE_SP_KEYIN =
		C_GENERAL_HI =
		C_DVID_HI =
		C_STOP_ERR =
		C_STOP_INFO = TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_LIGHT_WHITE); */
	_colors[0] =
		_colors[2] =
		_colors[5] =
		_colors[11] =
		_colors[16] =
		_colors[17] =
		_colors[22] =
		_colors[24] =
		_colors[25] = TextColor(TEXTCOLOR_BLACK, TEXTCOLOR_LIGHT_WHITE);
}

void TextColorMap::SetCustomColors(char const *text)
{
	int k = 0;
	while (k < sizeof(_colors))
	{
		if (*text == 0)
		{
			break;
		}
		if (*text != '/')
		{
			int hexval;
			sscanf(text, "%x", &hexval);
			int i = (hexval/16) & 7;
			int j = hexval & 15;
			if (i == j || (i == 0 && j == 8)) // force contrast 
			{
				j = 15;
			}
			_colors[k] = TextColor(TextColorType(i*16 + j));
			text = strchr(text, '/');
			if (text == 0)
			{
				break;
			}
		}
		++text;
		++k;
	}
}

static TextColorMap s_text_colors;

int get_text_color(int idx)
{
	return s_text_colors.GetColor(idx);
}

void set_text_color(int idx, int value)
{
	s_text_colors.SetColor(idx, TextColor(TextColorType(value)));
}

void set_mono_text_colors()
{
	s_text_colors.SetMonoColors();
}

void set_custom_text_colors(const char *text)
{
	s_text_colors.SetCustomColors(text);
}
