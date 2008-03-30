/*
	"Disk-Video" (and RAM-Video and Expanded-Memory Video) routines
*/
#include <sstream>
#include <string>

#include <string.h>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "externs.h"

#include "cmdfiles.h"
#include "drivers.h"
#include "diskvid.h"
#include "realdos.h"
#include "TextColors.h"

enum
{
	BOX_ROW		= 6,
	BOX_COL		= 11,
	BOX_WIDTH	= 57,
	BOX_DEPTH	= 12,
	BLOCK_LEN	= 2048,   // must be a power of 2, must match next
	BLOCK_SHIFT	= 11,   // must match above
	CACHE_MIN	= 4,      // minimum cache size in Kbytes
	CACHE_MAX	= 64,     // maximum cache size in Kbytes
	FREE_MEM	= 33,     // try to leave this much memory unallocated
	HASH_SIZE	= 1024   // power of 2, near CACHE_MAX/(BLOCK_LEN + 8)
};

bool g_disk_16bit = false;	// storing 16 bit values for continuous potential

static int s_time_to_display;
static FILE *s_file = 0;
static bool s_disk_targa = false;
static struct cache		// structure of each cache entry
{
	long offset;				// pixel offset in image
	BYTE pixel[BLOCK_LEN];		// one pixel per byte (this *is* faster)
	unsigned int hashlink;		// ptr to next cache entry with same hash
	bool dirty;					// changed since read?
	bool lru;					// recently used?
} *s_cache_end, *s_cache_lru, *s_cur_cache;
static cache *s_cache_start = 0;
static long s_high_offset;           // highwater mark of writes
static long s_seek_offset;           // what we'll get next if we don't seek
static long s_cur_offset;            // offset of last g_block referenced
static int s_cur_row;
static long s_cur_row_base;
static unsigned int s_hash_ptr[HASH_SIZE] = { 0 };
static int s_pixel_shift;
static int s_header_length;
static int s_row_size = 0;   // doubles as a disk video not ok flag
static int s_col_size;       // g_screen_height, *2 when g_potential_16bit
static BYTE *s_memory_buffer;
static U16 s_disk_video_handle = 0;
static long s_memory_offset = 0;
static long s_old_memory_offset = 0;
static BYTE *s_memory_buffer_ptr;

static void find_load_cache(long offset);
static cache *find_cache(long offset);
static void  write_cache_lru();
static void mem_putc(BYTE);
static BYTE  mem_getc();
static void mem_seek(long offset);

int disk_start()
{
	s_header_length = 0;
	s_disk_targa = false;
	return disk_start_common(g_screen_width, g_screen_height, g_colors);
}

int disk_start_potential()
{
	int i;
	if (driver_diskp())			// ditch the original disk file
	{
		disk_end();
	}
	else
	{
		show_temp_message("clearing 16bit pot work area");
	}
	s_header_length = 0;
	s_disk_targa = false;
	i = disk_start_common(g_screen_width, g_screen_height << 1, g_colors);
	clear_temp_message();
	if (i == 0)
	{
		g_disk_16bit = true;
	}

	return i;
}

int disk_start_targa(FILE *targafp, int overhead)
{
	int i;
	if (driver_diskp())	// ditch the original file, make just the targa
	{
		disk_end();      // close the 'screen'
		set_null_video(); // set readdot and writedot routines to do nothing
	}
	s_header_length = overhead;
	s_file = targafp;
	s_disk_targa = true;
	i = disk_start_common(g_x_dots*3, g_y_dots, g_colors);
	s_high_offset = 100000000L; // targa not necessarily init'd to zeros

	return i;
}

