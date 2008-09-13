/*
 * encoder.c - GIF Encoder and associated routines
 */
#include <algorithm>
#include <climits>
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"
#include "strcpy.h"

#include "calcfrac.h"
#include "drivers.h"
#include "diskvid.h"
#include "encoder.h"
#include "EnsureExtension.h"
#include "Externals.h"
#include "filesystem.h"
#include "loadfile.h"
#include "miscres.h"
#include "prompts2.h"
#include "realdos.h"
#include "StopMessage.h"

#include "busy.h"
#include "EscapeTime.h"
#include "FiniteAttractor.h"
#include "Formula.h"
#include "ThreeDimensionalState.h"
#include "ViewWindow.h"

static int compress(int rowlimit);
static int shftwrite(const BYTE *color, int num_colors);
static int shftwrite(const ColormapTable &colormap, int num_colors);
static int extend_blk_len(int datalen);
static int put_extend_blk(int block_id, int block_len, char *block_data);
static int store_item_name(const char *name);
static int store_item_name(const std::string &name);
static void setup_save_info(fractal_info *save_info);

/*
								Save-To-Disk Routines (GIF)

The following routines perform the GIF encoding when the 's' key is pressed.

The compression logic in this file has been replaced by the classic
UNIX compress code. We have extensively modified the sources,
but have left the original credits where they
appear. Thanks to the original authors for making available these
classic and reliable sources. Of course, they are not responsible for
all the changes we have made to integrate their sources.

MEMORY ALLOCATION

There are two large arrays:

	long htab[HSIZE]              (5003*4 = 20012 bytes)
	unsigned short codetab[HSIZE] (5003*2 = 10006 bytes)

At the moment these arrays reuse extraseg and g_string_location, respectively.

*/

static int numsaves = 0;        // For adjusting 'save-to-disk' filenames
static FILE *s_outfile;
static int last_colorbar;
static bool s_save_16bit;
static int outcolor1s, outcolor2s;
static int startbits;

static BYTE paletteBW[] =
{                               // B&W palette
	0, 0, 0, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX,
};

#ifndef XFRACT
static BYTE paletteCGA[] =
{                               // 4-color (CGA) palette
	0, 0, 0,
	COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX,
	COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX,
	COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX
};
#endif

static BYTE paletteEGA[] =
{                               // 16-color (EGA/CGA) pal
	0, 0, 0,
	0, 0, 2*COLOR_CHANNEL_MAX/3,
	0, 2*COLOR_CHANNEL_MAX/3, 0,
	0, 2*COLOR_CHANNEL_MAX/3, 2*COLOR_CHANNEL_MAX/3,
	2*COLOR_CHANNEL_MAX/3, 0, 0,
	2*COLOR_CHANNEL_MAX/3, 0, 2*COLOR_CHANNEL_MAX/3,
	2*COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX/3, 0,
	2*COLOR_CHANNEL_MAX/3, 2*COLOR_CHANNEL_MAX/3, 2*COLOR_CHANNEL_MAX/3,
	COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX/3,
	COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX,
	COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX/3,
	COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX,
	COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX/3,
	COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX/3, COLOR_CHANNEL_MAX,
	COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX/3,
	COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX, COLOR_CHANNEL_MAX
};

