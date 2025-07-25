// SPDX-License-Identifier: GPL-3.0-only
//
// C routines for bignumbers

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer

The bignumber format is simply a signed integer of variable length.  The
bytes are stored in reverse order (the least significant byte first, most
significant byte last).  The sign bit is the highest bit of the most
significant byte.  Negatives are stored in 2's complement form.  The byte
length of the bignumbers must be a multiple of 4 for 386+ operations, and
a multiple of 2 for 8086/286 and non 80x86 machines.

Some of the arithmetic operations coded here may alter some of the
operands used.  Therefore, take note of the SIDE-EFFECTS listed with each
procedure.  If the value of an operand needs to be retained, just use
copy_bn() first.  This was done for speed's sake to avoid unnecessary
copying.  If space is at such a premium that copying it would be
difficult, some of the operations only change the sign of the value.  In
this case, the original could be obtained by calling neg_a_bn().

Most of the bignumber routines operate on true integers.  Some of the
procedures, however, are designed specifically for fixed decimal point
operations.  The results and how the results are interpreted depend on
where the implied decimal point is located.  The routines that depend on
where the decimal is located are:  strtobn(), bntostr(), bntoint(), inttobn(),
bntofloat(), floattobn(), inv_bn(), div_bn().  The number of bytes
designated for the integer part may be 1, 2, or 4.

BIGNUMBER FORMAT:
The following is a discription of the bignumber format and associated
variables.  The number is stored in reverse order (The Least Significant Byte,
LSB, stored first in memory, Most Significant Byte, MSB, stored last).
Each '_' below represents a block of memory used for arithmetic (1 block =
4 bytes on 386+, 2 bytes on 286-).  All lengths are given in bytes.

LSB                                MSB
  _  _  _  _  _  _  _  _  _  _  _  _
n <---------- g_bn_length --------->
                 g_int_length  ---> <---

  g_bn_length  = the length in bytes of the bignumber
  g_int_length = the number of bytes used to represent the integer part of
              the bignumber.  Possible values are 1, 2, or 4.  This
              determines the largest number that can be represented by
              the bignumber.
                g_int_length = 1, max value = 127.99...
                g_int_length = 2, max value = 32,767.99...
                g_int_length = 4, max value = 2,147,483,647.99...

FULL DOUBLE PRECISION MULTIPLICATION:
( full_mult_bn(), full_square_bn() )

The product of two bignumbers, n1 and n2, will be a result, r, which is
a double wide bignumber.  The integer part will also be twice as wide,
thereby eliminating the possiblity of overflowing the number.

LSB                                                                    MSB
  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _
r <------------------------- 2*g_bn_length ---------------------------->
                                                  2*g_int_length  --->  <---

If this double wide bignumber, r, needs to be converted to a normal,
single width bignumber, this is easily done with pointer arithmetic.  The
converted value starts at r+g_shift_factor (where g_shift_factor =
g_bn_length-g_int_length) and continues for g_bn_length bytes.  The lower order
bytes and the upper integer part of the double wide number can then be
ignored.

LSB                                                                    MSB
  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _
r <------------------------- 2*g_bn_length ---------------------------->
                                                  2*g_int_length  --->  <---
                                 LSB                                  MSB
                r+g_shift_factor   <--------  g_bn_length  ----------->
                                                    g_int_length  ---> <---

PARTIAL PRECISION MULTIPLICATION:
( mult_bn(), square_bn() )

In most cases, full double precision multiplication is not necessary.  The
lower order bytes are usually thrown away anyway.  The non-"full"
multiplication routines only calculate g_r_length bytes in the result.  The
value of g_r_length must be in the range: 2*g_bn_length <= g_r_length < g_bn_length.
The amount by which g_r_length exceeds g_bn_length accounts for the extra bytes
that must be multiplied so that the first g_bn_length bytes are correct.
These extra bytes are referred to in the code as the "padding," that is:
g_r_length=g_bn_length+g_padding.

All three of the values, g_bn_length, g_r_length, and therefore g_padding, must be
multiples of the size of memory blocks being used for arithmetic (2 on
8086/286 and 4 on 386+).  Typically, the g_padding is 2*blocksize.  In the
case where g_bn_length=blocksize, g_padding can only be blocksize to keep
g_r_length from being too big.

The product of two bignumbers, n1 and n2, will then be a result, r, which
is of length g_r_length.  The integer part will be twice as wide, thereby
eliminating the possiblity of overflowing the number.

         LSB                                            MSB
           _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _
        r  <---- g_r_length = g_bn_length+g_padding ---->
                                   2*g_int_length  --->  <---

If r needs to be converted to a normal, single width bignumber, this is
easily done with pointer arithmetic.  The converted value starts at
r+g_shift_factor (where g_shift_factor = g_padding-g_int_length) and continues for
g_bn_length bytes.  The lower order bytes and the upper integer part of the
double wide number can then be ignored.

         LSB                                            MSB
           _  _  _  _  _  _  _  _  _  _  _  _  _  _  _  _
        r  <---- g_r_length = g_bn_length+g_padding ---->
                                   2*g_int_length  --->  <---
                   LSB                                MSB
   r+g_shift_factor  <--------  g_bn_length  --------->
                                     g_int_length ---> <---
