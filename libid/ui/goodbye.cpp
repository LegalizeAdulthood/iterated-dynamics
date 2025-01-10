// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/goodbye.h"

#include "drivers.h"
#include "engine/pixel_grid.h"
#include "engine/resume.h"
#include "fractals/ant.h"
#include "helpcom.h"
#include "memory.h"
#include "ui/cmdfiles.h"
#include "ui/diskvid.h"
#include "ui/evolve.h"
#include "ui/make_batch_file.h"
#include "ui/slideshw.h"

#include <cstdlib>

#if defined(_WIN32)
#include <crtdbg.h>
#endif

// we done.  Bail out
[[noreturn]] void goodbye()
{
    end_resume();
    release_param_box();
    if (!g_ifs_definition.empty())
    {
        g_ifs_definition.clear();
    }
    free_grid_pointers();
    free_ant_storage();
    end_disk();
    exit_check();
    if (!g_make_parameter_file)
    {
        driver_set_for_text();
        driver_move_cursor(6, 0);
    }
    stop_slide_show();
    end_help();
    int ret = 0;
    if (g_init_batch == BatchMode::BAILOUT_ERROR_NO_SAVE) // exit with error code for batch file
    {
        ret = 2;
    }
    else if (g_init_batch == BatchMode::BAILOUT_INTERRUPTED_TRY_SAVE)
    {
        ret = 1;
    }
    close_drivers();
#if defined(_WIN32)
    _CrtDumpMemoryLeaks();
#endif
    std::exit(ret);
}
