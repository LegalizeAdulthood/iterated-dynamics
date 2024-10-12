#include "ui/read_ticker.h"

#include <wx/app.h>

/*
; long read_ticker() returns current bios ticker value
*/
long read_ticker()
{
    return (long) GetTickCount();
}
