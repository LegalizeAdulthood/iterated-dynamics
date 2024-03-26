#include "port.h"
#include "prototyp.h"

#include "cmplx.h"
#include "drivers.h"
#include "id.h"
#include "mpmath.h"
#include "video_mode.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <cassert>
#include <cstring>
#include <string>

// Global variables (yuck!)

// Global variables that should be phased out (old video mode stuff)
int g_video_vram = 0;

unsigned long get_disk_space()
{
    // TODO
    return 0x7FFFFFFF;
}

void init_failure(char const *message)
{
    std::fputs(message, stderr);
}

extern void (*dotwrite)(int, int, int); // write-a-dot routine
extern int (*dotread)(int, int);    // read-a-dot routine
extern void (*linewrite)(int y, int x, int lastx, BYTE *pixels);     // write-a-line routine
extern void (*lineread)(int y, int x, int lastx, BYTE *pixels);      // read-a-line routine

void normaline(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx - x + 1;
    assert(dotwrite);
    for (int i = 0; i < width; i++)
    {
        (*dotwrite)(x + i, y, pixels[i]);
    }
}

void normalineread(int y, int x, int lastx, BYTE *pixels)
{
    int width = lastx - x + 1;
    assert(dotread);
    for (int i = 0; i < width; i++)
    {
        pixels[i] = (*dotread)(x + i, y);
    }
}

void set_normal_line()
{
    lineread = normalineread;
    linewrite = normaline;
}
