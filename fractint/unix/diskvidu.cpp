/*
   "Disk-Video" for Unix routines.

   All the expanded memory caching stuff has been removed for the Unix
   version.  We just keep the data in memory and write it out to a file
   when we're done.  (Let virtual memory look after the details.)

*/

#include <stdio.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"

#define BOXROW	 6
#define BOXCOL	 11
#define BOXWIDTH 57
#define BOXDEPTH 12

#define TIMETODISPLAY 10000

int g_disk_16bit = 0;	   /* storing 16 bit values for continuous potential */

static int timetodisplay;
static FILE *fp = NULL;
int g_disk_targa;

static int headerlength;
static unsigned int rowsize = 0;   /* doubles as a disk video not ok flag */
static unsigned int colsize;	   /* sydots, *2 when pot16bit */

static BYTE *dataPtr = NULL;

int disk_start();
int disk_start_potential();
void disk_end();
int disk_start_targa(FILE *, int);
void disk_read_targa(unsigned int, unsigned int, BYTE *, BYTE *, BYTE *);
void disk_write_targa(unsigned int, unsigned int, BYTE, BYTE, BYTE);
void disk_video_status(int, char *);
void setattr(int row, int col, int attr, int count);
void movecursor(int row, int col);

int made_dsktemp = 0;

int disk_start()
{
	headerlength = g_disk_targa = 0;
	return disk_start_common(g_screen_width, g_screen_height, g_colors);
}

int disk_start_potential()
{
	int i;
	if (g_dot_mode == 11) /* ditch the original disk file */
	{
		disk_end();
	}
	else
	{
		show_temp_message("clearing 16bit pot work area");
	}
	headerlength = g_disk_targa = 0;
	i = disk_start_common(g_screen_width, g_screen_height << 1, g_colors);
	clear_temp_message();
	g_disk_16bit = 1;
	return i;
}

int disk_start_targa(FILE *targafp, int overhead)
{
	int i;
	/* ditch the original disk file, make just the targa */
	if (g_dot_mode == 11)
	{
		disk_end();      /* close the 'screen' */
		set_null_video(); /* set readdot and writedot routines to do nothing */
	}
	headerlength = overhead;
	fp = targafp;
	g_disk_targa = 1;
	i = disk_start_common(g_screen_width*3, g_screen_height, g_colors);
	return (i);
}

int _fastcall disk_start_common(long newrowsize, long newcolsize, int colors)
{
	int i;
	long memorysize;

	if (g_disk_flag)
	{
		disk_end();
	}
	if (g_dot_mode == 11) /* otherwise, real screen also in use, don't hit it */
	{
		char buf[20];
		help_title();
		setattr(1, 0, C_DVID_BKGRD, 24*80);	/* init rest to background */
		for (i = 0; i < BOXDEPTH; ++i)
		{
			setattr(BOXROW+i, BOXCOL, C_DVID_LO, BOXWIDTH);  /* init box */
		}
		putstring(BOXROW+2, BOXCOL+4, C_DVID_HI, "'Disk-Video' mode");
		putstring(BOXROW+4, BOXCOL+4, C_DVID_LO, "Screen resolution: ");
		sprintf(buf, "%d x %d", g_screen_width, g_screen_height);
		putstring(-1, -1, C_DVID_LO, buf);
		if (g_disk_targa)
		{
			putstring(-1, -1, C_DVID_LO, "  24 bit Targa");
		}
		else
		{
			putstring(-1, -1, C_DVID_LO, "  Colors: ");
			sprintf(buf, "%d", colors);
			putstring(-1, -1, C_DVID_LO, buf);
		}
		putstring(BOXROW+8, BOXCOL+4, C_DVID_LO, "Status:");
		disk_video_status(0, "clearing the 'screen'");
	}
	timetodisplay = TIMETODISPLAY;  /* time-to-display-status counter */

	memorysize = (long) (newcolsize)*newrowsize;
	g_disk_flag = 1;
	rowsize = newrowsize;
	colsize = newcolsize;

	if (dataPtr != NULL)
	{
	   free(dataPtr);
	}
	dataPtr = (BYTE *) malloc(memorysize);

	bzero(dataPtr, memorysize);

	if (g_dot_mode == 11)
	{
		disk_video_status(0, "");
	}
	return(0);
}

void disk_end()
{
	g_disk_flag = rowsize = g_disk_16bit = 0;
	fp	       = NULL;
}

int disk_read(int col, int row)
{
	char buf[41];
	if (--timetodisplay < 0)  /* time to display status? */
	{
		if (g_dot_mode == 11)
		{
			sprintf(buf, " reading line %4d",
				(row >= g_screen_height) ? row-g_screen_height : row);
				/* adjust when potfile */
			disk_video_status(0, buf);
		}
		timetodisplay = TIMETODISPLAY;
	}
	if (row>=colsize || col>=rowsize)
	{
		return 0;
	}
	return dataPtr[row*rowsize+col];
}

int disk_from_memory(long offset, int size, void *dest)
{
	memcpy(dest, (void *) (dataPtr+offset), size);
	return 1;
}

void disk_read_targa(unsigned int col, unsigned int row,
		    BYTE *red, BYTE *green, BYTE *blue)
{
	col *= 3;
	*blue  = disk_read(col, row);
	*green = disk_read(++col, row);
	*red   = disk_read(col+1, row);
}

void disk_write(int col, int row, int color)
{
	char buf[41];
	if (--timetodisplay < 0)  /* time to display status? */
	{
		if (g_dot_mode == 11)
		{
			sprintf(buf, " writing line %4d",
				(row >= g_screen_height) ? row-g_screen_height : row);
				/* adjust when potfile */
			disk_video_status(0, buf);
		}
		timetodisplay = TIMETODISPLAY;
	}
	if (row>=colsize || col>=rowsize)
	{
		return;
	}
	dataPtr[row*rowsize+col] = color;
}

int disk_to_memory(long offset, int size, void *src)
{
	memcpy((void *) (dataPtr+offset), src, size);
	return 1;
}

void disk_write_targa(unsigned int col, unsigned int row,
		    BYTE red, BYTE green, BYTE blue)
{
	disk_write(col*=3, row, blue);
	disk_write(++col, row, green);
	disk_write(col+1, row, red);
}

void disk_video_status(int line, char *msg)
{
	char buf[41];
	int attrib;
	memset(buf, ' ', 40);
	memcpy(buf, msg, strlen(msg));
	buf[40] = 0;
	attrib = C_DVID_HI;
	if (line >= 100)
	{
		line -= 100;
		attrib = C_STOP_ERR;
	}
	putstring(BOXROW+8+line, BOXCOL+12, attrib, buf);
	movecursor(25, 80);
}
