// biginit.cpp - routines for bignumbers
#include <string>

#include <boost/format.hpp>

#include "port.h"
#include "prototyp.h"
#include "fractype.h"

#include "biginit.h"
#include "calcfrac.h"
#include "EscapeTime.h"
#include "Externals.h"
#include "fractalp.h"
#include "prompts2.h"
#include "StopMessage.h"

// globals
int g_step_bn = 0;
int g_bn_length = 0;
int g_int_length = 0;
int g_r_length = 0;
int g_padding = 0;
int g_shift_factor = 0;
int g_decimals = 0;
int g_bf_length = 0;
int g_rbf_length = 0;
int g_bf_decimals = 0;

// used internally by bignum.c routines
static unsigned char s_storage[4096];
static bn_t bnroot;
static bn_t stack_ptr;			// memory allocator base after global variables
bn_t bntmp1;
bn_t bntmp2;
bn_t bntmp3;
bn_t bntmp4;
bn_t bntmp5;
bn_t bntmp6;					// g_r_length
bn_t bntmpcpy1;
bn_t bntmpcpy2;					// g_bn_length

// used by other routines
bn_t bnxmin;
bn_t bnxmax;
bn_t bnymin;
bn_t bnymax;
bn_t bnx3rd;
bn_t bny3rd;					// g_bn_length
bn_t bnxdel;
bn_t bnydel;
bn_t bnxdel2;
bn_t bnydel2;
bn_t bnclosenuff;				// g_bn_length
bn_t bntmpsqrx;
bn_t bntmpsqry;
bn_t bntmp;						// g_r_length
ComplexBigNum g_old_z_bn;
ComplexBigNum bnparm;
ComplexBigNum bnsaved;			// g_bn_length
ComplexBigNum g_new_z_bn;		// g_r_length
bn_t bn_pi;						// TAKES NO SPACE

bf_t bftmp1;
bf_t bftmp2;
bf_t bftmp3;
bf_t bftmp4;
bf_t bftmp5;
bf_t bftmp6;					// g_rbf_length + 2
bf_t bftmpcpy1;
bf_t bftmpcpy2;					// g_rbf_length + 2
bf_t bfxdel;
bf_t bfydel;
bf_t bfxdel2;
bf_t bfydel2;
bf_t bfclosenuff;				// g_rbf_length + 2
bf_t bftmpsqrx;
bf_t bftmpsqry;					// g_rbf_length + 2
ComplexBigFloat bfparm;			// g_bf_length + 2
									// g_bf_length + 2
ComplexBigFloat bfsaved;		// g_old_z_bf,  g_new_z_bf,
									// g_bf_length + 2
ComplexBigFloat g_old_z_bf;
ComplexBigFloat g_new_z_bf;			// g_rbf_length + 2
bf_t bf_pi;						// TAKES NO SPACE
bf_t big_pi;						// g_bf_length + 2

// for testing only

// used by other routines
bf_t g_sx_min_bf;
bf_t g_sx_max_bf;
bf_t g_sy_min_bf;
bf_t g_sy_max_bf;
bf_t g_sx_3rd_bf;
bf_t g_sy_3rd_bf;				// g_bf_length + 2
bf_t bfparms[10];					// (g_bf_length + 2)*10
bf_t bftmp;

bf10_t bf10tmp;					// dec + 4

static int save_bf_vars();
static int restore_bf_vars();

/*********************************************************************/
// given g_bn_length, calculate_bignum_lengths will calculate all the other lengths
void calculate_bignum_lengths()
{
	g_step_bn = 4;  // use 4 in all cases

	if (g_bn_length % g_step_bn != 0)
	{
		g_bn_length = (g_bn_length/g_step_bn + 1)*g_step_bn;
	}
	g_padding = (g_bn_length == g_step_bn) ? g_bn_length : 2*g_step_bn;
	g_r_length = g_bn_length + g_padding;

	// This shiftfactor assumes non-full multiplications will be performed.
	// Change to g_bn_length-g_int_length for full multiplications.
	g_shift_factor = g_padding - g_int_length;

	g_bf_length = g_bn_length + g_step_bn; // one extra step for added precision
	g_rbf_length = g_bf_length + g_padding;
	g_bf_decimals = int((g_bf_length-2)*LOG10_256);
}

/************************************************************************/
// intended only to be called from init_bf_dec() or init_bf_length().
// initialize bignumber global variables

