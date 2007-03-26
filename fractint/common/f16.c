/**************************************
**
** F16.C : Code to read 16-bit fractal data sets.  Uses
** strictly Targa16 type 10 files (run-length encoded 16-bit RGB).
*/

/* Lee Daniel Crocker      CompuServe: 73407, 2030   <== Preferred
** 1380 Jewett Ave.               BIX: lcrocker
** Pittsburg, CA  94565        Usenet: ...!ames!pacbell!sactoh0!siva!lee
**
** This code is hereby placed in the public domain.  You are free to
** use, modify, usurp, laugh at, destroy, or otherwise abuse it in any
** way you see fit.
**
** "If you steal from one author it's plagiarism; if you steal from
** many it's research."  --Wilson Mizner
*/

/* 16 bit .tga files were generated for continuous potential "potfile"s
	from version 9.? thru version 14.  Replaced by double row gif type
	file (.pot) in version 15.  Delete this code after a few more revs.
	The part which wrote 16 bit .tga files has already been deleted.
*/

#include <string.h>
#include <ctype.h>
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "targa_lc.h"

#ifdef XFRACT
char rlebuf[258];    /* RLE-state variables */
#endif
static int state, count, bufp;

/**************************************
**
** Open previously saved Targa16 type 10 file filling in hs, vs, and
** csize with values in the header.  If *csize is not zero, the block
** pointed to by cp is filled with the comment block.  The caller
** _must_ allocate 256 bytes for this purpose before calling.
*/

FILE *t16_open(char *fname, int *hs, int *vs, int *csize, U8 *cp)
{
	char filename[64];
	U8 header[HEADERSIZE];
	FILE *fp;

	strcpy(filename, fname);
	if (has_ext(filename) == NULL) strcat(filename, ".TGA");
	fp = fopen(filename, READMODE);
	if (fp == NULL) return NULL;

	fread(header, HEADERSIZE, 1, fp);
	if ((header[O_FILETYPE] != T_RLERGB) || (header[O_ESIZE] != 16))
	{
		fclose(fp);
		return NULL;
	}
	GET16(header[O_HSIZE], *hs);
	GET16(header[O_VSIZE], *vs);
	*csize = header[O_COMMENTLEN];
	if (*csize != 0) fread(cp, *csize, 1, fp);

	state = count = bufp = 0;
	return fp;
}

int t16_getline(FILE *fp, int hs, U16 *data)
{
	int i;

	for (i = 0; i < hs; ++i)
	{
		if (state == 0)
		{
			bufp = 0;
			count = getc(fp);
			if (count > 127)
			{
				state = 1;
				count -= 127;
				fread(rlebuf, 2, 1, fp);
			}
			else
			{
				state = 2;
				++count;
				fread(rlebuf, 2, count, fp);
			}
		}
		GET16(rlebuf[bufp], data[i]);
		if (--count == 0) state = 0;
		if (state == 2) bufp += 2;
	}
	return 0;
}

