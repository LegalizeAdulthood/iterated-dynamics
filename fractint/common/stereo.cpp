/*
	stereo.cpp a module to view 3D images.
	From an idea in "New Scientist" 9 October 1993 pages 26 - 29.
*/
#include <sstream>
#include <string>
#include <vector>

#include <string.h>
#include <time.h>

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "encoder.h"
#include "fihelp.h"
#include "filesystem.h"
#include "gifview.h"
#include "prompts2.h"
#include "realdos.h"
#include "rotate.h"
#include "stereo.h"
#include "UIChoices.h"

#include <malloc.h>

std::string g_stereo_map_name = "";
static int s_auto_stereo_depth = 100;
double g_auto_stereo_width = 10;
bool g_grayscale_depth = false; /* flag to use gray value rather than color number */
static StereogramCalibrateType s_stereogram_calibrate = CALIBRATE_MIDDLE;	/* add calibration bars to image */
static bool s_image_map = false;

static long s_average;
static long s_average_count;
static long s_depth;
static int s_bar_height;
static int s_ground;
static int s_max_cc;
static int s_max_c;
static int s_min_c;
static bool s_reverse;
static int s_separation;
static double s_width;
static int s_x1;
static int s_x2;
static int s_x_center;
static int s_y;
static int s_y1;
static int s_y2;
static int s_y_center;
static ColormapTable s_save_dac;

/*
	The getdepth() function allows using the grayscale value of the color
	as s_depth, rather than the color number. Maybe I got a little too
	sophisticated trying to avoid a divide, so the comment tells what all
	the multiplies and shifts are trying to do. The result should be from
	0 to 255.
*/

