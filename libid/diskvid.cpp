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

#define BOXROW   6
#define BOXCOL   11
#define BOXWIDTH 57
#define BOXDEPTH 12

bool g_disk_16_bit = false;                 // storing 16 bit values for continuous potential

static int timetodisplay;
static std::FILE *fp = nullptr;
bool g_disk_targa = false;
bool g_disk_flag = false;
bool g_good_mode = false;        // if non-zero, OK to read/write pixels

#define BLOCKLEN 2048   // must be a power of 2, must match next
#define BLOCKSHIFT 11   // must match above
#define CACHEMIN 4      // minimum cache size in Kbytes
#define CACHEMAX 64     // maximum cache size in Kbytes
#define FREEMEM  33     // try to leave this much memory unallocated
#define HASHSIZE 1024   // power of 2, near CACHEMAX/(BLOCKLEN+8)

struct cache                // structure of each cache entry
{
    long offset;            // pixel offset in image
    BYTE pixel[BLOCKLEN];   // one pixel per byte
    unsigned int hashlink;  // ptr to next cache entry with same hash
    bool dirty;             // changed since read?
    bool lru;               // recently used?
};
static cache *cache_end = nullptr;
static cache *cache_lru = nullptr;
static cache *cur_cache = nullptr;
static cache *cache_start = nullptr;
static long high_offset;           // highwater mark of writes
static long seek_offset;           // what we'll get next if we don't seek
static long cur_offset;            // offset of last block referenced
static int cur_row;
static long cur_row_base;
static unsigned int hash_ptr[HASHSIZE] = { 0 };
static int pixelshift;
static int headerlength;
static int rowsize = 0;   // doubles as a disk video not ok flag
static int colsize;       // sydots, *2 when pot16bit

static std::vector<BYTE> membuf;
static U16 dv_handle = 0;
static long memoffset = 0;
static long oldmemoffset = 0;
static BYTE *membufptr;

static void findload_cache(long);
static cache *find_cache(long);
static void  write_cache_lru();
static void mem_putc(BYTE);
static BYTE  mem_getc();
static void mem_seek(long);

