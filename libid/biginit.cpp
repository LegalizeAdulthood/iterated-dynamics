// SPDX-License-Identifier: GPL-3.0-only
//
// C routines for bignumbers

/*
Note: This is NOT the biginit.c file that come the standard BigNum library,
but is a customized version specific to the original Fractint code.
The biggest difference is in the allocations of memory for the big numbers.
*/
#include "biginit.h"

#include "bailout_formula.h"
#include "calcfrac.h"
#include "cmdfiles.h"
#include "fractalp.h"
#include "fractype.h"
#include "goodbye.h"
#include "port.h"
#include "stop_msg.h"

#include <cstring>
#include <string>

// globals
int g_bn_step{};
int g_bn_length{};
int g_int_length{};
int g_r_length{};
int g_padding{};
int g_shift_factor{};
int g_decimals{};
int g_bf_length{};
int g_r_bf_length{};
int g_bf_decimals{};
bn_t g_bn_tmp1{};
bn_t g_bn_tmp2{};
bn_t g_bn_tmp3{};
bn_t g_bn_tmp4{};
bn_t g_bn_tmp5{};
bn_t g_bn_tmp6{};
bn_t g_bn_tmp_copy1{};
bn_t g_bn_tmp_copy2{};
// used by other routines, g_bn_length
bn_t g_x_min_bn{};
bn_t g_x_max_bn{};
bn_t g_y_min_bn{};
bn_t g_y_max_bn{};
bn_t g_x_3rd_bn{};
bn_t g_y_3rd_bn{};
// g_bn_length
bn_t g_delta_x_bn{};
bn_t g_delta_y_bn{};
bn_t g_delta2_x_bn{};
bn_t g_delta2_y_bn{};
bn_t g_close_enough_bn{};
// g_r_length
bn_t g_tmp_sqr_x_bn{};
bn_t g_tmp_sqr_y_bn{};
bn_t g_bn_tmp{};
// g_bn_length
BNComplex g_old_z_bn{};
BNComplex g_param_z_bn{};
BNComplex g_saved_z_bn{};
BNComplex g_new_z_bn{};   // g_r_length
bn_t g_bn_pi{};                      // TAKES NO SPACE
// g_r_bf_length+2
bf_t g_bf_tmp1{};
bf_t g_bf_tmp2{};
bf_t g_bf_tmp3{};
bf_t g_bf_tmp4{};
bf_t g_bf_tmp5{};
bf_t g_bf_tmp6{};
bf_t g_bf_tmp_copy1{};
bf_t g_bf_tmp_copy2{};
bf_t g_delta_x_bf{};
bf_t g_delta_y_bf{};
bf_t g_delta2_x_bf{};
bf_t g_delta2_y_bf{};
bf_t g_close_enough_bf{};
bf_t g_tmp_sqr_x_bf{};
bf_t g_tmp_sqr_y_bf{};
// g_bf_length+2
BFComplex g_parm_z_bf{};
BFComplex g_saved_z_bf{};
// g_r_bf_length+2
BFComplex g_old_z_bf{};
BFComplex g_new_z_bf{};
bf_t g_bf_pi{};      // TAKES NO SPACE
bf_t g_big_pi{};     // g_bf_length+2
// g_bf_length+2
bf_t g_bf_x_min{};
bf_t g_bf_x_max{};
bf_t g_bf_y_min{};
bf_t g_bf_y_max{};
bf_t g_bf_x_3rd{};
bf_t g_bf_y_3rd{};
bf_t g_bf_save_x_min{};
bf_t g_bf_save_x_max{};
bf_t g_bf_save_y_min{};
bf_t g_bf_save_y_max{};
bf_t g_bf_save_x_3rd{};
bf_t g_bf_save_y_3rd{};
bf_t g_bf_parms[10]{}; // (g_bf_length + 2)*10
bf_t g_bf_tmp{};
bf_t g_bf10_tmp{}; // g_bf_decimals + 4

static char s_storage[4096];
static bn_t s_bn_root{};
static bn_t s_stack_ptr{}; // memory allocator base after global variables

static int save_bf_vars();
static int restore_bf_vars();

/*********************************************************************/
// given g_bn_length, calc_lengths will calculate all the other lengths
void calc_lengths()
{
    g_bn_step = 4;  // use 4 in all cases

    if (g_bn_length % g_bn_step != 0)
    {
        g_bn_length = (g_bn_length / g_bn_step + 1) * g_bn_step;
    }
    if (g_bn_length == g_bn_step)
    {
        g_padding = g_bn_length;
    }
    else
    {
        g_padding = 2*g_bn_step;
    }
    g_r_length = g_bn_length + g_padding;

    // This shift factor assumes non-full multiplications will be performed.
    // Change to g_bn_length-g_int_length for full multiplications.
    g_shift_factor = g_padding - g_int_length;

    g_bf_length = g_bn_length+g_bn_step; // one extra step for added precision
    g_r_bf_length = g_bf_length + g_padding;
    g_bf_decimals = (int)((g_bf_length-2)*LOG10_256);
}

