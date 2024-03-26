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

void init_failure(char const *message)
{
    std::fputs(message, stderr);
}
