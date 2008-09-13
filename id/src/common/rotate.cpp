/*
	rotate.cpp - Routines that manipulate the colormap
*/
#include <ctime>
#include <fstream>
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "Externals.h"
#include "filesystem.h"
#include "idhelp.h"
#include "loadmap.h"
#include "prompts1.h"
#include "prompts2.h"
#include "rotate.h"

// routines in this module

static void pause_rotate();
static void set_palette(BYTE start[3], BYTE finish[3]);
static void set_palette2(BYTE start[3], BYTE finish[3]);
static void set_palette3(BYTE start[3], BYTE middle[3], BYTE finish[3]);

static bool s_paused;                      // rotate-is-paused flag
static BYTE Red[3]    = {COLOR_CHANNEL_MAX, 0, 0};     // for shifted-Fkeys
static BYTE Green[3]  = { 0,COLOR_CHANNEL_MAX, 0};
static BYTE Blue[3]   = { 0, 0,COLOR_CHANNEL_MAX};
static BYTE Black[3]  = { 0, 0, 0};
static BYTE White[3]  = {COLOR_CHANNEL_MAX,COLOR_CHANNEL_MAX,COLOR_CHANNEL_MAX};
static BYTE Yellow[3] = {COLOR_CHANNEL_MAX,COLOR_CHANNEL_MAX, 0};
static BYTE Brown[3]  = {COLOR_CHANNEL_MAX/2,COLOR_CHANNEL_MAX/2, 0};

static char mapmask[13] = {"*.map"};

