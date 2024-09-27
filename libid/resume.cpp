// SPDX-License-Identifier: GPL-3.0-only
//
#include "resume.h"

#include "port.h"

#include "id_data.h"

#include <algorithm>
#include <cstdarg>
#include <vector>

static int s_resume_offset{}; // offset in resume info gets

std::vector<BYTE> g_resume_data; // resume info
bool g_resuming{};               // true if resuming after interrupt
int g_resume_len{};              // length of resume info

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
        std::copy(&g_resume_data[s_resume_offset], &g_resume_data[s_resume_offset + len], &dest_ptr[0]);
        s_resume_offset += len;
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
    s_resume_offset = 0;
    get_resume(sizeof(version), &version, 0);
    return version;
}

void end_resume()
{
    g_resume_data.clear();
}
