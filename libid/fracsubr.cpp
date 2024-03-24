/*
FRACSUBR.C contains subroutines which belong primarily to CALCFRAC.C and
FRACTALS.C, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include "port.h"
#include "prototyp.h"

#include "fracsubr.h"

#include "drivers.h"
#include "id_data.h"

#include <sys/timeb.h>

#include <algorithm>
#include <iterator>

void sleepms(long ms)
{
    uclock_t       now = usec_clock();
    const uclock_t next_time = now + ms * 100;
    while (now < next_time)
    {
        if (driver_key_pressed())
        {
            break;
        }
        now = usec_clock();
    }
}

/*
 * wait until wait_time microseconds from the
 * last call has elapsed.
 */
#define MAX_INDEX 2
static uclock_t s_next_time[MAX_INDEX];
void wait_until(int index, uclock_t wait_time)
{
    uclock_t now;
    while ((now = usec_clock()) < s_next_time[index])
    {
        if (driver_key_pressed())
        {
            break;
        }
    }
    s_next_time[index] = now + wait_time * 100; // wait until this time next call
}

void reset_clock()
{
    restart_uclock();
    std::fill(std::begin(s_next_time), std::end(s_next_time), 0);
}

#define maxyblk 7    // must match calcfrac.c
#define maxxblk 202  // must match calcfrac.c
int ssg_blocksize() // used by solidguessing and by zoom panning
{
    int blocksize, i;
    // blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >=1200
    blocksize = 4;
    i = 300;
    while (i <= g_logical_screen_y_dots)
    {
        blocksize += blocksize;
        i += i;
    }
    // increase blocksize if prefix array not big enough
    while (blocksize*(maxxblk-2) < g_logical_screen_x_dots || blocksize*(maxyblk-2)*16 < g_logical_screen_y_dots)
    {
        blocksize += blocksize;
    }
    return blocksize;
}
