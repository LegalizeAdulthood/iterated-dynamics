// SPDX-License-Identifier: GPL-3.0-only
//
#include "misc/stack_avail.h"
#include "ui/id_main.h"

using namespace id::misc;
using namespace id::ui;

int main(int argc, char *argv[])
{
    int result{};
    g_top_of_stack = reinterpret_cast<char *>(&result);
    result = id_main(argc, argv);
    return result;
}
