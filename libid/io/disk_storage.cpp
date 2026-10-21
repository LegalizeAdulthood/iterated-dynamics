// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// Disk-backed pixel and byte storage.
//
#include "io/disk_storage.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace id::misc;

namespace id::io
{

enum
{
    BLOCK_LEN = DISK_STORAGE_BLOCK_SIZE,
    BLOCK_SHIFT = 11,
    HASH_SIZE = 2048
};

namespace
{

using DiskOffset = std::int64_t;

// structure of each cache entry
struct Cache
{
    DiskOffset offset;      // pixel offset in image
    Byte pixel[BLOCK_LEN];  // one pixel per byte
    unsigned int hash_link; // ptr to next cache entry with same hash
    bool dirty;             // changed since read?
    bool lru;               // recently used?
};

} // namespace

bool g_disk_16_bit{};                        // storing 16 bit values for continuous potential
bool g_disk_targa{};                         //
bool g_disk_flag{};                          //

static Cache *s_cache_end{};                 //
static Cache *s_cache_lru{};                 //
static Cache *s_cur_cache{};                 //
static Cache *s_cache_start{};               //
static DiskOffset s_high_offset{};           // highwater mark of writes
static DiskOffset s_seek_offset{};           // what we'll get next if we don't seek
static DiskOffset s_cur_offset{};            // offset of last block referenced
static int s_cur_row{};                      //
static DiskOffset s_cur_row_base{};          //
static unsigned int s_hash_ptr[HASH_SIZE]{}; //
static int s_pixel_shift{};                  //
static int s_header_length{};                //
static int s_row_size{};                     // doubles as a disk video not ok flag
static int s_col_size{};                     // sydots, *2 when pot16bit
static std::vector<Byte> s_mem_buf;          //
static MemoryHandle s_dv_handle{};           //
static std::filesystem::path s_targa_disk_path;
static std::filesystem::path s_active_targa_disk_path;
static std::uint64_t s_mem_offset{};         //
static std::uint64_t s_old_mem_offset{};     //
static Byte *s_mem_buf_ptr{};                //

static int get_pixel_shift(int colors);
static DiskOffset block_offset(DiskOffset offset);
static int block_index(DiskOffset offset);
static unsigned int hash_index(DiskOffset offset);
static void find_load_cache(DiskOffset offset);
static Cache *find_cache(DiskOffset offset);
static void write_cache_lru();
static void mem_putc(Byte c);
static Byte mem_getc();
static void mem_seek(std::uint64_t offset);

static DiskStorageProgress s_progress{};

void set_targa_disk_path(const std::filesystem::path &path)
{
    s_targa_disk_path = path;
}

const std::filesystem::path &targa_disk_path()
{
    return s_targa_disk_path;
}

static int get_pixel_shift(const int colors)
{
    int pixel_shift = 3;
    int color_count = 2;
    while (color_count < colors && pixel_shift > 0)
    {
        color_count *= color_count;
        --pixel_shift;
    }
    return pixel_shift;
}

static DiskOffset block_offset(const DiskOffset offset)
{
    return offset & ~(static_cast<DiskOffset>(BLOCK_LEN) - 1);
}

static int block_index(const DiskOffset offset)
{
    return static_cast<int>(offset & (BLOCK_LEN - 1));
}

static unsigned int hash_index(const DiskOffset offset)
{
    return static_cast<unsigned int>((static_cast<std::uint64_t>(offset) >> BLOCK_SHIFT) & (HASH_SIZE - 1));
}

bool disk_video_memory_blocks(const std::int64_t row_size, const std::int64_t col_size, const int colors,
    const int header_length, std::uint64_t &blocks)
{
    blocks = 0;
    if (row_size <= 0 || col_size <= 0 || colors < 2 || header_length < 0)
    {
        return false;
    }
    const std::uint64_t unsigned_row_size = static_cast<std::uint64_t>(row_size);
    const std::uint64_t unsigned_col_size = static_cast<std::uint64_t>(col_size);
    if (unsigned_row_size > std::numeric_limits<std::uint64_t>::max() / unsigned_col_size)
    {
        return false;
    }
    std::uint64_t storage_bytes = unsigned_row_size * unsigned_col_size;
    const int pixel_shift = get_pixel_shift(colors);
    if (pixel_shift > 0)
    {
        const std::uint64_t packed_pixels = (std::uint64_t{1} << pixel_shift) - 1;
        if (storage_bytes > std::numeric_limits<std::uint64_t>::max() - packed_pixels)
        {
            return false;
        }
        storage_bytes = (storage_bytes + packed_pixels) >> pixel_shift;
    }
    if (storage_bytes > std::numeric_limits<std::uint64_t>::max() - (BLOCK_LEN - 1))
    {
        return false;
    }
    // Cache transfers include the padding after the last image byte.
    storage_bytes = (storage_bytes + BLOCK_LEN - 1) / BLOCK_LEN * BLOCK_LEN;
    if (storage_bytes > std::numeric_limits<std::uint64_t>::max() - static_cast<std::uint64_t>(header_length))
    {
        return false;
    }
    storage_bytes += static_cast<std::uint64_t>(header_length);
    if (storage_bytes > std::numeric_limits<std::uint64_t>::max() - (BLOCK_LEN - 1))
    {
        return false;
    }
    blocks = (storage_bytes + BLOCK_LEN - 1) / BLOCK_LEN;
    return blocks > 0;
}

MemoryLocation disk_video_memory_type()
{
    return s_dv_handle ? memory_type(s_dv_handle) : MemoryLocation::NOWHERE;
}

std::vector<Byte> read_targa_header(std::FILE *file, const int length)
{
    const std::unique_ptr<std::FILE, decltype(&std::fclose)> source{file, std::fclose};
    if (source == nullptr || length <= 0 || length > DISK_STORAGE_BLOCK_SIZE)
    {
        throw std::logic_error("Invalid Targa backing header");
    }
    std::vector<Byte> header(static_cast<std::size_t>(length));
    if (std::fseek(source.get(), 0, SEEK_SET) != 0 ||
        std::fread(header.data(), 1, header.size(), source.get()) != header.size())
    {
        throw MemoryError("Cannot read Targa backing header");
    }
    return header;
}

void start_disk_storage(const std::int64_t new_row_size, const std::int64_t new_col_size, const int colors,
    const MemoryHandle backing, const std::vector<Byte> &header)
{
    if (g_disk_flag || !backing)
    {
        throw std::logic_error("Invalid disk storage initialization");
    }
    const std::uint64_t backing_size{backing.size()};
    s_dv_handle = backing;
    g_disk_targa = !header.empty();
    try
    {
        if (new_row_size > std::numeric_limits<int>::max() || new_col_size > std::numeric_limits<int>::max())
        {
            throw MemoryError("*** disk video dimensions are too large ***");
        }
        if (header.size() > BLOCK_LEN)
        {
            throw std::logic_error("Invalid disk storage header");
        }
        s_header_length = static_cast<int>(header.size());
        s_active_targa_disk_path = g_disk_targa ? s_targa_disk_path : std::filesystem::path{};
        std::uint64_t blocks{};
        if (!disk_video_memory_blocks(new_row_size, new_col_size, g_disk_targa ? 256 : colors, s_header_length, blocks))
        {
            throw MemoryError("*** disk video dimensions overflow allocation size ***");
        }
        if (backing_size / BLOCK_LEN < blocks)
        {
            throw std::logic_error("Disk backing allocation is too small");
        }
        s_high_offset = g_disk_targa ? 100000000 : -1;
        s_seek_offset = -1;
        s_cur_offset = -1;
        s_cur_row = -1;
        s_mem_offset = 0;
        s_old_mem_offset = 0;
        s_pixel_shift = g_disk_targa ? 0 : get_pixel_shift(colors);
        s_row_size = static_cast<int>(new_row_size);
        s_col_size = static_cast<int>(new_col_size);
        const std::size_t cache_bytes{static_cast<std::size_t>(DISK_STORAGE_CACHE_KB) << 10};
        s_cache_start = static_cast<Cache *>(std::malloc(cache_bytes));
        if (s_cache_start == nullptr)
        {
            throw MemoryError("*** insufficient free memory for cache buffers ***");
        }
        s_cache_lru = s_cache_start;
        s_cache_end = s_cache_start + cache_bytes / sizeof(*s_cache_start);
        s_mem_buf.assign(BLOCK_LEN, 0);
        std::copy(header.begin(), header.end(), s_mem_buf.begin());
        // preset cache to all invalid entries so we don't need free list logic
        for (unsigned int &elem : s_hash_ptr)
        {
            elem = 0xffff; // 0xffff marks the end of a hash chain
        }
        DiskOffset long_tmp = 100000000;
        for (Cache *ptr1 = s_cache_start; ptr1 < s_cache_end; ++ptr1)
        {
            ptr1->dirty = false;
            ptr1->lru = false;
            long_tmp += BLOCK_LEN;
            unsigned int *fwd_link = &s_hash_ptr[hash_index(long_tmp)];
            ptr1->offset = long_tmp;
            ptr1->hash_link = *fwd_link;
            *fwd_link = static_cast<int>(reinterpret_cast<char *>(ptr1) - reinterpret_cast<char *>(s_cache_start));
        }

        s_mem_buf_ptr = s_mem_buf.data();
        if (!header.empty())
        {
            s_dv_handle.from_memory(header.data(), static_cast<U16>(header.size()), 1, 0);
        }
        s_progress = {};
        g_disk_flag = true;
    }
    catch (...)
    {
        end_disk();
        throw;
    }
}

DiskStorageProgress disk_storage_progress()
{
    return s_progress;
}

void end_disk()
{
    const bool trim_targa{g_disk_flag && g_disk_targa && !s_active_targa_disk_path.empty()};
    const std::uintmax_t targa_size{static_cast<std::uintmax_t>(s_header_length) +
        static_cast<std::uintmax_t>(s_row_size) * static_cast<std::uintmax_t>(s_col_size)};

    std::exception_ptr failure;
    if (g_disk_flag && g_disk_targa && s_cache_start != nullptr)
    {
        try
        {
            for (s_cache_lru = s_cache_start; s_cache_lru < s_cache_end; ++s_cache_lru)
            {
                if (s_cache_lru->dirty)
                {
                    write_cache_lru();
                }
            }
        }
        catch (...)
        {
            failure = std::current_exception();
        }
    }

    if (s_dv_handle)
    {
        memory_release(s_dv_handle);
        s_dv_handle = MemoryHandle{};
    }
    if (trim_targa)
    {
        std::error_code ignore;
        std::filesystem::resize_file(s_active_targa_disk_path, targa_size, ignore);
    }
    if (s_cache_start != nullptr)
    {
        free(s_cache_start);
        s_cache_start = nullptr;
    }
    s_cache_end = nullptr;
    s_cache_lru = nullptr;
    s_cur_cache = nullptr;
    s_mem_buf_ptr = nullptr;
    s_mem_buf.clear();
    s_targa_disk_path.clear();
    s_active_targa_disk_path.clear();
    g_disk_flag = false;
    s_row_size = 0;
    s_col_size = 0;
    s_progress = {};
    g_disk_16_bit = false;
    if (failure)
    {
        std::rethrow_exception(failure);
    }
}

int read_disk_pixel(const int col, int row)
{
    if (!g_disk_flag)
    {
        throw std::logic_error("Disk storage is not initialized");
    }
    s_progress = {DiskStorageOperation::READ, row};
    if (row != s_cur_row)                 // try to avoid ghastly code generated for multiply
    {
        if (row < 0 || row >= s_col_size) // while we're at it avoid this test if not needed
        {
            return 0;
        }
        s_cur_row = row;
        s_cur_row_base = static_cast<DiskOffset>(s_cur_row) * s_row_size;
    }
    if (col < 0 || col >= s_row_size)
    {
        return 0;
    }
    const DiskOffset offset = s_cur_row_base + col;
    const int col_index = block_index(offset); // offset within cache entry
    if (s_cur_offset != block_offset(offset))  // same entry as last ref?
    {
        find_load_cache(block_offset(offset));
    }
    return s_cur_cache->pixel[col_index];
}

bool from_mem_disk(const std::int64_t offset, const int size, void *dest)
{
    if (!g_disk_flag)
    {
        throw std::logic_error("Disk storage is not initialized");
    }
    if (offset < 0 || size < 0)
    {
        return false;
    }
    const int col_index = block_index(offset);
    if (col_index + size > BLOCK_LEN)         // access violates  a
    {
        return false;                         //   cache boundary
    }
    if (s_cur_offset != block_offset(offset)) // same entry as last ref?
    {
        find_load_cache(block_offset(offset));
    }
    std::memcpy(dest, &s_cur_cache->pixel[col_index], size);
    s_cur_cache->dirty = false;
    return true;
}

void read_targa_pixel(unsigned int col, const unsigned int row, Byte *red, Byte *green, Byte *blue)
{
    col *= 3;
    *blue = static_cast<Byte>(read_disk_pixel(col, row));
    *green = static_cast<Byte>(read_disk_pixel(++col, row));
    *red = static_cast<Byte>(read_disk_pixel(col + 1, row));
}

void write_disk_pixel(const int col, int row, const int color)
{
    if (!g_disk_flag)
    {
        throw std::logic_error("Disk storage is not initialized");
    }
    s_progress = {DiskStorageOperation::WRITE, row};
    if (row != s_cur_row)                 // try to avoid ghastly code generated for multiply
    {
        if (row < 0 || row >= s_col_size) // while we're at it avoid this test if not needed
        {
            return;
        }
        s_cur_row = row;
        s_cur_row_base = static_cast<DiskOffset>(s_cur_row) * s_row_size;
    }
    if (col < 0 || col >= s_row_size)
    {
        return;
    }
    const DiskOffset offset = s_cur_row_base + col;
    const int col_index = block_index(offset);
    if (s_cur_offset != block_offset(offset)) // same entry as last ref?
    {
        find_load_cache(block_offset(offset));
    }
    if (s_cur_cache->pixel[col_index] != (color & 0xff))
    {
        s_cur_cache->pixel[col_index] = static_cast<Byte>(color);
        s_cur_cache->dirty = true;
    }
}

bool to_mem_disk(const std::int64_t offset, const int size, const void *src)
{
    if (!g_disk_flag)
    {
        throw std::logic_error("Disk storage is not initialized");
    }
    if (offset < 0 || size < 0)
    {
        return false;
    }
    const int col_index = block_index(offset);

    if (col_index + size > BLOCK_LEN)         // access violates  a
    {
        return false;                         //   cache boundary
    }

    if (s_cur_offset != block_offset(offset)) // same entry as last ref?
    {
        find_load_cache(block_offset(offset));
    }

    std::memcpy(&s_cur_cache->pixel[col_index], src, size);
    s_cur_cache->dirty = true;
    return true;
}

void write_targa_pixel(unsigned int col, const unsigned int row, const Byte red, const Byte green, const Byte blue)
{
    write_disk_pixel(col *= 3, row, blue);
    write_disk_pixel(++col, row, green);
    write_disk_pixel(col + 1, row, red);
}

static void find_load_cache(const DiskOffset offset) // used by read/write
{
    s_cur_offset = offset;                           // note this for next reference
    // check if required entry is in cache - lookup by hash
    unsigned int tbl_offset = s_hash_ptr[hash_index(offset)];
    while (tbl_offset != 0xffff)           // follow the hash chain
    {
        s_cur_cache = reinterpret_cast<Cache *>(reinterpret_cast<char *>(s_cache_start) + tbl_offset);
        if (s_cur_cache->offset == offset) // great, it is in the cache
        {
            s_cur_cache->lru = true;
            return;
        }
        tbl_offset = s_cur_cache->hash_link;
    }
    // must load the cache entry from backing store
    while (true) // look around for something not recently used
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
    unsigned int *fwd_link = &s_hash_ptr[hash_index(s_cache_lru->offset)];
    tbl_offset = static_cast<int>(reinterpret_cast<char *>(s_cache_lru) - reinterpret_cast<char *>(s_cache_start));
    while (*fwd_link != tbl_offset)
    {
        fwd_link = &reinterpret_cast<Cache *>(reinterpret_cast<char *>(s_cache_start) + *fwd_link)->hash_link;
    }
    *fwd_link = s_cache_lru->hash_link;
    // load block
    s_cache_lru->dirty = false;
    s_cache_lru->lru = true;
    s_cache_lru->offset = offset;
    Byte *pixel_ptr = &s_cache_lru->pixel[0];
    if (offset > s_high_offset) // never been this high before, just clear it
    {
        s_high_offset = offset;
        std::memset(pixel_ptr, 0, BLOCK_LEN);
    }
    else
    {
        if (offset != s_seek_offset)
        {
            mem_seek(static_cast<std::uint64_t>(offset) >> s_pixel_shift);
        }
        s_seek_offset = offset + BLOCK_LEN;
        switch (s_pixel_shift)
        {
        case 0:
            for (int i = 0; i < BLOCK_LEN; ++i)
            {
                *pixel_ptr++ = mem_getc();
            }
            break;

        case 1:
            for (int i = 0; i < BLOCK_LEN / 2; ++i)
            {
                const Byte tmp_char = mem_getc();
                *pixel_ptr++ = static_cast<Byte>(tmp_char >> 4);
                *pixel_ptr++ = static_cast<Byte>(tmp_char & 15);
            }
            break;
        case 2:
            for (int i = 0; i < BLOCK_LEN / 4; ++i)
            {
                const Byte tmp_char = mem_getc();
                for (int j = 6; j >= 0; j -= 2)
                {
                    *pixel_ptr++ = static_cast<Byte>(tmp_char >> j & 3);
                }
            }
            break;
        case 3:
            for (int i = 0; i < BLOCK_LEN / 8; ++i)
            {
                const Byte tmp_char = mem_getc();
                for (int j = 7; j >= 0; --j)
                {
                    *pixel_ptr++ = static_cast<Byte>(tmp_char >> j & 1);
                }
            }
            break;
        }
    }
    // add new block to its hash chain
    fwd_link = &s_hash_ptr[hash_index(offset)];
    s_cache_lru->hash_link = *fwd_link;
    *fwd_link = static_cast<int>(reinterpret_cast<char *>(s_cache_lru) - reinterpret_cast<char *>(s_cache_start));
    s_cur_cache = s_cache_lru;
}

// lookup for write_cache_lru
static Cache *find_cache(const DiskOffset offset)
{
    unsigned int tbl_offset = s_hash_ptr[hash_index(offset)];
    while (tbl_offset != 0xffff)
    {
        Cache *ptr1 = reinterpret_cast<Cache *>(reinterpret_cast<char *>(s_cache_start) + tbl_offset);
        if (ptr1->offset == offset)
        {
            return ptr1;
        }
        tbl_offset = ptr1->hash_link;
    }
    return nullptr;
}

enum
{
    WRITE_GAP = 4 // 1 for no gaps
};

static void write_cache_lru()
{
    Byte tmp_char = 0;
    // scan back to also write any preceding dirty blocks, skipping small gaps
    Cache *ptr1 = s_cache_lru;
    DiskOffset offset = ptr1->offset;
    int i = 0;
    while (++i <= WRITE_GAP)
    {
        offset -= BLOCK_LEN;
        Cache *ptr2 = find_cache(offset);
        if (ptr2 != nullptr && ptr2->dirty)
        {
            ptr1 = ptr2;
            i = 0;
        }
    }
    // write all consecutive dirty blocks (often whole cache in 1pass modes)
    // keep going past small gaps

    bool contiguous = false;
    while (ptr1 != nullptr)
    {
        if (!contiguous)
        {
            mem_seek(static_cast<std::uint64_t>(ptr1->offset) >> s_pixel_shift);
        }
        const Byte *pixel_ptr = &ptr1->pixel[0];
        switch (s_pixel_shift)
        {
        case 0:
            for (int j = 0; j < BLOCK_LEN; ++j)
            {
                mem_putc(*pixel_ptr++);
            }
            break;
        case 1:
            for (int j = 0; j < BLOCK_LEN / 2; ++j)
            {
                tmp_char = static_cast<Byte>(*pixel_ptr++ << 4);
                tmp_char = static_cast<Byte>(tmp_char + *pixel_ptr++);
                mem_putc(tmp_char);
            }
            break;
        case 2:
            for (int j = 0; j < BLOCK_LEN / 4; ++j)
            {
                for (int k = 6; k >= 0; k -= 2)
                {
                    tmp_char = static_cast<Byte>((tmp_char << 2) + *pixel_ptr++);
                }
                mem_putc(tmp_char);
            }
            break;
        case 3:
            for (int j = 0; j < BLOCK_LEN / 8; ++j)
            {
                // clang-format off
                mem_putc(static_cast<Byte>(((((((*pixel_ptr << 1
                        | *(pixel_ptr + 1)) << 1
                        | *(pixel_ptr + 2)) << 1
                        | *(pixel_ptr + 3)) << 1
                        | *(pixel_ptr + 4)) << 1
                        | *(pixel_ptr + 5)) << 1
                        | *(pixel_ptr + 6)) << 1
                        | *(pixel_ptr + 7)));
                // clang-format on
                pixel_ptr += 8;
            }
            break;
        }
        ptr1->dirty = false;
        offset = ptr1->offset + BLOCK_LEN;
        ptr1 = find_cache(offset);
        contiguous = ptr1 != nullptr && ptr1->dirty;
        if (contiguous)
        {
            continue;
        }
        ptr1 = nullptr;
        i = 1;
        while (++i <= WRITE_GAP)
        {
            offset += BLOCK_LEN;
            Cache *candidate = find_cache(offset);
            if (candidate != nullptr && candidate->dirty)
            {
                ptr1 = candidate;
                break;
            }
        }
    }
    s_seek_offset = -1; // force a seek before next read
}

// Seek, mem_getc, mem_putc routines follow.
// Note that the calling logic always separates mem_getc and mem_putc
// sequences with a seek between them.  A mem_getc is never followed by
// a mem_putc nor v.v. without a seek between them.
//
static void mem_seek(std::uint64_t offset) // mem seek
{
    offset += static_cast<std::uint64_t>(s_header_length);
    s_mem_offset = offset >> BLOCK_SHIFT;
    if (s_mem_offset != s_old_mem_offset)
    {
        s_dv_handle.from_memory(s_mem_buf.data(), BLOCK_LEN, 1L, s_old_mem_offset);
        s_dv_handle.to_memory(s_mem_buf.data(), BLOCK_LEN, 1L, s_mem_offset);
    }
    s_old_mem_offset = s_mem_offset;
    s_mem_buf_ptr = s_mem_buf.data() + (offset & BLOCK_LEN - 1);
}

static Byte mem_getc() // memory get_char
{
    if (s_mem_buf_ptr - s_mem_buf.data() >= BLOCK_LEN)
    {
        s_dv_handle.from_memory(s_mem_buf.data(), BLOCK_LEN, 1L, s_mem_offset);
        s_mem_offset++;
        s_dv_handle.to_memory(s_mem_buf.data(), BLOCK_LEN, 1L, s_mem_offset);
        s_mem_buf_ptr = s_mem_buf.data();
        s_old_mem_offset = s_mem_offset;
    }
    return *s_mem_buf_ptr++;
}

static void mem_putc(const Byte c) // memory get_char
{
    if (s_mem_buf_ptr - s_mem_buf.data() >= BLOCK_LEN)
    {
        s_dv_handle.from_memory(s_mem_buf.data(), BLOCK_LEN, 1L, s_mem_offset);
        s_mem_offset++;
        s_dv_handle.to_memory(s_mem_buf.data(), BLOCK_LEN, 1L, s_mem_offset);
        s_mem_buf_ptr = s_mem_buf.data();
        s_old_mem_offset = s_mem_offset;
    }
    *s_mem_buf_ptr++ = c;
}

} // namespace id::io
