// SPDX-License-Identifier: GPL-3.0-only
//
#pragma once

#include "port.h"

#include <vector>

extern std::vector<BYTE>     g_resume_data;
extern bool                  g_resuming;
extern int                   g_resume_len;

// Save/resume routines:
//
// Engines which aren't resumable can simply ignore all this.
//
// Before calling the (per_image,calctype) routines (engine), calcfract sets:
//    "g_resuming" to false if new image, true if resuming a partially done image
//    "g_calc_status" to IN_PROGRESS
// If an engine is interrupted and wants to be able to resume it must:
//    store whatever status info it needs to be able to resume later
//    set g_calc_status to RESUMABLE and return
// If subsequently called with resuming true, the engine must restore status
// info and continue from where it left off.
//
// Since the info required for resume can get rather large for some types,
// it is not stored directly in save_info.  Instead, memory is dynamically
// allocated as required, and stored in image files as a separate block.
// To save info for later resume, an engine routine can use:
//    alloc_resume(max_size, version)
//       Max_size must be >= max bytes subsequently saved + 2; over-allocation
//       is harmless except for possibility of insufficient mem available;
//       undersized is not checked and probably causes serious misbehaviour.
//       Version is an arbitrary number so that subsequent revisions of the
//       engine can be made backward compatible.
//       Alloc_resume sets g_calc_status to RESUMABLE if it succeeds;
//       to NON_RESUMABLE if it cannot allocate memory
//       (and issues warning to user).
//    put_resume({bytes, &argument}, ..., 0)
//       Can be called as often as required to store the info.
//       Is not protected against calls which use more space than allocated.
// To reload info when resuming, use:
//    start_resume()
//       initializes and returns version number
//    get_resume({bytes, &argument}, ..., 0)
//       inverse of store_resume
//    end_resume()
//       optional, frees the memory area sooner than would happen otherwise
//
// Example, save info:
//    alloc_resume(sizeof(params) + 100, 2);
//    put_resume(sizeof(row), &row, sizeof(col), &col,
//               sizeof(params), params, 0);
//  restore info:
//    version = start_resume();
//    get_resume(sizeof(row), &row, sizeof(col), &col, 0);
//    if (version >= 2)
//       get_resume(sizeof(params), params, 0);
//    end_resume();
//
// Engines which allocate a large memory chunk of their own might
// directly set resume_info, resume_len, g_calc_status to avoid doubling
// transient memory needs by using these routines.
//
// standard_fractal, calcmand, solid_guess, and bound_trace_main are a related
// set of engines for escape-time fractals.  They use a common worklist
// structure for save/resume.  Fractals using these must specify calcmand
// or standard_fractal as the engine in FractalSpecific.
// Other engines don't get btm nor ssg, don't get off-axis symmetry nor
// panning (the worklist stuff), and are on their own for save/resume.
//

int put_resume(int len, ...);
int get_resume(int len, ...);
int alloc_resume(int max_size, int version);
int start_resume();
void end_resume();
