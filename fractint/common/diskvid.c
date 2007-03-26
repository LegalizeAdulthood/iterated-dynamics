/*
	"Disk-Video" (and RAM-Video and Expanded-Memory Video) routines

	Reworked with fast caching July '90 by Pieter Branderhorst.
	(I'm proud of this cache handler, had to get my name on it!)
	Caution when modifying any code in here:  bugs are possible which
	slow the cache substantially but don't cause incorrect results.
	Do timing tests for a variety of situations after any change.

*/

#include <string.h>

/* see Fractint.c for a description of the "include"  hierarchy */
#include "port.h"
#include "prototyp.h"
#include "drivers.h"

#define BOXROW   6
#define BOXCOL   11
#define BOXWIDTH 57
#define BOXDEPTH 12

int disk16bit = 0;         /* storing 16 bit values for continuous potential */

static int timetodisplay;
static FILE *fp = NULL;
static int disktarga;

#define BLOCKLEN 2048   /* must be a power of 2, must match next */
#define BLOCKSHIFT 11   /* must match above */
#define CACHEMIN 4      /* minimum cache size in Kbytes */
#define CACHEMAX 64     /* maximum cache size in Kbytes */
#define FREEMEM  33     /* try to leave this much memory unallocated */
#define HASHSIZE 1024   /* power of 2, near CACHEMAX/(BLOCKLEN + 8) */

static struct cache		/* structure of each cache entry */
{
	long offset;                    /* pixel offset in image */
	BYTE pixel[BLOCKLEN];  /* one pixel per byte (this *is* faster) */
	unsigned int hashlink;          /* ptr to next cache entry with same hash */
	unsigned int dirty : 1;         /* changed since read? */
	unsigned int lru : 1;           /* recently used? */
} *cache_end, *cache_lru, *cur_cache;

static struct cache *cache_start = NULL;
static long high_offset;           /* highwater mark of writes */
static long seek_offset;           /* what we'll get next if we don't seek */
static long cur_offset;            /* offset of last block referenced */
static int cur_row;
static long cur_row_base;
static unsigned int hash_ptr[HASHSIZE] = { 0 };
static int pixelshift;
static int headerlength;
static int rowsize = 0;   /* doubles as a disk video not ok flag */
static int colsize;       /* sydots, *2 when pot16bit */

static BYTE *membuf;
static U16 dv_handle = 0;
static long memoffset = 0;
static long oldmemoffset = 0;
static BYTE *membufptr;

static void _fastcall  findload_cache(long);
static struct cache *_fastcall  find_cache(long);
static void  write_cache_lru(void);
static void _fastcall  mem_putc(BYTE);
static BYTE  mem_getc(void);
static void _fastcall  mem_seek(long);

int startdisk()
{
	headerlength = disktarga = 0;
	return common_startdisk(sxdots, sydots, colors);
}

int pot_startdisk()
{
	int i;
	if (driver_diskp())			/* ditch the original disk file */
	{
		enddisk();
	}
	else
	{
		showtempmsg("clearing 16bit pot work area");
	}
	headerlength = disktarga = 0;
	i = common_startdisk(sxdots, sydots << 1, colors);
	cleartempmsg();
	if (i == 0)
	{
		disk16bit = 1;
	}

	return i;
}

int targa_startdisk(FILE *targafp, int overhead)
{
	int i;
	if (driver_diskp())	/* ditch the original file, make just the targa */
	{
		enddisk();      /* close the 'screen' */
		setnullvideo(); /* set readdot and writedot routines to do nothing */
	}
	headerlength = overhead;
	fp = targafp;
	disktarga = 1;
	i = common_startdisk(xdots*3, ydots, colors);
	high_offset = 100000000L; /* targa not necessarily init'd to zeros */

	return i;
}