int disk_start_common(long newrowsize, long newcolsize, int g_colors)
{
	int i;
	int freemem;
	long memorysize;
	long offset;
	unsigned int *fwd_link = 0;
	cache *ptr1 = 0;
	long longtmp;
	unsigned int cache_size;
	if (g_disk_flag)
	{
		disk_end();
	}
	if (driver_diskp()) // otherwise, real screen also in use, don't hit it
	{
		help_title();
		driver_set_attr(1, 0, C_DVID_BKGRD, 24*80);  // init rest to background
		for (i = 0; i < BOX_DEPTH; ++i)
		{
			driver_set_attr(BOX_ROW + i, BOX_COL, C_DVID_LO, BOX_WIDTH);  // init box
		}
		driver_put_string(BOX_ROW + 2, BOX_COL + 4, C_DVID_HI, "'Disk-Video' mode");
		driver_put_string(BOX_ROW + 4, BOX_COL + 4, C_DVID_LO,
			str(boost::format("Screen resolution: %d x %d") % g_screen_width % g_screen_height));
		if (s_disk_targa)
		{
			driver_put_string(-1, -1, C_DVID_LO, "  24 bit Targa");
		}
		else
		{
			driver_put_string(-1, -1, C_DVID_LO, str(boost::format("  Colors: %d") % g_colors));
		}
		driver_put_string(BOX_ROW + 8, BOX_COL + 4, C_DVID_LO, ("Save name: " + g_save_name).c_str());
		driver_put_string(BOX_ROW + 10, BOX_COL + 4, C_DVID_LO, "Status:");
		disk_video_status(0, "clearing the 'screen'");
	}
	s_cur_offset = -1;
	s_seek_offset = -1;
	s_high_offset = -1;
	s_cur_row    = -1;
	if (s_disk_targa)
	{
		s_pixel_shift = 0;
	}
	else
	{
		s_pixel_shift = 3;
		i = 2;
		while (i < g_colors)
		{
			i *= i;
			--s_pixel_shift;
		}
	}
	s_time_to_display = g_bf_math ? 10 : 1000;  // time-to-display-status counter

	/* allocate cache: try for the max; leave FREEMEMk free if we can get
		that much or more; if we can't get that much leave 1/2 of whatever
		there is free; demand a certain minimum or nogo at all */
	freemem = FREE_MEM;

	if (DEBUGMODE_MIN_DISKVID_CACHE == g_debug_mode)
	{
		cache_size = CACHE_MIN;
	}
	else
	{
		for (cache_size = CACHE_MAX; cache_size >= CACHE_MIN; --cache_size)
		{
			longtmp = (int(cache_size) < freemem) ?
				long(cache_size) << 11 : long(cache_size + freemem) << 10;
			BYTE *tempfar = new BYTE[longtmp];
			if (tempfar != 0)
			{
				delete[] tempfar;
				break;
			}
		}
	}
	longtmp = long(cache_size) << 10;
	s_cache_start = reinterpret_cast<cache *>(new BYTE[longtmp]);
	if (cache_size == 64)
	{
		--longtmp; // safety for next line
	}
	s_cache_lru = s_cache_start;
	s_cache_end = s_cache_lru + longtmp/sizeof(*s_cache_start);
	s_memory_buffer = new BYTE[BLOCK_LEN];
	if (s_cache_start == 0 || s_memory_buffer == 0)
	{
		stop_message(STOPMSG_NORMAL, "*** insufficient free memory for cache buffers ***");
		return -1;
	}
	if (driver_diskp())
	{
		driver_put_string(BOX_ROW + 6, BOX_COL + 4, C_DVID_LO,
			str(boost::format("Cache size: %dK") % cache_size));
	}

	// preset cache to all invalid entries so we don't need free list logic
	for (i = 0; i < HASH_SIZE; ++i)
	{
		s_hash_ptr[i] = 0xffff; // 0xffff marks the end of a hash chain
	}
	longtmp = 100000000L;
	for (ptr1 = s_cache_start; ptr1 < s_cache_end; ++ptr1)
	{
		ptr1->dirty = false;
		ptr1->lru = false;
		longtmp += BLOCK_LEN;
		fwd_link = &s_hash_ptr[(((unsigned short)longtmp >> BLOCK_SHIFT) & (HASH_SIZE-1))];
		ptr1->offset = longtmp;
		ptr1->hashlink = *fwd_link;
		*fwd_link = int((char *)ptr1 - (char *)s_cache_start);
	}

	memorysize = long(newcolsize)*newrowsize + s_header_length;
	i = memorysize & (BLOCK_LEN-1);
	if (i != 0)
	{
		memorysize += BLOCK_LEN - i;
	}
	memorysize >>= s_pixel_shift;
	memorysize >>= BLOCK_SHIFT;
	g_disk_flag = true;
	s_row_size = (unsigned int) newrowsize;
	s_col_size = (unsigned int) newcolsize;

	if (s_disk_targa)
	{
		// Retrieve the header information first
		BYTE *tmpptr;
		tmpptr = s_memory_buffer;
		fseek(s_file, 0L, SEEK_SET);
		for (i = 0; i < s_header_length; i++)
		{
			*tmpptr++ = (BYTE)fgetc(s_file);
		}
		fclose(s_file);
		s_disk_video_handle = MemoryAlloc((U16)BLOCK_LEN, memorysize, DISK);
	}
	else
	{
		s_disk_video_handle = MemoryAlloc((U16)BLOCK_LEN, memorysize, MEMORY);
	}
	if (s_disk_video_handle == 0)
	{
		stop_message(STOPMSG_NORMAL, "*** insufficient free memory/disk space ***");
		g_.SetGoodMode(false);
		s_row_size = 0;
		return -1;
	}

	if (driver_diskp())
	{
		driver_put_string(BOX_ROW + 2, BOX_COL + 23, C_DVID_LO,
			(MemoryType(s_disk_video_handle) == DISK) ? "Using your Disk Drive" : "Using your memory");
	}

	s_memory_buffer_ptr = s_memory_buffer;

	if (s_disk_targa)
	{
		// Put header information in the file
		MoveToMemory(s_memory_buffer, (U16)s_header_length, 1L, 0, s_disk_video_handle);
	}
	else
	{
		for (offset = 0; offset < memorysize; offset++)
		{
			SetMemory(0, (U16)BLOCK_LEN, 1L, offset, s_disk_video_handle);
			// TODO: just do it all, don't abort in the middle
			if (driver_key_pressed())           // user interrupt
			{
				// esc to cancel, else continue
				if (stop_message(STOPMSG_CANCEL, "Disk Video initialization interrupted:\n"))
				{
					disk_end();
					g_.SetGoodMode(false);
					return -2;            // -1 == failed, -2 == cancel
				}
			}
		}
	}

	if (driver_diskp())
	{
		disk_video_status(0, "");
	}
	return 0;
}

