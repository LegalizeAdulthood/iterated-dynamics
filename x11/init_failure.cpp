// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/init_failure.h"

#include <cstdio>

namespace id::ui
{

void init_failure(const char *message)
{
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
}

} // namespace id::ui
