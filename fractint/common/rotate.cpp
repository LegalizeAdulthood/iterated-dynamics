/*
	rotate.c - Routines that manipulate the Video DAC on VGA Adapters
*/

#include <string.h>
#include <time.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "drivers.h"
#include "fihelp.h"

/* routines in this module      */

static void pause_rotate();
static void set_palette(BYTE start[3], BYTE finish[3]);
static void set_palette2(BYTE start[3], BYTE finish[3]);
static void set_palette3(BYTE start[3], BYTE middle[3], BYTE finish[3]);

static bool s_paused;                      /* rotate-is-paused flag */
static BYTE Red[3]    = {COLOR_CHANNEL_MAX, 0, 0};     /* for shifted-Fkeys */
static BYTE Green[3]  = { 0,COLOR_CHANNEL_MAX, 0};
static BYTE Blue[3]   = { 0, 0,COLOR_CHANNEL_MAX};
static BYTE Black[3]  = { 0, 0, 0};
static BYTE White[3]  = {COLOR_CHANNEL_MAX,COLOR_CHANNEL_MAX,COLOR_CHANNEL_MAX};
static BYTE Yellow[3] = {COLOR_CHANNEL_MAX,COLOR_CHANNEL_MAX, 0};
static BYTE Brown[3]  = {COLOR_CHANNEL_MAX/2,COLOR_CHANNEL_MAX/2, 0};

static char mapmask[13] = {"*.map"};

