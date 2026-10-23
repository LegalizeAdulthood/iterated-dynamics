// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
#include "misc/stack_avail.h"
#include "ui/id_main.h"

#include <config/driver_types.h>

using namespace id::misc;
using namespace id::ui;

int main(int argc, char *argv[])
{
    int result{};
    g_top_of_stack = reinterpret_cast<char *>(&result);
    result = id_main(argc, argv, {get_disk_driver(), get_x11_driver()});
    return result;
}