long g_bn_max_stack = 0;
long startstack = 0;
long maxstack = 0;
int g_bf_save_len = 0;

static big_t advance_ptr(long &ptr, int length)
{
	big_t result = bnroot.storage() + ptr;
	ptr += length;
	return result;
}

static big_t advance_ptr_r_length(long &ptr)
{
	return advance_ptr(ptr, g_r_length);
}

static big_t advance_ptr_bn_length(long &ptr)
{
	return advance_ptr(ptr, g_bn_length);
}

static big_t advance_ptr_bf_length_plus_2(long &ptr)
{
	return advance_ptr(ptr, g_bf_length + 2);
}

static big_t advance_ptr_rbf_length_plus_2(long &ptr)
{
	return advance_ptr(ptr, g_rbf_length + 2);
}

static void init_bf_2()
{
	save_bf_vars(); // copy corners values for conversion

	calculate_bignum_lengths();

	bnroot = bn_t(&s_storage[0]);

	/* at present time one call would suffice, but this logic allows
		multiple kinds of alternate math eg long double */
	alternate_math *alt = find_alternate_math(BIGNUM);
	if (alt != 0)
	{
		g_bf_math = alt->math;
	}
	else
	{
		alt = find_alternate_math(BIGFLT);
		// 1 => maybe called from cmdfiles.c and g_fractal_type not set
		g_bf_math = alt ? alt->math : BIGNUM;
	}
	g_float_flag = true;

	// Now split up the memory among the pointers
	// internal pointers
	long ptr = 0;
	bntmp1 = bn_t(advance_ptr_r_length(ptr));
	bntmp2 = bn_t(advance_ptr_r_length(ptr));
	bntmp3 = bn_t(advance_ptr_r_length(ptr));
	bntmp4 = bn_t(advance_ptr_r_length(ptr));
	bntmp5 = bn_t(advance_ptr_r_length(ptr));
	bntmp6 = bn_t(advance_ptr_r_length(ptr));

	bftmp1 = bf_t(advance_ptr_rbf_length_plus_2(ptr));
	bftmp2 = bf_t(advance_ptr_rbf_length_plus_2(ptr));
	bftmp3 = bf_t(advance_ptr_rbf_length_plus_2(ptr));
	bftmp4 = bf_t(advance_ptr_rbf_length_plus_2(ptr));
	bftmp5 = bf_t(advance_ptr_rbf_length_plus_2(ptr));
	bftmp6 = bf_t(advance_ptr_rbf_length_plus_2(ptr));

	bftmpcpy1 = bf_t(advance_ptr(ptr, (g_rbf_length + 2)*2));
	bftmpcpy2 = bf_t(advance_ptr(ptr, (g_rbf_length + 2)*2));

	bntmpcpy1 = bn_t(advance_ptr(ptr, g_r_length*2));
	bntmpcpy2 = bn_t(advance_ptr(ptr, g_r_length*2));

	if (g_bf_math == BIGNUM)
	{
		bnxmin = bn_t(advance_ptr_bn_length(ptr));
		bnxmax = bn_t(advance_ptr_bn_length(ptr));
		bnymin = bn_t(advance_ptr_bn_length(ptr));
		bnymax = bn_t(advance_ptr_bn_length(ptr));
		bnx3rd = bn_t(advance_ptr_bn_length(ptr));
		bny3rd = bn_t(advance_ptr_bn_length(ptr));
		bnxdel = bn_t(advance_ptr_bn_length(ptr));
		bnydel = bn_t(advance_ptr_bn_length(ptr));
		bnxdel2 = bn_t(advance_ptr_bn_length(ptr));
		bnydel2 = bn_t(advance_ptr_bn_length(ptr));
		g_old_z_bn.real(bn_t(advance_ptr_r_length(ptr)));
		g_old_z_bn.imag(bn_t(advance_ptr_r_length(ptr)));
		g_new_z_bn.real(bn_t(advance_ptr_r_length(ptr)));
		g_new_z_bn.imag(bn_t(advance_ptr_r_length(ptr)));
		bnsaved.real(bn_t(advance_ptr_bn_length(ptr)));
		bnsaved.imag(bn_t(advance_ptr_bn_length(ptr)));
		bnclosenuff = bn_t(advance_ptr_bn_length(ptr));
		bnparm.real(bn_t(advance_ptr_bn_length(ptr)));
		bnparm.imag(bn_t(advance_ptr_bn_length(ptr)));
		bntmpsqrx = bn_t(advance_ptr_r_length(ptr));
		bntmpsqry = bn_t(advance_ptr_r_length(ptr));
		bntmp = bn_t(advance_ptr_r_length(ptr));
	}
	if (g_bf_math == BIGFLT)
	{
		bfxdel     = bf_t(advance_ptr_bf_length_plus_2(ptr));
		bfydel     = bf_t(advance_ptr_bf_length_plus_2(ptr));
		bfxdel2    = bf_t(advance_ptr_bf_length_plus_2(ptr));
		bfydel2    = bf_t(advance_ptr_bf_length_plus_2(ptr));
		g_old_z_bf.real(bf_t(advance_ptr_rbf_length_plus_2(ptr)));
		g_old_z_bf.imag(bf_t(advance_ptr_rbf_length_plus_2(ptr)));
		g_new_z_bf.real(bf_t(advance_ptr_rbf_length_plus_2(ptr)));
		g_new_z_bf.imag(bf_t(advance_ptr_rbf_length_plus_2(ptr)));
		bfsaved.real(bf_t(advance_ptr_bf_length_plus_2(ptr)));
		bfsaved.imag(bf_t(advance_ptr_bf_length_plus_2(ptr)));
		bfclosenuff = bf_t(advance_ptr_bf_length_plus_2(ptr));
		bfparm.real(bf_t(advance_ptr_bf_length_plus_2(ptr)));
		bfparm.imag(bf_t(advance_ptr_bf_length_plus_2(ptr)));
		bftmpsqrx  = bf_t(advance_ptr_rbf_length_plus_2(ptr));
		bftmpsqry  = bf_t(advance_ptr_rbf_length_plus_2(ptr));
		big_pi     = bf_t(advance_ptr_bf_length_plus_2(ptr));
		bftmp      = bf_t(advance_ptr_rbf_length_plus_2(ptr));
	}
	bf10tmp    = advance_ptr(ptr, g_bf_decimals + 4);

	// ptr needs to be 16-bit aligned on some systems
	ptr = (ptr + 1) & ~1;

	stack_ptr  = bn_t(bnroot, ptr);
	startstack = ptr;

	// max stack offset from bnroot
	maxstack = long(0x10000l-(g_bf_length + 2)*22);

	// sanity check
	// leave room for NUMVARS variables allocated from stack
	// also leave room for the safe area at top of segment
	if (ptr + NUMVARS*(g_bf_length + 2) > maxstack)
	{
		stop_message(STOPMSG_NORMAL, str(boost::format("Requested precision of %d too high, aborting") % g_decimals));
		goodbye();
	}

	// room for 6 corners + 6 save corners + 10 params at top of extraseg
	// this area is safe - use for variables that are used outside fractal
	// generation - e.g. zoom box variables
	ptr  = maxstack;
	g_escape_time_state.m_grid_bf.x_min()     = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_escape_time_state.m_grid_bf.x_max()     = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_escape_time_state.m_grid_bf.y_min()     = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_escape_time_state.m_grid_bf.y_max()     = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_escape_time_state.m_grid_bf.x_3rd()     = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_escape_time_state.m_grid_bf.y_3rd()     = bf_t(advance_ptr_bf_length_plus_2(ptr));
	for (int i = 0; i < 10; i++)
	{
		bfparms[i]  = bf_t(bnroot.storage() + ptr);
		ptr += g_bf_length + 2;
	}
	g_sx_min_bf    = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_sx_max_bf    = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_sy_min_bf    = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_sy_max_bf    = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_sx_3rd_bf    = bf_t(advance_ptr_bf_length_plus_2(ptr));
	g_sy_3rd_bf    = bf_t(advance_ptr_bf_length_plus_2(ptr));
	// end safe vars

	// good citizens initialize variables
	if (g_bf_save_len)  // leave save area
	{
		memset(bnroot.storage() + (g_bf_save_len + 2)*22, 0, (unsigned)(startstack-(g_bf_save_len + 2)*22));
	}
	else // first time through - nothing saved
	{
		// high variables
		memset(bnroot.storage() + maxstack, 0, (g_bf_length + 2)*22);
		// low variables
		memset(bnroot.storage(), 0, (unsigned)startstack);
	}

	restore_bf_vars();

	// Initialize the value of pi.  Needed for trig functions.
	// init_big_pi();
	// call to init_big_pi() has been moved to fractal setup routine
	// so as to use only when necessary.
}


