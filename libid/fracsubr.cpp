/*
FRACSUBR.C contains subroutines which belong primarily to CALCFRAC.C and
FRACTALS.C, i.e. which are non-fractal-specific fractal engine subroutines.
*/
#include "port.h"
#include "prototyp.h"

#include "fracsubr.h"

#include "biginit.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "drivers.h"
#include "fractalp.h"
#include "fractals.h"
#include "fractype.h"
#include "id_data.h"
#include "miscovl.h"
#include "miscres.h"
#include "sound.h"
#include "type_has_param.h"

#include <sys/timeb.h>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <iterator>

// routines in this module

static void   plotdorbit(double, double, int);
static int    combine_worklist();

int    g_resume_len;               // length of resume info
static int    resume_offset;            // offset in resume info gets

enum
{
    NUM_SAVE_ORBIT = 1500
};

static int save_orbit[NUM_SAVE_ORBIT] = { 0 };           // array to save orbit values

/* Save/resume stuff:

   Engines which aren't resumable can simply ignore all this.

   Before calling the (per_image,calctype) routines (engine), calcfract sets:
      "resuming" to false if new image, true if resuming a partially done image
      "g_calc_status" to IN_PROGRESS
   If an engine is interrupted and wants to be able to resume it must:
      store whatever status info it needs to be able to resume later
      set g_calc_status to RESUMABLE and return
   If subsequently called with resuming true, the engine must restore status
   info and continue from where it left off.

   Since the info required for resume can get rather large for some types,
   it is not stored directly in save_info.  Instead, memory is dynamically
   allocated as required, and stored in .fra files as a separate block.
   To save info for later resume, an engine routine can use:
      alloc_resume(maxsize,version)
         Maxsize must be >= max bytes subsequently saved + 2; over-allocation
         is harmless except for possibility of insufficient mem available;
         undersize is not checked and probably causes serious misbehaviour.
         Version is an arbitrary number so that subsequent revisions of the
         engine can be made backward compatible.
         Alloc_resume sets g_calc_status to RESUMABLE if it succeeds;
         to NON_RESUMABLE if it cannot allocate memory
         (and issues warning to user).
      put_resume({bytes,&argument,} ... 0)
         Can be called as often as required to store the info.
         Is not protected against calls which use more space than allocated.
   To reload info when resuming, use:
      start_resume()
         initializes and returns version number
      get_resume({bytes,&argument,} ... 0)
         inverse of store_resume
      end_resume()
         optional, frees the memory area sooner than would happen otherwise

   Example, save info:
      alloc_resume(sizeof(parmarray)+100,2);
      put_resume(sizeof(row),&row, sizeof(col),&col,
                 sizeof(parmarray),parmarray, 0);
    restore info:
      vsn = start_resume();
      get_resume(sizeof(row),&row, sizeof(col),&col, 0);
      if (vsn >= 2)
         get_resume(sizeof(parmarray),parmarray,0);
      end_resume();

   Engines which allocate a large memory chunk of their own might
   directly set resume_info, resume_len, g_calc_status to avoid doubling
   transient memory needs by using these routines.

   standard_fractal, calcmand, solid_guess, and bound_trace_main are a related
   set of engines for escape-time fractals.  They use a common worklist
   structure for save/resume.  Fractals using these must specify calcmand
   or standard_fractal as the engine in fractalspecificinfo.
   Other engines don't get btm nor ssg, don't get off-axis symmetry nor
   panning (the worklist stuff), and are on their own for save/resume.

   */