static int gif_save_to_disk(char *filename)      // save-to-disk routine
{
	char tmpmsg[41];                 // before openfile in case of overrun
	char openfile[FILE_MAX_PATH];
	char openfiletype[10];
	char tmpfile[FILE_MAX_PATH];
	int i;
	int j;
	int interrupted;
	int outcolor1;
	int outcolor2;

restart:
	s_save_16bit = g_disk_16bit;
	if (g_gif87a_flag)             // not storing non-standard fractal info
	{
		s_save_16bit = false;
	}

	strcpy(openfile, filename);  // decode and open the filename
	strcpy(openfiletype, GIF_EXTENSION);    // determine the file extension
	if (s_save_16bit)
	{
		strcpy(openfiletype, ".pot");
	}

	ensure_extension(openfile, openfiletype);
	if (g_resave_mode != RESAVE_YES)
	{
		update_save_name(filename); // for next time
	}

	strcpy(tmpfile, openfile);
	bool new_file;
	if (!exists(openfile)) // file doesn't exist
	{
		new_file = true;
	}
	else
	{                                  // file already exists
		if (!g_fractal_overwrite)
		{
			if (g_resave_mode == RESAVE_NO)
			{
				goto restart;
			}
			if (!g_started_resaves)
			{                      // first save of a savetime set
				update_save_name(filename);
				goto restart;
			}
		}
		if (write_access(openfile))
		{
			stop_message(STOPMSG_NORMAL, std::string("Can't write ") + openfile);
			return -1;
		}
		new_file = false;
		i = int(strlen(tmpfile));
		while (--i >= 0 && tmpfile[i] != SLASHC)
		{
			tmpfile[i] = 0;
		}
		strcat(tmpfile, "id.tmp");
	}

	g_started_resaves = (g_resave_mode == RESAVE_YES);
	if (g_resave_mode == RESAVE_DONE)        // final save of savetime set?
	{
		g_resave_mode = RESAVE_NO;
	}

	s_outfile = fopen(tmpfile, "wb");
	if (s_outfile == 0)
	{
		stop_message(STOPMSG_NORMAL, std::string("Can't create ") + tmpfile);
		return -1;
	}

	if (driver_diskp())
	{                            // disk-video
		extract_filename(tmpmsg, openfile);
		disk_video_status(1, std::string("Saving ") + tmpmsg);
	}

	{
		BusyMarker marker;
		// invoke encoder() via timer
		interrupted = (g_debug_mode == DEBUGMODE_TIME_ENCODER)
			? timer_encoder() : encoder();
	}

	fclose(s_outfile);

	if (interrupted)
	{
		std::string buf = "Save of " + std::string(openfile) + " interrupted.\n"
			"Cancel to " + (new_file ?
				"delete the file,\n"
				"continue to keep the partial image."
				:
				"retain the original file,\n"
				"continue to replace original with new partial image.");
		interrupted = 1;
		if (stop_message(STOPMSG_CANCEL, buf) < 0)
		{
			interrupted = -1;
			unlink(tmpfile);
		}
	}

	if (!new_file && interrupted >= 0)
	{                            // replace the real file
		unlink(openfile);         // success assumed since we checked
		rename(tmpfile, openfile); // earlier with access
	}

	if (!driver_diskp())
	{                            // supress this on disk-video
		outcolor1 = outcolor1s;
		outcolor2 = outcolor2s;
		for (j = 0; j <= last_colorbar; j++)
		{
			if ((j & 4) == 0)
			{
				if (++outcolor1 >= g_colors)
				{
					outcolor1 = 0;
				}
				if (++outcolor2 >= g_colors)
				{
					outcolor2 = 0;
				}
			}
			for (i = 0; 250*i < g_x_dots; i++)
			{  // clear vert status bars
				g_plot_color_put_color(i, j, get_color(i, j) ^ outcolor1);
				g_plot_color_put_color(g_x_dots - 1 - i, j,
					get_color(g_x_dots - 1 - i, j) ^ outcolor2);
			}
		}
	}
	else                         // disk-video
	{
		disk_video_status(1, "");
	}

	if (interrupted)
	{
		text_temp_message(" *interrupted* save ");
		if (g_initialize_batch >= INITBATCH_NORMAL)
		{
			g_initialize_batch = INITBATCH_BAILOUT_ERROR;         // if batch mode, set error level
		}
		return -1;
	}
	if (g_timed_save == TIMEDSAVE_DONE)
	{
		driver_buzzer(BUZZER_COMPLETE);
		if (g_initialize_batch == INITBATCH_NONE)
		{
			extract_filename(tmpfile, openfile);
			text_temp_message((std::string(" File saved as ") + tmpfile + " ").c_str());
		}
	}
	if (g_save_time < 0)
	{
		goodbye();
	}
	return 0;
}

enum SaveFormatType
{
	SAVEFORMAT_GIF = 0,
	SAVEFORMAT_PNG,
	SAVEFORMAT_JPEG
};

// TODO: implement PNG case
int save_to_disk(char *filename)
{
	SaveFormatType format = SAVEFORMAT_GIF;

	switch (format)
	{
	case SAVEFORMAT_GIF:
		return gif_save_to_disk(filename);

	default:
		return -1;
	}
}

int save_to_disk(std::string &filename)
{
	char buffer[FILE_MAX_PATH];
	strcpy(buffer, filename.c_str());
	int result = save_to_disk(buffer);
	filename = buffer;
	return result;
}

static int write_byte(int value)
{
	return fputc(value & 0xFF, s_outfile);
}

static int write_short(int value)
{
	int count = write_byte(value);
	count += write_byte(value >> 8);
	return count;
}

