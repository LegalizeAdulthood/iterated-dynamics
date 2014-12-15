// Routine to decode Targa 16 bit RGB file

/* 16 bit .tga files were generated for continuous potential "potfile"s
   from version 9.? thru version 14.  Replaced by double row gif type
   file (.pot) in version 15.  Delete this code after a few more revs.
*/
#include <float.h>

#include "port.h"
#include "prototyp.h"
#include "targa_lc.h"
#include "drivers.h"

static FILE *fptarga = nullptr;            // FILE pointer

// Main entry decoder
int tgaview()
{
    int cs;
    unsigned int width;
    struct fractal_info info;

    fptarga = t16_open(readname, (int *)&width, (int *)&height, &cs, (U8 *)&info);
    if (fptarga==nullptr)
        return (-1);

    g_row_count = 0;
    for (int i = 0; i < (int)height; ++i)
    {
        t16_getline(fptarga, width, (U16 *)boxx);
        if ((*outln)(reinterpret_cast<BYTE *>(boxx),width))
        {
            fclose(fptarga);
            fptarga = nullptr;
            return (-1);
        }
        if (driver_key_pressed())
        {
            fclose(fptarga);
            fptarga = nullptr;
            return (-1);
        }
    }
    fclose(fptarga);
    fptarga = nullptr;
    return (0);
}

// Outline function for 16 bit data with 8 bit fudge
int outlin16(BYTE *buffer,int linelen)
{
    U16 *buf = (U16 *)buffer;
    for (int i = 0; i < linelen; i++)
        putcolor(i,g_row_count,buf[i]>>8);
    g_row_count++;
    return (0);
}
