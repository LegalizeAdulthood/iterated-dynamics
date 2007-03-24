/* DECODE.C - An LZW decoder for GIF
 * Copyright (C) 1987, by Steven A. Bennett
 *
 * Permission is given by the author to freely redistribute and include
 * this code in any program as long as this credit is given where due.
 *
 * In accordance with the above, I want to credit Steve Wilhite who wrote
 * the code which this is heavily inspired by...
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 * Release Notes: This file contains a decoder routine for GIF images
 * which is similar, structurally, to the original routine by Steve Wilhite.
 * It is, however, somewhat noticably faster in most cases.
 *
 == This routine was modified for use in FRACTINT in two ways.
 ==
 == 1) The original #includes were folded into the routine strictly to hold
 ==    down the number of files we were dealing with.
 ==
 == 2) The 'stack', 'suffix', 'prefix', and 'decoderline' arrays were
 ==    changed from static and 'malloc()'ed to external only so that
 ==    the assembler program could use the same array space for several
 ==    independent chunks of code.  Also, 'stack' was renamed to 'dstack'
 ==    for TASM compatibility.
 ==
 == 3) The 'out_line()' external function has been changed to reference
 ==    '*outln()' for flexibility (in particular, 3D transformations)
 ==
 == 4) A call to 'driver_key_pressed()' has been added after the 'outln()' calls
 ==    to check for the presenc of a key-press as a bail-out signal
 ==
 == (Bert Tyler and Timothy Wegner)
 */

/* Rev 01/02/91 - Revised by Mike Gelvin
 *                altered logic to allow newcode to input a line at a time
 *                altered logic to allow decoder to place characters
 *                directly into the output buffer if they fit
 */


/***** Application Includes *********************************************/
  /* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

/***** Application Function Prototypes **********************************/
static short get_next_code(void);

/* extern short out_line(pixels, linelen)
 *     UBYTE pixels[];
 *     short linelen;
 *
 *   - This function takes a full line of pixels (one byte per pixel) and
 * displays them (or does whatever your program wants with them...).  It
 * should return zero, or negative if an error or some other event occurs
 * which would require aborting the decode process...  Note that the length
 * passed will almost always be equal to the line length passed to the
 * decoder function, with the sole exception occurring when an ending code
 * occurs in an odd place in the GIF file...  In any case, linelen will be
 * equal to the number of pixels passed...
 */
int (*outln) (BYTE *, int) = out_line;

/***** Local Static Variables *******************************************/
/* Various error codes used by decoder
 * and my own routines...   It's okay
 * for you to define whatever you want,
 * as long as it's negative...  It will be
 * returned intact up the various subroutine
 * levels...
 */
#define OUT_OF_MEMORY -10
#define BAD_CODE_SIZE -20
#define READ_ERROR -1
#define WRITE_ERROR -2
#define OPEN_ERROR -3
#define CREATE_ERROR -4

#define MAX_CODES   4095

#define NOPE 0
#define YUP -1

static short curr_size;         /* The current code size */

/* The following static variables are used
 * for seperating out codes
 */
static short navail_bytes;      /* # bytes left in block */
static short nbits_left;        /* # bits left in current byte */
static BYTE *byte_buff;         /* Current block, reuse shared mem */
static BYTE *pbytes;            /* Pointer to next byte in block */

static short code_mask[13] =
{
	0,
	0x0001, 0x0003,
	0x0007, 0x000F,
	0x001F, 0x003F,
	0x007F, 0x00FF,
	0x01FF, 0x03FF,
	0x07FF, 0x0FFF
};

/***** External Variables ***********************************************/
/* extern short bad_code_count;
 *
 * This value is the only other global required by the using program, and
 * is incremented each time an out of range code is read by the decoder.
 * When this value is non-zero after a decode, your GIF file is probably
 * corrupt in some way...
 *
 * whups, here are more globals, added by PB:
 * extern short skipxdots;  0 to get every dot, 1 for every 2nd, 2 every 3rd, ...
 * extern short skipydots;
 *
 * All external declarations now in PROTOTYPE.H
 */


/*
I removed the LOCAL identifiers in the arrays below and replaced them
with 'extern's so as to declare (and re-use) the space elsewhere.
The arrays are actually declared in the assembler source.
                                                Bert Tyler
*/

#if 0
/* declarations moved to PROTOTYPE.H - these left for documentation */
BYTE dstack[MAX_CODES + 1];     /* Stack for storing pixels */
BYTE suffix[MAX_CODES + 1];     /* Suffix table */
unsigned short prefix[MAX_CODES + 1];   /* Prefix linked list */
BYTE decoderline[2];            /* decoded line goes here */
#endif

/* avoid using fixed near arrays by enabling next */
#if 0
BYTE dstack1[MAX_CODES + 1];     /* Stack for storing pixels */
#define dstack dstack1
#endif