*/

/************************************************************************/
// There are three parts to the bignum library:
//
// 1) bignum.c - initialization, general routines, routines that would
//    not be speeded up much with assembler.
//
// 2) bignuma.asm - hand coded assembler routines.
//
// 3) bignumc.c - portable C versions of routines in bignuma.asm
//
/************************************************************************/
#include "math/big.h"

#include <config/port.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

/*************************************************************************
* The original bignumber code was written specifically for a Little Endian
* system (80x86).  The following is not particularly efficient, but was
* simple to incorporate.  If speed with a Big Endian machine is critical,
* the bignumber format could be reversed.
**************************************************************************/
#ifdef ACCESS_BY_BYTE
U32 BIG_ACCESS32(Byte *addr)
{
    return addr[0] | ((U32)addr[1] << 8) | ((U32)addr[2] << 16) | ((U32)addr[3] << 24);
}

U16 BIG_ACCESS16(Byte *addr)
{
    return (U16)addr[0] | ((U16)addr[1] << 8);
}

S16 BIG_ACCESS_S16(S16 *addr)
{
    return (S16)((Byte *)addr)[0] | ((S16)((Byte *)addr)[1] << 8);
}

U32 BIG_SET32(Byte *addr, U32 val)
{
    addr[0] = (Byte)(val&0xff);
    addr[1] = (Byte)((val >> 8)&0xff);
    addr[2] = (Byte)((val >> 16)&0xff);
    addr[3] = (Byte)((val >> 24)&0xff);
    return val;
}

U16 BIG_SET16(Byte *addr, U16 val)
{
    addr[0] = (Byte)(val&0xff);
    addr[1] = (Byte)((val >> 8)&0xff);
    return val;
}

S16 BIG_SET_S16(S16 *addr, S16 val)
{
    ((Byte *)addr)[0] = (Byte)(val&0xff);
    ((Byte *)addr)[1] = (Byte)((val >> 8)&0xff);
    return val;
}

#endif

/************************************************************************/
// convert_bn  -- convert bignum numbers from old to new lengths
int convert_bn(BigNum new_num, BigNum old_num, int new_bn_len, int new_int_len,
               int old_bn_len, int old_int_len)
{
    // save lengths so not dependent on external environment
    int save_int_length = g_int_length;
    int save_bn_length = g_bn_length;

    g_int_length     = new_int_len;
    g_bn_length      = new_bn_len;
    clear_bn(new_num);

    if (new_bn_len - new_int_len > old_bn_len - old_int_len)
    {

        // This will keep the integer part from overflowing past the array.
        g_bn_length = old_bn_len - old_int_len + std::min(old_int_len, new_int_len);

        std::memcpy(new_num+new_bn_len-new_int_len-old_bn_len+old_int_len,
               old_num, g_bn_length);
    }
    else
    {
        g_bn_length = new_bn_len - new_int_len + std::min(old_int_len, new_int_len);
        std::memcpy(new_num, old_num+old_bn_len-old_int_len-new_bn_len+new_int_len,
               g_bn_length);
    }
    g_int_length = save_int_length;
    g_bn_length  = save_bn_length;
    return 0;
}

/********************************************************************/
// bn_hexdump() - for debugging, dumps to stdout

void bn_hex_dump(BigNum r)
{
    for (int i = 0; i < g_bn_length; i++)
    {
        std::printf("%02X ", r[i]);
    }
    std::printf("\n");
}

/**********************************************************************/
// strtobn() - converts a string into a bignumer
//   r - pointer to a bignumber
//   s - string in the floating point format [-][digits].[digits]
//   note: the string may not be empty or have extra space and may
//   not use scientific notation (2.3e4).

BigNum str_to_bn(BigNum r, char *s)
{
    bool sign_flag = false;
    long value;

    clear_bn(r);
    BigNum ones_byte = r + g_bn_length - g_int_length;

    if (s[0] == '+')    // for + sign
    {
        s++;
    }
    else if (s[0] == '-')    // for neg sign
    {
        sign_flag = true;
        s++;
    }

    if (std::strchr(s, '.') != nullptr) // is there a decimal point?
    {
        int l = (int) std::strlen(s) - 1;      // start with the last digit
        while (s[l] >= '0' && s[l] <= '9') // while a digit
        {
            *ones_byte = (Byte)(s[l--] - '0');
            div_a_bn_int(r, 10);
        }

        if (s[l] == '.')
        {
            value = std::atol(s);
            switch (g_int_length)
            {
                // only 1, 2, or 4 are allowed
            case 1:
                *ones_byte = (Byte)value;
                break;
            case 2:
                BIG_SET16(ones_byte, (U16)value);
                break;
            case 4:
                BIG_SET32(ones_byte, value);
                break;
            }
        }
    }
    else
    {
        value = std::atol(s);
        switch (g_int_length)
        {
            // only 1, 2, or 4 are allowed
        case 1:
            *ones_byte = (Byte)value;
            break;
        case 2:
            BIG_SET16(ones_byte, (U16)value);
            break;
        case 4:
            BIG_SET32(ones_byte, value);
            break;
        }
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }

    return r;
}

/********************************************************************/
// std::strlen_needed_bn() - returns string length needed to hold bignumber