void disk_end()
{
	if (s_file != 0)
	{
		if (s_disk_targa) // flush the cache
		{
			for (s_cache_lru = s_cache_start; s_cache_lru < s_cache_end; ++s_cache_lru)
			{
				if (s_cache_lru->dirty)
				{
					write_cache_lru();
				}
			}
		}
		fclose(s_file);
		s_file = 0;
	}

	if (s_disk_video_handle != 0)
	{
		MemoryRelease(s_disk_video_handle);
		s_disk_video_handle = 0;
	}
	delete[] reinterpret_cast<BYTE *>(s_cache_start);
	s_cache_start = 0;
	delete[] s_memory_buffer;
	s_memory_buffer = 0;
	g_disk_flag = false;
	s_row_size = 0;
	g_disk_16bit = false;
}

static void disk_line_status(bool reading, int row)
{
	// adjust when potfile
	disk_video_status(0, str(boost::format(" %s line %4d")
			% (reading ? "reading" : "writing")
			% ((row >= g_screen_height) ? row-g_screen_height : row)));
}

int disk_read(int col, int row)
{
	int col_subscr;
	long offset;
	if (--s_time_to_display < 0)  // time to display status?
	{
		if (driver_diskp())
		{
			disk_line_status(true, row);
		}
		s_time_to_display = g_bf_math ? 10 : 1000;  // time-to-display-status counter
	}
	if (row != s_cur_row) // try to avoid ghastly code generated for multiply
	{
		if (row >= s_col_size) // while we're at it avoid this test if not needed
		{
			return 0;
		}
		s_cur_row = row;
		s_cur_row_base = long(s_cur_row)*s_row_size;
	}
	if (col >= s_row_size)
	{
		return 0;
	}
	offset = s_cur_row_base + col;
	col_subscr = offset & (BLOCK_LEN-1); // offset within cache entry
	if (s_cur_offset != (offset & (0L-BLOCK_LEN))) // same entry as last ref?
	{
		find_load_cache(offset & (0L-BLOCK_LEN));
	}
	return s_cur_cache->pixel[col_subscr];
}