#if 0 /* remove this when suffix no longer used in diskvid.c */
BYTE suffix1[MAX_CODES + 1];     /* Suffix table */
#define suffix suffix1
#endif

#if 0
unsigned short prefix1[MAX_CODES + 1];   /* Prefix linked list */
#define prefix prefix1
#endif

/* for the time being, use a pointer to a buffer in the gifview stack */
extern BYTE *decoderline1;
#define decoderline decoderline1

/* The reason we have these separated like this instead of using
 * a structure like the original Wilhite code did, is because this
 * stuff generally produces significantly faster code when compiled...
 * This code is full of similar speedups...  (For a good book on writing
 * C for speed or for space optimization, see Efficient C by Tom Plum,
 * published by Plum-Hall Associates...)
 */


/***** Program **********************************************************/
/* short decoder(linewidth)
 *    short linewidth;              * Pixels per line of image *
 *
 * - This function decodes an LZW image, according to the method used
 * in the GIF spec.  Every *linewidth* "characters" (ie. pixels) decoded
 * will generate a call to out_line(), which is a user specific function
 * to display a line of pixels.  The function gets its codes from
 * get_next_code() which is responsible for reading blocks of data and
 * seperating them into the proper size codes.  Finally, get_byte() is
 * the global routine to read the next byte from the GIF file.
 *
 * It is generally a good idea to have linewidth correspond to the actual
 * width of a line (as specified in the Image header) to make your own
 * code a bit simpler, but it isn't absolutely necessary.
 *
 * Returns: 0 if successful, else negative.  (See ERRS.H)
 *
 */

/* moved sizeofstring here for possible re-use elsewhere */
short sizeofstring[MAX_CODES + 1];  /* size of string list */