static int getdepth(int xd, int yd)
{
	int pal = get_color(xd, yd);
	if (g_grayscale_depth)
	{
		/* effectively (30*R + 59*G + 11*B)/100 scaled 0 to 255 */
		pal = (int(s_save_dac.Red(pal))*77 +
				int(s_save_dac.Green(pal))*151 +
				int(s_save_dac.Blue(pal))*28);
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
	g_.DAC().FindSpecialColors();
	int ct = 0;
	for (int i = s_x_center; i < (s_x_center) + barwidth; i++)
	{
		for (int j = s_y_center; j < (s_y_center) + s_bar_height; j++)
		{
			if (bars)
			{
				g_plot_color_put_color(i + int(s_average), j , g_.DAC().Bright());
				g_plot_color_put_color(i - int(s_average), j , g_.DAC().Bright());
			}
			else
			{
				g_plot_color_put_color(i + int(s_average), j, colour[ct++]);
				g_plot_color_put_color(i - int(s_average), j, colour[ct++]);
			}
		}
	}
	bars = !bars;
}

int out_line_stereo(BYTE *pixels, int linelen)
{
	if (s_y >= g_y_dots)
	{
		return 1;
	}

	static std::vector<int> same;
	same.resize(g_x_dots);
	for (int x = 0; x < g_x_dots; ++x)
	{
		same[x] = x;
	}
	for (int x = 0; x < g_x_dots; ++x)
	{
		int depth = getdepth(x, s_y) - s_min_c;
		s_separation = s_reverse
			? (s_ground - int(s_depth*depth/s_max_cc))
			: (s_ground - int(s_depth*(s_max_cc - depth)/s_max_cc));
		s_separation =  int((s_separation*10.0)/s_width);        /* adjust for media s_width */

		/* get average value under calibration bars */
		if (s_x1 <= x && x <= s_x2 && s_y1 <= s_y && s_y <= s_y2)
		{
			s_average += s_separation;
			s_average_count++;
		}
		int i = x - (s_separation + (s_separation & s_y & 1))/2;
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
	static std::vector<int> color;
	color.resize(g_x_dots);
	for (int x = g_x_dots - 1; x >= 0; x--)
	{
		color[x] = (same[x] == x) ? int(pixels[x % linelen]) : color[same[x]];
		g_plot_color_put_color(x, s_y, color[x]);
	}
	s_y++;
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

	HelpModeSaver saved_help(FIHELP_RANDOM_DOT_STEREOGRAM_COMMANDS);
	driver_save_graphics();                      /* save graphics image */
	s_save_dac = g_.DAC();  /* save colors */

	int ret = 0;
	if (g_x_dots > OLD_MAX_PIXELS)
	{
		stop_message(STOPMSG_NORMAL, "Stereo not allowed with resolution > 2048 pixels wide");
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
	s_ground = g_x_dots/8;
	s_reverse = (s_auto_stereo_depth < 0);
	s_depth = (long(g_x_dots)*long(s_auto_stereo_depth))/4000L;
	s_depth = labs(s_depth) + 1;
	if (get_min_max())
	{
		driver_buzzer(BUZZER_INTERRUPT);
		ret = 1;
		goto exit_stereo;
	}
	s_max_cc = s_max_c - s_min_c + 1;
	s_average = 0;
	s_average_count = 0L;
	s_bar_height = 1 + g_y_dots/20;
	s_x_center = g_x_dots/2;
	s_y_center = (s_stereogram_calibrate == CALIBRATE_TOP) ? s_bar_height/2 : g_y_dots/2;

	/* box to average for calibration bars */
	s_x1 = s_x_center - g_x_dots/16;
	s_x2 = s_x_center + g_x_dots/16;
	s_y1 = s_y_center - s_bar_height/2;
	s_y2 = s_y_center + s_bar_height/2;

	s_y = 0;
	if (s_image_map)
	{
		g_out_line = out_line_stereo;
		while (s_y < g_y_dots)
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
		g_.DAC().FindSpecialColors();
		s_average /= 2*s_average_count;
		int ct = 0;
		int *colour = (int *) _alloca(sizeof(int)*g_x_dots);
		int barwidth = 1 + g_x_dots/200;
		for (int i = s_x_center; i < s_x_center + barwidth; i++)
		{
			for (int j = s_y_center; j < s_y_center + s_bar_height; j++)
			{
				colour[ct++] = get_color(i + int(s_average), j);
				colour[ct++] = get_color(i - int(s_average), j);
			}
		}
		bool bars = (s_stereogram_calibrate != CALIBRATE_NONE);
		toggle_bars(bars, barwidth, colour);
		int done = 0;
		while (done == 0)
		{
			driver_wait_key_pressed(0);
			int kbdchar = driver_get_key();
			switch (kbdchar)
			{
			case IDK_ENTER:   /* toggle bars */
			case IDK_SPACE:
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
				if (kbdchar == IDK_ESC)   /* if ESC avoid returning to menu */
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
	g_.DAC() = s_save_dac;
	load_dac();
	return ret;
}

int get_random_dot_stereogram_parameters()
{
	const char *stereobars[] =
	{
		"none", "middle", "top"
	};
	static bool reuse = false;
	ScreenStacker stacker;
	while (true)
	{
		UIChoices dialog(FIHELP_RANDOM_DOT_STEREOGRAM, "Random Dot Stereogram Parameters", 0);
		dialog.push("Depth Effect (negative reverses front and back)", s_auto_stereo_depth);
		dialog.push("Image width in inches", g_auto_stereo_width);
		dialog.push("Use grayscale value for depth? (if \"no\" uses color number)", g_grayscale_depth);
		dialog.push("Calibration bars", stereobars, NUM_OF(stereobars), s_stereogram_calibrate);
		dialog.push("Use image map? (if \"no\" uses random dots)", s_image_map);

		std::ostringstream buffer;
		if (g_stereo_map_name.length() != 0 && s_image_map)
		{
			dialog.push("  If yes, use current image map name? (see below)", reuse);

			std::string basename = fs::basename(fs::path(g_stereo_map_name));
			const int LINE_LENGTH = 60;
			for (unsigned p = 0; p < (LINE_LENGTH - basename.length())/2 + 1; p++)
			{
				buffer << ' ';
			}
			buffer << '[' << basename << ']';
			dialog.push(buffer.str().c_str());
		}
		else
		{
			g_stereo_map_name = "";
		}
		if (dialog.prompt() < 0)
		{
			return -1;
		}
		else
		{
			int k = 0;
			s_auto_stereo_depth = dialog.values(k++).uval.ival;
			g_auto_stereo_width = dialog.values(k++).uval.dval;
			g_grayscale_depth = (dialog.values(k++).uval.ch.val != 0);
			s_stereogram_calibrate = StereogramCalibrateType(dialog.values(k++).uval.ch.val);
			s_image_map = (dialog.values(k++).uval.ch.val != 0);
			reuse = (g_stereo_map_name.length() > 0)
				&& s_image_map
				&& (dialog.values(k++).uval.ch.val != 0);
			if (s_image_map && !reuse
				&& get_a_filename("Select an Imagemap File", std::string(g_masks[1]), g_stereo_map_name))
			{
				continue;
			}
		}
		break;
	}
	return 0;
}