/**********************************************************/
// save current corners and parameters to start of bnroot
// to preserve values across calls to init_bf()
static int save_bf_vars()
{
	int ret;
	unsigned int mem;
	if (bnroot.storage() != 0)
	{
		mem = (g_bf_length + 2)*22;  // 6 corners + 6 save corners + 10 params
		g_bf_save_len = g_bf_length;
		memcpy(bnroot.storage(), g_escape_time_state.m_grid_bf.x_min().storage(), mem);
		// scrub old high area
		memset(g_escape_time_state.m_grid_bf.x_min().storage(), 0, mem);
		ret = 0;
	}
	else
	{
		g_bf_save_len = 0;
		ret = -1;
	}
	return ret;
}

static void restore_bf_vars_convert(bf_t &dest, bf_t &ptr, int &convert_count)
{
	convert_bf(dest, ptr, g_bf_length, g_bf_save_len);
	ptr = bf_t(ptr.storage() + g_bf_save_len + 2);
	convert_count++;
}

/************************************************************************/
// copy current corners and parameters from save location
static int restore_bf_vars()
{
	int i;
	if (g_bf_save_len == 0)
	{
		return -1;
	}
	bf_t ptr(bnroot.storage());
	int convert_count = 0;
	restore_bf_vars_convert(g_escape_time_state.m_grid_bf.x_min(), ptr, convert_count);
	restore_bf_vars_convert(g_escape_time_state.m_grid_bf.x_max(), ptr, convert_count);
	restore_bf_vars_convert(g_escape_time_state.m_grid_bf.y_min(), ptr, convert_count);
	restore_bf_vars_convert(g_escape_time_state.m_grid_bf.y_max(), ptr, convert_count);
	restore_bf_vars_convert(g_escape_time_state.m_grid_bf.x_3rd(), ptr, convert_count);
	restore_bf_vars_convert(g_escape_time_state.m_grid_bf.y_3rd(), ptr, convert_count);
	for (i = 0; i < 10; i++)
	{
		restore_bf_vars_convert(bfparms[i], ptr, convert_count);
	}
	restore_bf_vars_convert(g_sx_min_bf, ptr, convert_count);
	restore_bf_vars_convert(g_sx_max_bf, ptr, convert_count);
	restore_bf_vars_convert(g_sy_min_bf, ptr, convert_count);
	restore_bf_vars_convert(g_sy_max_bf, ptr, convert_count);
	restore_bf_vars_convert(g_sx_3rd_bf, ptr, convert_count);
	restore_bf_vars_convert(g_sy_3rd_bf, ptr, convert_count);

	// scrub save area
	std::fill(bnroot.storage(), bnroot.storage() + (g_bf_save_len + 2)*convert_count, 0);
	return 0;
}