int encoder()
{
	int i;
	int width;
	int rowlimit;
	int interrupted;
	BYTE bitsperpixel, x;
	fractal_info save_info;

	if (g_initialize_batch)               // flush any impending keystrokes
	{
		while (driver_key_pressed())
		{
			driver_get_key();
		}
	}

	setup_save_info(&save_info);

#ifndef XFRACT
	bitsperpixel = 0;            // calculate bits/pixel
	for (i = g_colors; i >= 2; i /= 2)
	{
		bitsperpixel++;
	}

	startbits = bitsperpixel + 1; // start coding with this many bits
#else
	bitsperpixel = 8;
	startbits = 9;
#endif

	if (g_gif87a_flag)
	{
		if (fwrite("GIF87a", 6, 1, s_outfile) != 1)
		{
			goto oops;             // old GIF Signature
		}
	}
	else if (fwrite("GIF89a", 6, 1, s_outfile) != 1)
	{
		goto oops;             // new GIF Signature
	}

	width = g_x_dots;
	rowlimit = g_y_dots;
	if (s_save_16bit)
	{
		/* g_potential_16bit info is stored as: file:    double width rows, right side
		* of row is low 8 bits diskvid: g_y_dots rows of colors followed by g_y_dots
		* rows of low 8 bits decoder: returns (row of color info then row of
		* low 8 bits)*g_y_dots */
		rowlimit <<= 1;
		width <<= 1;
	}
	if (write_short(width) != 1)
	{
		goto oops;                // screen descriptor
	}
	if (write_short(g_y_dots) != 1)
	{
		goto oops;
	}
	// color resolution == 6 bits worth
	x = BYTE(128 + ((6 - 1) << 4) + (bitsperpixel - 1));
	if (write_byte(x) != 1)
	{
		goto oops;
	}
	if (fputc(0, s_outfile) != 0)
	{
		goto oops;                // background color
	}
	i = 0;

	// TODO: pixel aspect ratio should be 1:1?
	if (g_viewWindow.Visible()                               // less than full screen?
		&& (g_viewWindow.Width() == 0 || g_viewWindow.Height() == 0))   // and we picked the dots?
	{
		i = int((double(g_screen_height)/double(g_screen_width))*64.0/g_screen_aspect_ratio - 14.5);
	}
	else   // must risk loss of precision if numbers low
	{
		i = int(((double(g_y_dots)/double(g_x_dots))/g_viewWindow.AspectRatio())*64 - 14.5);
	}
	if (i < 1)
	{
		i = 1;
	}
	if (i > 255)
	{
		i = 255;
	}
	if (g_gif87a_flag)
	{
		i = 0;                    // for some decoders which can't handle aspect
	}

	if (fputc(i, s_outfile) != i)
	{
		goto oops;                // pixel aspect ratio
	}

	// write out the 256-color palette
	if (g_.RealDAC())
	{                         // got a DAC - must be a VGA
		if (!shftwrite(g_.DAC(), g_colors))
		{
			goto oops;
		}
	}
	else
	{                         // uh oh - better fake it
		for (i = 0; i < 256; i += 16)
		{
			if (!shftwrite((BYTE *)paletteEGA, 16))
			{
				goto oops;
			}
		}
	}

	if (fwrite(",", 1, 1, s_outfile) != 1)
	{
		goto oops;                // Image Descriptor
	}
	i = 0;
	if (write_short(i) != 1)
	{
		goto oops;
	}
	if (write_short(i) != 1)
	{
		goto oops;
	}
	if (write_short(width) != 1)
	{
		goto oops;
	}
	if (write_short(g_y_dots) != 1)
	{
		goto oops;
	}
	if (write_byte(i) != 1)
	{
		goto oops;
	}

	bitsperpixel = BYTE(startbits - 1);

	if (write_byte(bitsperpixel) != 1)
	{
		goto oops;
	}

	interrupted = compress(rowlimit);

	if (ferror(s_outfile))
	{
		goto oops;
	}

	if (fputc(0, s_outfile) != 0)
	{
		goto oops;
	}

	if (!g_gif87a_flag)
	{                            // store non-standard fractal info
		// loadfile.c has notes about extension g_block structure
		if (interrupted)
		{
			save_info.calculation_status = CALCSTAT_PARAMS_CHANGED;     // partial save is not resumable
		}
		save_info.tot_extend_len = 0;
		if (g_resume_info != 0 && save_info.calculation_status == CALCSTAT_RESUMABLE)
		{
			// resume info g_block, 002
			save_info.tot_extend_len += extend_blk_len(g_resume_length);
			if (!put_extend_blk(2, g_resume_length, g_resume_info))
			{
				goto oops;
			}
		}
		/* save_info.fractal_type gets modified in setup_save_info() in float only
			version, so we need to use g_fractal_type.  JCO 06JAN01 */
		if (fractal_type_formula(g_fractal_type))
		{
			save_info.tot_extend_len += store_item_name(g_formula_state.get_formula());
		}
		if (g_fractal_type == FRACTYPE_L_SYSTEM)
		{
			save_info.tot_extend_len += store_item_name(g_l_system_name);
		}
		if (fractal_type_ifs(g_fractal_type))
		{
			save_info.tot_extend_len += store_item_name(g_ifs_name);
		}
		if ((g_display_3d == DISPLAY3D_GENERATED || g_display_3d == DISPLAY3D_NONE)
			&& g_ranges_length)
		{
			// ranges g_block, 004
			save_info.tot_extend_len += extend_blk_len(g_ranges_length*2);
#ifdef XFRACT
			fix_ranges(g_ranges, g_ranges_length, 0);
#endif
			if (!put_extend_blk(4, g_ranges_length*2, (char *) g_ranges))
			{
				goto oops;
			}
		}
		// Extended parameters g_block 005
		if (g_bf_math)
		{
			save_info.tot_extend_len += extend_blk_len(22*(g_bf_length + 2));
			/* note: this assumes variables allocated in order starting with
			 * bfxmin in init_bf2() in BIGNUM.C */
			if (!put_extend_blk(5, 22*(g_bf_length + 2), reinterpret_cast<char *>(g_escape_time_state.m_grid_bf.x_min().storage())))
			{
				goto oops;
			}
		}

		// Extended parameters g_block 006
		if (g_evolving_flags & EVOLVE_FIELD_MAP)
		{
			evolution_info esave_info;
			int i;
			evolution_info resume_e_info;
			if (g_evolve_info == 0 || g_externs.CalculationStatus() == CALCSTAT_COMPLETED)
			{
				esave_info.parameter_range_x = g_parameter_range_x;
				esave_info.parameter_range_y = g_parameter_range_y;
				esave_info.opx = g_parameter_offset_x;
				esave_info.opy = g_parameter_offset_y;
				esave_info.odpx = short(g_discrete_parameter_offset_x);
				esave_info.odpy = short(g_discrete_parameter_offset_y);
				esave_info.px = short(g_px);
				esave_info.py = short(g_py);
				esave_info.screen_x_offset = short(g_screen_x_offset);
				esave_info.screen_y_offset = short(g_screen_y_offset);
				esave_info.x_dots = short(g_x_dots);
				esave_info.y_dots = short(g_y_dots);
				esave_info.grid_size = short(g_grid_size);
				esave_info.evolving = short(g_evolving_flags);
				esave_info.this_generation_random_seed = (unsigned short)g_this_generation_random_seed;
				esave_info.fiddle_factor = g_fiddle_factor;
				esave_info.ecount = short(g_grid_size*g_grid_size); // flag for done
			}
			else  // we will need the resuming information
			{
				resume_e_info = *g_evolve_info;
				esave_info.parameter_range_x = resume_e_info.parameter_range_x;
				esave_info.parameter_range_y = resume_e_info.parameter_range_y;
				esave_info.opx = resume_e_info.opx;
				esave_info.opy = resume_e_info.opy;
				esave_info.odpx = short(resume_e_info.odpx);
				esave_info.odpy = short(resume_e_info.odpy);
				esave_info.px = short(resume_e_info.px);
				esave_info.py = short(resume_e_info.py);
				esave_info.screen_x_offset = short(resume_e_info.screen_x_offset);
				esave_info.screen_y_offset = short(resume_e_info.screen_y_offset);
				esave_info.x_dots = short(resume_e_info.x_dots);
				esave_info.y_dots = short(resume_e_info.y_dots);
				esave_info.grid_size = short(resume_e_info.grid_size);
				esave_info.evolving = short(resume_e_info.evolving);
				esave_info.this_generation_random_seed = (unsigned short)resume_e_info.this_generation_random_seed;
				esave_info.fiddle_factor = resume_e_info.fiddle_factor;
				esave_info.ecount = resume_e_info.ecount;
			}
			for (i = 0; i < NUM_GENES; i++)
			{
				esave_info.mutate[i] = short(g_genes[i].mutate);
			}

			for (i = 0; i < NUM_OF(esave_info.future); i++)
			{
				esave_info.future[i] = 0;
			}

			// some XFRACT logic for the doubles needed here
#ifdef XFRACT
			decode_evolver_info(&esave_info, 0);
#endif
			// evolution info g_block, 006
			save_info.tot_extend_len += extend_blk_len(sizeof(esave_info));
			if (!put_extend_blk(6, sizeof(esave_info), (char *) &esave_info))
			{
				goto oops;
			}
		}

		// Extended parameters g_block 007
		if (g_externs.StandardCalculationMode() == CALCMODE_ORBITS)
		{
			orbits_info osave_info;
			int i;
			osave_info.oxmin = g_orbit_x_min;
			osave_info.oxmax = g_orbit_x_max;
			osave_info.oymin = g_orbit_y_min;
			osave_info.oymax = g_orbit_y_max;
			osave_info.ox3rd = g_orbit_x_3rd;
			osave_info.oy3rd = g_orbit_y_3rd;
			osave_info.keep_scrn_coords = short(g_keep_screen_coords) ? 1 : 0;
			osave_info.drawmode = (char) g_orbit_draw_mode;
			for (i = 0; i < NUM_OF(osave_info.future); i++)
			{
				osave_info.future[i] = 0;
			}

			// some XFRACT logic for the doubles needed here
#ifdef XFRACT
			decode_orbits_info(&osave_info, 0);
#endif
			// orbits info g_block, 007
			save_info.tot_extend_len += extend_blk_len(sizeof(osave_info));
			if (!put_extend_blk(7, sizeof(osave_info), (char *) &osave_info))
			{
				goto oops;
			}
		}

		// main and last g_block, 001
		save_info.tot_extend_len += extend_blk_len(FRACTAL_INFO_SIZE);
#ifdef XFRACT
		decode_fractal_info(&save_info, 0);
#endif
		if (!put_extend_blk(1, FRACTAL_INFO_SIZE, (char *) &save_info))
		{
			goto oops;
		}
	}

	if (fwrite(";", 1, 1, s_outfile) != 1)
	{
		goto oops;                // GIF Terminator
	}

	return interrupted;

oops:
	fflush(s_outfile);
	stop_message(STOPMSG_NORMAL, "Error Writing to disk (Disk full?)");
	return 1;
}

