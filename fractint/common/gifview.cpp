/*
 *
 * This GIF decoder is designed for use with the FRACTINT program.
 * This decoder code lacks full generality in the following respects:
 * supports non-interlaced GIF files only, and calmly ignores any
 * local color maps and non-Fractint-specific extension blocks.
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 *                                                                                      Tim Wegner
 */

#include <string.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"
#include "busy.h"
#include "SoundState.h"

#define MAXCOLORS       256

unsigned int g_height;
unsigned g_num_colors;
int g_bad_code_count = 0;         /* needed by decoder module */

static FILE *fpin = NULL;       /* FILE pointer           */
static int colcount; /* keeps track of current column for wide images */
static unsigned int gifview_image_top;    /* (for migs) */
static unsigned int gifview_image_left;   /* (for migs) */
static unsigned int gifview_image_twidth; /* (for migs) */

static void close_file();
static int out_line_dither(BYTE *, int);
static int out_line_migs(BYTE *, int);
static int out_line_too_wide(BYTE *, int);

int get_byte()
{
	return getc(fpin); /* EOF is -1, as desired */
}

int get_bytes(BYTE *where, int how_many)
{
	return (int) fread((char *)where, 1, how_many, fpin); /* EOF is -1, as desired */
}

/*
 * DECODERLINEWIDTH is the width of the pixel buffer used by the decoder. A
 * larger buffer gives better performance. However, this buffer does not
 * have to be a whole line width, although historically in Fractint it has
 * been: images were decoded line by line and a whole line written to the
 * screen at once. The requirement to have a whole line buffered at once
 * has now been relaxed in order to support larger images. The one exception
 * to this is in the case where the image is being decoded to a smaller size.
 * The g_skip_x_dots and g_skip_y_dots logic assumes that the buffer holds one line.
 */

BYTE g_decoder_line[MAX_PIXELS + 1]; /* write-line routines use this */
#define DECODERLINE_WIDTH MAX_PIXELS

static char *ditherbuf = NULL;