int strlen_needed_bn()
{
    int length = 3;

    // first space for integer part
    switch (g_int_length)
    {
    case 1:
        length = 3;  // max 127
        break;
    case 2:
        length = 5;  // max 32767
        break;
    case 4:
        length = 10; // max 2147483647
        break;
    }
    length += g_decimals;  // decimal part
    length += 2;         // decimal point and sign
    length += 4;         // null and a little extra for safety
    return length;
}

/********************************************************************/
// bntostr() - converts a bignumber into a string
//   s - string, must be large enough to hold the number.
//   r - bignumber
//   will covert to a floating point notation
//   SIDE-EFFECT: the bignumber, r, is destroyed.
//                Copy it first if necessary.

char *unsafe_bn_to_str(char *s, BigNum r, int dec)
{
    int l = 0;
    long value = 0;

    if (dec == 0)
    {
        dec = g_decimals;
    }
    BigNum ones_byte = r + g_bn_length - g_int_length;

    if (is_bn_neg(r))
    {
        neg_a_bn(r);
        *(s++) = '-';
    }
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        value = *ones_byte;
        break;
    case 2:
        value = BIG_ACCESS16(ones_byte);
        break;
    case 4:
        value = BIG_ACCESS32(ones_byte);
        break;
    }
    strcpy(s, std::to_string(value).c_str());
    l = (int) std::strlen(s);
    s[l++] = '.';
    for (int d = 0; d < dec; d++)
    {
        *ones_byte = 0;  // clear out highest byte
        mult_a_bn_int(r, 10);
        if (is_bn_zero(r))
        {
            break;
        }
        s[l++] = (Byte)(*ones_byte + '0');
    }
    s[l] = '\0'; // don't forget nul char

    return s;
}

/*********************************************************************/
//  b = l
//  Converts a long to a bignumber
BigNum int_to_bn(BigNum r, long value)
{
    clear_bn(r);
    BigNum ones_byte = r + g_bn_length - g_int_length;
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = (Byte)value;
        break;
    case 2:
        BIG_SET16(ones_byte, (U16)value);
        break;
    case 4:
        BIG_SET32(ones_byte, value);
        break;
    }
    return r;
}

/*********************************************************************/
//  l = floor(b), floor rounds down
//  Converts the integer part a bignumber to a long
long bn_to_int(BigNum n)
{
    long value = 0;

    BigNum ones_byte = n + g_bn_length - g_int_length;
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        value = *ones_byte;
        break;
    case 2:
        value = BIG_ACCESS16(ones_byte);
        break;
    case 4:
        value = BIG_ACCESS32(ones_byte);
        break;
    }
    return value;
}

/*********************************************************************/
//  b = f
//  Converts a double to a bignumber
BigNum float_to_bn(BigNum r, LDouble f)
{
    bool sign_flag = false;

    clear_bn(r);
    BigNum ones_byte = r + g_bn_length - g_int_length;

    if (f < 0)
    {
        sign_flag = true;
        f = -f;
    }

    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = (Byte)f;
        break;
    case 2:
        BIG_SET16(ones_byte, (U16)f);
        break;
    case 4:
        BIG_SET32(ones_byte, (U32)f);
        break;
    }

    f -= (long)f; // keep only the decimal part
    for (int i = g_bn_length-g_int_length-1; i >= 0 && f != 0.0; i--)
    {
        f *= 256;
        r[i] = (Byte)f;  // keep use the integer part
        f -= (Byte)f; // now throw away the integer part
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }

    return r;
}

/********************************************************************/
// sign(r)
int sign_bn(BigNum n)
{
    return is_bn_neg(n) ? -1 : is_bn_not_zero(n) ? 1 : 0;
}