int disk_from_memory(long offset, int size, void *dest)
{
	int col_subscr = int(offset & (BLOCK_LEN - 1));
	if (col_subscr + size > BLOCK_LEN)            // access violates  a
	{
		return 0;                                 // cache boundary
	}
	if (s_cur_offset != (offset & (0L-BLOCK_LEN))) // same entry as last ref?
	{
		find_load_cache(offset & (0L-BLOCK_LEN));
	}
	memcpy(dest, (void *) &s_cur_cache->pixel[col_subscr], size);
	s_cur_cache->dirty = false;
	return 1;
}


void disk_read_targa(unsigned int col, unsigned int row,
					BYTE *red, BYTE *green, BYTE *blue)
{
	col *= 3;
	*blue  = (BYTE)disk_read(col, row);
	*green = (BYTE)disk_read(++col, row);
	*red   = (BYTE)disk_read(col + 1, row);
}

void disk_write(int col, int row, int color)
{
	int col_subscr;
	long offset;
	if (--s_time_to_display < 0)  // time to display status?
	{
		if (driver_diskp())
		{
			disk_line_status(false, row);
		}
		s_time_to_display = 1000;
	}
	if (row != (unsigned int) s_cur_row)     // try to avoid ghastly code generated for multiply
	{
		if (row >= s_col_size) // while we're at it avoid this test if not needed
		{
			return;
		}
		s_cur_row = row;
		s_cur_row_base = long(s_cur_row)*s_row_size;
	}
	if (col >= s_row_size)
	{
		return;
	}
	offset = s_cur_row_base + col;
	col_subscr = offset & (BLOCK_LEN-1);
	if (s_cur_offset != (offset & (0L-BLOCK_LEN))) // same entry as last ref?
	{
		find_load_cache(offset & (0L-BLOCK_LEN));
	}
	if (s_cur_cache->pixel[col_subscr] != (color & 0xff))
	{
		s_cur_cache->pixel[col_subscr] = (BYTE) color;
		s_cur_cache->dirty = true;
	}
}

int disk_to_memory(long offset, int size, void *src)
{
	int col_subscr =  int(offset & (BLOCK_LEN - 1));

	if (col_subscr + size > BLOCK_LEN)            // access violates  a
	{
		return 0;                                 // cache boundary
	}

	if (s_cur_offset != (offset & (0L-BLOCK_LEN))) // same entry as last ref?
	{
		find_load_cache (offset & (0L-BLOCK_LEN));
	}

	memcpy((void *) &s_cur_cache->pixel[col_subscr], src, size);
	s_cur_cache->dirty = true;
	return 1;
}

void disk_write_targa(unsigned int col, unsigned int row,
					BYTE red, BYTE green, BYTE blue)
{
	disk_write(col *= 3, row, blue);
	disk_write(++col, row, green);
	disk_write(col + 1, row, red);
}