int _fastcall common_startdisk(long newrowsize, long newcolsize, int colors)
{
	int i, freemem;
	long memorysize, offset;
	unsigned int *fwd_link = NULL;
	struct cache *ptr1 = NULL;
	long longtmp;
	unsigned int cache_size;
	BYTE *tempfar = NULL;
	if (g_disk_flag)
	{
		enddisk();
	}
	if (driver_diskp()) /* otherwise, real screen also in use, don't hit it */
	{
		char buf[128];
		helptitle();
		driver_set_attr(1, 0, C_DVID_BKGRD, 24*80);  /* init rest to background */
		for (i = 0; i < BOXDEPTH; ++i)
		{
			driver_set_attr(BOXROW + i, BOXCOL, C_DVID_LO, BOXWIDTH);  /* init box */
		}
		driver_put_string(BOXROW + 2, BOXCOL + 4, C_DVID_HI, "'Disk-Video' mode");
		sprintf(buf, "Screen resolution: %d x %d", sxdots, sydots);
		driver_put_string(BOXROW + 4, BOXCOL + 4, C_DVID_LO, buf);
		if (disktarga)
		{
			driver_put_string(-1, -1, C_DVID_LO, "  24 bit Targa");
		}
		else
		{
			driver_put_string(-1, -1, C_DVID_LO, "  Colors: ");
			sprintf(buf, "%d", colors);
			driver_put_string(-1, -1, C_DVID_LO, buf);
		}
		sprintf(buf, "Save name: %s", savename);
		driver_put_string(BOXROW + 8, BOXCOL + 4, C_DVID_LO, buf);
		driver_put_string(BOXROW + 10, BOXCOL + 4, C_DVID_LO, "Status:");
		dvid_status(0, "clearing the 'screen'");
	}
	cur_offset = seek_offset = high_offset = -1;
	cur_row    = -1;
	if (disktarga)
	{
		pixelshift = 0;
	}
	else
	{
		pixelshift = 3;
		i = 2;
		while (i < colors)
		{
			i *= i;
			--pixelshift;
		}
	}
	timetodisplay = bf_math ? 10 : 1000;  /* time-to-g_driver-status counter */

	/* allocate cache: try for the max; leave FREEMEMk free if we can get
		that much or more; if we can't get that much leave 1/2 of whatever
		there is free; demand a certain minimum or nogo at all */
	freemem = FREEMEM;

	for (cache_size = CACHEMAX; cache_size >= CACHEMIN; --cache_size)
	{
		longtmp = ((int)cache_size < freemem) ?
			(long)cache_size << 11 : (long)(cache_size + freemem) << 10;
		tempfar = malloc(longtmp);
		if (tempfar != NULL)
		{
			free(tempfar);
			break;
		}
	}
	if (debugflag == 4200)
	{
		cache_size = CACHEMIN;
	}
	longtmp = (long)cache_size << 10;
	cache_start = (struct cache *)malloc(longtmp);
	if (cache_size == 64)
	{
		--longtmp; /* safety for next line */
	}
	cache_end = (cache_lru = cache_start) + longtmp / sizeof(*cache_start);
	membuf = (BYTE *)malloc((long)BLOCKLEN);
	if (cache_start == NULL || membuf == NULL)
	{
		stopmsg(0, "*** insufficient free memory for cache buffers ***");
		return -1;
	}
	if (driver_diskp())
	{
		char buf[50];
		sprintf(buf, "Cache size: %dK", cache_size);
		driver_put_string(BOXROW + 6, BOXCOL + 4, C_DVID_LO, buf);
	}

	/* preset cache to all invalid entries so we don't need free list logic */
	for (i = 0; i < HASHSIZE; ++i)
	{
		hash_ptr[i] = 0xffff; /* 0xffff marks the end of a hash chain */
	}
	longtmp = 100000000L;
	for (ptr1 = cache_start; ptr1 < cache_end; ++ptr1)
	{
		ptr1->dirty = ptr1->lru = 0;
		longtmp += BLOCKLEN;
		fwd_link = &hash_ptr[(((unsigned short)longtmp >> BLOCKSHIFT) & (HASHSIZE-1))];
		ptr1->offset = longtmp;
		ptr1->hashlink = *fwd_link;
		*fwd_link = (int) ((char *)ptr1 - (char *)cache_start);
	}

	memorysize = (long)(newcolsize)*newrowsize + headerlength;
	i = (short) memorysize & (BLOCKLEN-1);
	if (i != 0)
	{
		memorysize += BLOCKLEN - i;
	}
	memorysize >>= pixelshift;
	memorysize >>= BLOCKSHIFT;
	g_disk_flag = 1;
	rowsize = (unsigned int) newrowsize;
	colsize = (unsigned int) newcolsize;

	if (disktarga)
	{
		/* Retrieve the header information first */
		BYTE *tmpptr;
		tmpptr = membuf;
		fseek(fp, 0L, SEEK_SET);
		for (i = 0; i < headerlength; i++)
		{
			*tmpptr++ = (BYTE)fgetc(fp);
		}
		fclose(fp);
		dv_handle = MemoryAlloc((U16)BLOCKLEN, memorysize, DISK);
	}
	else
	{
		dv_handle = MemoryAlloc((U16)BLOCKLEN, memorysize, MEMORY);
	}
	if (dv_handle == 0)
	{
		stopmsg(0, "*** insufficient free memory/disk space ***");
		g_good_mode = 0;
		rowsize = 0;
		return -1;
	}

	if (driver_diskp())
	{
		driver_put_string(BOXROW + 2, BOXCOL + 23, C_DVID_LO,
			(MemoryType(dv_handle) == DISK) ? "Using your Disk Drive" : "Using your memory");
	}

	membufptr = membuf;

	if (disktarga)
	{
		/* Put header information in the file */
		MoveToMemory(membuf, (U16)headerlength, 1L, 0, dv_handle);
	}
	else
	{
		for (offset = 0; offset < memorysize; offset++)
		{
			SetMemory(0, (U16)BLOCKLEN, 1L, offset, dv_handle);
			if (driver_key_pressed())           /* user interrupt */
			{
				/* esc to cancel, else continue */
				if (stopmsg(STOPMSG_CANCEL, "Disk Video initialization interrupted:\n"))
				{
					enddisk();
					g_good_mode = 0;
					return -2;            /* -1 == failed, -2 == cancel   */
				}
			}
		}
	}

	if (driver_diskp())
	{
		dvid_status(0, "");
	}
	return 0;
}

