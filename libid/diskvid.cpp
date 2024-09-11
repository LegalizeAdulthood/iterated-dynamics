//
// "Disk-Video" routines
//
// Caution when modifying any code in here:  bugs are possible which
// slow the cache substantially but don't cause incorrect results.
// Do timing tests for a variety of situations after any change.
//
//
#include "diskvid.h"

#include "big.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "help_title.h"
#include "id_data.h"
#include "memory.h"
#include "set_null_video.h"
#include "stop_msg.h"
#include "temp_msg.h"

#include <array>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

enum
{
    BOXROW = 6,
    BOXCOL = 11,
    BOXWIDTH = 57,
    BOXDEPTH = 12,
    BLOCKLEN = 2048, // must be a power of 2, must match next
    BLOCKSHIFT = 11, // must match above
    CACHEMIN = 64,   // minimum cache size in Kbytes
    CACHEMAX = 4096, // maximum cache size in Kbytes
    FREEMEM = 33,    // try to leave this much memory unallocated
    HASHSIZE = 2048 // power of 2, near CACHEMAX/(BLOCKLEN+8)
};

struct cache                // structure of each cache entry
{
    long offset;            // pixel offset in image
    BYTE pixel[BLOCKLEN];   // one pixel per byte
    unsigned int hashlink;  // ptr to next cache entry with same hash
    bool dirty;             // changed since read?
    bool lru;               // recently used?
};

bool g_disk_16_bit{}; // storing 16 bit values for continuous potential
bool g_disk_targa{};  //
bool g_disk_flag{};   //
bool g_good_mode{};   // if non-zero, OK to read/write pixels

static int s_time_to_display{};             //
static std::FILE *s_fp{};                   //
static cache *s_cache_end{};                //
static cache *s_cache_lru{};                //
static cache *s_cur_cache{};                //
static cache *s_cache_start{};              //
static long s_high_offset{};                // highwater mark of writes
static long s_seek_offset{};                // what we'll get next if we don't seek
static long s_cur_offset{};                 // offset of last block referenced
static int s_cur_row{};                     //
static long s_cur_row_base{};               //
static unsigned int s_hash_ptr[HASHSIZE]{}; //
static int s_pixel_shift{};                 //
static int s_header_length{};               //
static int s_row_size{};                    // doubles as a disk video not ok flag
static int s_col_size{};                    // sydots, *2 when pot16bit
static std::vector<BYTE> s_mem_buf;         //
static U16 s_dv_handle{};                   //
static long s_mem_offset{};                 //
static long s_old_mem_offset{};             //
static BYTE *s_mem_buf_ptr{};               //

static void findload_cache(long);
static cache *find_cache(long);
static void  write_cache_lru();
static void mem_putc(BYTE);
static BYTE  mem_getc();
static void mem_seek(long);

int startdisk()
{
    s_header_length = 0;
    g_disk_targa = false;
    return common_startdisk(g_screen_x_dots, g_screen_y_dots, g_colors);
}

int pot_startdisk()
{
    int i;
    if (driver_diskp())         // ditch the original disk file
    {
        enddisk();
    }
    else
    {
        showtempmsg("clearing 16bit pot work area");
    }
    s_header_length = 0;
    g_disk_targa = false;
    i = common_startdisk(g_screen_x_dots, g_screen_y_dots << 1, g_colors);
    cleartempmsg();
    if (i == 0)
    {
        g_disk_16_bit = true;
    }

    return i;
}

int targa_startdisk(std::FILE *targafp, int overhead)
{
    int i;
    if (driver_diskp()) // ditch the original file, make just the targa
    {
        enddisk();      // close the 'screen'
        setnullvideo(); // set readdot and writedot routines to do nothing
    }
    s_header_length = overhead;
    s_fp = targafp;
    g_disk_targa = true;
    i = common_startdisk(g_logical_screen_x_dots*3, g_logical_screen_y_dots, g_colors);
    s_high_offset = 100000000L; // targa not necessarily init'd to zeros

    return i;
}

