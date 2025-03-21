// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/has_ext.h"

#include "io/split_path.h"

#include <config/path_limits.h>

#include <cstring>

// tells if filename has extension
// returns pointer to period or nullptr
const char *has_ext(const char *source)
{
    char fname[ID_FILE_MAX_FNAME];
    char ext[ID_FILE_MAX_EXT]{};
    split_fname_ext(source, fname, ext);
    const char *ret = nullptr;
    if (ext[0] != 0)
    {
        ret = std::strrchr(source, '.');
    }
    return ret;
}