static void find_load_cache(long offset) // used by read/write
{
#ifndef XFRACT
	unsigned int tbloffset;
	int i;
	int j;
	unsigned int *fwd_link;
	BYTE *pixelptr;
	BYTE tmpchar;
	s_cur_offset = offset; // note this for next reference
	// check if required entry is in cache - lookup by hash
	tbloffset = s_hash_ptr[ ((unsigned short)offset >> BLOCK_SHIFT) & (HASH_SIZE-1) ];
	while (tbloffset != 0xffff)  // follow the hash chain
	{
		s_cur_cache = (cache *)((char *)s_cache_start + tbloffset);
		if (s_cur_cache->offset == offset)  // great, it is in the cache
		{
			s_cur_cache->lru = true;
			return;
		}
		tbloffset = s_cur_cache->hashlink;
	}
	// must load the cache entry from backing store
	while (true)  // look around for something not recently used
	{
		if (++s_cache_lru >= s_cache_end)
		{
			s_cache_lru = s_cache_start;
		}
		if (!s_cache_lru->lru)
		{
			break;
		}
		s_cache_lru->lru = false;
	}
	if (s_cache_lru->dirty) // must write this g_block before reusing it
	{
		write_cache_lru();
	}
	// remove g_block at s_cache_lru from its hash chain
	fwd_link = &s_hash_ptr[(((unsigned short) s_cache_lru->offset >> BLOCK_SHIFT) & (HASH_SIZE-1))];
	tbloffset = int((char *) s_cache_lru - (char *) s_cache_start);
	while (*fwd_link != tbloffset)
	{
		fwd_link = &((cache *) ((char *) s_cache_start + *fwd_link))->hashlink;
	}
	*fwd_link = s_cache_lru->hashlink;
	// load g_block
	s_cache_lru->dirty  = false;
	s_cache_lru->lru    = true;
	s_cache_lru->offset = offset;
	pixelptr = &s_cache_lru->pixel[0];
	if (offset > s_high_offset)  // never been this high before, just clear it
	{
		s_high_offset = offset;
		memset(pixelptr, 0, BLOCK_LEN);
		pixelptr += BLOCK_LEN;
	}
	else
	{
		if (offset != s_seek_offset)
		{
			mem_seek(offset >> s_pixel_shift);
		}
		s_seek_offset = offset + BLOCK_LEN;
		switch (s_pixel_shift)
		{
		case 0:
			for (i = 0; i < BLOCK_LEN; ++i)
			{
				*(pixelptr++) = mem_getc();
			}
			break;

		case 1:
			for (i = 0; i < BLOCK_LEN/2; ++i)
			{
				tmpchar = mem_getc();
				*(pixelptr++) = BYTE(tmpchar >> 4);
				*(pixelptr++) = BYTE(tmpchar & 15);
			}
			break;
		case 2:
			for (i = 0; i < BLOCK_LEN/4; ++i)
			{
				tmpchar = mem_getc();
				for (j = 6; j >= 0; j -= 2)
				{
					*(pixelptr++) = BYTE((tmpchar >> j) & 3);
				}
			}
			break;
		case 3:
			for (i = 0; i < BLOCK_LEN/8; ++i)
			{
				tmpchar = mem_getc();
				for (j = 7; j >= 0; --j)
				{
					*(pixelptr++) = BYTE((tmpchar >> j) & 1);
				}
			}
			break;
		}
	}
	// add new g_block to its hash chain
	fwd_link = &s_hash_ptr[(((unsigned short)offset >> BLOCK_SHIFT) & (HASH_SIZE-1))];
	s_cache_lru->hashlink = *fwd_link;
	*fwd_link = int((char *)s_cache_lru - (char *)s_cache_start);
	s_cur_cache = s_cache_lru;
#endif
}

// lookup for write_cache_lru
static cache *find_cache(long offset)
{
#ifndef XFRACT
	unsigned int tbloffset;
	cache *ptr1;
	tbloffset = s_hash_ptr[((unsigned short)offset >> BLOCK_SHIFT) & (HASH_SIZE-1)];
	while (tbloffset != 0xffff)
	{
		ptr1 = (cache *)((char *)s_cache_start + tbloffset);
		if (ptr1->offset == offset)
		{
			return ptr1;
		}
		tbloffset = ptr1->hashlink;
	}
	return 0;
#endif
}