int startdisk()
{
    headerlength = 0;
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
    headerlength = 0;
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
    headerlength = overhead;
    fp = targafp;
    g_disk_targa = true;
    i = common_startdisk(g_logical_screen_x_dots*3, g_logical_screen_y_dots, g_colors);
    high_offset = 100000000L; // targa not necessarily init'd to zeros

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
    high_offset = -1;
    seek_offset = high_offset;
    cur_offset = seek_offset;
    cur_row    = -1;
    if (g_disk_targa)
    {
        pixelshift = 0;
    }
    else
    {
        pixelshift = 3;
        int i = 2;
        while (i < colors)
        {
            i *= i;
            --pixelshift;
        }
    }
    timetodisplay = bf_math != bf_math_type::NONE ? 10 : 1000;  // time-to-g_driver-status counter

    constexpr unsigned int cache_size = CACHEMAX;
    long longtmp = (long) cache_size << 10;
    cache_start = (cache *)malloc(longtmp);
    if (cache_size == 64)
    {
        --longtmp; // safety for next line
    }
    cache_lru = cache_start;
    cache_end = cache_lru + longtmp/sizeof(*cache_start);
    membuf.resize(BLOCKLEN);
    if (cache_start == nullptr)
    {
        stopmsg(STOPMSG_NONE,
            "*** insufficient free memory for cache buffers ***");
        return -1;
    }
    if (driver_diskp())
    {
        driver_put_string(BOXROW + 6, BOXCOL + 4, C_DVID_LO, "Cache size: " + std::to_string(cache_size) + "K");
    }

    // preset cache to all invalid entries so we don't need free list logic
    for (auto &elem : hash_ptr)
    {
        elem = 0xffff; // 0xffff marks the end of a hash chain
    }
    longtmp = 100000000L;
    for (cache *ptr1 = cache_start; ptr1 < cache_end; ++ptr1)
    {
        ptr1->dirty = false;
        ptr1->lru = false;
        longtmp += BLOCKLEN;
        unsigned int *fwd_link = &hash_ptr[((unsigned short) longtmp >> BLOCKSHIFT) & (HASHSIZE - 1)];
        ptr1->offset = longtmp;
        ptr1->hashlink = *fwd_link;
        *fwd_link = (int)((char *)ptr1 - (char *)cache_start);
    }

    long memorysize = (long) (newcolsize) *newrowsize + headerlength;
    {
        const int i = (short) memorysize & (BLOCKLEN - 1);
        if (i != 0)
        {
            memorysize += BLOCKLEN - i;
        }
    }
    memorysize >>= pixelshift;
    memorysize >>= BLOCKSHIFT;
    g_disk_flag = true;
    rowsize = (unsigned int) newrowsize;
    colsize = (unsigned int) newcolsize;

    if (g_disk_targa)
    {
        // Retrieve the header information first
        std::fseek(fp, 0L, SEEK_SET);
        for (int i = 0; i < headerlength; i++)
        {
            membuf[i] = (BYTE)fgetc(fp);
        }
        std::fclose(fp);
        dv_handle = MemoryAlloc((U16)BLOCKLEN, memorysize, DISK);
    }
    else
    {
        dv_handle = MemoryAlloc((U16)BLOCKLEN, memorysize, MEMORY);
    }
    if (dv_handle == 0)
    {
        stopmsg(STOPMSG_NONE, "*** insufficient free memory/disk space ***");
        g_good_mode = false;
        rowsize = 0;
        return -1;
    }

    if (driver_diskp())
    {
        driver_put_string(BOXROW+2, BOXCOL+23, C_DVID_LO,
                          (MemoryType(dv_handle) == DISK) ? "Using your Disk Drive" : "Using your memory");
    }

    membufptr = &membuf[0];

    if (g_disk_targa)
    {
        // Put header information in the file
        CopyFromMemoryToHandle(&membuf[0], (U16)headerlength, 1L, 0, dv_handle);
    }
    else
    {
        for (long offset = 0; offset < memorysize; offset++)
        {
            SetMemory(0, (U16)BLOCKLEN, 1L, offset, dv_handle);
            if (driver_key_pressed())           // user interrupt
            {
                // esc to cancel, else continue
                if (stopmsg(STOPMSG_CANCEL, "Disk Video initialization interrupted:\n"))
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
    if (fp != nullptr)
    {
        if (g_disk_targa) // flush the cache
        {
            for (cache_lru = cache_start; cache_lru < cache_end; ++cache_lru)
            {
                if (cache_lru->dirty)
                {
                    write_cache_lru();
                }
            }
        }
        std::fclose(fp);
        fp = nullptr;
    }

    if (dv_handle != 0)
    {
        MemoryRelease(dv_handle);
        dv_handle = 0;
    }
    if (cache_start != nullptr)
    {
        free((void *)cache_start);
        cache_start = nullptr;
    }
    membuf.clear();
    g_disk_flag = false;
    rowsize = 0;
    g_disk_16_bit = false;
}

int readdisk(int col, int row)
{
    int col_subscr;
    long offset;
    char buf[41];
    if (--timetodisplay < 0)  // time to g_driver status?
    {
        if (driver_diskp())
        {
            std::snprintf(buf, std::size(buf), " reading line %4d",
                    (row >= g_screen_y_dots) ? row-g_screen_y_dots : row); // adjust when potfile
            dvid_status(0, buf);
        }
        if (bf_math != bf_math_type::NONE)
        {
            timetodisplay = 10;  // time-to-g_driver-status counter
        }
        else
        {
            timetodisplay = 1000;  // time-to-g_driver-status counter
        }
    }
    if (row != cur_row) // try to avoid ghastly code generated for multiply
    {
        if (row >= colsize) // while we're at it avoid this test if not needed
        {
            return 0;
        }
        cur_row = row;
        cur_row_base = (long) cur_row * rowsize;
    }
    if (col >= rowsize)
    {
        return 0;
    }
    offset = cur_row_base + col;
    col_subscr = (short) offset & (BLOCKLEN-1); // offset within cache entry
    if (cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }
    return cur_cache->pixel[col_subscr];
}

int FromMemDisk(long offset, int size, void *dest)
{
    int col_subscr = (int)(offset & (BLOCKLEN - 1));
    if (col_subscr + size > BLOCKLEN)            // access violates  a
    {
        return 0;                                 //   cache boundary
    }
    if (cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }
    std::memcpy(dest, (void *) &cur_cache->pixel[col_subscr], size);
    cur_cache->dirty = false;
    return 1;
}


void targa_readdisk(unsigned int col, unsigned int row,
                    BYTE *red, BYTE *green, BYTE *blue)
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
    if (--timetodisplay < 0)  // time to display status?
    {
        if (driver_diskp())
        {
            std::snprintf(buf, std::size(buf), " writing line %4d",
                    (row >= g_screen_y_dots) ? row-g_screen_y_dots : row); // adjust when potfile
            dvid_status(0, buf);
        }
        timetodisplay = 1000;
    }
    if (row != (unsigned int) cur_row)     // try to avoid ghastly code generated for multiply
    {
        if (row >= colsize) // while we're at it avoid this test if not needed
        {
            return;
        }
        cur_row = row;
        cur_row_base = (long) cur_row*rowsize;
    }
    if (col >= rowsize)
    {
        return;
    }
    offset = cur_row_base + col;
    col_subscr = (short) offset & (BLOCKLEN-1);
    if (cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }
    if (cur_cache->pixel[col_subscr] != (color & 0xff))
    {
        cur_cache->pixel[col_subscr] = (BYTE) color;
        cur_cache->dirty = true;
    }
}

bool ToMemDisk(long offset, int size, void *src)
{
    int col_subscr = (int)(offset & (BLOCKLEN - 1));

    if (col_subscr + size > BLOCKLEN)           // access violates  a
    {
        return false;                           //   cache boundary
    }

    if (cur_offset != (offset & (0L-BLOCKLEN))) // same entry as last ref?
    {
        findload_cache(offset & (0L-BLOCKLEN));
    }

    std::memcpy((void *) &cur_cache->pixel[col_subscr], src, size);
    cur_cache->dirty = true;
    return true;
}

void targa_writedisk(unsigned int col, unsigned int row,
                     BYTE red, BYTE green, BYTE blue)
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
    cur_offset = offset; // note this for next reference
    // check if required entry is in cache - lookup by hash
    tbloffset = hash_ptr[((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1) ];
    while (tbloffset != 0xffff)  // follow the hash chain
    {
        cur_cache = (cache *)((char *)cache_start + tbloffset);
        if (cur_cache->offset == offset)  // great, it is in the cache
        {
            cur_cache->lru = true;
            return;
        }
        tbloffset = cur_cache->hashlink;
    }
    // must load the cache entry from backing store
    while (true)  // look around for something not recently used
    {
        if (++cache_lru >= cache_end)
        {
            cache_lru = cache_start;
        }
        if (!cache_lru->lru)
        {
            break;
        }
        cache_lru->lru = false;
    }
    if (cache_lru->dirty) // must write this block before reusing it
    {
        write_cache_lru();
    }
    // remove block at cache_lru from its hash chain
    fwd_link = &hash_ptr[(((unsigned short)cache_lru->offset >> BLOCKSHIFT) & (HASHSIZE-1))];
    tbloffset = (int)((char *)cache_lru - (char *)cache_start);
    while (*fwd_link != tbloffset)
    {
        fwd_link = &((cache *)((char *)cache_start+*fwd_link))->hashlink;
    }
    *fwd_link = cache_lru->hashlink;
    // load block
    cache_lru->dirty  = false;
    cache_lru->lru    = true;
    cache_lru->offset = offset;
    pixelptr = &cache_lru->pixel[0];
    if (offset > high_offset)  // never been this high before, just clear it
    {
        high_offset = offset;
        std::memset(pixelptr, 0, BLOCKLEN);
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
    fwd_link = &hash_ptr[(((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1))];
    cache_lru->hashlink = *fwd_link;
    *fwd_link = (int)((char *)cache_lru - (char *)cache_start);
    cur_cache = cache_lru;
}

// lookup for write_cache_lru
static cache *find_cache(long offset)
{
    unsigned int tbloffset;
    cache *ptr1;
    tbloffset = hash_ptr[((unsigned short)offset >> BLOCKSHIFT) & (HASHSIZE-1)];
    while (tbloffset != 0xffff)
    {
        ptr1 = (cache *)((char *)cache_start + tbloffset);
        if (ptr1->offset == offset)
        {
            return ptr1;
        }
        tbloffset = ptr1->hashlink;
    }
    return nullptr;
}

static void  write_cache_lru()
{
    int i;
    BYTE *pixelptr;
    long offset;
    BYTE tmpchar = 0;
    cache *ptr1, *ptr2;
#define WRITEGAP 4 // 1 for no gaps
    // scan back to also write any preceding dirty blocks, skipping small gaps
    ptr1 = cache_lru;
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
    mem_seek(ptr1->offset >> pixelshift);

write_stuff:
    pixelptr = &ptr1->pixel[0];
    switch (pixelshift)
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
    seek_offset = -1; // force a seek before next read
}

// Seek, mem_getc, mem_putc routines follow.
// Note that the calling logic always separates mem_getc and mem_putc
// sequences with a seek between them.  A mem_getc is never followed by
// a mem_putc nor v.v. without a seek between them.
//
static void mem_seek(long offset)        // mem seek
{
    offset += headerlength;
    memoffset = offset >> BLOCKSHIFT;
    if (memoffset != oldmemoffset)
    {
        CopyFromMemoryToHandle(&membuf[0], (U16)BLOCKLEN, 1L, oldmemoffset, dv_handle);
        CopyFromHandleToMemory(&membuf[0], (U16)BLOCKLEN, 1L, memoffset, dv_handle);
    }
    oldmemoffset = memoffset;
    membufptr = &membuf[0] + (offset & (BLOCKLEN - 1));
}

static BYTE  mem_getc()                     // memory get_char
{
    if (membufptr - &membuf[0] >= BLOCKLEN)
    {
        CopyFromMemoryToHandle(&membuf[0], (U16)BLOCKLEN, 1L, memoffset, dv_handle);
        memoffset++;
        CopyFromHandleToMemory(&membuf[0], (U16)BLOCKLEN, 1L, memoffset, dv_handle);
        membufptr = &membuf[0];
        oldmemoffset = memoffset;
    }
    return *(membufptr++);
}

static void mem_putc(BYTE c)     // memory get_char
{
    if (membufptr - &membuf[0] >= BLOCKLEN)
    {
        CopyFromMemoryToHandle(&membuf[0], (U16)BLOCKLEN, 1L, memoffset, dv_handle);
        memoffset++;
        CopyFromHandleToMemory(&membuf[0], (U16)BLOCKLEN, 1L, memoffset, dv_handle);
        membufptr = &membuf[0];
        oldmemoffset = memoffset;
    }
    *(membufptr++) = c;
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
