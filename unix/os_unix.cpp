#include "port.h"
#include "cmplx.h"
#include "fractint.h"
#include "mpmath.h"
#include <sys/statvfs.h>

/* Global variables (yuck!) */
struct MP Ans = { 0 };
static char extrasegment[0x18000] = { 0 };
void *extraseg = &extrasegment[0];
int fm_attack = 0;
int fm_decay = 0;
int fm_release = 0;
int fm_sustain = 0;
int fm_vol = 0;
int fm_wavetype = 0;
int hi_atten = 0;
long linitx = 0;
long linity = 0;
int polyphony = 0;
long savebase = 0;              /* base clock ticks */
long saveticks = 0;             /* save after this many ticks */

/* g_video_table
 *
 *  |--Adapter/Mode-Name------|-------Comments-----------|
 *  |------INT 10H------|Dot-|--Resolution---|
 *  |key|--AX---BX---CX---DX|Mode|--X-|--Y-|Color|
 */
VIDEOINFO g_video_table[MAXVIDEOMODES] = { 0 };
int g_vxdots = 0;

/* Global variables that should be phased out (old video mode stuff) */
int g_video_vram = 0;
int g_virtual_screens = 0;

/* converts relative path to absolute path */
int expand_dirname(char *dirname, char *drive)
{
    return -1;
}

unsigned long get_disk_space(void)
{
    /* TODO */
    return 0x7FFFFFFF;
}

void init_failure(const char *message)
{
    fputs(message, stderr);
}