void enddisk()
{
	if (fp != NULL)
	{
		if (disktarga) /* flush the cache */
		{
			for (cache_lru = cache_start; cache_lru < cache_end; ++cache_lru)
			{
				if (cache_lru->dirty)
				{
					write_cache_lru();
				}
			}
		}
		fclose(fp);
		fp = NULL;
	}

	if (dv_handle != 0)
	{
		MemoryRelease(dv_handle);
		dv_handle = 0;
	}
	if (cache_start != NULL)
	{
		free((void *)cache_start);
		cache_start = NULL;
	}
	if (membuf != NULL)
	{
		free((void *)membuf);
		membuf = NULL;
	}
	g_disk_flag = rowsize = disk16bit = 0;
}

int readdisk(int col, int row)
{
	int col_subscr;
	long offset;
	char buf[41];
	if (--timetodisplay < 0)  /* time to g_driver status? */
	{
		if (driver_diskp())
		{
			sprintf(buf, " reading line %4d",
					(row >= sydots) ? row-sydots : row); /* adjust when potfile */
			dvid_status(0, buf);
		}
		timetodisplay = bf_math ? 10 : 1000;  /* time-to-g_driver-status counter */
	}
	if (row != cur_row) /* try to avoid ghastly code generated for multiply */
	{
		if (row >= colsize) /* while we're at it avoid this test if not needed  */
		{
			return 0;
		}
		cur_row = row;
		cur_row_base = (long) cur_row*rowsize;
	}
	if (col >= rowsize)
	{
		return 0;
	}
	offset = cur_row_base + col;
	col_subscr = (short) offset & (BLOCKLEN-1); /* offset within cache entry */
	if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
	{
		findload_cache(offset & (0L-BLOCKLEN));
	}
	return cur_cache->pixel[col_subscr];
}