int put_resume(int len, ...)
{
    std::va_list arg_marker;

    if (g_resume_data.empty())
    {
        return -1;
    }

    va_start(arg_marker, len);
    while (len)
    {
        BYTE const *source_ptr = va_arg(arg_marker, BYTE *);
        std::copy(&source_ptr[0], &source_ptr[len], &g_resume_data[g_resume_len]);
        g_resume_len += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return 0;
}

int alloc_resume(int alloclen, int version)
{
    g_resume_data.clear();
    g_resume_data.resize(sizeof(int)*alloclen);
    g_resume_len = 0;
    put_resume(sizeof(version), &version, 0);
    g_calc_status = calc_status_value::RESUMABLE;
    return 0;
}

int get_resume(int len, ...)
{
    std::va_list arg_marker;

    if (g_resume_data.empty())
    {
        return -1;
    }
    va_start(arg_marker, len);
    while (len)
    {
        BYTE *dest_ptr = va_arg(arg_marker, BYTE *);
        std::copy(&g_resume_data[resume_offset], &g_resume_data[resume_offset + len], &dest_ptr[0]);
        resume_offset += len;
        len = va_arg(arg_marker, int);
    }
    va_end(arg_marker);
    return 0;
}

int start_resume()
{
    int version;
    if (g_resume_data.empty())
    {
        return -1;
    }
    resume_offset = 0;
    get_resume(sizeof(version), &version, 0);
    return version;
}

void end_resume()
{
    g_resume_data.clear();
}


/* Showing orbit requires converting real co-ords to screen co-ords.
   Define:
       Xs == xxmax-xx3rd               Ys == yy3rd-yymax
       W  == xdots-1                   D  == ydots-1
   We know that:
       realx == lx0[col] + lx1[row]
       realy == ly0[row] + ly1[col]
       lx0[col] == (col/width) * Xs + xxmin
       lx1[row] == row * delxx
       ly0[row] == (row/D) * Ys + yymax
       ly1[col] == col * (0-delyy)
  so:
       realx == (col/W) * Xs + xxmin + row * delxx
       realy == (row/D) * Ys + yymax + col * (0-delyy)
  and therefore:
       row == (realx-xxmin - (col/W)*Xs) / Xv    (1)
       col == (realy-yymax - (row/D)*Ys) / Yv    (2)
  substitute (2) into (1) and solve for row:
       row == ((realx-xxmin)*(0-delyy2)*W*D - (realy-yymax)*Xs*D)
                      / ((0-delyy2)*W*delxx2*D-Ys*Xs)
  */

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

#define LOG2  0.693147180F
#define LOG32 3.465735902F

static void plotdorbit(double dx, double dy, int color)
{
    int i;
    int j;
    int save_sxoffs, save_syoffs;
    if (g_orbit_save_index >= NUM_SAVE_ORBIT-3)
    {
        return;
    }
    i = (int)(dy * g_plot_mx1 - dx * g_plot_mx2);
    i += g_logical_screen_x_offset;
    if (i < 0 || i >= g_screen_x_dots)
    {
        return;
    }
    j = (int)(dx * g_plot_my1 - dy * g_plot_my2);
    j += g_logical_screen_y_offset;
    if (j < 0 || j >= g_screen_y_dots)
    {
        return;
    }
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = g_logical_screen_y_offset;
    // save orbit value
    if (color == -1)
    {
        save_orbit[g_orbit_save_index++] = i;
        save_orbit[g_orbit_save_index++] = j;
        int const c = getcolor(i, j);
        save_orbit[g_orbit_save_index++] = c;
        g_put_color(i, j, c^g_orbit_color);
    }
    else
    {
        g_put_color(i, j, color);
    }
    g_logical_screen_x_offset = save_sxoffs;
    g_logical_screen_y_offset = save_syoffs;
    if (g_debug_flag == debug_flags::force_scaled_sound_formula)
    {
        if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)   // sound = x
        {
            w_snd((int)(i*1000/g_logical_screen_x_dots+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) > SOUNDFLAG_X)     // sound = y or z
        {
            w_snd((int)(j*1000/g_logical_screen_y_dots+g_base_hertz));
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(0, g_orbit_delay);
        }
    }
    else
    {
        if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_X)   // sound = x
        {
            w_snd((int)(i+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Y)     // sound = y
        {
            w_snd((int)(j+g_base_hertz));
        }
        else if ((g_sound_flag & SOUNDFLAG_ORBITMASK) == SOUNDFLAG_Z)     // sound = z
        {
            w_snd((int)(i+j+g_base_hertz));
        }
        else if (g_orbit_delay > 0)
        {
            wait_until(0, g_orbit_delay);
        }
    }

    // placing sleepms here delays each dot
}

void iplot_orbit(long ix, long iy, int color)
{
    plotdorbit((double)ix/g_fudge_factor-g_x_min, (double)iy/g_fudge_factor-g_y_max, color);
}

void plot_orbit(double real, double imag, int color)
{
    plotdorbit(real-g_x_min, imag-g_y_max, color);
}

void scrub_orbit()
{
    int i, j, c;
    int save_sxoffs, save_syoffs;
    driver_mute();
    save_sxoffs = g_logical_screen_x_offset;
    save_syoffs = g_logical_screen_y_offset;
    g_logical_screen_y_offset = 0;
    g_logical_screen_x_offset = g_logical_screen_y_offset;
    while (g_orbit_save_index >= 3)
    {
        c = save_orbit[--g_orbit_save_index];
        j = save_orbit[--g_orbit_save_index];
        i = save_orbit[--g_orbit_save_index];
        g_put_color(i, j, c);
    }
    g_logical_screen_x_offset = save_sxoffs;
    g_logical_screen_y_offset = save_syoffs;
}


int add_worklist(int xfrom, int xto, int xbegin,
                 int yfrom, int yto, int ybegin,
                 int pass, int sym)
{
    if (g_num_work_list >= MAX_CALC_WORK)
    {
        return -1;
    }
    g_work_list[g_num_work_list].xxstart = xfrom;
    g_work_list[g_num_work_list].xxstop  = xto;
    g_work_list[g_num_work_list].xxbegin = xbegin;
    g_work_list[g_num_work_list].yystart = yfrom;
    g_work_list[g_num_work_list].yystop  = yto;
    g_work_list[g_num_work_list].yybegin = ybegin;
    g_work_list[g_num_work_list].pass    = pass;
    g_work_list[g_num_work_list].sym     = sym;
    ++g_num_work_list;
    tidy_worklist();
    return 0;
}

static int combine_worklist() // look for 2 entries which can freely merge
{
    for (int i = 0; i < g_num_work_list; ++i)
    {
        if (g_work_list[i].yystart == g_work_list[i].yybegin)
        {
            for (int j = i+1; j < g_num_work_list; ++j)
            {
                if (g_work_list[j].sym == g_work_list[i].sym
                    && g_work_list[j].yystart == g_work_list[j].yybegin
                    && g_work_list[j].xxstart == g_work_list[j].xxbegin
                    && g_work_list[i].pass == g_work_list[j].pass)
                {
                    if (g_work_list[i].xxstart == g_work_list[j].xxstart
                        && g_work_list[i].xxbegin == g_work_list[j].xxbegin
                        && g_work_list[i].xxstop  == g_work_list[j].xxstop)
                    {
                        if (g_work_list[i].yystop+1 == g_work_list[j].yystart)
                        {
                            g_work_list[i].yystop = g_work_list[j].yystop;
                            return j;
                        }
                        if (g_work_list[j].yystop+1 == g_work_list[i].yystart)
                        {
                            g_work_list[i].yystart = g_work_list[j].yystart;
                            g_work_list[i].yybegin = g_work_list[j].yybegin;
                            return j;
                        }
                    }
                    if (g_work_list[i].yystart == g_work_list[j].yystart
                        && g_work_list[i].yybegin == g_work_list[j].yybegin
                        && g_work_list[i].yystop  == g_work_list[j].yystop)
                    {
                        if (g_work_list[i].xxstop+1 == g_work_list[j].xxstart)
                        {
                            g_work_list[i].xxstop = g_work_list[j].xxstop;
                            return j;
                        }
                        if (g_work_list[j].xxstop+1 == g_work_list[i].xxstart)
                        {
                            g_work_list[i].xxstart = g_work_list[j].xxstart;
                            g_work_list[i].xxbegin = g_work_list[j].xxbegin;
                            return j;
                        }
                    }
                }
            }
        }
    }
    return 0; // nothing combined
}

// combine mergeable entries, resort
void tidy_worklist()
{
    {
        int i;
        while ((i = combine_worklist()) != 0)
        {
            // merged two, delete the gone one
            while (++i < g_num_work_list)
            {
                g_work_list[i-1] = g_work_list[i];
            }
            --g_num_work_list;
        }
    }
    for (int i = 0; i < g_num_work_list; ++i)
    {
        for (int j = i+1; j < g_num_work_list; ++j)
        {
            if (g_work_list[j].pass < g_work_list[i].pass
                || (g_work_list[j].pass == g_work_list[i].pass
                    && (g_work_list[j].yystart < g_work_list[i].yystart
                        || (g_work_list[j].yystart == g_work_list[i].yystart
                            && g_work_list[j].xxstart <  g_work_list[i].xxstart))))
            {
                // dumb sort, swap 2 entries to correct order
                WORKLIST tempwork = g_work_list[i];
                g_work_list[i] = g_work_list[j];
                g_work_list[j] = tempwork;
            }
        }
    }
}


void get_julia_attractor(double real, double imag)
{
    LComplex lresult = { 0 };
    DComplex result = { 0.0 };
    int savper;
    long savmaxit;

    if (g_attractors == 0 && !g_finite_attractor)   // not magnet & not requested
    {
        return;
    }

    if (g_attractors >= MAX_NUM_ATTRACTORS)       // space for more attractors ?
    {
        return;                  // Bad luck - no room left !
    }

    savper = g_periodicity_check;
    savmaxit = g_max_iterations;
    g_periodicity_check = 0;
    g_old_z.x = real;                    // prepare for f.p orbit calc
    g_old_z.y = imag;
    g_temp_sqr_x = sqr(g_old_z.x);
    g_temp_sqr_y = sqr(g_old_z.y);

    g_l_old_z.x = (long)real;     // prepare for int orbit calc
    g_l_old_z.y = (long)imag;
    g_l_temp_sqr_x = (long)g_temp_sqr_x;
    g_l_temp_sqr_y = (long)g_temp_sqr_y;

    g_l_old_z.x = g_l_old_z.x << g_bit_shift;
    g_l_old_z.y = g_l_old_z.y << g_bit_shift;
    g_l_temp_sqr_x = g_l_temp_sqr_x << g_bit_shift;
    g_l_temp_sqr_y = g_l_temp_sqr_y << g_bit_shift;

    if (g_max_iterations < 500)           // we're going to try at least this hard
    {
        g_max_iterations = 500;
    }
    g_color_iter = 0;
    g_overflow = false;
    while (++g_color_iter < g_max_iterations)
    {
        if (g_cur_fractal_specific->orbitcalc() || g_overflow)
        {
            break;
        }
    }
    if (g_color_iter >= g_max_iterations)      // if orbit stays in the lake
    {
        if (g_integer_fractal)     // remember where it went to
        {
            lresult = g_l_new_z;
        }
        else
        {
            result =  g_new_z;
        }
        for (int i = 0; i < 10; i++)
        {
            g_overflow = false;
            if (!g_cur_fractal_specific->orbitcalc() && !g_overflow) // if it stays in the lake
            {
                // and doesn't move far, probably
                if (g_integer_fractal)   //   found a finite attractor
                {
                    if (labs(lresult.x-g_l_new_z.x) < g_l_close_enough
                        && labs(lresult.y-g_l_new_z.y) < g_l_close_enough)
                    {
                        g_l_attractor[g_attractors] = g_l_new_z;
                        g_attractor_period[g_attractors] = i+1;
                        g_attractors++;   // another attractor - coloured lakes !
                        break;
                    }
                }
                else
                {
                    if (std::fabs(result.x-g_new_z.x) < g_close_enough
                        && std::fabs(result.y-g_new_z.y) < g_close_enough)
                    {
                        g_attractor[g_attractors] = g_new_z;
                        g_attractor_period[g_attractors] = i+1;
                        g_attractors++;   // another attractor - coloured lakes !
                        break;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }
    if (g_attractors == 0)
    {
        g_periodicity_check = savper;
    }
    g_max_iterations = savmaxit;
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