/*******************************************/
// free corners and parameters save memory
void free_bf_vars()
{
	g_bf_save_len = 0;
	g_bf_math = 0;
	g_step_bn = 0;
	g_bn_length = 0;
	g_int_length = 0;
	g_r_length = 0;
	g_padding = 0;
	g_shift_factor = 0;
	g_decimals = 0;
	g_bf_length = 0;
	g_rbf_length = 0;
	g_bf_decimals = 0;
}

/************************************************************************/
// Memory allocator routines start here.
/************************************************************************/
// Allocates a bn_t variable on stack
big_t alloc_stack(size_t size)
{
	long stack_addr;
	if (g_bf_math == 0)
	{
		stop_message(STOPMSG_NORMAL, "alloc_stack called with g_bf_math == 0");
		return 0;
	}
	stack_addr = long(stack_ptr.storage() - bnroot.storage() + size); // part of bnroot

	if (stack_addr > maxstack)
	{
		stop_message(STOPMSG_NORMAL, "Aborting, Out of Bignum Stack Space");
		goodbye();
	}
	// keep track of max ptr
	if (stack_addr > g_bn_max_stack)
	{
		g_bn_max_stack = stack_addr;
	}
	stack_ptr = bn_t(stack_ptr, int(size));
	return stack_ptr.storage() - size;
}

/************************************************************************/
// Returns stack pointer offset so it can be saved.
int save_stack()
{
	return int(stack_ptr.storage() - bnroot.storage());
}

/************************************************************************/
// Restores stack pointer, effectively freeing local variables
// allocated since save_stack()
void restore_stack(int old_offset)
{
	stack_ptr = bn_t(bnroot, old_offset);
}