static void  write_cache_lru()
{
	int i;
	int j;
	BYTE *pixelptr;
	long offset;
	BYTE tmpchar = 0;
	cache *ptr1, *ptr2;
	int const WRITEGAP = 4; // 1 for no gaps
	// scan back to also write any preceding dirty blocks, skipping small gaps
	ptr1 = s_cache_lru;
	offset = ptr1->offset;
	i = 0;
	while (++i <= WRITEGAP)
	{
		ptr2 = find_cache(offset -= BLOCK_LEN);
		if (ptr2 != 0 && ptr2->dirty)
		{
			ptr1 = ptr2;
			i = 0;
		}
	}
	// write all consecutive dirty blocks (often whole cache in 1pass modes)
	// keep going past small gaps

write_seek:
	mem_seek(ptr1->offset >> s_pixel_shift);

write_stuff:
	pixelptr = &ptr1->pixel[0];
	switch (s_pixel_shift)
	{
	case 0:
		for (i = 0; i < BLOCK_LEN; ++i)
		{
			mem_putc(*(pixelptr++));
		}
		break;
	case 1:
		for (i = 0; i < BLOCK_LEN/2; ++i)
		{
			tmpchar = BYTE(*(pixelptr++) << 4);
			tmpchar = BYTE(tmpchar + *(pixelptr++));
			mem_putc(tmpchar);
		}
		break;
	case 2:
		for (i = 0; i < BLOCK_LEN/4; ++i)
		{
			for (j = 6; j >= 0; j -= 2)
			{
				tmpchar = BYTE((tmpchar << 2) + *(pixelptr++));
			}
			mem_putc(tmpchar);
		}
		break;
	case 3:
		for (i = 0; i < BLOCK_LEN/8; ++i)
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
	ptr1->dirty = false;
	offset = ptr1->offset + BLOCK_LEN;
	ptr1 = find_cache(offset);
	if (ptr1 != 0 && ptr1->dirty)
	{
		goto write_stuff;
	}
	i = 1;
	while (++i <= WRITEGAP)
	{
		ptr1 = find_cache(offset += BLOCK_LEN);
		if (ptr1 != 0 && ptr1->dirty)
		{
			goto write_seek;
		}
	}
	s_seek_offset = -1; // force a seek before next read
}

/* Seek, mem_getc, mem_putc routines follow.
	Note that the calling logic always separates mem_getc and mem_putc
	sequences with a seek between them.  A mem_getc is never followed by
	a mem_putc nor v.v. without a seek between them.
	*/
static void mem_seek(long offset)        // mem seek
{
	offset += s_header_length;
	s_memory_offset = offset >> BLOCK_SHIFT;
	if (s_memory_offset != s_old_memory_offset)
	{
		MoveToMemory(s_memory_buffer, (U16)BLOCK_LEN, 1L, s_old_memory_offset, s_disk_video_handle);
		MoveFromMemory(s_memory_buffer, (U16)BLOCK_LEN, 1L, s_memory_offset, s_disk_video_handle);
	}
	s_old_memory_offset = s_memory_offset;
	s_memory_buffer_ptr = s_memory_buffer + (offset & (BLOCK_LEN - 1));
}

static BYTE mem_getc()                     // memory get_char
{
	if (s_memory_buffer_ptr - s_memory_buffer >= BLOCK_LEN)
	{
		MoveToMemory(s_memory_buffer, (U16)BLOCK_LEN, 1L, s_memory_offset, s_disk_video_handle);
		s_memory_offset++;
		MoveFromMemory(s_memory_buffer, (U16)BLOCK_LEN, 1L, s_memory_offset, s_disk_video_handle);
		s_memory_buffer_ptr = s_memory_buffer;
		s_old_memory_offset = s_memory_offset;
	}
	return *(s_memory_buffer_ptr++);
}

static void mem_putc(BYTE c)     // memory get_char
{
	if (s_memory_buffer_ptr - s_memory_buffer >= BLOCK_LEN)
	{
		MoveToMemory(s_memory_buffer, (U16)BLOCK_LEN, 1L, s_memory_offset, s_disk_video_handle);
		s_memory_offset++;
		MoveFromMemory(s_memory_buffer, (U16)BLOCK_LEN, 1L, s_memory_offset, s_disk_video_handle);
		s_memory_buffer_ptr = s_memory_buffer;
		s_old_memory_offset = s_memory_offset;
	}
	*(s_memory_buffer_ptr++) = c;
}


void disk_video_status(int line, const char *msg)
{
	char buf[41];
	int attrib;
	memset(buf, ' ', 40);
	memcpy(buf, msg, int(strlen(msg)));
	buf[40] = 0;
	attrib = C_DVID_HI;
	if (line >= 100)
	{
		line -= 100;
		attrib = C_STOP_ERR;
	}
	driver_put_string(BOX_ROW + 10 + line, BOX_COL + 12, attrib, buf);
	driver_hide_text_cursor();
}

void disk_video_status(int line, const std::string &str)
{
	disk_video_status(line, str.c_str());
}