void rotate(int direction)      // rotate-the-palette routine
{
	static int fsteps[] = {2, 4, 8, 12, 16, 24, 32, 40, 54, 100}; // (for Fkeys)

#ifndef XFRACT
	if (!g_.RealDAC())					// ??? no DAC to rotate!
#else
	if (!(g_.RealDAC() || g_fake_lut))		// ??? no DAC to rotate!
#endif
	{
		driver_buzzer(BUZZER_ERROR);
		return;
	}

	HelpModeSaver saved_help(IDHELP_COLOR_CYCLING);

	s_paused = false;						// not paused
	int fkey = 0;							// no random coloring
	int step = 1;
	int oldstep = 1;						// single-step
	int fstep = 1;
	int change_color = -1;					// no color (rgb) to change
	int change_direction = 0;				// no color direction to change
	int incr = 999;							// ready to randomize
	srand((unsigned) time(0));			// randomize things

	if (direction == 0)  // firing up in paused mode?
	{
		pause_rotate();                    // then force a pause
		direction = 1;                    // and set a rotate direction
	}

	int rotate_max = (g_rotate_hi < g_colors) ? g_rotate_hi : g_colors-1;
	int rotate_size = rotate_max - g_rotate_lo + 1;
	int last = rotate_max;                   // last box that was filled
	int next = g_rotate_lo;                    // next box to be filled
	if (direction < 0)
	{
		last = g_rotate_lo;
		next = rotate_max;
	}

	bool more = true;
	while (more)
	{
		if (driver_diskp())
		{
			if (!s_paused)
			{
				pause_rotate();
			}
		}
		else while (!driver_key_pressed())  // rotate until key hit, at least once so step = oldstep ok
		{
			if (fkey > 0)  // randomizing is on
			{
				int fromred = 0;
				int fromblue = 0;
				int fromgreen = 0;
				int tored = 0;
				int toblue = 0;
				int togreen = 0;
				for (int istep = 0; istep < step; istep++)
				{
					int jstep = next + (istep*direction);
					while (jstep < g_rotate_lo)
					{
						jstep += rotate_size;
					}
					while (jstep > rotate_max)
					{
						jstep -= rotate_size;
					}
					if (++incr > fstep)  // time to randomize
					{
						// TODO: revirew for case when COLOR_CHANNEL_MAX != 63
						incr = 1;
						fstep = ((fsteps[fkey-1]* (rand15() >> 8)) >> 6) + 1;
						fromred   = g_.DAC().Red(last);
						fromgreen = g_.DAC().Green(last);
						fromblue  = g_.DAC().Blue(last);
						tored     = rand15() >> 9;
						togreen   = rand15() >> 9;
						toblue    = rand15() >> 9;
					}
					// TODO: revirew for case when COLOR_CHANNEL_MAX != 63
					g_.DAC().Set(jstep,
						BYTE(fromred   + (((tored    - fromred)*incr)/fstep)),
						BYTE(fromgreen + (((togreen - fromgreen)*incr)/fstep)),
						BYTE(fromblue  + (((toblue  - fromblue)*incr)/fstep)));
				}
			}
			if (step >= rotate_size)
			{
				step = oldstep;
			}
			spin_dac(direction, step);
		}
		if (step >= rotate_size)
		{
			step = oldstep;
		}
		int kbdchar = driver_get_key();
		if (s_paused
			&& (kbdchar != ' '
				&& kbdchar != 'c'
				&& kbdchar != IDK_HOME
				&& kbdchar != 'C'))
		{
			s_paused = false;                    // clear paused condition
		}
		switch (kbdchar)
		{
		case '+':                      // '+' means rotate forward
		case IDK_RIGHT_ARROW:              // RightArrow = rotate fwd
			fkey = 0;
			direction = 1;
			last = rotate_max;
			next = g_rotate_lo;
			incr = 999;
			break;
		case '-':                      // '-' means rotate backward
		case IDK_LEFT_ARROW:               // LeftArrow = rotate bkwd
			fkey = 0;
			direction = -1;
			last = g_rotate_lo;
			next = rotate_max;
			incr = 999;
			break;
		case IDK_UP_ARROW:                 // UpArrow means speed up
			g_.IncreaseDACSleepCount();
			break;
		case IDK_DOWN_ARROW:               // DownArrow means slow down
			g_.DecreaseDACSleepCount();
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			step = kbdchar - '0';   // change step-size
			if (step > rotate_size)
			{
				step = rotate_size;
			}
			break;
		case IDK_F1:                       // IDK_F1 - IDK_F10:
		case IDK_F2:                       // select a shading factor
		case IDK_F3:
		case IDK_F4:
		case IDK_F5:
		case IDK_F6:
		case IDK_F7:
		case IDK_F8:
		case IDK_F9:
		case IDK_F10:
#ifndef XFRACT
			fkey = kbdchar-1058;
#else
			switch (kbdchar)
			{
			case IDK_F1: fkey = 1; break;
			case IDK_F2: fkey = 2; break;
			case IDK_F3: fkey = 3; break;
			case IDK_F4: fkey = 4; break;
			case IDK_F5: fkey = 5; break;
			case IDK_F6: fkey = 6; break;
			case IDK_F7: fkey = 7; break;
			case IDK_F8: fkey = 8; break;
			case IDK_F9: fkey = 9; break;
			case IDK_F10: fkey = 10; break;
			}
#endif
			fstep = 1;
			incr = 999;
			break;
		case IDK_ENTER:                    // enter key: randomize all colors
		case IDK_ENTER_2:                  // also the Numeric-Keypad Enter
			fkey = rand15()/3277 + 1;
			fstep = 1;
			incr = 999;
			oldstep = step;
			step = rotate_size;
			break;
		case 'r':                      // color changes
			if (change_color    == -1)
			{
				change_color = 0;
			}
		case 'g':                      // color changes
			if (change_color    == -1)
			{
				change_color = 1;
			}
		case 'b':                      // color changes
			if (change_color    == -1)
			{
				change_color = 2;
			}
			if (change_direction == 0)
			{
				change_direction = -1;
			}
		case 'R':                      // color changes
			if (change_color    == -1)
			{
				change_color = 0;
			}
		case 'G':                      // color changes
			if (change_color    == -1)
			{
				change_color = 1;
			}
		case 'B':                      // color changes
			if (driver_diskp())
			{
				break;
			}
			if (change_color    == -1)
			{
				change_color = 2;
			}
			if (change_direction == 0)
			{
				change_direction = 1;
			}
			for (int i = 1; i < 256; i++)
			{
				g_.DAC().SetChannel(i, change_color, BYTE(g_.DAC().Channel(i, change_color) + change_direction));
				if (g_.DAC().Channel(i, change_color) == COLOR_CHANNEL_MAX+1)
				{
					g_.DAC().SetChannel(i, change_color, COLOR_CHANNEL_MAX);
				}
				if (g_.DAC().Channel(i, change_color) == 255)
				{
					g_.DAC().SetChannel(i, change_color, 0);
				}
			}
			change_color = -1;				// clear flags for next time
			change_direction = 0;
			s_paused = false;				// clear any pause
		case ' ':							// use the spacebar as a "pause" toggle
		case 'c':							// for completeness' sake, the 'c' too
		case 'C':
			pause_rotate();					// pause
			break;
		case '>':							// single-step
		case '.':
		case '<':
		case ',':
			if (kbdchar == '>' || kbdchar == '.')
			{
				direction = -1;
				last = g_rotate_lo;
				next = rotate_max;
				incr = 999;
			}
			else
			{
				direction = 1;
				last = rotate_max;
				next = g_rotate_lo;
				incr = 999;
			}
			fkey = 0;
			spin_dac(direction, 1);
			if (! s_paused)
			{
				pause_rotate();				// pause
			}
			break;

		case 'd':							// load colors from "default.map"
		case 'D':
			if (validate_luts("default"))
			{
				break;
			}
			fkey = 0;                   // disable random generation
			pause_rotate();              // update palette and pause
			break;

		case 'a':                      // load colors from "altern.map"
		case 'A':
			if (validate_luts("altern"))
			{
				break;
			}
			fkey = 0;                   // disable random generation
			pause_rotate();              // update palette and pause
			break;

		case 'l':                      // load colors from a specified map
#ifndef XFRACT // L is used for IDK_RIGHT_ARROW in Unix keyboard mapping
		case 'L':
#endif
			load_palette();
			fkey = 0;                   // disable random generation
			pause_rotate();              // update palette and pause
			break;

		case 's':                      // save the palette
		case 'S':
			save_palette();
			fkey = 0;                   // disable random generation
			pause_rotate();              // update palette and pause
			break;

		case IDK_ESC:                      // escape
			more = false;                   // time to bail out
			break;

		case IDK_HOME:                     // restore palette
			g_.PopDAC();
			pause_rotate();              // pause
			break;

		default:						// maybe a new palette
			fkey = 0;                   // disable random generation
			switch (kbdchar)
			{
			case IDK_SF1:		set_palette(Black, White);			break;
			case IDK_SF2:		set_palette(Red, Yellow);			break;
			case IDK_SF3:		set_palette(Blue, Green);			break;
			case IDK_SF4:		set_palette(Black, Yellow);			break;
			case IDK_SF5:		set_palette(Black, Red);			break;
			case IDK_SF6:		set_palette(Black, Blue);			break;
			case IDK_SF7:		set_palette(Black, Green);			break;
			case IDK_SF8:		set_palette(Blue, Yellow);			break;
			case IDK_SF9:		set_palette(Red, Green);			break;
			case IDK_SF10:		set_palette(Green, White);			break;
			case IDK_CTL_F1:	set_palette2(Black, White);			break;
			case IDK_CTL_F2:	set_palette2(Red, Yellow);			break;
			case IDK_CTL_F3:	set_palette2(Blue, Green);			break;
			case IDK_CTL_F4:	set_palette2(Black, Yellow);		break;
			case IDK_CTL_F5:	set_palette2(Black, Red);			break;
			case IDK_CTL_F6:	set_palette2(Black, Blue);			break;
			case IDK_CTL_F7:	set_palette2(Black, Green);			break;
			case IDK_CTL_F8:	set_palette2(Blue, Yellow);			break;
			case IDK_CTL_F9:	set_palette2(Red, Green);			break;
			case IDK_CTL_F10:	set_palette2(Green, White);			break;
			case IDK_ALT_F1:	set_palette3(Blue, Green, Red);		break;
			case IDK_ALT_F2:	set_palette3(Blue, Yellow, Red);	break;
			case IDK_ALT_F3:	set_palette3(Red, White, Blue);		break;
			case IDK_ALT_F4:	set_palette3(Red, Yellow, White);	break;
			case IDK_ALT_F5:	set_palette3(Black, Brown, Yellow);	break;
			case IDK_ALT_F6:	set_palette3(Blue, Brown, Green);	break;
			case IDK_ALT_F7:	set_palette3(Blue, Green, Green);	break;
			case IDK_ALT_F8:	set_palette3(Blue, Green, White);	break;
			case IDK_ALT_F9:	set_palette3(Green, Green, White);	break;
			case IDK_ALT_F10:	set_palette3(Red, Blue, White);		break;
			}
			pause_rotate();  // update palette and pause
			break;
		}
	}
}

