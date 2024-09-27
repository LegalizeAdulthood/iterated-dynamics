// SPDX-License-Identifier: GPL-3.0-only
//
#include "version.h"

#include <port_config.h>

// g_release is pushed/popped on the history stack to provide backward compatibility with previous
// behavior, so it can't be const.
int g_release{ID_VERSION_MAJOR * 100 + ID_VERSION_MINOR};

// g_patch_level can't be modified.
const int g_patch_level{ID_VERSION_PATCH};