int FromMemDisk(long offset, int size, void *dest)
{
	int col_subscr = (int) (offset & (BLOCKLEN - 1));
	if (col_subscr + size > BLOCKLEN)            /* access violates  a */
	{
		return 0;                                 /*   cache boundary   */
	}
	if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
	{
		findload_cache(offset & (0L-BLOCKLEN));
	}
	memcpy(dest, (void *) &cur_cache->pixel[col_subscr], size);
	cur_cache->dirty = 0;
	return 1;
}


void targa_readdisk(unsigned int col, unsigned int row,
					BYTE *red, BYTE *green, BYTE *blue)
{
	col *= 3;
	*blue  = (BYTE)readdisk(col, row);
	*green = (BYTE)readdisk(++col, row);
	*red   = (BYTE)readdisk(col + 1, row);
}

void writedisk(int col, int row, int color)
{
	int col_subscr;
	long offset;
	char buf[41];
	if (--timetodisplay < 0)  /* time to display status? */
	{
		if (driver_diskp())
		{
			sprintf(buf, " writing line %4d",
					(row >= sydots) ? row-sydots : row); /* adjust when potfile */
			dvid_status(0, buf);
		}
		timetodisplay = 1000;
	}
	if (row != (unsigned int) cur_row)     /* try to avoid ghastly code generated for multiply */
	{
		if (row >= colsize) /* while we're at it avoid this test if not needed  */
		{
			return;
		}
		cur_row_base = (long) (cur_row = row)*rowsize;
	}
	if (col >= rowsize)
	{
		return;
	}
	offset = cur_row_base + col;
	col_subscr = (short) offset & (BLOCKLEN-1);
	if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
	{
		findload_cache(offset & (0L-BLOCKLEN));
	}
	if (cur_cache->pixel[col_subscr] != (color & 0xff))
	{
		cur_cache->pixel[col_subscr] = (BYTE) color;
		cur_cache->dirty = 1;
	}
}

int ToMemDisk(long offset, int size, void *src)
{
	int col_subscr =  (int)(offset & (BLOCKLEN - 1));

	if (col_subscr + size > BLOCKLEN)            /* access violates  a */
	{
		return 0;                                 /*   cache boundary   */
	}

	if (cur_offset != (offset & (0L-BLOCKLEN))) /* same entry as last ref? */
	{
		findload_cache (offset & (0L-BLOCKLEN));
	}

	memcpy((void *) &cur_cache->pixel[col_subscr], src, size);
	cur_cache->dirty = 1;
	return 1;
}

void targa_writedisk(unsigned int col, unsigned int row,
					BYTE red, BYTE green, BYTE blue)
{
	writedisk(col *= 3, row, blue);
	writedisk(++col, row, green);
	writedisk(col + 1, row, red);
}

