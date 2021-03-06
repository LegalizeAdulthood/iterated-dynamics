#include "port.h"
#include "prototyp.h"

#include "cmplx.h"
#include "drivers.h"
#include "fractint.h"
#include "mpmath.h"
#include "prompts2.h"

#include <dirent.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#include <cassert>
#include <cstring>
#include <string>

// Global variables (yuck!)
int g_hi_attenuation = 0;
long g_l_init_x = 0;
long g_l_init_y = 0;
long g_save_base = 0;              // base clock ticks
long g_save_ticks = 0;             // save after this many ticks

/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAX_VIDEO_MODES]{};

// Global variables that should be phased out (old video mode stuff)
int g_video_vram = 0;

bool isadirectory(char const *s)
{
    int len;
    char sv;
    if (std::strchr(s, '*') || std::strchr(s, '?'))
        return false; // for my purposes, not a directory

    len = (int) std::strlen(s);
    if (len > 0)
        sv = s[len-1];   // last char
    else
        sv = 0;

    if (fr_findfirst(s) != 0) // couldn't find it
    {
        // any better ideas??
        if (sv == SLASHC) // we'll guess it is a directory
            return true;
        else
            return false; // no slashes - we'll guess it's a file
    }
    else if ((DTA.attribute & SUBDIR) != 0)
    {
        if (sv == SLASHC)
        {
            // strip trailing slash and try again
            std::string path{s, &s[len-1]};
            if (fr_findfirst(path.c_str()) != 0) // couldn't find it
                return false;
            else if ((DTA.attribute & SUBDIR) != 0)
                return true;  // we're SURE it's a directory
            else
                return false;
        }
        else
            return true;  // we're SURE it's a directory
    }
    return false;
}

// converts relative path to absolute path
int expand_dirname(char *dirname, char *drive)
{
    // TODO
    return -1;
}

unsigned long get_disk_space()
{
    // TODO
    return 0x7FFFFFFF;
}

void init_failure(char const *message)
{
    fputs(message, stderr);
}

extern void (*dotwrite)(int, int, int); // write-a-dot routine
extern int (*dotread)(int, int);    // read-a-dot routine
extern void (*linewrite)(int y, int x, int lastx, BYTE *pixels);     // write-a-line routine
extern void (*lineread)(int y, int x, int lastx, BYTE *pixels);      // read-a-line routine

#if defined(USE_DRIVER_FUNCTIONS)
void set_normal_dot()
{
    dotwrite = driver_write_pixel;
    dotread = driver_read_pixel;
}
#else
static void driver_dot_write(int x, int y, int color)
{
    driver_write_pixel(x, y, color);
}

static int driver_dot_read(int x, int y)
{
    return driver_read_pixel(x, y);
}

void set_normal_dot()
{
    dotwrite = driver_dot_write;
    dotread = driver_dot_read;
}
#endif

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

static char searchdir[FILE_MAX_DIR];
static char searchname[FILE_MAX_PATH];
static char searchext[FILE_MAX_EXT];
static DIR *currdir = nullptr;

int  fr_findfirst(char const *path)       // Find 1st file (or subdir) meeting path/filespec
{
    if (currdir != nullptr)
    {
        closedir(currdir);
        currdir = nullptr;
    }
    splitpath(path, nullptr, searchdir, searchname, searchext);
    if (searchdir[0] == '\0')
    {
        currdir = opendir(".");
    }
    else
    {
        currdir = opendir(searchdir);
    }
    if (currdir == nullptr)
    {
        return -1;
    }
    else
    {
        return fr_findnext();
    }
}

int  fr_findnext()              // Find next file (or subdir) meeting above path/filespec
{
    struct stat sbuf;
    char thisname[FILE_MAX_PATH];
    char tmpname[FILE_MAX_PATH];
    char thisext[FILE_MAX_EXT];
    while (true)
    {
        struct dirent *dirEntry = readdir(currdir);
        if (dirEntry == nullptr)
        {
            closedir(currdir);
            currdir = nullptr;
            return -1;
        }
        else if (dirEntry->d_ino != 0)
        {
            splitpath(dirEntry->d_name, nullptr, nullptr, thisname, thisext);
            DTA.filename = dirEntry->d_name;
            std::strcpy(tmpname, searchdir);
            std::strcat(tmpname, dirEntry->d_name);
            stat(tmpname, &sbuf);
            DTA.size = sbuf.st_size;
            if ((sbuf.st_mode&S_IFMT) == S_IFREG &&
                    (searchname[0] == '*' || std::strcmp(searchname, thisname) == 0) &&
                    (searchext[0] == '*' || std::strcmp(searchext, thisext) == 0))
            {
                DTA.attribute = 0;
                return 0;
            }
            else if (((sbuf.st_mode&S_IFMT) == S_IFDIR) &&
                     ((searchname[0] == '*' || searchext[0] == '*') ||
                      (std::strcmp(searchname, thisname) == 0)))
            {
                DTA.attribute = SUBDIR;
                return 0;
            }
        }
    }
}