/********************************************************************/
// r = |n|
BigNum abs_bn(BigNum r, BigNum n)
{
    copy_bn(r, n);
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r = |r|
BigNum abs_a_bn(BigNum r)
{
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r = 1/n
// uses g_bn_tmp1 - g_bn_tmp3 - global temp bignumbers
//  SIDE-EFFECTS:
//      n ends up as |n|    Make copy first if necessary.
BigNum unsafe_inv_bn(BigNum r, BigNum n)
{
    // use Newton's recursive method for zeroing in on 1/n : r=r(2-rn)

    bool sign_flag = false;
    if (is_bn_neg(n))
    {
        // will be a lot easier to deal with just positives
        sign_flag = true;
        neg_a_bn(n);
    }

    LDouble f = bn_to_float(n);
    if (f == 0) // division by zero
    {
        max_bn(r);
        return r;
    }
    f = 1/f; // approximate inverse
    long max_val = (1L << ((g_int_length << 3) - 1)) - 1;
    if (f > max_val) // check for overflow
    {
        max_bn(r);
        return r;
    }
    if (f <= -max_val)
    {
        max_bn(r);
        neg_a_bn(r);
        return r;
    }

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    // orig_bntmp1 not needed here
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    BigNum orig_r = r;
    BigNum orig_n = n;
    // orig_bntmp1        = g_bn_tmp1;

    // calculate new starting values
    g_bn_length = g_int_length + (int)(LDBL_DIG/LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bn_length - g_bn_length;
    // g_bn_tmp1 = orig_bntmp1 + orig_bn_length - g_bn_length;

    float_to_bn(r, f); // start with approximate inverse
    clear_bn(g_bn_tmp2); // will be used as 1.0 and 2.0

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bn_length - g_bn_length;
        n = orig_n + orig_bn_length - g_bn_length;
        // g_bn_tmp1 = orig_bntmp1 + orig_bn_length - g_bn_length;

        unsafe_mult_bn(g_bn_tmp1, r, n); // g_bn_tmp1=rn
        int_to_bn(g_bn_tmp2, 1);  // g_bn_tmp2 = 1.0
        if (g_bn_length == orig_bn_length && cmp_bn(g_bn_tmp2, g_bn_tmp1+g_shift_factor) == 0)    // if not different
        {
            break;  // they must be the same
        }
        int_to_bn(g_bn_tmp2, 2); // g_bn_tmp2 = 2.0
        sub_bn(g_bn_tmp3, g_bn_tmp2, g_bn_tmp1+g_shift_factor); // g_bn_tmp3=2-rn
        unsafe_mult_bn(g_bn_tmp1, r, g_bn_tmp3); // g_bn_tmp1=r(2-rn)
        copy_bn(r, g_bn_tmp1+g_shift_factor); // r = g_bn_tmp1
    }

    // restore original values
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    r             = orig_r;

    if (sign_flag)
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// r = n1/n2
//      r - result of length g_bn_length
// uses g_bn_tmp1 - g_bn_tmp3 - global temp bignumbers
//  SIDE-EFFECTS:
//      n1, n2 can end up as GARBAGE
//      Make copies first if necessary.
BigNum unsafe_div_bn(BigNum r, BigNum n1, BigNum n2)
{
    // first, check for valid data
    LDouble a = bn_to_float(n1);
    if (a == 0) // division into zero
    {
        clear_bn(r); // return 0
        return r;
    }
    LDouble b = bn_to_float(n2);
    if (b == 0) // division by zero
    {
        max_bn(r);
        return r;
    }
    LDouble f = a / b; // approximate quotient
    long max_val = (1L << ((g_int_length << 3) - 1)) - 1;
    if (f > max_val) // check for overflow
    {
        max_bn(r);
        return r;
    }
    if (f <= -max_val)
    {
        max_bn(r);
        neg_a_bn(r);
        return r;
    }
    // appears to be ok, do division

    bool sign = false;
    if (is_bn_neg(n1))
    {
        neg_a_bn(n1);
        sign = !sign;
    }
    if (is_bn_neg(n2))
    {
        neg_a_bn(n2);
        sign = !sign;
    }

    // scale n1 and n2 so: |n| >= 1/256
    // scale = (int)(log(1/fabs(a))/LOG_256) = LOG_256(1/|a|)
    int i = g_bn_length - 1;
    while (i >= 0 && n1[i] == 0)
    {
        i--;
    }
    int scale1 = g_bn_length - i - 2;
    scale1 = std::max(scale1, 0);
    i = g_bn_length-1;
    while (i >= 0 && n2[i] == 0)
    {
        i--;
    }
    int scale2 = g_bn_length - i - 2;
    scale2 = std::max(scale2, 0);

    // shift n1, n2
    // important!, use std::memmove(), not std::memcpy()
    std::memmove(n1+scale1, n1, g_bn_length-scale1); // shift bytes over
    std::memset(n1, 0, scale1);  // zero out the rest
    std::memmove(n2+scale2, n2, g_bn_length-scale2); // shift bytes over
    std::memset(n2, 0, scale2);  // zero out the rest

    unsafe_inv_bn(r, n2);
    unsafe_mult_bn(g_bn_tmp1, n1, r);
    copy_bn(r, g_bn_tmp1+g_shift_factor); // r = g_bn_tmp1

    if (scale1 != scale2)
    {
        // Rescale r back to what it should be.  Overflow has already been checked
        if (scale1 > scale2) // answer is too big, adjust it
        {
            int scale = scale1-scale2;
            std::memmove(r, r+scale, g_bn_length-scale); // shift bytes over
            std::memset(r+g_bn_length-scale, 0, scale);  // zero out the rest
        }
        else if (scale1 < scale2) // answer is too small, adjust it
        {
            int scale = scale2-scale1;
            std::memmove(r+scale, r, g_bn_length-scale); // shift bytes over
            std::memset(r, 0, scale);                 // zero out the rest
        }
        // else scale1 == scale2
    }

    if (sign)
    {
        neg_a_bn(r);
    }

    return r;
}

/********************************************************************/
// sqrt(r)
// uses g_bn_tmp1 - g_bn_tmp6 - global temp bignumbers
//  SIDE-EFFECTS:
//      n ends up as |n|
BigNum sqrt_bn(BigNum r, BigNum n)
{
    int almost_match = 0;

    // use Newton's recursive method for zeroing in on sqrt(n): r=.5(r+n/r)

    if (is_bn_neg(n))
    {
        // sqrt of a neg, return 0
        clear_bn(r);
        return r;
    }

    LDouble f = bn_to_float(n);
    if (f == 0) // division by zero will occur
    {
        clear_bn(r); // sqrt(0) = 0
        return r;
    }
    f = std::sqrt(f); // approximate square root
    // no need to check overflow

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    BigNum orig_r = r;
    BigNum orig_n = n;

    // calculate new starting values
    g_bn_length = g_int_length + (int)(LDBL_DIG/LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bn_length - g_bn_length;

    float_to_bn(r, f); // start with approximate sqrt
    copy_bn(g_bn_tmp4, r);

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bn_length - g_bn_length;
        n = orig_n + orig_bn_length - g_bn_length;

        copy_bn(g_bn_tmp6, r);
        copy_bn(g_bn_tmp5, n);
        unsafe_div_bn(g_bn_tmp4, g_bn_tmp5, g_bn_tmp6);
        add_a_bn(r, g_bn_tmp4);
        half_a_bn(r);
        if (g_bn_length == orig_bn_length)
        {
            const int comp = std::abs(cmp_bn(r, g_bn_tmp4));
            if (comp < 8)  // if match or almost match
            {
                if (comp < 4  // perfect or near perfect match
                    || almost_match == 1)   // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
        }
    }

    // restore original values
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    // cppcheck-suppress uselessAssignmentPtrArg
    r             = orig_r;

    return r;
}

/********************************************************************/
// exp(r)
// uses g_bn_tmp1, g_bn_tmp2, g_bn_tmp3 - global temp bignumbers
BigNum exp_bn(BigNum r, BigNum n)
{
    U16 fact = 1;

    if (is_bn_zero(n))
    {
        int_to_bn(r, 1);
        return r;
    }

    // use Taylor Series (very slow convergence)
    int_to_bn(r, 1); // start with r=1.0
    copy_bn(g_bn_tmp2, r);
    while (true)
    {
        // copy n, if n is negative, mult_bn() alters n
        unsafe_mult_bn(g_bn_tmp3, g_bn_tmp2, copy_bn(g_bn_tmp1, n));
        copy_bn(g_bn_tmp2, g_bn_tmp3+g_shift_factor);
        div_a_bn_int(g_bn_tmp2, fact);
        if (!is_bn_not_zero(g_bn_tmp2))
        {
            break; // too small to register
        }
        add_a_bn(r, g_bn_tmp2);
        fact++;
    }
    return r;
}

/********************************************************************/
// ln(r)
// uses g_bn_tmp1 - g_bn_tmp6 - global temp bignumbers
//  SIDE-EFFECTS:
//      n ends up as |n|
BigNum unsafe_ln_bn(BigNum r, BigNum n)
{
    int almost_match = 0;

    // use Newton's recursive method for zeroing in on ln(n): r=r+n*exp(-r)-1

    if (is_bn_neg(n) || is_bn_zero(n))
    {
        // error, return largest neg value
        max_bn(r);
        neg_a_bn(r);
        return r;
    }

    LDouble f = bn_to_float(n);
    f = std::log(f); // approximate ln(x)
    long max_val = (1L << ((g_int_length << 3) - 1)) - 1;
    if (f > max_val) // check for overflow
    {
        max_bn(r);
        return r;
    }
    if (f <= -max_val)
    {
        max_bn(r);
        neg_a_bn(r);
        return r;
    }
    // appears to be ok, do ln

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    BigNum orig_r = r;
    BigNum orig_n = n;
    BigNum orig_bn_tmp5 = g_bn_tmp5;
    BigNum orig_bn_tmp4 = g_bn_tmp4;

    int_to_bn(g_bn_tmp4, 1); // set before setting new values

    // calculate new starting values
    g_bn_length = g_int_length + (int)(LDBL_DIG/LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bn_length - g_bn_length;
    g_bn_tmp5 = orig_bn_tmp5 + orig_bn_length - g_bn_length;
    g_bn_tmp4 = orig_bn_tmp4 + orig_bn_length - g_bn_length;

    float_to_bn(r, f); // start with approximate ln
    neg_a_bn(r); // -r
    copy_bn(g_bn_tmp5, r); // -r

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bn_length - g_bn_length;
        n = orig_n + orig_bn_length - g_bn_length;
        g_bn_tmp5 = orig_bn_tmp5 + orig_bn_length - g_bn_length;
        g_bn_tmp4 = orig_bn_tmp4 + orig_bn_length - g_bn_length;
        exp_bn(g_bn_tmp6, r);     // exp(-r)
        unsafe_mult_bn(g_bn_tmp2, g_bn_tmp6, n);  // n*exp(-r)
        sub_a_bn(g_bn_tmp2+g_shift_factor, g_bn_tmp4);   // n*exp(-r) - 1
        sub_a_bn(r, g_bn_tmp2+g_shift_factor);        // -r - (n*exp(-r) - 1)

        if (g_bn_length == orig_bn_length)
        {
            const int comp = std::abs(cmp_bn(r, g_bn_tmp5));
            if (comp < 8)  // if match or almost match
            {
                if (comp < 4  // perfect or near perfect match
                    || almost_match == 1)   // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
        }
        copy_bn(g_bn_tmp5, r); // -r
    }

    // restore original values
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    r             = orig_r;
    g_bn_tmp5        = orig_bn_tmp5;
    g_bn_tmp4        = orig_bn_tmp4;

    neg_a_bn(r); // -(-r)
    return r;
}

/********************************************************************/
// sin_cos_bn(r)
// uses g_bn_tmp1 - g_bn_tmp2 - global temp bignumbers
//  SIDE-EFFECTS:
//      n ends up as |n| mod (pi/4)
BigNum unsafe_sin_cos_bn(BigNum s, BigNum c, BigNum n)
{
    U16 fact = 2;
    bool k = false;

#ifndef CALCULATING_BIG_PI
    // assure range 0 <= x < pi/4

    if (is_bn_zero(n))
    {
        clear_bn(s);    // sin(0) = 0
        int_to_bn(c, 1);  // cos(0) = 1
        return s;
    }

    bool sign_cos = false;
    bool sign_sin = false;
    bool switch_sin_cos = false;
    if (is_bn_neg(n))
    {
        sign_sin = !sign_sin; // sin(-x) = -sin(x), odd; cos(-x) = cos(x), even
        neg_a_bn(n);
    }
    // n >= 0

    double_bn(g_bn_tmp1, g_bn_pi); // 2*pi
    // this could be done with remainders, but it would probably be slower
    while (cmp_bn(n, g_bn_tmp1) >= 0)   // while n >= 2*pi
    {
        sub_a_bn(n, g_bn_tmp1);
    }
    // 0 <= n < 2*pi

    copy_bn(g_bn_tmp1, g_bn_pi); // pi
    if (cmp_bn(n, g_bn_tmp1) >= 0) // if n >= pi
    {
        sub_a_bn(n, g_bn_tmp1);
        sign_sin = !sign_sin;
        sign_cos = !sign_cos;
    }
    // 0 <= n < pi

    half_bn(g_bn_tmp1, g_bn_pi); // pi/2
    if (cmp_bn(n, g_bn_tmp1) > 0) // if n > pi/2
    {
        sub_bn(n, g_bn_pi, n);   // pi - n
        sign_cos = !sign_cos;
    }
    // 0 <= n < pi/2

    half_bn(g_bn_tmp1, g_bn_pi); // pi/2
    half_a_bn(g_bn_tmp1);      // pi/4
    if (cmp_bn(n, g_bn_tmp1) > 0) // if n > pi/4
    {
        half_bn(g_bn_tmp1, g_bn_pi); // pi/2
        sub_bn(n, g_bn_tmp1, n);  // pi/2 - n
        switch_sin_cos = !switch_sin_cos;
    }
    // 0 <= n < pi/4

    // this looks redundant, but n could now be zero when it wasn't before
    if (is_bn_zero(n))
    {
        clear_bn(s);    // sin(0) = 0
        int_to_bn(c, 1);  // cos(0) = 1
        return s;
    }

    // at this point, the double angle trig identities could be used as many
    // times as desired to reduce the range to pi/8, pi/16, etc...  Each time
    // the range is cut in half, the number of iterations required is reduced
    // by "quite a bit."  It's just a matter of testing to see what gives the
    // optimal results.
    // halves = g_bn_length / 10; */ /* this is experimental
    int halves = 1;
    for (int i = 0; i < halves; i++)
    {
        half_a_bn(n);
    }
#endif

    // use Taylor Series (very slow convergence)
    copy_bn(s, n); // start with s=n
    int_to_bn(c, 1); // start with c=1
    copy_bn(g_bn_tmp1, n); // the current x^n/n!

    while (true)
    {
        // even terms for cosine
        unsafe_mult_bn(g_bn_tmp2, g_bn_tmp1, n);
        copy_bn(g_bn_tmp1, g_bn_tmp2+g_shift_factor);
        div_a_bn_int(g_bn_tmp1, fact++);
        if (!is_bn_not_zero(g_bn_tmp1))
        {
            break; // too small to register
        }
        if (k)   // alternate between adding and subtracting
        {
            add_a_bn(c, g_bn_tmp1);
        }
        else
        {
            sub_a_bn(c, g_bn_tmp1);
        }

        // odd terms for sine
        unsafe_mult_bn(g_bn_tmp2, g_bn_tmp1, n);
        copy_bn(g_bn_tmp1, g_bn_tmp2+g_shift_factor);
        div_a_bn_int(g_bn_tmp1, fact++);
        if (!is_bn_not_zero(g_bn_tmp1))
        {
            break; // too small to register
        }
        if (k)   // alternate between adding and subtracting
        {
            add_a_bn(s, g_bn_tmp1);
        }
        else
        {
            sub_a_bn(s, g_bn_tmp1);
        }
        k = !k; // toggle
#ifdef CALCULATING_BIG_PI
        std::printf("."); // lets you know it's doing something
#endif
    }

#ifndef CALCULATING_BIG_PI
    // now need to undo what was done by cutting angles in half
    int_to_bn(g_bn_tmp1, 1);
    for (int i = 0; i < halves; i++)
    {
        unsafe_mult_bn(g_bn_tmp2, s, c); // no need for safe mult
        double_bn(s, g_bn_tmp2+g_shift_factor); // sin(2x) = 2*sin(x)*cos(x)
        unsafe_square_bn(g_bn_tmp2, c);
        double_a_bn(g_bn_tmp2+g_shift_factor);
        sub_bn(c, g_bn_tmp2+g_shift_factor, g_bn_tmp1); // cos(2x) = 2*cos(x)*cos(x) - 1
    }

    if (switch_sin_cos)
    {
        copy_bn(g_bn_tmp1, s);
        copy_bn(s, c);
        copy_bn(c, g_bn_tmp1);
    }
    if (sign_sin)
    {
        neg_a_bn(s);
    }
    if (sign_cos)
    {
        neg_a_bn(c);
    }
#endif

    return s; // return sine I guess
}

/********************************************************************/
// atan(r)
// uses g_bn_tmp1 - g_bn_tmp5 - global temp bignumbers
//  SIDE-EFFECTS:
//      n ends up as |n| or 1/|n|
BigNum unsafe_atan_bn(BigNum r, BigNum n)
{
    int almost_match = 0;

    // use Newton's recursive method for zeroing in on atan(n): r=r-cos(r)(sin(r)-n*cos(r))
    bool sign_flag = false;
    if (is_bn_neg(n))
    {
        sign_flag = true;
        neg_a_bn(n);
    }

    // If n is very large, atanl() won't give enough decimal places to be a
    // good enough initial guess for Newton's Method.  If it is larger than
    // say, 1, atan(n) = pi/2 - acot(n) = pi/2 - atan(1/n).

    LDouble f = bn_to_float(n);
    bool large_arg = f > 1.0;
    if (large_arg)
    {
        unsafe_inv_bn(g_bn_tmp3, n);
        copy_bn(n, g_bn_tmp3);
        f = bn_to_float(n);
    }

    clear_bn(g_bn_tmp3); // not really necessary, but makes things more consistent

    // With Newton's Method, there is no need to calculate all the digits
    // every time.  The precision approximately doubles each iteration.
    // Save original values.
    int orig_bn_length = g_bn_length;
    int orig_padding = g_padding;
    int orig_r_length = g_r_length;
    int orig_shift_factor = g_shift_factor;
    BigNum orig_bn_pi = g_bn_pi;
    BigNum orig_r = r;
    BigNum orig_n = n;
    BigNum orig_bn_tmp3 = g_bn_tmp3;

    // calculate new starting values
    g_bn_length = g_int_length + (int)(LDBL_DIG/LOG10_256) + 1; // round up
    g_bn_length = std::min(g_bn_length, orig_bn_length);
    calc_lengths();

    // adjust pointers
    r = orig_r + orig_bn_length - g_bn_length;
    g_bn_pi = orig_bn_pi + orig_bn_length - g_bn_length;
    g_bn_tmp3 = orig_bn_tmp3 + orig_bn_length - g_bn_length;

    f = atanl(f); // approximate arctangent
    // no need to check overflow

    float_to_bn(r, f); // start with approximate atan
    copy_bn(g_bn_tmp3, r);

    for (int i = 0; i < 25; i++) // safety net, this shouldn't ever be needed
    {
        // adjust lengths
        g_bn_length <<= 1; // double precision
        g_bn_length = std::min(g_bn_length, orig_bn_length);
        calc_lengths();
        r = orig_r + orig_bn_length - g_bn_length;
        n = orig_n + orig_bn_length - g_bn_length;
        g_bn_pi = orig_bn_pi + orig_bn_length - g_bn_length;
        g_bn_tmp3 = orig_bn_tmp3 + orig_bn_length - g_bn_length;

#ifdef CALCULATING_BIG_PI
        std::printf("\natan() loop #%i, g_bn_length=%i\nsin_cos() loops\n", i, g_bn_length);
#endif
        unsafe_sin_cos_bn(g_bn_tmp4, g_bn_tmp5, g_bn_tmp3);   // sin(r), cos(r)
        copy_bn(g_bn_tmp3, r); // restore g_bn_tmp3 from sin_cos_bn()
        copy_bn(g_bn_tmp1, g_bn_tmp5);
        unsafe_mult_bn(g_bn_tmp2, n, g_bn_tmp1);     // n*cos(r)
        sub_a_bn(g_bn_tmp4, g_bn_tmp2+g_shift_factor); // sin(r) - n*cos(r)
        unsafe_mult_bn(g_bn_tmp1, g_bn_tmp5, g_bn_tmp4); // cos(r) * (sin(r) - n*cos(r))
        sub_a_bn(r, g_bn_tmp1+g_shift_factor); // r - cos(r) * (sin(r) - n*cos(r))

#ifdef CALCULATING_BIG_PI
        putchar('\n');
        bn_hexdump(r);
#endif
        if (g_bn_length == orig_bn_length)
        {
            const int comp = std::abs(cmp_bn(r, g_bn_tmp3));
            if (comp < 8)  // if match or almost match
            {
#ifdef CALCULATING_BIG_PI
                std::printf("atan() loop comp=%i\n", comp);
#endif
                if (comp < 4              // perfect or near perfect match
                    || almost_match == 1) // close enough for 2nd time
                {
                    break;
                }
                // this is the first time they almost matched
                almost_match++;
            }
#ifdef CALCULATING_BIG_PI
            if (comp >= 8)
            {
                std::printf("atan() loop comp=%i\n", comp);
            }
#endif
        }

        copy_bn(g_bn_tmp3, r); // make a copy for later comparison
    }

    // restore original values
    g_bn_length   = orig_bn_length;
    g_padding     = orig_padding;
    g_r_length    = orig_r_length;
    g_shift_factor = orig_shift_factor;
    g_bn_pi         = orig_bn_pi;
    r             = orig_r;
    g_bn_tmp3        = orig_bn_tmp3;

    if (large_arg)
    {
        half_bn(g_bn_tmp3, g_bn_pi);  // pi/2
        sub_a_bn(g_bn_tmp3, r);     // pi/2 - atan(1/n)
        copy_bn(r, g_bn_tmp3);
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }
    return r;
}

/********************************************************************/
// atan2(r, ny, nx)
// uses g_bn_tmp1 - g_bn_tmp6 - global temp bigfloats
BigNum unsafe_atan2_bn(BigNum r, BigNum ny, BigNum nx)
{
    int sign_x = sign_bn(nx);
    int sign_y = sign_bn(ny);

    if (sign_y == 0)
    {
        if (sign_x < 0)
        {
            copy_bn(r, g_bn_pi); // negative x axis, 180 deg
        }
        else        // sign_x >= 0    positive x axis, 0
        {
            clear_bn(r);
        }
        return r;
    }
    if (sign_x == 0)
    {
        copy_bn(r, g_bn_pi); // y axis
        half_a_bn(r);      // +90 deg
        if (sign_y < 0)
        {
            neg_a_bn(r);    // -90 deg
        }
        return r;
    }

    if (sign_y < 0)
    {
        neg_a_bn(ny);
    }
    if (sign_x < 0)
    {
        neg_a_bn(nx);
    }
    unsafe_div_bn(g_bn_tmp6, ny, nx);
    unsafe_atan_bn(r, g_bn_tmp6);
    if (sign_x < 0)
    {
        sub_bn(r, g_bn_pi, r);
    }
    if (sign_y < 0)
    {
        neg_a_bn(r);
    }
    return r;
}

/**********************************************************************/
// The rest of the functions are "safe" versions of the routines that
// have side effects which alter the parameters
/**********************************************************************/

/**********************************************************************/
BigNum full_mult_bn(BigNum r, BigNum n1, BigNum n2)
{
    bool sign1 = is_bn_neg(n1);
    bool sign2 = is_bn_neg(n2);
    unsafe_full_mult_bn(r, n1, n2);
    if (sign1)
    {
        neg_a_bn(n1);
    }
    if (sign2)
    {
        neg_a_bn(n2);
    }
    return r;
}

/**********************************************************************/
BigNum mult_bn(BigNum r, BigNum n1, BigNum n2)
{
    bool sign1 = is_bn_neg(n1);
    bool sign2 = is_bn_neg(n2);
    unsafe_mult_bn(r, n1, n2);
    if (sign1)
    {
        neg_a_bn(n1);
    }
    if (sign2)
    {
        neg_a_bn(n2);
    }
    return r;
}

/**********************************************************************/
BigNum full_square_bn(BigNum r, BigNum n)
{
    bool sign = is_bn_neg(n);
    unsafe_full_square_bn(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

/**********************************************************************/
BigNum square_bn(BigNum r, BigNum n)
{
    bool sign = is_bn_neg(n);
    unsafe_square_bn(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

/**********************************************************************/
BigNum div_bn_int(BigNum r, BigNum n, U16 u)
{
    bool sign = is_bn_neg(n);
    unsafe_div_bn_int(r, n, u);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

/**********************************************************************/
char *bn_to_str(char *s, BigNum r, int dec)
{
    return unsafe_bn_to_str(s, copy_bn(g_bn_tmp_copy2, r), dec);
}

/**********************************************************************/
BigNum inv_bn(BigNum r, BigNum n)
{
    bool sign = is_bn_neg(n);
    unsafe_inv_bn(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

/**********************************************************************/
BigNum div_bn(BigNum r, BigNum n1, BigNum n2)
{
    copy_bn(g_bn_tmp_copy1, n1);
    copy_bn(g_bn_tmp_copy2, n2);
    return unsafe_div_bn(r, g_bn_tmp_copy1, g_bn_tmp_copy2);
}

/**********************************************************************/
BigNum ln_bn(BigNum r, BigNum n)
{
    copy_bn(g_bn_tmp_copy1, n); // allows r and n to overlap memory
    unsafe_ln_bn(r, g_bn_tmp_copy1);
    return r;
}

/**********************************************************************/
BigNum sin_cos_bn(BigNum s, BigNum c, BigNum n)
{
    return unsafe_sin_cos_bn(s, c, copy_bn(g_bn_tmp_copy1, n));
}

/**********************************************************************/
BigNum atan_bn(BigNum r, BigNum n)
{
    bool sign = is_bn_neg(n);
    unsafe_atan_bn(r, n);
    if (sign)
    {
        neg_a_bn(n);
    }
    return r;
}

/**********************************************************************/
BigNum atan2_bn(BigNum r, BigNum ny, BigNum nx)
{
    copy_bn(g_bn_tmp_copy1, ny);
    copy_bn(g_bn_tmp_copy2, nx);
    unsafe_atan2_bn(r, g_bn_tmp_copy1, g_bn_tmp_copy2);
    return r;
}

bool is_bn_zero(BigNum n)
{
    return !is_bn_not_zero(n);
}