void rotate(int direction)      /* rotate-the-palette routine */
{
	static int fsteps[] = {2, 4, 8, 12, 16, 24, 32, 40, 54, 100}; /* (for Fkeys) */

#ifndef XFRACT
	if (g_got_real_dac == 0                  /* ??? no DAC to rotate! */
#else
	if (!(g_got_real_dac || g_fake_lut)        /* ??? no DAC to rotate! */
#endif
		|| g_colors < 16)  /* strange things happen in 2x modes */
	{
		driver_buzzer(BUZZER_ERROR);
		return;
	}

	HelpModeSaver saved_help(HELPCYCLING);

	s_paused = false;						/* not paused                   */
	int fkey = 0;							/* no random coloring           */
	int step = 1;
	int oldstep = 1;						/* single-step                  */
	int fstep = 1;
	int change_color = -1;					/* no color (rgb) to change     */
	int change_direction = 0;				/* no color direction to change */
	int incr = 999;							/* ready to randomize           */
	srand((unsigned) time(NULL));			/* randomize things             */

	if (direction == 0)  /* firing up in paused mode?    */
	{
		pause_rotate();                    /* then force a pause           */
		direction = 1;                    /* and set a rotate direction   */
	}

	int rotate_max = (g_rotate_hi < g_colors) ? g_rotate_hi : g_colors-1;
	int rotate_size = rotate_max - g_rotate_lo + 1;
	int last = rotate_max;                   /* last box that was filled     */
	int next = g_rotate_lo;                    /* next box to be filled        */
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
		else while (!driver_key_pressed())  /* rotate until key hit, at least once so step = oldstep ok */
		{
			if (fkey > 0)  /* randomizing is on */
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
					if (++incr > fstep)  /* time to randomize */
					{
						/* TODO: revirew for case when COLOR_CHANNEL_MAX != 63 */
						incr = 1;
						fstep = ((fsteps[fkey-1]* (rand15() >> 8)) >> 6) + 1;
						fromred   = g_dac_box[last][0];
						fromgreen = g_dac_box[last][1];
						fromblue  = g_dac_box[last][2];
						tored     = rand15() >> 9;
						togreen   = rand15() >> 9;
						toblue    = rand15() >> 9;
					}
					/* TODO: revirew for case when COLOR_CHANNEL_MAX != 63 */
					g_dac_box[jstep][0] = (BYTE)(fromred   + (((tored    - fromred)*incr)/fstep));
					g_dac_box[jstep][1] = (BYTE)(fromgreen + (((togreen - fromgreen)*incr)/fstep));
					g_dac_box[jstep][2] = (BYTE)(fromblue  + (((toblue  - fromblue)*incr)/fstep));
				}
			}
			if (step >= rotate_size)
			{
				step = oldstep;
			}
			spindac(direction, step);
		}
		if (step >= rotate_size)
		{
			step = oldstep;
		}
		int kbdchar = driver_get_key();
		if (s_paused
			&& (kbdchar != ' '
				&& kbdchar != 'c'
				&& kbdchar != FIK_HOME
				&& kbdchar != 'C'))
		{
			s_paused = false;                    /* clear paused condition       */
		}
		switch (kbdchar)
		{
		case '+':                      /* '+' means rotate forward     */
		case FIK_RIGHT_ARROW:              /* RightArrow = rotate fwd      */
			fkey = 0;
			direction = 1;
			last = rotate_max;
			next = g_rotate_lo;
			incr = 999;
			break;
		case '-':                      /* '-' means rotate backward    */
		case FIK_LEFT_ARROW:               /* LeftArrow = rotate bkwd      */
			fkey = 0;
			direction = -1;
			last = g_rotate_lo;
			next = rotate_max;
			incr = 999;
			break;
		case FIK_UP_ARROW:                 /* UpArrow means speed up       */
			g_dac_learn = 1;
			if (++g_dac_count >= g_colors)
			{
				--g_dac_count;
			}
			break;
		case FIK_DOWN_ARROW:               /* DownArrow means slow down    */
			g_dac_learn = 1;
			if (g_dac_count > 1)
			{
				g_dac_count--;
			}
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
			step = kbdchar - '0';   /* change step-size */
			if (step > rotate_size)
			{
				step = rotate_size;
			}
			break;
		case FIK_F1:                       /* FIK_F1 - FIK_F10:                    */
		case FIK_F2:                       /* select a shading factor      */
		case FIK_F3:
		case FIK_F4:
		case FIK_F5:
		case FIK_F6:
		case FIK_F7:
		case FIK_F8:
		case FIK_F9:
		case FIK_F10:
#ifndef XFRACT
			fkey = kbdchar-1058;
#else
			switch (kbdchar)
			{
			case FIK_F1: fkey = 1; break;
			case FIK_F2: fkey = 2; break;
			case FIK_F3: fkey = 3; break;
			case FIK_F4: fkey = 4; break;
			case FIK_F5: fkey = 5; break;
			case FIK_F6: fkey = 6; break;
			case FIK_F7: fkey = 7; break;
			case FIK_F8: fkey = 8; break;
			case FIK_F9: fkey = 9; break;
			case FIK_F10: fkey = 10; break;
			}
#endif
			fstep = 1;
			incr = 999;
			break;
		case FIK_ENTER:                    /* enter key: randomize all colors */
		case FIK_ENTER_2:                  /* also the Numeric-Keypad Enter */
			fkey = rand15()/3277 + 1;
			fstep = 1;
			incr = 999;
			oldstep = step;
			step = rotate_size;
			break;
		case 'r':                      /* color changes */
			if (change_color    == -1)
			{
				change_color = 0;
			}
		case 'g':                      /* color changes */
			if (change_color    == -1)
			{
				change_color = 1;
			}
		case 'b':                      /* color changes */
			if (change_color    == -1)
			{
				change_color = 2;
			}
			if (change_direction == 0)
			{
				change_direction = -1;
			}
		case 'R':                      /* color changes */
			if (change_color    == -1)
			{
				change_color = 0;
			}
		case 'G':                      /* color changes */
			if (change_color    == -1)
			{
				change_color = 1;
			}
		case 'B':                      /* color changes */
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
				g_dac_box[i][change_color] = (BYTE) (g_dac_box[i][change_color] + change_direction);
				if (g_dac_box[i][change_color] == COLOR_CHANNEL_MAX+1)
				{
					g_dac_box[i][change_color] = COLOR_CHANNEL_MAX;
				}
				if (g_dac_box[i][change_color] == 255)
				{
					g_dac_box[i][change_color] = 0;
				}
			}
			change_color = -1;				/* clear flags for next time */
			change_direction = 0;
			s_paused = false;				/* clear any pause */
		case ' ':							/* use the spacebar as a "pause" toggle */
		case 'c':							/* for completeness' sake, the 'c' too */
		case 'C':
			pause_rotate();					/* pause */
			break;
		case '>':							/* single-step */
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
			spindac(direction, 1);
			if (! s_paused)
			{
				pause_rotate();				/* pause */
			}
			break;

		case 'd':							/* load colors from "default.map" */
		case 'D':
			if (validate_luts("default") != 0)
			{
				break;
			}
			fkey = 0;                   /* disable random generation */
			pause_rotate();              /* update palette and pause */
			break;

		case 'a':                      /* load colors from "altern.map" */
		case 'A':
			if (validate_luts("altern") != 0)
			{
				break;
			}
			fkey = 0;                   /* disable random generation */
			pause_rotate();              /* update palette and pause */
			break;

		case 'l':                      /* load colors from a specified map */