static void pause_rotate()               // pause-the-rotate routine
{
	// saved dac-count value goes here
	if (s_paused)                          // if already paused , just clear
	{
		s_paused = false;
		return;
	}

	// set border, wait for a key
	int olddaccount = g_.DACSleepCount();
	BYTE olddac0 = g_.DAC().Red(0);
	BYTE olddac1 = g_.DAC().Green(0);
	BYTE olddac2 = g_.DAC().Blue(0);
	g_.SetDACSleepCount(256);
	g_.DAC().Set(0, 3*COLOR_CHANNEL_MAX/4, 3*COLOR_CHANNEL_MAX/4, 3*COLOR_CHANNEL_MAX/4);
	load_dac();                     // show white border
	if (driver_diskp())
	{
		disk_video_status(100, " Paused in \"color cycling\" mode ");
	}
	driver_wait_key_pressed(0);                // wait for any key

	if (driver_diskp())
	{
		disk_video_status(0, "");
	}
	g_.DAC().Set(0, olddac0, olddac1, olddac2);
	load_dac();                     // show black border
	g_.SetDACSleepCount(olddaccount);
	s_paused = true;
}

// TODO: review case when COLOR_CHANNEL_MAX != 63
static void set_palette(BYTE start[3], BYTE finish[3])
{
	g_.DAC().Set(0, 0, 0, 0);
	for (int i = 1; i <= 255; i++)                  // fill the palette
	{
		for (int j = 0; j < 3; j++)
		{
			g_.DAC().SetChannel(i, j, BYTE((i*start[j] + (256-i)*finish[j])/255));
		}
	}
}

