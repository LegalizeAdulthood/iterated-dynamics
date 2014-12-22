#include <float.h>
#include <string.h>

#include "port.h"
#include "prototyp.h"

//*************************************************************************

#define dac ((Palettetype *)g_dac_box)

bool ValidateLuts(const char *fn)
{
    FILE * f;
    unsigned        r, g, b;
    char    line[160];
    char    temp[FILE_MAX_PATH+1];
    char    temp_fn[FILE_MAX_PATH];
    strcpy(temp, MAP_name);
    strcpy(temp_fn, fn);
#ifdef XFRACT
    merge_pathnames(temp, temp_fn, cmd_file::AT_CMD_LINE_SET_NAME);
#else
    merge_pathnames(temp, temp_fn, cmd_file::AT_CMD_LINE);
#endif
    if (has_ext(temp) == nullptr) // Did name have an extension?
        strcat(temp, ".map");  // No? Then add .map
    findpath(temp, line);         // search the dos path
    f = fopen(line, "r");
    if (f == nullptr)
    {
        sprintf(line, "Could not load color map %s", fn);
        stopmsg(STOPMSG_NONE, line);
        return true;
    }
    unsigned index;
    for (index = 0; index < 256; index++)
    {
        if (fgets(line, 100, f) == nullptr)
            break;
        sscanf(line, "%u %u %u", &r, &g, &b);
        //* load global dac values *
        dac[index].red   = (BYTE)((r%256) >> 2);// maps default to 8 bits
        dac[index].green = (BYTE)((g%256) >> 2);// DAC wants 6 bits
        dac[index].blue  = (BYTE)((b%256) >> 2);
    }
    fclose(f);
    while (index < 256)
    { // zap unset entries
        dac[index].green = 40;
        dac[index].blue = dac[index].green;
        dac[index].red = dac[index].blue;
        ++index;
    }
    colorstate = 2;
    strcpy(colorfile, fn);
    return false;
}


//*************************************************************************

int SetColorPaletteName(char * fn)
{
    if (ValidateLuts(fn))
    {
        return 1;
    }
    if (mapdacbox == nullptr)
    {
        mapdacbox = (BYTE *) malloc(768L);
        if (mapdacbox == nullptr)
        {
            stopmsg(STOPMSG_NONE, "Insufficient memory for color map.");
            return 1;
        }
    }
    memcpy((char *) mapdacbox, (char *) g_dac_box, 768);
    return 0;
}