/************************************************************************/
// intended only to be called from init_bf_dec() or init_bf_length().
// initialize bignumber global variables

long g_bignum_max_stack_addr{};
long g_start_stack{};
long g_max_stack{};
int g_bf_save_len{};

static void init_bf_2()
{
    long ptr;
    save_bf_vars(); // copy corners values for conversion

    calc_lengths();

    s_bn_root = (bf_t) &s_storage[0];

    /* at present time one call would suffice, but this logic allows
       multiple kinds of alternate math eg long double */
    if (int i = find_alternate_math(g_fractal_type, bf_math_type::BIGNUM); i > -1)
    {
        g_bf_math = g_alternate_math[i].math;
    }
    else
    {
        i = find_alternate_math(g_fractal_type, bf_math_type::BIGFLT);
        if (i > -1)
        {
            g_bf_math = g_alternate_math[i].math;
        }
        else
        {
            g_bf_math = bf_math_type::BIGNUM; // maybe called from cmdfiles.c and g_fractal_type not set
        }
    }

    g_float_flag = true;

    // Now split up the memory among the pointers
    const auto alloc_size = [&ptr](int size)
    {
        bn_t result = s_bn_root + ptr;
        ptr += size;
        return result;
    };
    ptr        = 0;
    g_bn_tmp1 = alloc_size(g_r_length);
    g_bn_tmp2 = alloc_size(g_r_length);
    g_bn_tmp3 = alloc_size(g_r_length);
    g_bn_tmp4 = alloc_size(g_r_length);
    g_bn_tmp5 = alloc_size(g_r_length);
    g_bn_tmp6 = alloc_size(g_r_length);

    g_bf_tmp1 = alloc_size(g_r_bf_length+2);
    g_bf_tmp2 = alloc_size(g_r_bf_length+2);
    g_bf_tmp3 = alloc_size(g_r_bf_length+2);
    g_bf_tmp4 = alloc_size(g_r_bf_length+2);
    g_bf_tmp5 = alloc_size(g_r_bf_length+2);
    g_bf_tmp6 = alloc_size(g_r_bf_length+2);

    g_bf_tmp_copy1 = alloc_size((g_r_bf_length+2)*2);
    g_bf_tmp_copy2 = alloc_size((g_r_bf_length+2)*2);

    g_bn_tmp_copy1 = alloc_size((g_r_length*2));
    g_bn_tmp_copy2 = alloc_size((g_r_length*2));

    if (g_bf_math == bf_math_type::BIGNUM)
    {
        g_x_min_bn = alloc_size(g_bn_length);
        g_x_max_bn = alloc_size(g_bn_length);
        g_y_min_bn = alloc_size(g_bn_length);
        g_y_max_bn = alloc_size(g_bn_length);
        g_x_3rd_bn = alloc_size(g_bn_length);
        g_y_3rd_bn = alloc_size(g_bn_length);
        g_delta_x_bn = alloc_size(g_bn_length);
        g_delta_y_bn = alloc_size(g_bn_length);
        g_delta2_x_bn = alloc_size(g_bn_length);
        g_delta2_y_bn = alloc_size(g_bn_length);
        g_old_z_bn.x = alloc_size(g_r_length);
        g_old_z_bn.y = alloc_size(g_r_length);
        g_new_z_bn.x = alloc_size(g_r_length);
        g_new_z_bn.y = alloc_size(g_r_length);
        g_saved_z_bn.x = alloc_size(g_bn_length);
        g_saved_z_bn.y = alloc_size(g_bn_length);
        g_close_enough_bn = alloc_size(g_bn_length);
        g_param_z_bn.x = alloc_size(g_bn_length);
        g_param_z_bn.y = alloc_size(g_bn_length);
        g_tmp_sqr_x_bn = alloc_size(g_r_length);
        g_tmp_sqr_y_bn = alloc_size(g_r_length);
        g_bn_tmp = alloc_size(g_r_length);
    }
    else if (g_bf_math == bf_math_type::BIGFLT)
    {
        g_delta_x_bf = alloc_size(g_bf_length+2);
        g_delta_y_bf = alloc_size(g_bf_length+2);
        g_delta2_x_bf = alloc_size(g_bf_length+2);
        g_delta2_y_bf = alloc_size(g_bf_length+2);
        g_old_z_bf.x = alloc_size(g_r_bf_length+2);
        g_old_z_bf.y = alloc_size(g_r_bf_length+2);
        g_new_z_bf.x = alloc_size(g_r_bf_length+2);
        g_new_z_bf.y = alloc_size(g_r_bf_length+2);
        g_saved_z_bf.x = alloc_size(g_bf_length+2);
        g_saved_z_bf.y = alloc_size(g_bf_length+2);
        g_close_enough_bf = alloc_size(g_bf_length+2);
        g_parm_z_bf.x = alloc_size(g_bf_length+2);
        g_parm_z_bf.y = alloc_size(g_bf_length+2);
        g_tmp_sqr_x_bf = alloc_size(g_r_bf_length+2);
        g_tmp_sqr_y_bf = alloc_size(g_r_bf_length+2);
        g_big_pi = alloc_size(g_bf_length+2);
        g_bf_tmp = alloc_size(g_r_bf_length+2);
    }
    g_bf10_tmp = alloc_size(g_bf_decimals+4);

    // ptr needs to be 16-bit aligned on some systems
    ptr = (ptr+1)&~1;

    s_stack_ptr  = s_bn_root + ptr;
    g_start_stack = ptr;

    // max stack offset from s_bn_root
    g_max_stack = (long)0x10000L-(g_bf_length+2)*22;

    // sanity check
    // leave room for NUMVARS variables allocated from stack
    // also leave room for the safe area at top of segment
    if (ptr + NUMVARS*(g_bf_length+2) > g_max_stack)
    {
        stopmsg("Requested precision of " + std::to_string(g_decimals) + " too high, aborting");
        goodbye();
    }

    // room for 6 corners + 6 save corners + 10 params at top of extraseg
    // this area is safe - use for variables that are used outside fractal
    // generation - e.g. zoom box variables
    ptr  = g_max_stack;
    g_bf_x_min = alloc_size(g_bf_length+2);
    g_bf_x_max = alloc_size(g_bf_length+2);
    g_bf_y_min = alloc_size(g_bf_length+2);
    g_bf_y_max = alloc_size(g_bf_length+2);
    g_bf_x_3rd = alloc_size(g_bf_length+2);
    g_bf_y_3rd = alloc_size(g_bf_length+2);
    for (bf_t &param : g_bf_parms)
    {
        param = s_bn_root + ptr;
        ptr += g_bf_length + 2;
    }
    g_bf_save_x_min = alloc_size(g_bf_length+2);
    g_bf_save_x_max = alloc_size(g_bf_length+2);
    g_bf_save_y_min = alloc_size(g_bf_length+2);
    g_bf_save_y_max = alloc_size(g_bf_length+2);
    g_bf_save_x_3rd = alloc_size(g_bf_length+2);
    g_bf_save_y_3rd    = s_bn_root+ptr;
    // end safe vars

    // good citizens initialize variables
    if (g_bf_save_len > 0)    // leave save area
    {
        std::memset(s_bn_root+(g_bf_save_len+2)*22, 0, (unsigned)(g_start_stack-(g_bf_save_len+2)*22));
    }
    else   // first time through - nothing saved
    {
        // high variables
        std::memset(s_bn_root+g_max_stack, 0, (g_bf_length+2)*22);
        // low variables
        std::memset(s_bn_root, 0, (unsigned)g_start_stack);
    }

    restore_bf_vars();
}

