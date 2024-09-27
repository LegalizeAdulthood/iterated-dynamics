// SPDX-License-Identifier: GPL-3.0-only
//
#include "get_key_no_help.h"

#include "port.h"

#include "id.h"

#include "drivers.h"
#include "id_keys.h"

/*
 * This routine returns a key, ignoring F1
 */
int getakeynohelp()
{
    int ch;
    do
    {
        ch = driver_get_key();
    }
    while (ID_KEY_F1 == ch);
    return ch;
}
