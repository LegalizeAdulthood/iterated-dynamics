/*
	STEREO.C a module to view 3D images.
	Written in Borland 'C++' by Paul de Leeuw.
	From an idea in "New Scientist" 9 October 1993 pages 26 - 29.

	Change History:
		11 June 94 - Modified to reuse existing Fractint arrays        TW
		11 July 94 - Added depth parameter                             PDL
		14 July 94 - Added grayscale option and did general cleanup    TW
		19 July 94 - Fixed negative depth                              PDL
		19 July 94 - Added calibration bars, get_min_max()             TW
		24 Sep  94 - Added image save/restore, color cycle, and save   TW
		28 Sep  94 - Added image map                                   TW
		20 Mar  95 - Fixed endless loop bug with bad depth values      TW
		23 Mar  95 - Allow arbitrary dimension image maps              TW

		(TW is Tim Wegner, PDL is Paul de Leeuw)
*/
#include <string.h>
#include <time.h>

/* see Fractint.cpp for a description of the include hierarchy */
#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fihelp.h"

#include "drivers.h"
#include "encoder.h"
#include "gifview.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"

#include <malloc.h>

char g_stereo_map_name[FILE_MAX_DIR + 1] = {""};
int g_auto_stereo_depth = 100;
double g_auto_stereo_width = 10;
int g_grayscale_depth = 0; /* flag to use gray value rather than color number */
char g_calibrate = 1;             /* add calibration bars to image */
bool g_image_map = false;

static long s_average;
static long s_average_count;
static long s_depth;
static int s_bar_height;
static int s_ground;
static int s_max_cc;
static int s_max_c;
static int s_min_c;
static int s_reverse;
static int s_separation;
static double s_width;
static int s_x1;
static int s_x2;
static int s_x_center;
static int s_y;
static int s_y1;
static int s_y2;
static int s_y_center;
static BYTE s_save_dac[256][3];

/*
	The getdepth() function allows using the grayscale value of the color
	as s_depth, rather than the color number. Maybe I got a little too
	sophisticated trying to avoid a divide, so the comment tells what all
	the multiplies and shifts are trying to do. The result should be from
	0 to 255.
*/

static int getdepth(int xd, int yd)
{
	int pal = getcolor(xd, yd);
	if (g_grayscale_depth)
	{
		/* effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255 */
		pal = ((int) s_save_dac[pal][0]*77 +
				(int) s_save_dac[pal][1]*151 +
				(int) s_save_dac[pal][2]*28);
		pal >>= 6;
	}
	return pal;
}

/*
	Get min and max DEPTH value in picture
*/
static int get_min_max()
{
	s_min_c = g_colors;
	s_max_c = 0;
	for (int yd = 0; yd < g_y_dots; yd++)
	{
		if (driver_key_pressed())
		{
			return 1;
		}
		if (yd == 20)
		{
			show_temp_message("Getting min and max");
		}
		for (int xd = 0; xd < g_x_dots; xd++)
		{
			int ldepth = getdepth(xd, yd);
			if (ldepth < s_min_c)
			{
				s_min_c = ldepth;
			}
			if (ldepth > s_max_c)
			{
				s_max_c = ldepth;
			}
		}
	}
	clear_temp_message();
	return 0;
}

static void toggle_bars(bool &bars, int barwidth, int *colour)
{
	find_special_colors();
	int ct = 0;
	for (int i = s_x_center; i < (s_x_center) + barwidth; i++)
	{
		for (int j = s_y_center; j < (s_y_center) + s_bar_height; j++)
		{
			if (bars)
			{
				g_put_color(i + (int) s_average, j , g_color_bright);
				g_put_color(i - (int) s_average, j , g_color_bright);
			}
			else
			{
				g_put_color(i + (int) s_average, j, colour[ct++]);
				g_put_color(i - (int) s_average, j, colour[ct++]);
			}
		}
	}
	bars = !bars;
}

int out_line_stereo(BYTE *pixels, int linelen)
{
	int *colour = (int *) _alloca(sizeof(int)*g_x_dots);
	if ((s_y) >= g_y_dots)
	{
		return 1;
	}

	int *same = (int *) _alloca(sizeof(int)*g_x_dots);
	for (int x = 0; x < g_x_dots; ++x)
	{
		same[x] = x;
	}
	for (int x = 0; x < g_x_dots; ++x)
	{
		s_separation = s_reverse
			? (s_ground - (int) (s_depth*(getdepth(x, s_y) - s_min_c) / s_max_cc))
			: (s_ground - (int) (s_depth*(s_max_cc - (getdepth(x, s_y) - s_min_c)) / s_max_cc));
		s_separation =  (int) ((s_separation*10.0) / s_width);        /* adjust for media s_width */

		/* get average value under calibration bars */
		if (s_x1 <= x && x <= s_x2 && s_y1 <= s_y && s_y <= s_y2)
		{
			s_average += s_separation;
			(s_average_count)++;
		}
		int i = x - (s_separation + (s_separation & s_y & 1)) / 2;
		int j = i + s_separation;
		if (0 <= i && j < g_x_dots)
		{
			/* there are cases where next never terminates so we timeout */
			int ct = 0;
			for (int s = same[i]; s != i && s != j && ct++ < g_x_dots; s = same[i])
			{
				if (s > j)
				{
					same[i] = j;
					i = j;
					j = s;
				}
				else
				{
					i = s;
				}
			}
			same[i] = j;
		}
	}
	for (int x = g_x_dots - 1; x >= 0; x--)
	{
		colour[x] = (same[x] == x) ? (int) pixels[x % linelen] : colour[same[x]];
		g_put_color(x, s_y, colour[x]);
	}
	(s_y)++;
	return 0;
}


