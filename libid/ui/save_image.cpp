// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "ui/save_image.h"

#include "engine/calcfrac.h"
#include "engine/cmdfiles.h"
#include "io/check_write_file.h"
#include "io/disk_storage.h"
#include "io/ImageSave.h"
#include "io/is_writeable.h"
#include "io/library.h"
#include "io/save_timer.h"
#include "io/update_save_name.h"
#include "misc/Driver.h"
#include "misc/id.h"
#include "misc/ValueSaver.h"
#include "ui/goodbye.h"
#include "ui/SaveImageProgress.h"
#include "ui/slideshw.h"
#include "ui/stop_msg.h"
#include "ui/temp_msg.h"

#include <stdexcept>

using namespace id::engine;
using namespace id::io;
using namespace id::misc;

namespace id::ui
{

bool write_image(ImageSave &image)
{
    ValueSaver busy{g_busy, true};
    if (!image.done())
    {
        SaveImageProgress progress{image.filename()};
        while (!image.done())
        {
            const int rows{image.rows_written()};
            image.iterate();
            if (image.rows_written() != rows)
            {
                progress.advance(rows);
            }
        }
    }
    return image.succeeded();
}

int save_image(std::filesystem::path &filename)
{
    const bool potential{g_disk_16_bit};
    std::filesystem::path open_file;
    bool new_file{};
    while (true)
    {
        open_file = filename;
        if (!open_file.has_extension())
        {
            open_file.replace_extension(potential ? ".pot" : DEFAULT_FRACTAL_TYPE);
        }
        if (g_resave_flag != TimedSave::STARTED)
        {
            update_save_name(filename);
        }
        open_file = get_save_path(WriteFile::IMAGE, open_file.string());
        if (open_file.empty())
        {
            throw std::logic_error("Empty image save path");
        }
        new_file = !std::filesystem::exists(open_file);
        if (new_file)
        {
            break;
        }
        if (!g_overwrite_file)
        {
            if (g_resave_flag == TimedSave::NONE)
            {
                continue;
            }
            if (!g_started_resaves)
            {
                update_save_name(filename);
                continue;
            }
        }
        if (!is_writeable(open_file))
        {
            stop_msg("Can't write " + open_file.string());
            return -1;
        }
        break;
    }
    g_started_resaves = g_resave_flag == TimedSave::STARTED;
    if (g_resave_flag == TimedSave::FINAL)
    {
        g_resave_flag = TimedSave::NONE;
    }

    ImageSave image{open_file};
    if (!write_image(image))
    {
        stop_msg(image.diagnostic());
        if (g_init_batch >= BatchMode::NORMAL)
        {
            g_init_batch = BatchMode::BAILOUT_ERROR_NO_SAVE;
        }
        return -1;
    }
    if (g_timed_save == TimedSave::NONE)
    {
        driver_buzzer(Buzzer::COMPLETE);
        if (g_init_batch == BatchMode::NONE)
        {
            text_temp_msg((" File saved as " + open_file.filename().string() + ' ').c_str());
        }
    }
    if (g_save_time_interval < 0)
    {
        goodbye();
    }
    return 0;
}

} // namespace id::ui