static int shftwrite(const ColormapTable &colormap, int num_colors)
{
	for (int i = 0; i < num_colors; i++)
	{
		char value = char(colormap.Red(i));
		if (fputc(value, s_outfile) != value)
		{
			return 0;
		}
		value = char(colormap.Green(i));
		if (fputc(value, s_outfile) != value)
		{
			return 0;
		}
		value = char(colormap.Blue(i));
		if (fputc(value, s_outfile) != value)
		{
			return 0;
		}
	}
	return 1;
}

// TODO: should we be doing this?  We need to store full colors, not the VGA truncated business.
// shift IBM colors to GIF
static int shftwrite(const BYTE *color, int num_colors)
{
	BYTE thiscolor;
	int i;
	int j;
	for (i = 0; i < num_colors; i++)
	{
		for (j = 0; j < 3; j++)
		{
			thiscolor = color[3*i + j];
			thiscolor = BYTE(thiscolor << 2);
			thiscolor = BYTE(thiscolor + BYTE(thiscolor >> 6));
			if (fputc(thiscolor, s_outfile) != int(thiscolor))
			{
				return 0;
			}
		}
	}
	return 1;
}

static int extend_blk_len(int datalen)
{
	return datalen + (datalen + 254)/255 + 15;
	// data   +     1.per.block   + 14 for id + 1 for null at end
}