static void _fastcall  findload_cache(long offset) /* used by read/write */
{
#ifndef XFRACT
	unsigned int tbloffset;
	int i, j;
	unsigned int *fwd_link;
	BYTE *pixelptr;
	BYTE tmpchar;
	cur_offset = offset; /* note this for next reference */
	/* check if required entry is in cache - lookup by hash */
	tbloffset = hash_ptr[ ((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
	while (tbloffset != 0xffff)  /* follow the hash chain */
	{
		cur_cache = (struct cache *)((char *)cache_start + tbloffset);
		if (cur_cache->offset == offset)  /* great, it is in the cache */
		{
			cur_cache->lru = 1;
			return;
		}
		tbloffset = cur_cache->hashlink;
	}
	/* must load the cache entry from backing store */
	while (1)  /* look around for something not recently used */
	{
		if (++cache_lru >= cache_end)
		{
			cache_lru = cache_start;
		}
		if (cache_lru->lru == 0)
		{
			break;
		}
		cache_lru->lru = 0;
	}
	if (cache_lru->dirty) /* must write this block before reusing it */
	{
		write_cache_lru();
	}
	/* remove block at cache_lru from its hash chain */
	fwd_link = &hash_ptr[(((unsigned short)cache_lru->offset >> BLOCKSHIFT) & (HASHSIZE-1))];
	tbloffset = (int) ((char *)cache_lru - (char *)cache_start);
	while (*fwd_link != tbloffset)
	{
		fwd_link = &((struct cache *)((char *)cache_start + *fwd_link))->hashlink;
	}
	*fwd_link = cache_lru->hashlink;
	/* load block */
	cache_lru->dirty  = 0;
	cache_lru->lru    = 1;
	cache_lru->offset = offset;
	pixelptr = &cache_lru->pixel[0];
	if (offset > high_offset)  /* never been this high before, just clear it */
	{
		high_offset = offset;
		memset(pixelptr, 0, BLOCKLEN);
		pixelptr += BLOCKLEN;
	}
	else
	{
		if (offset != seek_offset)
		{
			mem_seek(offset >> pixelshift);
		}
		seek_offset = offset + BLOCKLEN;
		switch (pixelshift)
		{
		case 0:
			for (i = 0; i < BLOCKLEN; ++i)
			{
				*(pixelptr++) = mem_getc();
			}
			break;

		case 1:
			for (i = 0; i < BLOCKLEN/2; ++i)
			{
				tmpchar = mem_getc();
				*(pixelptr++) = (BYTE)(tmpchar >> 4);
				*(pixelptr++) = (BYTE)(tmpchar & 15);
			}
			break;
		case 2:
			for (i = 0; i < BLOCKLEN/4; ++i)
			{
				tmpchar = mem_getc();
				for (j = 6; j >= 0; j -= 2)
				{
					*(pixelptr++) = (BYTE)((tmpchar >> j) & 3);
				}
			}
			break;
		case 3:
			for (i = 0; i < BLOCKLEN/8; ++i)
			{
				tmpchar = mem_getc();
				for (j = 7; j >= 0; --j)
				{
					*(pixelptr++) = (BYTE)((tmpchar >> j) & 1);
				}
			}
			break;
		}
	}
	/* add new block to its hash chain */
	fwd_link = &hash_ptr[(((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1))];
	cache_lru->hashlink = *fwd_link;
	*fwd_link = (int) ((char *)cache_lru - (char *)cache_start);
	cur_cache = cache_lru;
#endif
}

/* lookup for write_cache_lru */
static struct cache *_fastcall  find_cache(long offset)
{
#ifndef XFRACT
	unsigned int tbloffset;
	struct cache *ptr1;
	tbloffset = hash_ptr[((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1)];
	while (tbloffset != 0xffff)
	{
		ptr1 = (struct cache *)((char *)cache_start + tbloffset);
		if (ptr1->offset == offset)
		{
			return ptr1;
		}
		tbloffset = ptr1->hashlink;
	}
	return NULL;
#endif
}

static void  write_cache_lru()
{
	int i, j;
	BYTE *pixelptr;
	long offset;
	BYTE tmpchar = 0;
	struct cache *ptr1, *ptr2;
#define WRITEGAP 4 /* 1 for no gaps */
	/* scan back to also write any preceding dirty blocks, skipping small gaps */
	ptr1 = cache_lru;
	offset = ptr1->offset;
	i = 0;
	while (++i <= WRITEGAP)
	{
		ptr2 = find_cache(offset -= BLOCKLEN);
		if (ptr2 != NULL && ptr2->dirty)
		{
			ptr1 = ptr2;
			i = 0;
		}
	}
	/* write all consecutive dirty blocks (often whole cache in 1pass modes) */
	/* keep going past small gaps */

write_seek:
	mem_seek(ptr1->offset >> pixelshift);

write_stuff:
	pixelptr = &ptr1->pixel[0];
	switch (pixelshift)
	{
	case 0:
		for (i = 0; i < BLOCKLEN; ++i)
		{
			mem_putc(*(pixelptr++));
		}
		break;
	case 1:
		for (i = 0; i < BLOCKLEN/2; ++i)
		{
			tmpchar = (BYTE)(*(pixelptr++) << 4);
			tmpchar = (BYTE)(tmpchar + *(pixelptr++));
			mem_putc(tmpchar);
		}
		break;
	case 2:
		for (i = 0; i < BLOCKLEN/4; ++i)
		{
			for (j = 6; j >= 0; j -= 2)
			{
				tmpchar = (BYTE)((tmpchar << 2) + *(pixelptr++));
			}
			mem_putc(tmpchar);
		}
		break;
	case 3:
		for (i = 0; i < BLOCKLEN/8; ++i)
		{
			mem_putc((BYTE)
						((((((((((((((*pixelptr
						<< 1)
						| *(pixelptr + 1))
						<< 1)
						| *(pixelptr + 2))
						<< 1)
						| *(pixelptr + 3))
						<< 1)
						| *(pixelptr + 4))
						<< 1)
						| *(pixelptr + 5))
						<< 1)
						| *(pixelptr + 6))
						<< 1)
						| *(pixelptr + 7)));
			pixelptr += 8;
		}
		break;
	}
	ptr1->dirty = 0;
	offset = ptr1->offset + BLOCKLEN;
	ptr1 = find_cache(offset);
	if (ptr1 != NULL && ptr1->dirty != 0)
	{
		goto write_stuff;
	}
	i = 1;
	while (++i <= WRITEGAP)
	{
		ptr1 = find_cache(offset += BLOCKLEN);
		if (ptr1 != NULL && ptr1->dirty != 0)
		{
			goto write_seek;
		}
	}
	seek_offset = -1; /* force a seek before next read */
}

/* Seek, mem_getc, mem_putc routines follow.
	Note that the calling logic always separates mem_getc and mem_putc
	sequences with a seek between them.  A mem_getc is never followed by
	a mem_putc nor v.v. without a seek between them.
	*/
static void _fastcall  mem_seek(long offset)        /* mem seek */
{
	offset += headerlength;
	memoffset = offset >> BLOCKSHIFT;
	if (memoffset != oldmemoffset)
	{
		MoveToMemory(membuf, (U16)BLOCKLEN, 1L, oldmemoffset, dv_handle);
		MoveFromMemory(membuf, (U16)BLOCKLEN, 1L, memoffset, dv_handle);
	}
	oldmemoffset = memoffset;
	membufptr = membuf + (offset & (BLOCKLEN - 1));
}

static BYTE  mem_getc()                     /* memory get_char */
{
	if (membufptr - membuf >= BLOCKLEN)
	{
		MoveToMemory(membuf, (U16)BLOCKLEN, 1L, memoffset, dv_handle);
		memoffset++;
		MoveFromMemory(membuf, (U16)BLOCKLEN, 1L, memoffset, dv_handle);
		membufptr = membuf;
		oldmemoffset = memoffset;
	}
	return *(membufptr++);
}

static void _fastcall mem_putc(BYTE c)     /* memory get_char */
{
	if (membufptr - membuf >= BLOCKLEN)
	{
		MoveToMemory(membuf, (U16)BLOCKLEN, 1L, memoffset, dv_handle);
		memoffset++;
		MoveFromMemory(membuf, (U16)BLOCKLEN, 1L, memoffset, dv_handle);
		membufptr = membuf;
		oldmemoffset = memoffset;
	}
	*(membufptr++) = c;
}


void dvid_status(int line, char *msg)
{
	char buf[41];
	int attrib;
	memset(buf, ' ', 40);
	memcpy(buf, msg, (int) strlen(msg));
	buf[40] = 0;
	attrib = C_DVID_HI;
	if (line >= 100)
	{
		line -= 100;
		attrib = C_STOP_ERR;
	}
	driver_put_string(BOXROW + 10 + line, BOXCOL + 12, attrib, buf);
	driver_hide_text_cursor();
}