/**************************************************************************
		Convert current image into Auto Stereo Picture
**************************************************************************/

int auto_stereo()
{
	/* Use the current time to randomize the random number sequence. */
	{
		time_t ltime;
		time(&ltime);
		srand((unsigned int) ltime);
	}

	HelpModeSaver saved_help(RDSKEYS);
	driver_save_graphics();                      /* save graphics image */
	memcpy(s_save_dac, g_dac_box, 256*3);  /* save colors */

	int ret = 0;
	if (g_x_dots > OLD_MAX_PIXELS)
	{
		stop_message(0, "Stereo not allowed with resolution > 2048 pixels wide");
		driver_buzzer(BUZZER_INTERRUPT);
		ret = 1;
		goto exit_stereo;
	}

	/* empircally determined adjustment to make s_width scale correctly */
	s_width = g_auto_stereo_width*.67;
	if (s_width < 1)
	{
		s_width = 1;
	}
	s_ground = g_x_dots / 8;
	s_reverse = (g_auto_stereo_depth < 0) ? 1 : 0;
	s_depth = ((long) g_x_dots*(long) g_auto_stereo_depth) / 4000L;
	s_depth = labs(s_depth) + 1;
	if (get_min_max())
	{
		driver_buzzer(BUZZER_INTERRUPT);
		ret = 1;
		goto exit_stereo;
	}
	s_max_cc = s_max_c - s_min_c + 1;
	s_average = s_average_count = 0L;
	s_bar_height = 1 + g_y_dots / 20;
	s_x_center = g_x_dots/2;
	s_y_center = (g_calibrate > 1) ? s_bar_height/2 : g_y_dots/2;

	/* box to average for calibration bars */
	s_x1 = s_x_center - g_x_dots/16;
	s_x2 = s_x_center + g_x_dots/16;
	s_y1 = s_y_center - s_bar_height/2;
	s_y2 = s_y_center + s_bar_height/2;

	s_y = 0;
	if (g_image_map)
	{
		g_out_line = out_line_stereo;
		while ((s_y) < g_y_dots)
		{
			if (gifview())
			{
				ret = 1;
				goto exit_stereo;
			}
		}
	}
	else
	{
		while (s_y < g_y_dots)
		{
			if (driver_key_pressed())
			{
				ret = 1;
				goto exit_stereo;
			}
			unsigned char *buf = (unsigned char *) g_decoder_line;
			for (int i = 0; i < g_x_dots; i++)
			{
				buf[i] = (unsigned char) (rand() % g_colors);
			}
			out_line_stereo(buf, g_x_dots);
		}
	}

	{
		find_special_colors();
		s_average /= 2*s_average_count;
		int ct = 0;
		int *colour = (int *) _alloca(sizeof(int)*g_x_dots);
		int barwidth = 1 + g_x_dots / 200;
		for (int i = s_x_center; i < s_x_center + barwidth; i++)
		{
			for (int j = s_y_center; j < s_y_center + s_bar_height; j++)
			{
				colour[ct++] = getcolor(i + (int) s_average, j);
				colour[ct++] = getcolor(i - (int) s_average, j);
			}
		}
		bool bars = (g_calibrate != 0);
		toggle_bars(bars, barwidth, colour);
		int done = 0;
		while (done == 0)
		{
			driver_wait_key_pressed(0);
			int kbdchar = driver_get_key();
			switch (kbdchar)
			{
			case FIK_ENTER:   /* toggle bars */
			case FIK_SPACE:
				toggle_bars(bars, barwidth, colour);
				break;
			case 'c':
			case '+':
			case '-':
				rotate((kbdchar == 'c') ? 0 : ((kbdchar == '+') ? 1 : -1));
				break;
			case 's':
			case 'S':
				save_to_disk(g_save_name);
				break;
			default:
				if (kbdchar == FIK_ESC)   /* if ESC avoid returning to menu */
				{
					kbdchar = 255;
				}
				driver_unget_key(kbdchar);
				driver_buzzer(BUZZER_COMPLETE);
				done = 1;
				break;
			}
		}
	}

exit_stereo:
	driver_restore_graphics();
	memcpy(g_dac_box, s_save_dac, 256*3);
	spindac(0, 1);
	return ret;
}