int common_startdisk(long newrowsize, long newcolsize, int colors)
{
    if (g_disk_flag)
    {
        enddisk();
    }
    if (driver_diskp()) // otherwise, real screen also in use, don't hit it
    {
        helptitle();
        driver_set_attr(1, 0, C_DVID_BKGRD, 24*80);  // init rest to background
        for (int i = 0; i < BOXDEPTH; ++i)
        {
            driver_set_attr(BOXROW+i, BOXCOL, C_DVID_LO, BOXWIDTH);  // init box
        }
        driver_put_string(BOXROW+2, BOXCOL+4, C_DVID_HI, "'Disk-Video' mode");
        driver_put_string(BOXROW + 4, BOXCOL + 4, C_DVID_LO,
            "Screen resolution: " + std::to_string(g_screen_x_dots) + " x " + std::to_string(g_screen_y_dots));
        if (g_disk_targa)
        {
            driver_put_string(-1, -1, C_DVID_LO, "  24 bit Targa");
        }
        else
        {
            driver_put_string(-1, -1, C_DVID_LO, "  Colors: ");
            driver_put_string(-1, -1, C_DVID_LO, std::to_string(colors));
        }
        driver_put_string(BOXROW+8, BOXCOL+4, C_DVID_LO, "Save name: " + g_save_filename);
        driver_put_string(BOXROW+10, BOXCOL+4, C_DVID_LO, "Status:");
        dvid_status(0, "clearing the 'screen'");
    }
    s_high_offset = -1;
    s_seek_offset = s_high_offset;
    s_cur_offset = s_seek_offset;
    s_cur_row    = -1;
    if (g_disk_targa)
    {
        s_pixel_shift = 0;
    }
    else
    {
        s_pixel_shift = 3;
        int i = 2;
        while (i < colors)
        {
            i *= i;
            --s_pixel_shift;
        }
    }
    s_time_to_display = g_bf_math != bf_math_type::NONE ? 10 : 1000;  // time-to-g_driver-status counter

    constexpr unsigned int cache_size = CACHEMAX;
    long longtmp = (long) cache_size << 10;
    s_cache_start = (cache *)malloc(longtmp);
    if (cache_size == 64)
    {
        --longtmp; // safety for next line
    }
    s_cache_lru = s_cache_start;
    s_cache_end = s_cache_lru + longtmp/sizeof(*s_cache_start);
    s_mem_buf.resize(BLOCKLEN);
    if (s_cache_start == nullptr)
    {
        stopmsg("*** insufficient free memory for cache buffers ***");
        return -1;
    }
    if (driver_diskp())
    {
        driver_put_string(BOXROW + 6, BOXCOL + 4, C_DVID_LO, "Cache size: " + std::to_string(cache_size) + "K");
    }

    // preset cache to all invalid entries so we don't need free list logic
    for (auto &elem : s_hash_ptr)
    {
        elem = 0xffff; // 0xffff marks the end of a hash chain
    }
    longtmp = 100000000L;
    for (cache *ptr1 = s_cache_start; ptr1 < s_cache_end; ++ptr1)
    {
        ptr1->dirty = false;
        ptr1->lru = false;
        longtmp += BLOCKLEN;
        unsigned int *fwd_link = &s_hash_ptr[((unsigned short) longtmp >> BLOCKSHIFT) & (HASHSIZE - 1)];
        ptr1->offset = longtmp;
        ptr1->hashlink = *fwd_link;
        *fwd_link = (int)((char *)ptr1 - (char *)s_cache_start);
    }

    long memorysize = (long) (newcolsize) *newrowsize + s_header_length;
    {
        const int i = (short) memorysize & (BLOCKLEN - 1);
        if (i != 0)
        {
            memorysize += BLOCKLEN - i;
        }
    }
    memorysize >>= s_pixel_shift;
    memorysize >>= BLOCKSHIFT;
    g_disk_flag = true;
    s_row_size = (unsigned int) newrowsize;
    s_col_size = (unsigned int) newcolsize;

    if (g_disk_targa)
    {
        // Retrieve the header information first
        std::fseek(s_fp, 0L, SEEK_SET);
        for (int i = 0; i < s_header_length; i++)
        {
            s_mem_buf[i] = (BYTE)fgetc(s_fp);
        }
        std::fclose(s_fp);
        s_dv_handle = MemoryAlloc((U16)BLOCKLEN, memorysize, DISK);
    }
    else
    {
        s_dv_handle = MemoryAlloc((U16)BLOCKLEN, memorysize, MEMORY);
    }
    if (s_dv_handle == 0)
    {
        stopmsg("*** insufficient free memory/disk space ***");
        g_good_mode = false;
        s_row_size = 0;
        return -1;
    }

    if (driver_diskp())
    {
        driver_put_string(BOXROW+2, BOXCOL+23, C_DVID_LO,
                          (MemoryType(s_dv_handle) == DISK) ? "Using your Disk Drive" : "Using your memory");
    }

    s_mem_buf_ptr = &s_mem_buf[0];

    if (g_disk_targa)
    {
        // Put header information in the file
        CopyFromMemoryToHandle(&s_mem_buf[0], (U16)s_header_length, 1L, 0, s_dv_handle);
    }
    else
    {
        for (long offset = 0; offset < memorysize; offset++)
        {
            SetMemory(0, (U16)BLOCKLEN, 1L, offset, s_dv_handle);
            if (driver_key_pressed())           // user interrupt
            {
                // esc to cancel, else continue
                if (stopmsg(stopmsg_flags::CANCEL, "Disk Video initialization interrupted:\n"))
                {
                    enddisk();
                    g_good_mode = false;
                    return -2;            // -1 == failed, -2 == cancel
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
    if (s_fp != nullptr)
    {
        if (g_disk_targa) // flush the cache
        {
            for (s_cache_lru = s_cache_start; s_cache_lru < s_cache_end; ++s_cache_lru)
            {
                if (s_cache_lru->dirty)
                {
                    write_cache_lru();
                }
            }
        }
        std::fclose(s_fp);
        s_fp = nullptr;
    }

    if (s_dv_handle != 0)
    {
        MemoryRelease(s_dv_handle);
        s_dv_handle = 0;
    }
    if (s_cache_start != nullptr)
    {
        free((void *)s_cache_start);
        s_cache_start = nullptr;
    }
    s_mem_buf.clear();
    g_disk_flag = false;
    s_row_size = 0;
    g_disk_16_bit = false;
}

int readdisk(int col, int row)
{
    int col_subscr;
    long offset;
    char buf[41];
    if (--s_time_to_display < 0)  // time to g_driver status?
    {
        if (driver_diskp())
        {
            std::snprintf(buf, std::size(buf), " reading line %4d",
                    (row >= g_screen_y_dots) ? row-g_screen_y_dots : row); // adjust when potfile
            dvid_status(0, buf);
        }
        if (g_bf_math != bf_math_type::NONE)
        {
            s_time_to_display = 10;  // time-to-g_driver-status counter
        }
        else
        {
            s_time_to_display = 1000;  // time-to-g_driver-status counter
        }
    }
    if (row != s_cur_row) // try to avoid ghastly code generated for multiply
    {
        if (row >= s_col_size) // while we're at it avoid this test if not needed
        {
            return 0;
        }
        s_cur_row = row;
        s_cur_row_base = (long) s_cur_row * s_row_size;
    }
    if (col >= s_row_size)
    {
        return 0;
    }
    offset = s_cur_row_base + col;
    col_subscr = (short) offset & (BLOCKLEN-1); // offset within cache entry
    if (s_cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }
    return s_cur_cache->pixel[col_subscr];
}

int FromMemDisk(long offset, int size, void *dest)
{
    int col_subscr = (int)(offset & (BLOCKLEN - 1));
    if (col_subscr + size > BLOCKLEN)            // access violates  a
    {
        return 0;                                 //   cache boundary
    }
    if (s_cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }
    std::memcpy(dest, (void *) &s_cur_cache->pixel[col_subscr], size);
    s_cur_cache->dirty = false;
    return 1;
}


void targa_readdisk(unsigned int col, unsigned int row, BYTE *red, BYTE *green, BYTE *blue)
{
    col *= 3;
    *blue  = (BYTE)readdisk(col, row);
    *green = (BYTE)readdisk(++col, row);
    *red   = (BYTE)readdisk(col+1, row);
}

void writedisk(int col, int row, int color)
{
    int col_subscr;
    long offset;
    char buf[41];
    if (--s_time_to_display < 0)  // time to display status?
    {
        if (driver_diskp())
        {
            std::snprintf(buf, std::size(buf), " writing line %4d",
                    (row >= g_screen_y_dots) ? row-g_screen_y_dots : row); // adjust when potfile
            dvid_status(0, buf);
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
        s_cur_row_base = (long) s_cur_row*s_row_size;
    }
    if (col >= s_row_size)
    {
        return;
    }
    offset = s_cur_row_base + col;
    col_subscr = (short) offset & (BLOCKLEN-1);
    if (s_cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }
    if (s_cur_cache->pixel[col_subscr] != (color & 0xff))
    {
        s_cur_cache->pixel[col_subscr] = (BYTE) color;
        s_cur_cache->dirty = true;
    }
}

bool ToMemDisk(long offset, int size, void *src)
{
    int col_subscr = (int)(offset & (BLOCKLEN - 1));

    if (col_subscr + size > BLOCKLEN)           // access violates  a
    {
        return false;                           //   cache boundary
    }

    if (s_cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }

    std::memcpy((void *) &s_cur_cache->pixel[col_subscr], src, size);
    s_cur_cache->dirty = true;
    return true;
}

void targa_writedisk(unsigned int col, unsigned int row, BYTE red, BYTE green, BYTE blue)
{
    writedisk(col *= 3, row, blue);
    writedisk(++col, row, green);
    writedisk(col+1, row, red);
}

static void findload_cache(long offset) // used by read/write
{
    unsigned int tbloffset;
    unsigned int *fwd_link;
    BYTE *pixelptr = nullptr;
    s_cur_offset = offset; // note this for next reference
    // check if required entry is in cache - lookup by hash
    tbloffset = s_hash_ptr[((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
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
    if (s_cache_lru->dirty) // must write this block before reusing it
    {
        write_cache_lru();
    }
    // remove block at cache_lru from its hash chain
    fwd_link = &s_hash_ptr[(((unsigned short)s_cache_lru->offset >> BLOCKSHIFT) & (HASHSIZE-1))];
    tbloffset = (int)((char *)s_cache_lru - (char *)s_cache_start);
    while (*fwd_link != tbloffset)
    {
        fwd_link = &((cache *)((char *)s_cache_start+*fwd_link))->hashlink;
    }
    *fwd_link = s_cache_lru->hashlink;
    // load block
    s_cache_lru->dirty  = false;
    s_cache_lru->lru    = true;
    s_cache_lru->offset = offset;
    pixelptr = &s_cache_lru->pixel[0];
    if (offset > s_high_offset)  // never been this high before, just clear it
    {
        s_high_offset = offset;
        std::memset(pixelptr, 0, BLOCKLEN);
    }
    else
    {
        if (offset != s_seek_offset)
        {
            mem_seek(offset >> s_pixel_shift);
        }
        s_seek_offset = offset + BLOCKLEN;
        switch (s_pixel_shift)
        {
        case 0:
            for (int i = 0; i < BLOCKLEN; ++i)
            {
                *(pixelptr++) = mem_getc();
            }
            break;

        case 1:
            for (int i = 0; i < BLOCKLEN/2; ++i)
            {
                BYTE const tmpchar = mem_getc();
                *(pixelptr++) = (BYTE)(tmpchar >> 4);
                *(pixelptr++) = (BYTE)(tmpchar & 15);
            }
            break;
        case 2:
            for (int i = 0; i < BLOCKLEN/4; ++i)
            {
                BYTE const tmpchar = mem_getc();
                for (int j = 6; j >= 0; j -= 2)
                {
                    *(pixelptr++) = (BYTE)((tmpchar >> j) & 3);
                }
            }
            break;
        case 3:
            for (int i = 0; i < BLOCKLEN/8; ++i)
            {
                BYTE const tmpchar = mem_getc();
                for (int j = 7; j >= 0; --j)
                {
                    *(pixelptr++) = (BYTE)((tmpchar >> j) & 1);
                }
            }
            break;
        }
    }
    // add new block to its hash chain
    fwd_link = &s_hash_ptr[(((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1))];
    s_cache_lru->hashlink = *fwd_link;
    *fwd_link = (int)((char *)s_cache_lru - (char *)s_cache_start);
    s_cur_cache = s_cache_lru;
}

// lookup for write_cache_lru
static cache *find_cache(long offset)
{
    unsigned int tbloffset;
    cache *ptr1;
    tbloffset = s_hash_ptr[((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1)];
    while (tbloffset != 0xffff)
    {
        ptr1 = (cache *)((char *)s_cache_start + tbloffset);
        if (ptr1->offset == offset)
        {
            return ptr1;
        }
        tbloffset = ptr1->hashlink;
    }
    return nullptr;
}

enum
{
    WRITEGAP = 4 // 1 for no gaps
};

static void  write_cache_lru()
{
    int i;
    BYTE *pixelptr;
    long offset;
    BYTE tmpchar = 0;
    cache *ptr1;
    cache *ptr2;
    // scan back to also write any preceding dirty blocks, skipping small gaps
    ptr1 = s_cache_lru;
    offset = ptr1->offset;
    i = 0;
    while (++i <= WRITEGAP)
    {
        offset -= BLOCKLEN;
        ptr2 = find_cache(offset);
        if (ptr2 != nullptr && ptr2->dirty)
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
        for (int j = 0; j < BLOCKLEN; ++j)
        {
            mem_putc(*(pixelptr++));
        }
        break;
    case 1:
        for (int j = 0; j < BLOCKLEN/2; ++j)
        {
            tmpchar = (BYTE)(*(pixelptr++) << 4);
            tmpchar = (BYTE)(tmpchar + *(pixelptr++));
            mem_putc(tmpchar);
        }
        break;
    case 2:
        for (int j = 0; j < BLOCKLEN/4; ++j)
        {
            for (int k = 6; k >= 0; k -= 2)
            {
                tmpchar = (BYTE)((tmpchar << 2) + *(pixelptr++));
            }
            mem_putc(tmpchar);
        }
        break;
    case 3:
        for (int j = 0; j < BLOCKLEN/8; ++j)
        {
            // clang-format off
            mem_putc((BYTE)
                (((((((*pixelptr << 1
                    | *(pixelptr + 1)) << 1
                    | *(pixelptr + 2)) << 1
                    | *(pixelptr + 3)) << 1
                    | *(pixelptr + 4)) << 1
                    | *(pixelptr + 5)) << 1
                    | *(pixelptr + 6)) << 1
                    | *(pixelptr + 7)));
            // clang-format on
            pixelptr += 8;
        }
        break;
    }
    ptr1->dirty = false;
    offset = ptr1->offset + BLOCKLEN;
    ptr1 = find_cache(offset);
    if (ptr1 != nullptr && ptr1->dirty)
    {
        goto write_stuff;
    }
    i = 1;
    while (++i <= WRITEGAP)
    {
        offset += BLOCKLEN;
        ptr1 = find_cache(offset);
        if (ptr1 != nullptr && ptr1->dirty)
        {
            goto write_seek;
        }
    }
    s_seek_offset = -1; // force a seek before next read
}

// Seek, mem_getc, mem_putc routines follow.
// Note that the calling logic always separates mem_getc and mem_putc
// sequences with a seek between them.  A mem_getc is never followed by
// a mem_putc nor v.v. without a seek between them.
//
static void mem_seek(long offset)        // mem seek
{
    offset += s_header_length;
    s_mem_offset = offset >> BLOCKSHIFT;
    if (s_mem_offset != s_old_mem_offset)
    {
        CopyFromMemoryToHandle(&s_mem_buf[0], (U16)BLOCKLEN, 1L, s_old_mem_offset, s_dv_handle);
        CopyFromHandleToMemory(&s_mem_buf[0], (U16)BLOCKLEN, 1L, s_mem_offset, s_dv_handle);
    }
    s_old_mem_offset = s_mem_offset;
    s_mem_buf_ptr = &s_mem_buf[0] + (offset & (BLOCKLEN - 1));
}

static BYTE  mem_getc()                     // memory get_char
{
    if (s_mem_buf_ptr - &s_mem_buf[0] >= BLOCKLEN)
    {
        CopyFromMemoryToHandle(&s_mem_buf[0], (U16)BLOCKLEN, 1L, s_mem_offset, s_dv_handle);
        s_mem_offset++;
        CopyFromHandleToMemory(&s_mem_buf[0], (U16)BLOCKLEN, 1L, s_mem_offset, s_dv_handle);
        s_mem_buf_ptr = &s_mem_buf[0];
        s_old_mem_offset = s_mem_offset;
    }
    return *(s_mem_buf_ptr++);
}

static void mem_putc(BYTE c)     // memory get_char
{
    if (s_mem_buf_ptr - &s_mem_buf[0] >= BLOCKLEN)
    {
        CopyFromMemoryToHandle(&s_mem_buf[0], (U16)BLOCKLEN, 1L, s_mem_offset, s_dv_handle);
        s_mem_offset++;
        CopyFromHandleToMemory(&s_mem_buf[0], (U16)BLOCKLEN, 1L, s_mem_offset, s_dv_handle);
        s_mem_buf_ptr = &s_mem_buf[0];
        s_old_mem_offset = s_mem_offset;
    }
    *(s_mem_buf_ptr++) = c;
}


void dvid_status(int line, char const *msg)
{
    assert(msg != nullptr);
    const std::string buff = (msg + std::string(40, ' ')).substr(0, 40);
    int attrib = C_DVID_HI;
    if (line >= 100)
    {
        line -= 100;
        attrib = C_STOP_ERR;
    }
    driver_put_string(BOXROW+10+line, BOXCOL+12, attrib, buff);
    driver_hide_text_cursor();
}
