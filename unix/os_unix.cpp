#include "port.h"
#include "prototyp.h"

#include "cmplx.h"
#include "drivers.h"
#include "id.h"
#include "mpmath.h"
#include "video_mode.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <cassert>
#include <cstring>
#include <string>

// Global variables (yuck!)

// Global variables that should be phased out (old video mode stuff)
int g_video_vram = 0;

unsigned long get_disk_space()
{
    // TODO
    return 0x7FFFFFFF;
}

void init_failure(char const *message)
{
    std::fputs(message, stderr);
}