short decoder(short linewidth)
{
#if defined(XFRACT) || defined(_WIN32)
	U16 prefix[MAX_CODES + 1];     /* Prefix linked list */
#endif
	BYTE *sp;
	short code;
	short old_code;
	short ret;
	short c;
	short size;
	short i;
	short j;
	short fastloop;
	short bufcnt;                /* how many empty spaces left in buffer */
	short xskip;
	short slot;                  /* Last read code */
	short newcodes;              /* First available code */
	BYTE *bufptr;
	short yskip;
	short top_slot;              /* Highest code for current size */
	short clear;                 /* Value for a clear code */
	short ending;                /* Value for a ending code */
	BYTE out_value;

	/* Initialize for decoding a new image... */

	if ((size = (short) get_byte()) < 0)
		return size;
	if (size < 2 || 9 < size)
		return BAD_CODE_SIZE;

	curr_size = (short) (size + 1);
	top_slot = (short) (1 << curr_size);
	clear = (short) (1 << size);
	ending = (short) (clear + 1);
	slot = newcodes = (short) (ending + 1);
	navail_bytes = nbits_left = sizeofstring[slot] = xskip = yskip
		= old_code = 0;
	out_value = 0;
	for (i = 0; i < slot; i++)
	{
		sizeofstring[i] = 0;
	}

	/* Initialize in case they forgot to put in a clear code. (This shouldn't
	* happen, but we'll try and decode it anyway...) */

	/* Set up the stack pointer and decode buffer pointer */
	sp = dstack;
	bufptr = decoderline;
	bufcnt = linewidth;

	/* This is the main loop.  For each code we get we pass through the linked
	* list of prefix codes, pushing the corresponding "character" for each
	* code onto the stack.  When the list reaches a single "character" we
	* push that on the stack too, and then start unstacking each character
	* for output in the correct order.  Special handling is included for the
	* clear code, and the whole thing ends when we get an ending code. */
	while ((c = get_next_code()) != ending)
	{

		/* If we had a file error, return without completing the decode */
		if (c < 0)
			return 0;

		/* If the code is a clear code, reinitialize all necessary items. */
		if (c == clear)
		{
			curr_size = (short) (size + 1);
			slot = newcodes;
			sizeofstring[slot] = 0;
			top_slot = (short) (1 << curr_size);

			/* Continue reading codes until we get a non-clear code (Another
          * unlikely, but possible case...) */
		 do
		 {
			c = get_next_code();
		 } while (c == clear);

			/* If we get an ending code immediately after a clear code (Yet
          * another unlikely case), then break out of the loop. */
			if (c == ending)
				break;

			/* Finally, if the code is beyond the range of already set codes,
          * (This one had better NOT happen...   I have no idea what will
          * result from this, but I doubt it will look good...) then set it
          * to color zero. */
			if (c >= slot)
				c = 0;

			out_value = (BYTE) (old_code = c);

			/* And let us not forget to put the char into the buffer... */
			*sp++ = (BYTE) c;
		}
		else
		{
			/* In this case, it's not a clear code or an ending code, so it must
          * be a code code...  So we can now decode the code into a stack of
          * character codes. (Clear as mud, right?) */
			code = c;

			/* Here we go again with one of those off chances...  If, on the off
          * chance, the code we got is beyond the range of those already set
          * up (Another thing which had better NOT happen...) we trick the
          * decoder into thinking it actually got the next slot avail. */

			if (code >= slot)
			{
				if (code > slot)
				{
					++bad_code_count;
					c = slot;
				}
				code = old_code;
				*sp++ = out_value;
			}

			/* Here we scan back along the linked list of prefixes.  If they can
          * fit into the output buffer then transfer them direct.  ELSE push
          * them into the stack until we are down to enough characters that
          * they do fit.  Output the line then fall through to unstack the
          * ones that would not fit. */
			fastloop = NOPE;
			while (code >= newcodes)
			{
				j = i = sizeofstring[code];
				if (i > 0 && bufcnt - i > 0 && skipxdots == 0)
				{
					fastloop = YUP;

					do
					{
						*(bufptr + j) = suffix[code];
						code = prefix[code];
					} while (--j > 0);
					*bufptr = (BYTE) code;
					bufptr += ++i;
					bufcnt -= i;
					if (bufcnt == 0) /* finished an input row? */
					{
						if (--yskip < 0)
						{
							if ((ret = (short) ((*outln) (decoderline, (int) (bufptr - decoderline)))) < 0)
								return ret;
							yskip = skipydots;
						}
						if (driver_key_pressed())
							return -1;
						bufptr = decoderline;
						bufcnt = linewidth;
						xskip = 0;
					}
				}
				else
				{
					*sp++ = suffix[code];
					code = prefix[code];
				}
			}

			/* Push the last character on the stack, and set up the new prefix
          * and suffix, and if the required slot number is greater than that
          * allowed by the current bit size, increase the bit size.  (NOTE -
          * If we are all full, we *don't* save the new suffix and prefix...
          * I'm not certain if this is correct... it might be more proper to
          * overwrite the last code... */
			if (fastloop == NOPE)
				*sp++ = (BYTE) code;

			if (slot < top_slot)
			{
				sizeofstring[slot] = (short) (sizeofstring[old_code] + 1);
				suffix[slot] = out_value = (BYTE) code;
				prefix[slot++] = old_code;
				old_code = c;
			}
			if (slot >= top_slot)
				if (curr_size < 12)
				{
					top_slot <<= 1;
					++curr_size;
				}
		}
		while (sp > dstack)
		{
			--sp;
			if (--xskip < 0)
			{
				xskip = skipxdots;
				*bufptr++ = *sp;
			}
			if (--bufcnt == 0)     /* finished an input row? */
			{
				if (--yskip < 0)
				{
					if ((ret = (short) ((*outln) (decoderline, (int) (bufptr - decoderline)))) < 0)
						return ret;
					yskip = skipydots;
				}
				if (driver_key_pressed())
					return -1;
				bufptr = decoderline;
				bufcnt = linewidth;
				xskip = 0;
			}
		}
	}
	/* PB note that if last line is incomplete, we're not going to try to emit
	* it;  original code did, but did so via out_line and therefore couldn't
	* have worked well in all cases... */
	return 0;
}

/***** Program **********************************************************/
/* get_next_code()
 * - gets the next code from the GIF file.  Returns the code, or else
 * a negative number in case of file errors...
 */
static short get_next_code()
{
	static BYTE b1;              /* Current byte */
	static unsigned short ret_code;

	if (nbits_left == 0)
	{
		if (navail_bytes <= 0)
		{

			/* Out of bytes in current block, so read next block */
			pbytes = byte_buff;
			if ((navail_bytes = (short) get_byte()) < 0)
				return navail_bytes;
			else if (navail_bytes)
				get_bytes(byte_buff, navail_bytes);
		}
		b1 = *pbytes++;
		nbits_left = 8;
		--navail_bytes;
	}

	ret_code = (short) (b1 >> (8 - nbits_left));
	while (curr_size > nbits_left)
	{
		if (navail_bytes <= 0)
		{

			/* Out of bytes in current block, so read next block */
			pbytes = byte_buff;
			if ((navail_bytes = (short) get_byte()) < 0)
				return navail_bytes;
			else if (navail_bytes)
				get_bytes(byte_buff, navail_bytes);
		}
		b1 = *pbytes++;
		ret_code |= b1 << nbits_left;
		nbits_left += 8;
		--navail_bytes;
	}
	nbits_left -= curr_size;
	return (short) (ret_code & code_mask[curr_size]);
}

/* called in parent reoutine to set byte_buff */
void set_byte_buff(BYTE * ptr)
{
	byte_buff = ptr;
}