/**********************************************************/
// save current corners and parameters to start of s_bn_root
// to preserve values across calls to init_bf()
static int save_bf_vars()
{
    int ret;
    if (s_bn_root != nullptr)
    {
        unsigned int mem = (g_bf_length+2)*22;  // 6 corners + 6 save corners + 10 params
        g_bf_save_len = g_bf_length;
        std::memcpy(s_bn_root, g_bf_x_min, mem);
        // scrub old high area
        std::memset(g_bf_x_min, 0, mem);
        ret = 0;
    }
    else
    {
        g_bf_save_len = 0;
        ret = -1;
    }
    return ret;
}

/************************************************************************/
// copy current corners and parameters from save location
static int restore_bf_vars()
{
    bf_t ptr;
    if (g_bf_save_len == 0)
    {
        return -1;
    }
    ptr  = s_bn_root;
    convert_bf(g_bf_x_min, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_x_max, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_y_min, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_y_max, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_x_3rd, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_y_3rd, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    for (bf_t &param : g_bf_parms)
    {
        convert_bf(param, ptr, g_bf_length, g_bf_save_len);
        ptr += g_bf_save_len + 2;
    }
    convert_bf(g_bf_save_x_min, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_save_x_max, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_save_y_min, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_save_y_max, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_save_x_3rd, ptr, g_bf_length, g_bf_save_len);
    ptr += g_bf_save_len+2;
    convert_bf(g_bf_save_y_3rd, ptr, g_bf_length, g_bf_save_len);

    // scrub save area
    std::memset(s_bn_root, 0, (g_bf_save_len+2)*22);
    return 0;
}

/*******************************************/
// free corners and parameters save memory
void free_bf_vars()
{
    g_bf_save_len = 0;
    g_bf_math = bf_math_type::NONE;
    g_bn_step = 0;
    g_bn_length = 0;
    g_int_length = 0;
    g_r_length = 0;
    g_padding = 0;
    g_shift_factor = 0;
    g_decimals = 0;
    g_bf_length = 0;
    g_r_bf_length = 0;
    g_bf_decimals = 0;
}

/************************************************************************/
// Memory allocator routines start here.
/************************************************************************/
// Allocates a bn_t variable on stack
bn_t alloc_stack(size_t size)
{
    if (g_bf_math == bf_math_type::NONE)
    {
        stopmsg("alloc_stack called with g_bf_math==0");
        return nullptr;
    }
    const long stack_addr = (long)((s_stack_ptr-s_bn_root)+size); // part of s_bn_root

    if (stack_addr > g_max_stack)
    {
        stopmsg("Aborting, Out of Bignum Stack Space");
        goodbye();
    }
    // keep track of max ptr
    if (stack_addr > g_bignum_max_stack_addr)
    {
        g_bignum_max_stack_addr = stack_addr;
    }
    s_stack_ptr += size;   // increment stack pointer
    return s_stack_ptr - size;
}

/************************************************************************/
// Returns stack pointer offset so it can be saved.
int save_stack()
{
    return (int)(s_stack_ptr - s_bn_root);
}

/************************************************************************/
// Restores stack pointer, effectively freeing local variables
//    allocated since save_stack()
void restore_stack(int old_offset)
{
    s_stack_ptr  = s_bn_root+old_offset;
}

/************************************************************************/
// Memory allocator routines end here.
/************************************************************************/

/************************************************************************/
// initialize bignumber global variables
//   dec = decimal places after decimal point
//   intl = bytes for integer part (1, 2, or 4)

void init_bf_dec(int dec)
{
    if (g_bf_digits > 0)
    {
        g_decimals = g_bf_digits;   // blindly force
    }
    else
    {
        g_decimals = dec;
    }
    if (g_bail_out > 10)      // arbitrary value
    {
        // using 2 doesn't gain much and requires another test
        g_int_length = 4;
    }
    else if (g_fractal_type == fractal_type::FPMANDELZPOWER //
        || g_fractal_type == fractal_type::FPJULIAZPOWER    //
        || g_fractal_type == fractal_type::DIVIDE_BROT5)
    {
        g_int_length = 4; // 2 leaves artifacts in the center of the lakes
    }
    // the bailout tests need greater dynamic range
    else if (g_bail_out_test == bailouts::Real //
        || g_bail_out_test == bailouts::Imag   //
        || g_bail_out_test == bailouts::And    //
        || g_bail_out_test == bailouts::Manr)
    {
        g_int_length = 2;
    }
    else
    {
        g_int_length = 1;
    }
    // conservative estimate
    g_bn_length = g_int_length + (int)(g_decimals/LOG10_256) + 1; // round up
    init_bf_2();
}

/************************************************************************/
// initialize bignumber global variables
//   bnl = bignumber length
//   intl = bytes for integer part (1, 2, or 4)
void init_bf_length(int bnl)
{
    g_bn_length = bnl;

    if (g_bail_out > 10)      // arbitrary value
    {
        // using 2 doesn't gain much and requires another test
        g_int_length = 4;
    }
    else if (g_fractal_type == fractal_type::FPMANDELZPOWER //
        || g_fractal_type == fractal_type::FPJULIAZPOWER    //
        || g_fractal_type == fractal_type::DIVIDE_BROT5)
    {
        g_int_length = 4; // 2 leaves artifacts in the center of the lakes
    }
    // the bailout tests need greater dynamic range
    else if (g_bail_out_test == bailouts::Real //
        || g_bail_out_test == bailouts::Imag   //
        || g_bail_out_test == bailouts::And    //
        || g_bail_out_test == bailouts::Manr)
    {
        g_int_length = 2;
    }
    else
    {
        g_int_length = 1;
    }
    // conservative estimate
    g_decimals = (int)((g_bn_length-g_int_length)*LOG10_256);
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
        /* . */  0x03, 0x00, 0x00, 0x00
        //  <- up to g_int_length 4 ->
        // or bf_t int length of 2 + 2 byte exp
    };

    length = g_bf_length+2; // 2 byte exp
    pi_offset = sizeof pi_table - length;
    std::memcpy(g_big_pi, pi_table + pi_offset, length);

    // notice that g_bf_pi and g_bn_pi can share the same memory space
    g_bf_pi = g_big_pi;
    g_bn_pi = g_big_pi + (g_bf_length-2) - (g_bn_length-g_int_length);
}