#ifndef XFRACT /* L is used for FIK_RIGHT_ARROW in Unix keyboard mapping */
		case 'L':
#endif
			load_palette();
			fkey = 0;                   /* disable random generation */
			pause_rotate();              /* update palette and pause */
			break;

		case 's':                      /* save the palette */
		case 'S':
			save_palette();
			fkey = 0;                   /* disable random generation */
			pause_rotate();              /* update palette and pause */
			break;

		case FIK_ESC:                      /* escape */
			more = false;                   /* time to bail out */
			break;

		case FIK_HOME:                     /* restore palette */
			memcpy(g_dac_box, g_old_dac_box, 256*3);
			pause_rotate();              /* pause */
			break;

		default:						/* maybe a new palette */
			fkey = 0;                   /* disable random generation */
			switch (kbdchar)
			{
			case FIK_SF1:		set_palette(Black, White);			break;
			case FIK_SF2:		set_palette(Red, Yellow);			break;
			case FIK_SF3:		set_palette(Blue, Green);			break;
			case FIK_SF4:		set_palette(Black, Yellow);			break;
			case FIK_SF5:		set_palette(Black, Red);			break;
			case FIK_SF6:		set_palette(Black, Blue);			break;
			case FIK_SF7:		set_palette(Black, Green);			break;
			case FIK_SF8:		set_palette(Blue, Yellow);			break;
			case FIK_SF9:		set_palette(Red, Green);			break;
			case FIK_SF10:		set_palette(Green, White);			break;
			case FIK_CTL_F1:	set_palette2(Black, White);			break;
			case FIK_CTL_F2:	set_palette2(Red, Yellow);			break;
			case FIK_CTL_F3:	set_palette2(Blue, Green);			break;
			case FIK_CTL_F4:	set_palette2(Black, Yellow);		break;
			case FIK_CTL_F5:	set_palette2(Black, Red);			break;
			case FIK_CTL_F6:	set_palette2(Black, Blue);			break;
			case FIK_CTL_F7:	set_palette2(Black, Green);			break;
			case FIK_CTL_F8:	set_palette2(Blue, Yellow);			break;
			case FIK_CTL_F9:	set_palette2(Red, Green);			break;
			case FIK_CTL_F10:	set_palette2(Green, White);			break;
			case FIK_ALT_F1:	set_palette3(Blue, Green, Red);		break;
			case FIK_ALT_F2:	set_palette3(Blue, Yellow, Red);	break;
			case FIK_ALT_F3:	set_palette3(Red, White, Blue);		break;
			case FIK_ALT_F4:	set_palette3(Red, Yellow, White);	break;
			case FIK_ALT_F5:	set_palette3(Black, Brown, Yellow);	break;
			case FIK_ALT_F6:	set_palette3(Blue, Brown, Green);	break;
			case FIK_ALT_F7:	set_palette3(Blue, Green, Green);	break;
			case FIK_ALT_F8:	set_palette3(Blue, Green, White);	break;
			case FIK_ALT_F9:	set_palette3(Green, Green, White);	break;
			case FIK_ALT_F10:	set_palette3(Red, Blue, White);		break;
			}
			pause_rotate();  /* update palette and pause */
			break;
		}
	}
}