static int put_extend_blk(int block_id, int block_len, char *block_data)
{
	int i;
	int j;
	std::string header = (boost::format("!\377\013fractint%03u") % block_id).str();
	if (fwrite(header.c_str(), 14, 1, s_outfile) != 1)
	{
		return 0;
	}
	i = (block_len + 254)/255;
	while (--i >= 0)
	{
		block_len -= (j = std::min(block_len, 255));
		if (fputc(j, s_outfile) != j)
		{
			return 0;
		}
		while (--j >= 0)
		{
			fputc(*(block_data++), s_outfile);
		}
	}
	if (fputc(0, s_outfile) != 0)
	{
		return 0;
	}
	return 1;
}

static int store_item_name(const std::string &name)
{
	return store_item_name(name.c_str());
}

static int store_item_name(const char *nameptr)
{
	formula_info fsave_info;
	int i;
	for (i = 0; i < 40; i++)
	{
		fsave_info.form_name[i] = 0;      // initialize string
	}
	strcpy(fsave_info.form_name, nameptr);
	if (fractal_type_formula(g_fractal_type))
	{
		fsave_info.uses_p1 = short(g_formula_state.uses_p1());
		fsave_info.uses_p2 = short(g_formula_state.uses_p2());
		fsave_info.uses_p3 = short(g_formula_state.uses_p3());
		fsave_info.uses_is_mand = g_formula_state.uses_is_mand() ? 1 : 0;
		fsave_info.ismand = g_is_mandelbrot ? 1 : 0;
		fsave_info.uses_p4 = short(g_formula_state.uses_p4());
		fsave_info.uses_p5 = short(g_formula_state.uses_p5());
	}
	else
	{
		fsave_info.uses_p1 = 0;
		fsave_info.uses_p2 = 0;
		fsave_info.uses_p3 = 0;
		fsave_info.uses_is_mand = 0;
		fsave_info.ismand = 0;
		fsave_info.uses_p4 = 0;
		fsave_info.uses_p5 = 0;
	}
	for (i = 0; i < NUM_OF(fsave_info.future); i++)
	{
		fsave_info.future[i] = 0;
	}
	// formula/lsys/ifs info g_block, 003
	put_extend_blk(3, sizeof(fsave_info), (char *) &fsave_info);
	return extend_blk_len(sizeof(fsave_info));
}