/* Main entry decoder */
int gifview()
{
	BYTE buffer[16];
	unsigned top, left, width, finished;
	char temp1[FILE_MAX_DIR];
	BYTE byte_buf[257]; /* for decoder */
	int status;
	int i, j, k, planes;

	/* using stack for decoder byte buf rather than static mem */
	set_byte_buff(byte_buf);

	status = 0;

	/* initialize the col and row count for write-lines */
	colcount = g_row_count = 0;

	/* Open the file */
	strcpy(temp1, (g_out_line == out_line_stereo) ? g_stereo_map_name : g_read_name);
	if (has_extension(temp1) == NULL)
	{
		strcat(temp1, DEFAULTFRACTALTYPE);
		fpin = fopen(temp1, "rb");
		if (fpin != NULL)
		{
			fclose(fpin);
		}
		else
		{
			strcpy(temp1, (g_out_line == out_line_stereo) ? g_stereo_map_name : g_read_name);
			strcat(temp1, ALTERNATEFRACTALTYPE);
		}
	}
	fpin = fopen(temp1, "rb");
	if (fpin == NULL)
	{
		return -1;
		}

	/* Get the screen description */
	for (i = 0; i < 13; i++)
	{
		int tmp = get_byte();

		buffer[i] = (BYTE) tmp;
		if (tmp < 0)
		{
			close_file();
			return -1;
		}
	}

	if (strncmp((char *)buffer, "GIF87a", 3) ||             /* use updated GIF specs */
		buffer[3] < '0' || buffer[3] > '9' ||
		buffer[4] < '0' || buffer[4] > '9' ||
		buffer[5] < 'A' || buffer[5] > 'z')
	{
		close_file();
		return -1;
	}

	width  = buffer[6] | (buffer[7] << 8);
	g_height = buffer[8] | (buffer[9] << 8);
	planes = (buffer[10] & 0x0F) + 1;
	gifview_image_twidth = width;

	if ((buffer[10] & 0x80) == 0)    /* color map (better be!) */
	{
		close_file();
		return -1;
	}
	g_num_colors = 1 << planes;

	if (g_dither_flag && g_num_colors > 2 && g_colors == 2 && g_out_line == out_line)
	{
			g_out_line = out_line_dither;
	}

	for (i = 0; i < (int)g_num_colors; i++)
	{
		for (j = 0; j < 3; j++)
		{
			k = get_byte();
			if (k < 0)
			{
				close_file();
				return -1;
			}
			if ((!g_display_3d || (g_glasses_type != STEREO_ALTERNATE && g_glasses_type != STEREO_SUPERIMPOSE))
				&& !g_dont_read_color)
			{
				g_dac_box[i][j] = (BYTE)(k >> 2); /* TODO: don't right shift color table by 2 */
			}
		}
	}
	g_color_state = COLORSTATE_UNKNOWN; /* g_colors aren't default and not a known .map file */

	/* don't read if glasses */
	if (g_display_3d && g_map_set && g_glasses_type != STEREO_ALTERNATE && g_glasses_type != STEREO_SUPERIMPOSE)
	{
		validate_luts(g_map_name);  /* read the palette file */
		spindac(0, 1); /* load it, but don't spin */
	}
	if (g_dac_box[0][0] != 255)
	{
		spindac(0, 1);       /* update the DAC */
	}
	if (driver_diskp()) /* disk-video */
	{
		char fname[FILE_MAX_FNAME];
		char ext[FILE_MAX_EXT];
		char tmpname[15];
		char msg[40];
		split_path(temp1, NULL, NULL, fname, ext);
		make_path(tmpname, NULL, NULL, fname, ext);
		sprintf(msg, "restoring %s", tmpname);
		disk_video_status(1, msg);
	}
	g_dont_read_color = FALSE;

	/* Now display one or more GIF objects */
	finished = 0;
	while (!finished)
	{
		switch (get_byte())
		{
		case ';':
			/* End of the GIF dataset */

			finished = 1;
			status = 0;
			break;

		case '!':                               /* GIF Extension Block */
			get_byte();                     /* read (and ignore) the ID */
			while ((i = get_byte()) > 0)    /* get the data length */
			{
				for (j = 0; j < i; j++)
				{
					get_byte();     /* flush the data */
				}
			}
			break;
		case ',':
			/*
			* Start of an image object. Read the image description.
			*/
			for (i = 0; i < 9; i++)
			{
				int tmp = get_byte();

				buffer[i] = (BYTE) tmp;
				if (tmp < 0)
				{
					status = -1;
					break;
				}
			}
			if (status < 0)
			{
				finished = 1;
				break;
			}

			left   = buffer[0] | (buffer[1] << 8);
			top    = buffer[2] | (buffer[3] << 8);
			width  = buffer[4] | (buffer[5] << 8);
			g_height = buffer[6] | (buffer[7] << 8);

			/* adjustments for handling MIGs */
			gifview_image_top  = top;
			if (g_skip_x_dots > 0)
			{
				gifview_image_top /= (g_skip_y_dots + 1);
			}
			gifview_image_left = left;
			if (g_skip_y_dots > 0)
			{
				gifview_image_left /= (g_skip_x_dots + 1);
			}
			if (g_out_line == out_line)
			{
				/* what about continuous potential???? */
				if (width != gifview_image_twidth || top != 0)
				{   /* we're using normal decoding and we have a MIG */
					g_out_line = out_line_migs;
				}
				else if (width > DECODERLINE_WIDTH && g_skip_x_dots == 0)
				{
					g_out_line = out_line_too_wide;
				}
			}

			if (g_potential_16bit)
			{
				width >>= 1;
			}

			/* Skip local color palette */
			if ((buffer[8] & 0x80) == 0x80)  /* local map? */
			{
				int g_num_colors;    /* make this local */
				planes = (buffer[8] & 0x0F) + 1;
				g_num_colors = 1 << planes;
				/* skip local map */
				for (i = 0; i < g_num_colors; i++)
				{
					for (j = 0; j < 3; j++)
					{
						k = get_byte();
						if (k < 0)
						{
							close_file();
							return -1;
						}
					}
				}
			}

			/* initialize the row count for write-lines */
			g_row_count = 0;

			if (g_calculation_status == CALCSTAT_IN_PROGRESS) /* should never be so, but make sure */
			{
				g_calculation_status = CALCSTAT_PARAMS_CHANGED;
			}
			/*
			* Call decoder(width) via timer.
			* Width is limited to DECODERLINE_WIDTH.
			*/
			if (g_skip_x_dots == 0)
			{
				width = min(width, DECODERLINE_WIDTH);
			}
			{
				BusyMarker marker;
				status = timer(TIMER_DECODER, NULL, width);
			}
			if (g_calculation_status == CALCSTAT_IN_PROGRESS) /* e.g., set by line3d */
			{
				g_calculation_time = g_timer_interval; /* note how long it took */
				if (driver_key_pressed() != 0)
				{
					g_calculation_status = CALCSTAT_NON_RESUMABLE; /* interrupted, not resumable */
					finished = 1;
				}
				else
				{
					g_calculation_status = CALCSTAT_COMPLETED; /* complete */
				}
			}
			/* Hey! the decoder doesn't read the last (0-length) g_block!! */
			if (get_byte() != 0)
			{
				status = -1;
				finished = 1;
			}
			break;
		default:
			status = -1;
			finished = 1;
			break;
		}
	}
	close_file();
	if (driver_diskp())  /* disk-video */
	{
		disk_video_status(0, "Restore completed");
		disk_video_status(1, "");
	}

	if (ditherbuf != NULL)  /* we're done, free dither memory */
	{
		free(ditherbuf);
		ditherbuf = NULL;
	}

	return status;
}

