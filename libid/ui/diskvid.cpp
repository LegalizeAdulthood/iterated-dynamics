// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/diskvid.h"

#include "engine/LogicalScreen.h"
#include "engine/text_color.h"
#include "engine/VideoInfo.h"
#include "io/disk_storage.h"
#include "io/encoder.h"
#include "io/get_disk_space.h"
#include "misc/debug_flags.h"
#include "misc/Driver.h"
#include "misc/memory.h"
#include "ui/help_title.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"
#include "ui/video.h"

#include <fmt/format.h>

#include <chrono>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

namespace
{

constexpr int BOX_ROW{6};
constexpr int BOX_COL{11};
constexpr int BOX_WIDTH{57};
constexpr int BOX_DEPTH{12};

std::chrono::time_point<std::chrono::high_resolution_clock> s_last_status{};

} // namespace

bool prepare_disk_storage(const std::int64_t row_size, const std::int64_t col_size, const int colors,
    const std::vector<Byte> &header, const std::filesystem::path &backing_path)
{
    const bool targa{!header.empty()};
    if (g_disk_flag)
    {
        end_disk();
    }
    g_disk_targa = targa;
    if (targa)
    {
        set_targa_disk_path(backing_path);
    }
    if (driver_is_disk())
    {
        help_title();
        driver_set_attr(1, 0, C_DVID_BKGRD, 24 * 80);
        for (int i = 0; i < BOX_DEPTH; ++i)
        {
            driver_set_attr(BOX_ROW + i, BOX_COL, C_DVID_LO, BOX_WIDTH);
        }
        driver_put_string(BOX_ROW + 2, BOX_COL + 4, C_DVID_HI, "'Disk-Video' mode");
        driver_put_string(BOX_ROW + 4, BOX_COL + 4, C_DVID_LO,
            fmt::format("Screen resolution: {:d} x {:d}", g_screen_x_dots, g_screen_y_dots));
        if (targa)
        {
            driver_put_string(-1, -1, C_DVID_LO, "  24 bit Targa");
        }
        else
        {
            driver_put_string(-1, -1, C_DVID_LO, "  Colors: ");
            driver_put_string(-1, -1, C_DVID_LO, std::to_string(colors));
        }
        driver_put_string(BOX_ROW + 8, BOX_COL + 4, C_DVID_LO, "Save name: " + g_save_filename.string());
        driver_put_string(BOX_ROW + 10, BOX_COL + 4, C_DVID_LO, "Status:");
        dvid_status(0, "allocating the 'screen'");
    }
    if (row_size > std::numeric_limits<int>::max() || col_size > std::numeric_limits<int>::max())
    {
        stop_msg("*** disk video dimensions are too large ***");
        return false;
    }
    std::uint64_t blocks{};
    if (!disk_video_memory_blocks(row_size, col_size, targa ? 256 : colors, static_cast<int>(header.size()), blocks))
    {
        stop_msg("*** disk video dimensions overflow allocation size ***");
        return false;
    }
    if (driver_is_disk())
    {
        driver_put_string(BOX_ROW + 6, BOX_COL + 4, C_DVID_LO, fmt::format("Cache size: {:d}K", DISK_STORAGE_CACHE_KB));
    }
    const MemoryLocation location{targa ? MemoryLocation::DISK : MemoryLocation::MEMORY};
    MemoryHandle backing;
    for (;;)
    {
        try
        {
            backing = memory_alloc(DISK_STORAGE_BLOCK_SIZE, blocks, location, backing_path);
            break;
        }
        catch (const MemoryError &error)
        {
            if (stop_msg(StopMsgFlags::CANCEL, std::string{error.what()} + "\nPress a key to retry."))
            {
                g_good_mode = false;
                return false;
            }
        }
    }
    try
    {
        const MemoryLocation actual{memory_type(backing)};
        if (location != actual && g_debug_flag == DebugFlags::DISPLAY_MEMORY_STATISTICS)
        {
            stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER,
                fmt::format("Asked for {}, allocated {} bytes of {}, handle = {}.",
                    location == MemoryLocation::MEMORY ? "memory" : "disk", backing.size(),
                    actual == MemoryLocation::MEMORY ? "memory" : "disk", backing.index));
            stop_msg(StopMsgFlags::INFO_ONLY | StopMsgFlags::NO_BUZZER, fmt::format("disk={}", get_disk_space()));
        }
    }
    catch (...)
    {
        memory_release(backing);
        throw;
    }
    try
    {
        start_disk_storage(row_size, col_size, colors, backing, header);
    }
    catch (const std::bad_alloc &)
    {
        stop_msg("*** insufficient free memory for cache buffers ***");
        g_good_mode = false;
        return false;
    }
    catch (const MemoryError &error)
    {
        stop_msg(error.what());
        g_good_mode = false;
        return false;
    }
    if (driver_is_disk())
    {
        driver_put_string(BOX_ROW + 2, BOX_COL + 23, C_DVID_LO,
            disk_video_memory_type() == MemoryLocation::DISK ? "Using your Disk Drive" : "Using your memory");
        dvid_status(0, "");
    }
    return true;
}

bool g_good_mode{};

int start_disk()
{
    return prepare_disk_storage(g_screen_x_dots, g_screen_y_dots, g_colors) ? 0 : -1;
}

bool prepare_potential_storage()
{
    if (driver_is_disk())
    {
        end_disk();
    }
    else
    {
        show_temp_msg("clearing 16bit pot work area");
    }
    const bool available{prepare_disk_storage(g_screen_x_dots, static_cast<std::int64_t>(g_screen_y_dots) * 2, g_colors)};
    clear_temp_msg();
    if (available)
    {
        g_disk_16_bit = true;
    }
    return available;
}

bool prepare_targa_storage(const TargaStorageRequest &request)
{
    if (request.path.empty() || request.header.empty() || request.width <= 0 || request.height <= 0)
    {
        throw std::logic_error("Targa storage requires a prepared file");
    }
    if (driver_is_disk())
    {
        end_disk();
        set_null_video();
    }
    return prepare_disk_storage(
        static_cast<std::int64_t>(request.width) * 3, request.height, 256, request.header, request.path);
}

void display_disk_storage_progress()
{
    if (!g_disk_flag)
    {
        return;
    }
    using namespace std::literals::chrono_literals;
    if (std::chrono::high_resolution_clock::now() - s_last_status < 100ms)
    {
        return;
    }
    const DiskStorageProgress progress{disk_storage_progress()};
    if (progress.operation != DiskStorageOperation::NONE && driver_is_disk())
    {
        dvid_status(0,
            fmt::format(" {} line {:4d}", progress.operation == DiskStorageOperation::READ ? "reading" : "writing",
                progress.row >= g_screen_y_dots ? progress.row - g_screen_y_dots : progress.row));
    }
}

void dvid_status(int line, const char *msg)
{
    using namespace std::literals::chrono_literals;
    const std::chrono::time_point now{std::chrono::high_resolution_clock::now()};
    if (now - s_last_status < 100ms)
    {
        return;
    }
    s_last_status = now;
    if (msg == nullptr)
    {
        throw std::logic_error("Disk video status text must be present");
    }
    const std::string buff = (msg + std::string(40, ' ')).substr(0, 40);
    int attrib = C_DVID_HI;
    if (line >= 100)
    {
        line -= 100;
        attrib = C_STOP_ERR;
    }
    driver_put_string(BOX_ROW + 10 + line, BOX_COL + 12, attrib, buff);
    driver_hide_text_cursor();
}

} // namespace id::ui