static void setup_save_info(fractal_info *save_info)
{
	int i;
	if (!fractal_type_formula(g_fractal_type))
	{
		g_formula_state.set_max_fn(0);
	}
	// set save parameters in save structure
	strcpy(save_info->info_id, INFO_ID);
	save_info->version = FRACTAL_INFO_VERSION;

	save_info->iterationsold = (g_max_iteration <= SHRT_MAX) ? short(g_max_iteration) : short(SHRT_MAX);

	save_info->fractal_type = short(g_fractal_type);
	save_info->x_min = g_escape_time_state.m_grid_fp.x_min();
	save_info->x_max = g_escape_time_state.m_grid_fp.x_max();
	save_info->y_min = g_escape_time_state.m_grid_fp.y_min();
	save_info->y_max = g_escape_time_state.m_grid_fp.y_max();
	save_info->c_real = g_parameters[P1_REAL];
	save_info->c_imag = g_parameters[P1_IMAG];
	save_info->deprecated_video_mode_ax = 0;
	save_info->deprecated_video_mode_bx = 0;
	save_info->deprecated_video_mode_cx = 0;
	save_info->deprecated_video_mode_dx = 0;
	save_info->deprecated_dotmode = 0;
	save_info->x_dots = short(g_.VideoEntry().x_dots);
	save_info->y_dots = short(g_.VideoEntry().y_dots);
	save_info->colors = short(g_.VideoEntry().colors);
	save_info->parm3 = 0;        // pre version == 7 fields
	save_info->parm4 = 0;
	save_info->dparm3 = g_parameters[P2_REAL];
	save_info->dparm4 = g_parameters[P2_IMAG];
	save_info->dparm5 = g_parameters[P3_REAL];
	save_info->dparm6 = g_parameters[P3_IMAG];
	save_info->dparm7 = g_parameters[P4_REAL];
	save_info->dparm8 = g_parameters[P4_IMAG];
	save_info->dparm9 = g_parameters[P5_REAL];
	save_info->dparm10 = g_parameters[P5_IMAG];
	save_info->fill_color = short(g_fill_color);
	save_info->potential[0] = float(g_potential_parameter[0]);
	save_info->potential[1] = float(g_potential_parameter[1]);
	save_info->potential[2] = float(g_potential_parameter[2]);
	save_info->random_flag = short(g_use_fixed_random_seed) ? 1 : 0;
	save_info->random_seed = short(g_random_seed);
	save_info->inside = short(g_externs.Inside());
	save_info->logmapold = (g_log_palette_mode <= SHRT_MAX) ? short(g_log_palette_mode) : short(SHRT_MAX);
	save_info->invert[0] = float(g_inversion[0]);
	save_info->invert[1] = float(g_inversion[1]);
	save_info->invert[2] = float(g_inversion[2]);
	save_info->decomposition[0] = short(g_decomposition[0]);
	save_info->biomorph = short(g_externs.UserBiomorph());
	save_info->symmetry = short(g_force_symmetry);
	g_3d_state.get_raytrace_parameters(&save_info->init_3d[0]);
	save_info->previewfactor = short(g_3d_state.preview_factor());
	save_info->xtrans = short(g_3d_state.x_trans());
	save_info->ytrans = short(g_3d_state.y_trans());
	save_info->red_crop_left = short(g_3d_state.red().crop_left());
	save_info->red_crop_right = short(g_3d_state.red().crop_right());
	save_info->blue_crop_left = short(g_3d_state.blue().crop_left());
	save_info->blue_crop_right = short(g_3d_state.blue().crop_right());
	save_info->red_bright = short(g_3d_state.red().bright());
	save_info->blue_bright = short(g_3d_state.blue().bright());
	save_info->xadjust = short(g_3d_state.x_adjust());
	save_info->yadjust = short(g_3d_state.y_adjust());
	save_info->eyeseparation = short(g_3d_state.eye_separation());
	save_info->glassestype = short(g_3d_state.glasses_type());
	save_info->outside = short(g_externs.Outside());
	save_info->x_3rd = g_escape_time_state.m_grid_fp.x_3rd();
	save_info->y_3rd = g_escape_time_state.m_grid_fp.y_3rd();
	save_info->calculation_status = short(g_externs.CalculationStatus());
	save_info->stdcalcmode = char(
		(g_three_pass && g_externs.StandardCalculationMode() == CALCMODE_TRIPLE_PASS) ?
			127 : g_externs.StandardCalculationMode());
	save_info->distestold = (g_distance_test <= 32000) ? short(g_distance_test) : 32000;
	save_info->float_flag = g_float_flag ? 1 : 0;
	save_info->bailoutold = (g_externs.BailOut() >= 4 && g_externs.BailOut() <= 32000) ? short(g_externs.BailOut()) : 0;

	save_info->calculation_time = g_calculation_time;
	save_info->function_index[0] = BYTE(g_function_index[0]);
	save_info->function_index[1] = BYTE(g_function_index[1]);
	save_info->function_index[2] = BYTE(g_function_index[2]);
	save_info->function_index[3] = BYTE(g_function_index[3]);
	save_info->finattract = short(g_finite_attractor);
	save_info->initial_orbit_z[0] = g_initial_orbit_z.real();
	save_info->initial_orbit_z[1] = g_initial_orbit_z.imag();
	save_info->use_initial_orbit_z = char(g_externs.UseInitialOrbitZ());
	save_info->periodicity = short(g_periodicity_check);
	save_info->potential_16bit = short(g_disk_16bit) ? 1 : 0;
	save_info->aspect_ratio = g_viewWindow.AspectRatio();
	save_info->system = 1;

	save_info->release = check_back() ? short(std::min(g_save_release, g_release)) : short(g_release);

	save_info->flag3d = short(g_display_3d);
	save_info->ambient = short(g_3d_state.ambient());
	save_info->randomize = short(g_3d_state.randomize_colors());
	save_info->haze = short(g_3d_state.haze());
	save_info->transparent[0] = short(g_3d_state.transparent0());
	save_info->transparent[1] = short(g_3d_state.transparent1());
	save_info->rotate_lo = short(g_rotate_lo);
	save_info->rotate_hi = short(g_rotate_hi);
	save_info->distance_test_width = short(g_distance_test_width);
	save_info->mxmaxfp = g_m_x_max_fp;
	save_info->mxminfp = g_m_x_min_fp;
	save_info->mymaxfp = g_m_y_max_fp;
	save_info->myminfp = g_m_y_min_fp;
	save_info->zdots = short(g_z_dots);
	save_info->originfp = g_origin_fp;
	save_info->depthfp = g_depth_fp;
	save_info->heightfp = g_height_fp;
	save_info->widthfp = g_width_fp;
	save_info->screen_distance_fp = g_screen_distance_fp;
	save_info->eyesfp = g_eyes_fp;
	save_info->orbittype = short(g_new_orbit_type);
	save_info->juli3Dmode = short(g_juli_3d_mode);
	save_info->max_fn = char(g_formula_state.max_fn());
	save_info->inversejulia = short((g_major_method << 8) + g_minor_method);      // MVS
	save_info->bail_out = g_externs.BailOut();
	save_info->bailoutest = short(g_externs.BailOutTest());
	save_info->iterations = g_max_iteration;
	save_info->bflength = short(g_bn_length);
	save_info->bf_math = short(g_bf_math);
	save_info->old_demm_colors = short(g_old_demm_colors) ? 1 : 0;
	save_info->logmap = g_log_palette_mode;
	save_info->distance_test = g_distance_test;
	save_info->dinvert[0] = g_inversion[0];
	save_info->dinvert[1] = g_inversion[1];
	save_info->dinvert[2] = g_inversion[2];
	save_info->logcalc = short(g_log_dynamic_calculate);
	save_info->stop_pass = short(g_externs.StopPass());
	save_info->quick_calculate = g_quick_calculate ? 1 : 0;
	save_info->proximity = g_proximity;
	save_info->no_bof = !g_beauty_of_fractals ? 1 : 0;
	save_info->orbit_interval = g_orbit_interval;
	save_info->orbit_delay = short(g_orbit_delay);
	save_info->math_tolerance[0] = g_math_tolerance[0];
	save_info->math_tolerance[1] = g_math_tolerance[1];
	for (i = 0; i < NUM_OF(save_info->future); i++)
	{
		save_info->future[i] = 0;
	}

}