static void close_file()
{
	fclose(fpin);
	fpin = NULL;
}

/* routine for MIGS that generates partial output lines */

static int out_line_migs(BYTE *pixels, int linelen)
{
	int row, startcol, stopcol;

	row = gifview_image_top + g_row_count;
	startcol = gifview_image_left;
	stopcol = startcol + linelen-1;
	put_line(row, startcol, stopcol, pixels);
	g_row_count++;

	return 0;
}

static int out_line_dither(BYTE *pixels, int linelen)
{
	int i, nexterr, brt, err;
	if (ditherbuf == NULL)
	{
		ditherbuf = (char *) malloc(linelen + 1);
	}
	memset(ditherbuf, 0, linelen + 1);

	nexterr = (rand()&0x1f)-16;
	for (i = 0; i < linelen; i++)
	{
		/* TODO: does not work when COLOR_CHANNEL_MAX != 63 */
		brt = (g_dac_box[pixels[i]][0]*5 +
			   g_dac_box[pixels[i]][1]*9 +
			   g_dac_box[pixels[i]][2]*2) >> 4; /* brightness from 0 to COLOR_CHANNEL_MAX */
		brt += nexterr;
		if (brt > (COLOR_CHANNEL_MAX + 1)/2)
		{
			pixels[i] = 1;
			err = brt - COLOR_CHANNEL_MAX;
		}
		else
		{
			pixels[i] = 0;
			err = brt;
		}
		nexterr = ditherbuf[i + 1] + err/3;
		ditherbuf[i] = (char)(err/3);
		ditherbuf[i + 1] = (char)(err/3);
	}
	return out_line(pixels, linelen);
}

/* routine for images wider than the row buffer */

static int out_line_too_wide(BYTE *pixels, int linelen)
{
	/* int twidth = gifview_image_twidth; */
	int twidth = g_x_dots;
	int extra;
	while (linelen > 0)
	{
		extra = colcount + linelen-twidth;
		if (extra > 0) /* line wraps */
		{
			put_line(g_row_count, colcount, twidth-1, pixels);
			pixels += twidth-colcount;
			linelen -= twidth-colcount;
			colcount = twidth;
		}
		else
		{
			put_line(g_row_count, colcount, colcount + linelen-1, pixels);
			colcount += linelen;
			linelen = 0;
		}
		if (colcount >= twidth)
		{
			colcount = 0;
			g_row_count++;
		}
	}
	return 0;
}

static int put_sound_line(int row, int colstart, int colstop, BYTE *pixels)
{
	for (int col = colstart; col <= colstop; col++)
	{
		g_put_color(col, row, *pixels);
		if (g_orbit_delay > 0)
		{
			sleep_ms(g_orbit_delay);
		}
		g_sound_state.tone((int)((int)(*pixels++)*3000/g_colors + g_sound_state.base_hertz()));
		if (driver_key_pressed())
		{
			driver_mute();
			return -1;
		}
	}
	return 0;
}

int sound_line(BYTE *pixels, int linelen)
{
	/* int twidth = gifview_image_twidth; */
	int twidth = g_x_dots;
	int extra;
	int ret = 0;
	while (linelen > 0)
	{
		extra = colcount + linelen-twidth;
		if (extra > 0) /* line wraps */
		{
			if (put_sound_line(g_row_count, colcount, twidth-1, pixels))
			{
				break;
			}
			pixels += twidth-colcount;
			linelen -= twidth-colcount;
			colcount = twidth;
		}
		else
		{
			if (put_sound_line(g_row_count, colcount, colcount + linelen-1, pixels))
			{
				break;
			}
			colcount += linelen;
			linelen = 0;
		}
		if (colcount >= twidth)
		{
			colcount = 0;
			g_row_count++;
		}
	}
	driver_mute();
	if (driver_key_pressed())
	{
		ret = -1;
	}
	return ret;
}

int potential_line(BYTE *pixels, int linelen)
{
	int row, col, saverowcount;
	if (g_row_count == 0)
	{
		if (disk_start_potential() < 0)
		{
			return -1;
		}
	}
	saverowcount = g_row_count;
	row = (g_row_count >>= 1);
	if ((saverowcount & 1) != 0) /* odd line */
	{
		row += g_y_dots;
	}
	else if (!driver_diskp()) /* even line - display the line too */
	{
		out_line(pixels, linelen);
	}
	for (col = 0; col < g_x_dots; ++col)
	{
		disk_write(col + g_sx_offset, row + g_sy_offset, *(pixels + col));
	}
	g_row_count = saverowcount + 1;
	return 0;
}