/************************************************************************/
// Memory allocator routines end here.
/************************************************************************/

/************************************************************************/
// initialize bignumber global variables
// dec = decimal places after decimal point
// intl = bytes for integer part (1, 2, or 4)

void init_bf_dec(int dec)
{
	g_decimals = g_bf_digits ? g_bf_digits : dec;
	if (g_externs.BailOut() > 10)    // arbitrary value
	{
		// using 2 doesn't gain much and requires another test
		g_int_length = 4;
	}
	else if (g_fractal_type == FRACTYPE_MANDELBROT_Z_POWER_FP || g_fractal_type == FRACTYPE_JULIA_Z_POWER_FP)
	{
		g_int_length = 2;
	}
	// the bailout tests need greater dynamic range
	else if (g_externs.BailOutTest() == BAILOUT_REAL || g_externs.BailOutTest() == BAILOUT_IMAGINARY || g_externs.BailOutTest() == BAILOUT_AND ||
				g_externs.BailOutTest() == BAILOUT_MANHATTAN_R)
	{
		g_int_length = 2;
	}
	else
	{
		g_int_length = 1;
	}
	// conservative estimate
	g_bn_length = g_int_length + int(g_decimals/LOG10_256) + 1; // round up
	init_bf_2();
}

/************************************************************************/
// initialize bignumber global variables
// bnl = bignumber length
// intl = bytes for integer part (1, 2, or 4)
void init_bf_length(int bnl)
{
	g_bn_length = bnl;

	if (g_externs.BailOut() > 10)    // arbitrary value
	{
		// using 2 doesn't gain much and requires another test
		g_int_length = 4;
	}
	else if (g_fractal_type == FRACTYPE_MANDELBROT_Z_POWER_FP || g_fractal_type == FRACTYPE_JULIA_Z_POWER_FP)
	{
		g_int_length = 2;
	}
	// the bailout tests need greater dynamic range
	else if (g_externs.BailOutTest() == BAILOUT_REAL || g_externs.BailOutTest() == BAILOUT_IMAGINARY || g_externs.BailOutTest() == BAILOUT_AND ||
				g_externs.BailOutTest() == BAILOUT_MANHATTAN_R)
	{
		g_int_length = 2;
	}
	else
	{
		g_int_length = 1;
	}
	// conservative estimate
	g_decimals = int((g_bn_length-g_int_length)*LOG10_256);
	init_bf_2();
}