/***************************************************************************
 *
 *  GIFENCOD.C       - GIF Image compression routines
 *
 *  Lempel-Ziv compression based on 'compress'.  GIF modifications by
 *  David Rowley (mgardi@watdcsu.waterloo.edu).
 *
 ***************************************************************************/

enum
{
	BITSF = 12,
	HSIZE = 5003            // 80% occupancy
};

/*
 *
 * GIF Image compression - modified 'compress'
 *
 * Based on: compress.c - File compression ala IEEE Computer, June 1984.
 *
 * By Authors:  Spencer W. Thomas       (decvax!harpo!utah-cs!utah-gr!thomas)
 *              Jim McKie               (decvax!mcvax!jim)
 *              Steve Davies            (decvax!vax135!petsd!peora!srd)
 *              Ken Turkowski           (decvax!decwrl!turtlevax!ken)
 *              James A. Woods          (decvax!ihnp4!ames!jaw)
 *              Joe Orost               (decvax!vax135!petsd!joe)
 *
 */

// prototypes

static void output(int code);
static void char_out(int c);
static void flush_char();
static void cl_block();

static int n_bits;                        // number of bits/code
static int maxbits = BITSF;                // user settable max # bits/code
static int maxcode;                  // maximum code, given n_bits
static int maxmaxcode = 1 << BITSF; // should NEVER generate this code
inline int MAXCODE(int n_bits)
{
	return ((1 << (n_bits)) - 1);
}

#ifdef XFRACT
unsigned int g_string_location[10240];
BYTE g_block[4096];
#endif

static long htab[HSIZE];
static unsigned short *codetab = (unsigned short *)g_string_location;

/*
 * To save much memory, we overlay the table used by compress() with those
 * used by decompress().  The tab_prefix table is the same size and type
 * as the codetab.  The tab_suffix table needs 2**BITSF characters.  We
 * get this from the beginning of htab.  The output stack uses the rest
 * of htab, and contains characters.  There is plenty of room for any
 * possible stack (stack used to be 8000 characters).
 */

static int free_ent;                  // first unused entry

/*
 * g_block compression parameters -- after all codes are used up,
 * and compression rate changes, start over.
 */
static int clear_flg = 0;

/*
 * compress stdin to stdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code/next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do g_block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */

static int ClearCode;
static int EOFCode;
static int a_count; // Number of characters so far in this 'packet'
static unsigned long cur_accum = 0;
static int  cur_bits = 0;

/*
 * Define the storage for the packet accumulator
 */
static char *accum; // 256 bytes

