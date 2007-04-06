
/* Routine to decode Targa 16 bit RGB file
	*/

/* 16 bit .tga files were generated for continuous potential "potfile"s
	from version 9.? thru version 14.  Replaced by double row gif type
	file (.pot) in version 15.  Delete this code after a few more revs.
*/


/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "targa_lc.h"
#include "drivers.h"

static FILE *fptarga = NULL;            /* FILE pointer           */

/* Main entry decoder */
int
tgaview()
{
	int i;
	int cs;
	unsigned int width;
	struct fractal_info info;

	fptarga = t16_open(readname, (int *)&width, (int *)&height, &cs, (U8 *)&info);
	if (fptarga == NULL)
	{
		return -1;
	}

	g_row_count = 0;
	for (i = 0; i < (int)height; ++i)
	{
		t16_getline(fptarga, width, (U16 *)boxx);
		if ((*outln)((void *)boxx, width))
		{
			fclose(fptarga);
			fptarga = NULL;
			return -1;
		}
		if (driver_key_pressed())
		{
			fclose(fptarga);
			fptarga = NULL;
			return -1;
		}
	}
	fclose(fptarga);
	fptarga = NULL;
	return 0;
}

/* Outline function for 16 bit data with 8 bit fudge */
int
outlin16(BYTE *buffer, int linelen)
{
	int i;
	U16 *buf;
	buf = (U16 *)buffer;
	for (i = 0; i < linelen; i++)
	{
		g_put_color(i, g_row_count, buf[i] >> 8);
	}
	g_row_count++;
	return 0;
}