// TODO: review case when COLOR_CHANNEL_MAX != 63
static void set_palette2(BYTE start[3], BYTE finish[3])
{
	g_.DAC().Set(0, 0, 0, 0);
	for (int i = 1; i <= 128; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			g_.DAC().SetChannel(i, j, BYTE((i*finish[j] + (128 - i)*start[j])/128));
			g_.DAC().SetChannel(i + 127, j, BYTE((i*start[j]  + (128 - i)*finish[j])/128));
		}
	}
}

// TODO: review case when COLOR_CHANNEL_MAX != 63
static void set_palette3(BYTE start[3], BYTE middle[3], BYTE finish[3])
{
	g_.DAC().Set(0, 0, 0, 0);
	for (int i = 1; i <= 85; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			g_.DAC().SetChannel(i, j, BYTE((i*middle[j] + (86 - i)*start[j])/85));
			g_.DAC().SetChannel(i + 85, j, BYTE((i*finish[j] + (86 - i)*middle[j])/85));
			g_.DAC().SetChannel(i + 170, j, BYTE((i*start[j]  + (86 - i)*finish[j])/85));
		}
	}
}

void save_palette()
{
	char temp1[256] = { 0 };
	{
		ScreenStacker stacker;
		if (field_prompt_help(IDHELP_COLORMAP, "Name of map file to write", temp1, 60) < 0)
		{
			return;
		}
	}
	if (!temp1[0])
	{
		return;
	}

	if (strchr(temp1, '.') == 0)
	{
		strcat(temp1, ".map");
	}

	std::string mapName = g_.MapName();
	std::string arg = temp1;
	merge_path_names(false, mapName, arg);
	g_.SetMapName(mapName);
	std::ofstream dac_file(mapName.c_str());
	if (!dac_file)
	{
		driver_buzzer(BUZZER_ERROR);
	}
	else
	{
		for (int i = 0; i < g_colors; i++)
		{
			// TODO: review case when COLOR_CHANNEL_MAX != 63
			dac_file << boost::format("%3d %3d %3d\n")
				% (g_.DAC().Red(i) << 2)
				% (g_.DAC().Green(i) << 2)
				% (g_.DAC().Blue(i) << 2);
		}
		g_.PushDAC();
		g_.SetColorState(COLORSTATE_MAP);
		g_externs.SetColorFile(temp1);
	}
	dac_file.close();
}

int load_palette()
{
	char filename[FILE_MAX_PATH];
	strcpy(filename, g_.MapName().c_str());
	int i;
	{
		ScreenStacker stacker;
		i = get_a_filename_help(IDHELP_COLORMAP, "Select a MAP File", mapmask, filename);
	}
	if (i >= 0)
	{
		if (!validate_luts(filename))
		{
			g_.PushDAC();
		}
		std::string mapName = g_.MapName();
		std::string arg = filename;
		merge_path_names(true, mapName, arg);
		g_.SetMapName(mapName);
	}
	return i;
}