static int compress(int rowlimit)
{
	int outcolor1;
	int outcolor2;
	long fcode;
	int i = 0;
	int ent;
	int disp;
	int hsize_reg;
	int hshift;
	int ydot;
	int xdot;
	int color;
	int rownum;
	int in_count = 0;
	int interrupted = 0;
	int tempkey;
	char accum_stack[256];
	accum = accum_stack;

	outcolor1 = 2;               // use these colors to show progress
	outcolor2 = 3;               // (this has nothing to do with GIF)

	if (((++numsaves) & 1) == 0)
	{                            // reverse the colors on alt saves
		i = outcolor1;
		outcolor1 = outcolor2;
		outcolor2 = i;
	}
	outcolor1s = outcolor1;
	outcolor2s = outcolor2;

	/*
	* Set up the necessary values
	*/
	cur_accum = 0;
	cur_bits = 0;
	clear_flg = 0;
	ydot = 0;
	ent = 0;
	maxcode = MAXCODE(n_bits = startbits);

	ClearCode = (1 << (startbits - 1));
	EOFCode = ClearCode + 1;
	free_ent = ClearCode + 2;

	a_count = 0;
	hshift = 0;
	for (fcode = long(HSIZE);  fcode < 65536L; fcode *= 2L)
	{
		hshift++;
	}
	hshift = 8 - hshift;                // set hash code range bound

	std::fill(htab, htab + unsigned(HSIZE)*sizeof(long), 0xff);
	hsize_reg = HSIZE;

	output(int(ClearCode));

	for (rownum = 0; rownum < g_y_dots; rownum++)
	{                            // scan through the dots
		for (ydot = rownum; ydot < rowlimit; ydot += g_y_dots)
		{
			for (xdot = 0; xdot < g_x_dots; xdot++)
			{
				color = (!s_save_16bit || ydot < g_y_dots)
					? get_color(xdot, ydot) : disk_read(xdot + g_screen_x_offset, ydot + g_screen_y_offset);
				if (in_count == 0)
				{
					in_count = 1;
					ent = color;
					continue;
				}
				fcode = long((long(color) << maxbits) + ent);
				i = ((int(color) << hshift) ^ ent);    // xor hashing

				if (htab[i] == fcode)
				{
					ent = codetab[i];
					continue;
				}
				else if (long(htab[i]) < 0)      // empty slot
				{
					goto nomatch;
				}
				disp = hsize_reg - i;           // secondary hash (after G. Knott)
				if (i == 0)
				{
					disp = 1;
				}
probe:
				i -= disp;
				if (i < 0)
				{
					i += hsize_reg;
				}

				if (htab[i] == fcode)
				{
					ent = codetab[i];
					continue;
				}
				if (long(htab[i]) > 0)
				{
					goto probe;
				}
nomatch:
				output (int(ent));
				ent = color;
				if (free_ent < maxmaxcode)
				{
					// code -> hashtable
					codetab[i] = (unsigned short)free_ent++;
					htab[i] = fcode;
				}
				else
				{
					cl_block();
				}
			} // end for xdot
			if (! driver_diskp()		// supress this on disk-video
				&& ydot == rownum)
			{
				if ((ydot & 4) == 0)
				{
					if (++outcolor1 >= g_colors)
					{
						outcolor1 = 0;
					}
					if (++outcolor2 >= g_colors)
					{
						outcolor2 = 0;
					}
				}
				for (i = 0; 250*i < g_x_dots; i++)
				{  // display vert status bars
					// (this is NOT GIF-related)
					g_plot_color_put_color(i, ydot, get_color(i, ydot) ^ outcolor1);
					g_plot_color_put_color(g_x_dots - 1 - i, ydot,
						get_color(g_x_dots - 1 - i, ydot) ^ outcolor2);
				}
				last_colorbar = ydot;
			} // end if !driver_diskp()
			tempkey = driver_key_pressed();
			if (tempkey && (tempkey != int('s')))  // keyboard hit - bail out
			{
				interrupted = 1;
				rownum = g_y_dots;
				break;
			}
			if (tempkey == int('s'))
			{
				driver_get_key();   // eat the keystroke
			}
		} // end for ydot
	} // end for rownum

	/*
	* Put out the final code.
	*/
	output(int(ent));
	output(int(EOFCode));
	return interrupted;
}

/*****************************************************************
 * TAG(output)
 *
 * Output the given code.
 * Inputs:
 *      code:   A n_bits-bit integer.  If == -1, then EOF.  This assumes
 *              that n_bits =< long(wordsize) - 1.
 * Outputs:
 *      Outputs code to the file.
 * Assumptions:
 *      Chars are 8 bits long.
 * Algorithm:
 *      Maintain a BITSF character long buffer (so that 8 codes will
 * fit in it exactly).  Use the VAX insv instruction to insert each
 * code in turn.  When the buffer fills up empty it and start over.
 */


static void output(int code)
{
	static unsigned long masks[] =
	{
		0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
		0x001F, 0x003F, 0x007F, 0x00FF,
		0x01FF, 0x03FF, 0x07FF, 0x0FFF,
		0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
	};

	cur_accum &= masks[ cur_bits ];

	if (cur_bits > 0)
	{
		cur_accum |= (long(code) << cur_bits);
	}
	else
	{
		cur_accum = code;
	}

	cur_bits += n_bits;

	while (cur_bits >= 8)
	{
		char_out((unsigned int)(cur_accum & 0xff));
		cur_accum >>= 8;
		cur_bits -= 8;
	}

	/*
	* If the next entry is going to be too big for the code size,
	* then increase it, if possible.
	*/
	if (free_ent > maxcode || clear_flg)
	{
		if (clear_flg)
		{
			n_bits = startbits;
			maxcode = MAXCODE(n_bits);
			clear_flg = 0;

		}
		else
		{
			n_bits++;
			maxcode = (n_bits == maxbits) ? maxmaxcode : MAXCODE(n_bits);
		}
	}

	if (code == EOFCode)
	{
		/*
		* At EOF, write the rest of the buffer.
		*/
		while (cur_bits > 0)
		{
			char_out((unsigned int)(cur_accum & 0xff));
			cur_accum >>= 8;
			cur_bits -= 8;
		}

		flush_char();

		fflush(s_outfile);
	}
}

/*
 * Clear out the hash table
 */
static void cl_block()             // table clear for g_block compress
{
	std::fill(htab, htab + unsigned(HSIZE)*sizeof(long), 0xff);
	free_ent = ClearCode + 2;
	clear_flg = 1;
	output(int(ClearCode));
}

/*
 * Add a character to the end of the current packet, and if it is 254
 * characters, flush the packet to disk.
 */
static void char_out(int c)
{
	accum[ a_count++ ] = (char)c;
	if (a_count >= 254)
	{
		flush_char();
	}
}

/*
 * Flush the packet to disk, and reset the accumulator
 */
static void flush_char()
{
	if (a_count > 0)
	{
		fputc(a_count, s_outfile);
		fwrite(accum, 1, a_count, s_outfile);
		a_count = 0;
	}
}
