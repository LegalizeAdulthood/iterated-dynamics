// SPDX-License-Identifier: GPL-3.0-only
//
#include "ui/read_ticker.h"

#include <wx/datetime.h>

/*
; long read_ticker() returns current bios ticker value
*/
long read_ticker()
{
    return wxDateTime::Now().GetValue().ToLong();
}
