#include <cassert>
#include <string>

#include "port.h"
#include "prototyp.h"

#include "realdos.h"
#include "resume.h"

char *g_resume_info = 0;					// resume info if allocated 
int g_resume_length = 0;					// length of resume info 
bool g_resuming;							// true if resuming after interrupt 

static int s_resume_offset;					// offset in resume info gets 
static int s_resume_info_length = 0;

// Save/resume stuff:
//
//	Engines which aren't resumable can simply ignore all this.
//
//	Before calling the (per_image, calctype) routines (engine), calculate_fractal sets:
//		"g_resuming" to false if new image, true if resuming a partially done image
//		"g_calculation_status" to CALCSTAT_IN_PROGRESS
//	If an engine is interrupted and wants to be able to resume it must:
//		store whatever status info it needs to be able to resume later
//		set g_calculation_status to CALCSTAT_RESUMABLE and return
//	If subsequently called with g_resuming true, the engine must restore status
//	info and continue from where it left off.
//
//	Since the info required for resume can get rather large for some types,
//	it is not stored directly in save_info.  Instead, memory is dynamically
//	allocated as required, and stored in .gif files as a separate block.
//	To save info for later resume, an engine routine can use:
//		alloc_resume(maxsize, version)
//			Maxsize must be >= max bytes subsequently saved + 2; over-allocation
//			is harmless except for possibility of insufficient mem available;
//			undersize is not checked and probably causes serious misbehaviour.
//			Version is an arbitrary number so that subsequent revisions of the
//			engine can be made backward compatible.
//			Alloc_resume sets g_calculation_status to CALCSTAT_RESUMABLE if it succeeds;
//			to CALCSTAT_NON_RESUMABLE if it cannot allocate memory
//			(and issues warning to user).
//		put_resume(count, ptr)
//			Can be called as often as required to store the info.
//			Is not protected against calls which use more space than allocated.
//	To reload info when resuming, use:
//		start_resume()
//			initializes and returns version number
//		get_resume(count, ptr)
//			inverse of put_resume
//		end_resume()
//			optional, frees the memory area sooner than would happen otherwise
//
//	Example, save info:
//		alloc_resume(sizeof(parmarray) + 100, 2);
//		put_resume(sizeof(row), &row);
//		put_resume(sizeof(col), &col);
//		put_resume(sizeof(parmarray), parmarray);
//	restore info:
//		vsn = start_resume();
//		get_resume(sizeof(row), &row);
//		get_resume(sizeof(col), &col);
//		if (vsn >= 2)
//			get_resume(sizeof(parmarray), parmarray);
//		end_resume();
//
//	Engines which allocate a large memory chunk of their own might
//	directly set g_resume_info, g_resume_length, g_calculation_status to avoid doubling
//	transient memory needs by using these routines.
//
//	standard_fractal, calculate_mandelbrot_l, solid_guess, and boundary_trace_main are a related
//	set of engines for escape-time fractals.  They use a common g_work_list
//	structure for save/resume.  Fractals using these must specify calculate_mandelbrot_l
//	or standard_fractal as the engine in fractalspecificinfo.
//	Other engines don't get btm nor ssg, don't get off-axis symmetry nor
//	panning (the g_work_list stuff), and are on their own for save/resume.
//
//

int put_resume(int len, void const *source_ptr)
{
	if (g_resume_info == 0)
	{
		return -1;
	}
	assert(g_resume_length + len <= s_resume_info_length);
	memcpy(&g_resume_info[g_resume_length], source_ptr, len);
	g_resume_length += len;

	return 0;
}

int alloc_resume(int alloclen, int version)
{ // WARNING! if alloclen > 4096B, problems may occur with GIF save/restore 
	end_resume();

	s_resume_info_length = alloclen*alloclen;
	g_resume_info = new char[s_resume_info_length];
	if (g_resume_info == 0)
	{
		stop_message(STOPMSG_NORMAL, "Warning - insufficient free memory to save status.\n"
			"You will not be able to resume calculating this image.");
		g_calculation_status = CALCSTAT_NON_RESUMABLE;
		s_resume_info_length = 0;
		return -1;
	}
	g_resume_length = 0;
	put_resume(sizeof(version), &version);
	g_calculation_status = CALCSTAT_RESUMABLE;
	return 0;
}

int get_resume(int len, void *dest_ptr)
{
	if (g_resume_info == 0)
	{
		return -1;
	}
	assert(s_resume_offset + len <= s_resume_info_length);
	memcpy(dest_ptr, &g_resume_info[s_resume_offset], len);
	s_resume_offset += len;
	return 0;
}

int start_resume()
{
	int version;
	if (g_resume_info == 0)
	{
		return -1;
	}
	s_resume_offset = 0;
	get_resume(sizeof(version), &version);
	return version;
}

void end_resume()
{
	delete[] g_resume_info;
	g_resume_info = 0;
	s_resume_info_length = 0;
}