static void pause_rotate()               /* pause-the-rotate routine */
{
	/* saved dac-count value goes here */
	if (s_paused)                          /* if already paused , just clear */
	{
		s_paused = false;
		return;
	}

	/* set border, wait for a key */
	int olddaccount = g_dac_count;
	BYTE olddac0 = g_dac_box[0][0];
	BYTE olddac1 = g_dac_box[0][1];
	BYTE olddac2 = g_dac_box[0][2];
	g_dac_count = 256;
	g_dac_box[0][0] = 3*COLOR_CHANNEL_MAX/4;
	g_dac_box[0][1] = 3*COLOR_CHANNEL_MAX/4;
	g_dac_box[0][2] = 3*COLOR_CHANNEL_MAX/4;
	spindac(0, 1);                     /* show white border */
	if (driver_diskp())
	{
		disk_video_status(100, " Paused in \"color cycling\" mode ");
	}
	driver_wait_key_pressed(0);                /* wait for any key */

	if (driver_diskp())
	{
		disk_video_status(0, "");
	}
	g_dac_box[0][0] = olddac0;
	g_dac_box[0][1] = olddac1;
	g_dac_box[0][2] = olddac2;
	spindac(0, 1);                     /* show black border */
	g_dac_count = olddaccount;
	s_paused = true;
}

/* TODO: review case when COLOR_CHANNEL_MAX != 63 */
static void set_palette(BYTE start[3], BYTE finish[3])
{
	g_dac_box[0][0] = g_dac_box[0][1] = g_dac_box[0][2] = 0;
	for (int i = 1; i <= 255; i++)                  /* fill the palette     */
	{
		for (int j = 0; j < 3; j++)
		{
			g_dac_box[i][j] = (BYTE)((i*start[j] + (256-i)*finish[j])/255);
		}
	}
}

/* TODO: review case when COLOR_CHANNEL_MAX != 63 */
static void set_palette2(BYTE start[3], BYTE finish[3])
{
	g_dac_box[0][0] = 0;
	g_dac_box[0][1] = 0;
	g_dac_box[0][2] = 0;
	for (int i = 1; i <= 128; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			g_dac_box[i][j]       = (BYTE)((i*finish[j] + (128 - i)*start[j])/128);
			g_dac_box[i + 127][j] = (BYTE)((i*start[j]  + (128 - i)*finish[j])/128);
		}
	}
}

/* TODO: review case when COLOR_CHANNEL_MAX != 63 */
static void set_palette3(BYTE start[3], BYTE middle[3], BYTE finish[3])
{
	g_dac_box[0][0] = 0;
	g_dac_box[0][1] = 0;
	g_dac_box[0][2] = 0;
	for (int i = 1; i <= 85; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			g_dac_box[i][j]       = (BYTE)((i*middle[j] + (86 - i)*start[j])/85);
			g_dac_box[i + 85][j]  = (BYTE)((i*finish[j] + (86 - i)*middle[j])/85);
			g_dac_box[i + 170][j] = (BYTE)((i*start[j]  + (86 - i)*finish[j])/85);
		}
	}
}

void save_palette()
{
	char palname[FILE_MAX_PATH];
	strcpy(palname, g_map_name);
	driver_stack_screen();
	char temp1[256] = { 0 };
	int i = field_prompt_help(HELPCOLORMAP, "Name of map file to write", NULL, temp1, 60, NULL);
	driver_unstack_screen();
	if (i != -1 && temp1[0])
	{
		if (strchr(temp1, '.') == NULL)
		{
			strcat(temp1, ".map");
		}
		merge_path_names(palname, temp1, 2);
		FILE *dacfile = fopen(palname, "w");
		if (dacfile == NULL)
		{
			driver_buzzer(BUZZER_ERROR);
		}
		else
		{
#ifndef XFRACT
			for (i = 0; i < g_colors; i++)
#else
			for (i = 0; i < 256; i++)
#endif
			{
				/* TODO: review case when COLOR_CHANNEL_MAX != 63 */
				fprintf(dacfile, "%3d %3d %3d\n",
						g_dac_box[i][0] << 2,
						g_dac_box[i][1] << 2,
						g_dac_box[i][2] << 2);
			}
			memcpy(g_old_dac_box, g_dac_box, 256*3);
			g_color_state = COLORSTATE_MAP;
			strcpy(g_color_file, temp1);
		}
		fclose(dacfile);
	}
}

int load_palette()
{
	char filename[FILE_MAX_PATH];
	strcpy(filename, g_map_name);
	driver_stack_screen();
	int i = get_a_filename_help(HELPCOLORMAP, "Select a MAP File", mapmask, filename);
	driver_unstack_screen();
	if (i >= 0)
	{
		if (validate_luts(filename) == 0)
		{
			memcpy(g_old_dac_box, g_dac_box, 256*3);
		}
		merge_path_names(g_map_name, filename, 0);
	}
	return i;
}
