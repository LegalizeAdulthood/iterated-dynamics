#include "get_key_no_help.h"

#include "port.h"

#include "id.h"

#include "drivers.h"

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
    while (FIK_F1 == ch);
    return ch;
}