void init_big_pi()
{
	// What, don't you recognize the first 700 digits of pi,
	// in base 256, in reverse order?
	int length;
	int pi_offset;
	static BYTE pi_table[] =
	{
			0x44, 0xD5, 0xDB, 0x69, 0x17, 0xDF, 0x2E, 0x56, 0x87, 0x1A,
			0xA0, 0x8C, 0x6F, 0xCA, 0xBB, 0x57, 0x5C, 0x9E, 0x82, 0xDF,
			0x00, 0x3E, 0x48, 0x7B, 0x31, 0x53, 0x60, 0x87, 0x23, 0xFD,
			0xFA, 0xB5, 0x3D, 0x32, 0xAB, 0x52, 0x05, 0xAD, 0xC8, 0x1E,
			0x50, 0x2F, 0x15, 0x6B, 0x61, 0xFD, 0xDF, 0x16, 0x75, 0x3C,
			0xF8, 0x22, 0x32, 0xDB, 0xF8, 0xE9, 0xA5, 0x8E, 0xCC, 0xA3,
			0x1F, 0xFB, 0xFE, 0x25, 0x9F, 0x67, 0x79, 0x72, 0x2C, 0x40,
			0xC6, 0x00, 0xA1, 0xD6, 0x0A, 0x32, 0x60, 0x1A, 0xBD, 0xC0,
			0x79, 0x55, 0xDB, 0xFB, 0xD3, 0xB9, 0x39, 0x5F, 0x0B, 0xD2,
			0x0F, 0x74, 0xC8, 0x45, 0x57, 0xA8, 0xCB, 0xC0, 0xB3, 0x4B,
			0x2E, 0x19, 0x07, 0x28, 0x0F, 0x66, 0xFD, 0x4A, 0x33, 0xDE,
			0x04, 0xD0, 0xE3, 0xBE, 0x09, 0xBD, 0x5E, 0xAF, 0x44, 0x45,
			0x81, 0xCC, 0x2C, 0x95, 0x30, 0x9B, 0x1F, 0x51, 0xFC, 0x6D,
			0x6F, 0xEC, 0x52, 0x3B, 0xEB, 0xB2, 0x39, 0x13, 0xB5, 0x53,
			0x6C, 0x3E, 0xAF, 0x6F, 0xFB, 0x68, 0x63, 0x24, 0x6A, 0x19,
			0xC2, 0x9E, 0x5C, 0x5E, 0xC4, 0x60, 0x9F, 0x40, 0xB6, 0x4F,
			0xA9, 0xC1, 0xBA, 0x06, 0xC0, 0x04, 0xBD, 0xE0, 0x6C, 0x97,
			0x3B, 0x4C, 0x79, 0xB6, 0x1A, 0x50, 0xFE, 0xE3, 0xF7, 0xDE,
			0xE8, 0xF6, 0xD8, 0x79, 0xD4, 0x25, 0x7B, 0x1B, 0x99, 0x80,
			0xC9, 0x72, 0x53, 0x07, 0x9B, 0xC0, 0xF1, 0x49, 0xD3, 0xEA,
			0x0F, 0xDB, 0x48, 0x12, 0x0A, 0xD0, 0x24, 0xD7, 0xD0, 0x37,
			0x3D, 0x02, 0x9B, 0x42, 0x72, 0xDF, 0xFE, 0x1B, 0x06, 0x77,
			0x3F, 0x36, 0x62, 0xAA, 0xD3, 0x4E, 0xA6, 0x6A, 0xC1, 0x56,
			0x9F, 0x44, 0x1A, 0x40, 0x73, 0x20, 0xC1, 0x85, 0xD8, 0x75,
			0x6F, 0xE0, 0xBE, 0x5E, 0x8B, 0x3B, 0xC3, 0xA5, 0x84, 0x7D,
			0xB4, 0x9F, 0x6F, 0x45, 0x19, 0x86, 0xEE, 0x8C, 0x88, 0x0E,
			0x43, 0x82, 0x3E, 0x59, 0xCA, 0x66, 0x76, 0x01, 0xAF, 0x39,
			0x1D, 0x65, 0xF1, 0xA1, 0x98, 0x2A, 0xFB, 0x7E, 0x50, 0xF0,
			0x3B, 0xBA, 0xE4, 0x3B, 0x7A, 0x13, 0x6C, 0x0B, 0xEF, 0x6E,
			0xA3, 0x33, 0x51, 0xAB, 0x28, 0xA7, 0x0F, 0x96, 0x68, 0x2F,
			0x54, 0xD8, 0xD2, 0xA0, 0x51, 0x6A, 0xF0, 0x88, 0xD3, 0xAB,
			0x61, 0x9C, 0x0C, 0x67, 0x9A, 0x6C, 0xE9, 0xF6, 0x42, 0x68,
			0xC6, 0x21, 0x5E, 0x9B, 0x1F, 0x9E, 0x4A, 0xF0, 0xC8, 0x69,
			0x04, 0x20, 0x84, 0xA4, 0x82, 0x44, 0x0B, 0x2E, 0x39, 0x42,
			0xF4, 0x83, 0xF3, 0x6F, 0x6D, 0x0F, 0xC5, 0xAC, 0x96, 0xD3,
			0x81, 0x3E, 0x89, 0x23, 0x88, 0x1B, 0x65, 0xEB, 0x02, 0x23,
			0x26, 0xDC, 0xB1, 0x75, 0x85, 0xE9, 0x5D, 0x5D, 0x84, 0xEF,
			0x32, 0x80, 0xEC, 0x5D, 0x60, 0xAC, 0x7C, 0x48, 0x91, 0xA9,
			0x21, 0xFB, 0xCC, 0x09, 0xD8, 0x61, 0x93, 0x21, 0x28, 0x66,
			0x1B, 0xE8, 0xBF, 0xC4, 0xAF, 0xB9, 0x4B, 0x6B, 0x98, 0x48,
			0x8F, 0x3B, 0x77, 0x86, 0x95, 0x28, 0x81, 0x53, 0x32, 0x7A,
			0x5C, 0xCF, 0x24, 0x6C, 0x33, 0xBA, 0xD6, 0xAF, 0x1E, 0x93,
			0x87, 0x9B, 0x16, 0x3E, 0x5C, 0xCE, 0xF6, 0x31, 0x18, 0x74,
			0x5D, 0xC5, 0xA9, 0x2B, 0x2A, 0xBC, 0x6F, 0x63, 0x11, 0x14,
			0xEE, 0xB3, 0x93, 0xE9, 0x72, 0x7C, 0xAF, 0x86, 0x54, 0xA1,
			0xCE, 0xE8, 0x41, 0x11, 0x34, 0x5C, 0xCC, 0xB4, 0xB6, 0x10,
			0xAB, 0x2A, 0x6A, 0x39, 0xCA, 0x55, 0x40, 0x14, 0xE8, 0x63,
			0x62, 0x98, 0x48, 0x57, 0x94, 0xAB, 0x55, 0xAA, 0xF3, 0x25,
			0x55, 0xE6, 0x60, 0x5C, 0x60, 0x55, 0xDA, 0x2F, 0xAF, 0x78,
			0x27, 0x4B, 0x31, 0xBD, 0xC1, 0x77, 0x15, 0xD7, 0x3E, 0x8A,
			0x1E, 0xB0, 0x8B, 0x0E, 0x9E, 0x6C, 0x0E, 0x18, 0x3A, 0x60,
			0xB0, 0xDC, 0x79, 0x8E, 0xEF, 0x38, 0xDB, 0xB8, 0x18, 0x79,
			0x41, 0xCA, 0xF0, 0x85, 0x60, 0x28, 0x23, 0xB0, 0xD1, 0xC5,
			0x13, 0x60, 0xF2, 0x2A, 0x39, 0xD5, 0x30, 0x9C, 0xB5, 0x59,
			0x5A, 0xC2, 0x1D, 0xA4, 0x54, 0x7B, 0xEE, 0x4A, 0x15, 0x82,
			0x58, 0xCD, 0x8B, 0x71, 0x58, 0xB6, 0x8E, 0x72, 0x8F, 0x74,
			0x95, 0x0D, 0x7E, 0x3D, 0x93, 0xF4, 0xA3, 0xFE, 0x58, 0xA4,
			0x69, 0x4E, 0x57, 0x71, 0xD8, 0x20, 0x69, 0x63, 0x16, 0xFC,
			0x8E, 0x85, 0xE2, 0xF2, 0x01, 0x08, 0xF7, 0x6C, 0x91, 0xB3,
			0x47, 0x99, 0xA1, 0x24, 0x99, 0x7F, 0x2C, 0xF1, 0x45, 0x90,
			0x7C, 0xBA, 0x96, 0x7E, 0x26, 0x6A, 0xED, 0xAF, 0xE1, 0xB8,
			0xB7, 0xDF, 0x1A, 0xD0, 0xDB, 0x72, 0xFD, 0x2F, 0xAC, 0xB5,
			0xDF, 0x98, 0xA6, 0x0B, 0x31, 0xD1, 0x1B, 0xFB, 0x79, 0x89,
			0xD9, 0xD5, 0x16, 0x92, 0x17, 0x09, 0x47, 0xB5, 0xB5, 0xD5,
			0x84, 0x3F, 0xDD, 0x50, 0x7C, 0xC9, 0xB7, 0x29, 0xAC, 0xC0,
			0x6C, 0x0C, 0xE9, 0x34, 0xCF, 0x66, 0x54, 0xBE, 0x77, 0x13,
			0xD0, 0x38, 0xE6, 0x21, 0x28, 0x45, 0x89, 0x6C, 0x4E, 0xEC,
			0x98, 0xFA, 0x2E, 0x08, 0xD0, 0x31, 0x9F, 0x29, 0x22, 0x38,
			0x09, 0xA4, 0x44, 0x73, 0x70, 0x03, 0x2E, 0x8A, 0x19, 0x13,
			0xD3, 0x08, 0xA3, 0x85, 0x88, 0x6A, 0x3F, 0x24,
			// .   0x03, 0x00, 0x00, 0x00
			// <- up to g_int_length 4 ->
			// or bf_t int length of 2 + 2 byte exp
			};

	length = g_bf_length + 2; // 2 byte exp
	pi_offset = sizeof pi_table - length;
	std::copy(pi_table + pi_offset, pi_table + pi_offset + length, big_pi.storage());

	// notice that bf_pi and bn_pi can share the same memory space
	bf_pi = big_pi;
	bn_pi = bn_t(big_pi.storage() + (g_bf_length-2) - (g_bn_length-g_int_length));
}
