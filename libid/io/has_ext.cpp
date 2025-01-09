// SPDX-License-Identifier: GPL-3.0-only
//
#include "io/has_ext.h"

#include "id.h"
#include "io/split_path.h"

#include <cstring>

// tells if filename has extension
// returns pointer to period or nullptr
char const *has_ext(char const *source)
{
    char fname[FILE_MAX_FNAME];
    char ext[FILE_MAX_EXT]{};
    split_fname_ext(source, fname, ext);
    char const *ret = nullptr;
    if (ext[0] != 0)
    {
        ret = std::strrchr(source, '.');
    }
    return ret;
}
