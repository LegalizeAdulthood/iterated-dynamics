// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_a_number.h"

#include "port.h"
#include "prototyp.h"

#include "choice_builder.h"
#include "drivers.h"

int get_a_number(double *x, double *y)
{
    driver_stack_screen();

    // fill up the previous values arrays
    ChoiceBuilder<2> builder;
    builder.double_number("X coordinate at cursor", *x);
    builder.double_number("Y coordinate at cursor", *y);

    const int i = builder.prompt("Set Cursor Coordinates", 16 | 8 | 1);
    if (i < 0)
    {
        driver_unstack_screen();
        return -1;
    }

    // now check out the results (*hopefully* in the same order <grin>)
    *x = builder.read_double_number();
    *y = builder.read_double_number();

    driver_unstack_screen();
    return i;
}
