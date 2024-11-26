// SPDX-License-Identifier: GPL-3.0-only
//
#include "goodbye.h"

#include "port.h"
#include "prototyp.h"

#include "ant.h"
#include "cmdfiles.h"
#include "diskvid.h"
#include "drivers.h"
#include "evolve.h"
#include "helpcom.h"
#include "make_batch_file.h"
#include "memory.h"
#include "pixel_grid.h"
#include "resume.h"
#include "slideshw.h"

#include <cstdio>
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
    if (g_init_batch == batch_modes::BAILOUT_ERROR_NO_SAVE) // exit with error code for batch file
    {
        ret = 2;
    }
    else if (g_init_batch == batch_modes::BAILOUT_INTERRUPTED_TRY_SAVE)
    {
        ret = 1;
    }
    close_drivers();
#if defined(_WIN32)
    _CrtDumpMemoryLeaks();
#endif
    std::exit(ret);
}
