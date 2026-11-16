// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
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
bntofloat(), floattobn(), div_bn().  The number of bytes
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
#include <math/big.h>
#include <math/big_init.h>
#include <math/BigNum.h>

#include <config/port.h>

#include <algorithm>
#include <cfloat>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>

using namespace id::misc;

namespace id::math
{
namespace
{

long parse_long(const std::string_view text)
{
    long value{};
    std::from_chars(text.data(), text.data() + text.size(), value);
    return value;
}

void set_integer_part(BigNumPtr ones_byte, const long value)
{
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = static_cast<Byte>(value);
        break;
    case 2:
        BIG_SET16(ones_byte, static_cast<U16>(value));
        break;
    case 4:
        BIG_SET32(ones_byte, value);
        break;
    }
}

std::string unsafe_bn_to_string(BigNumPtr r, int dec)
{
    long value = 0;

    if (dec == 0)
    {
        dec = g_decimals;
    }
    BigNumPtr ones_byte = r + g_bn_length - g_int_length;

    std::string result;
    if (is_bn_neg(r))
    {
        neg_a_bn(r);
        result += '-';
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
    result += std::to_string(value);
    result += '.';
    for (int d = 0; d < dec; d++)
    {
        *ones_byte = 0; // clear out highest byte
        mult_a_bn_int(r, 10);
        if (is_bn_zero(r))
        {
            break;
        }
        result += static_cast<char>(*ones_byte + '0');
    }

    return result;
}

} // namespace

/*************************************************************************
* The original bignumber code was written specifically for a Little Endian
* system (80x86).  The following is not particularly efficient, but was
* simple to incorporate.  If speed with a Big Endian machine is critical,
* the bignumber format could be reversed.
**************************************************************************/
#ifdef ACCESS_BY_BYTE
U32 BIG_ACCESS32(const Byte *addr)
{
    return addr[0] | ((U32)addr[1] << 8) | ((U32)addr[2] << 16) | ((U32)addr[3] << 24);
}

U16 BIG_ACCESS16(const Byte *addr)
{
    return (U16)addr[0] | ((U16)addr[1] << 8);
}

S16 BIG_ACCESS_S16(const S16 *addr)
{
    return (S16) ((const Byte *) addr)[0] | ((S16) ((const Byte *) addr)[1] << 8);
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
int convert_bn(BigNumPtr new_num, BigNumConstPtr old_num, const int new_bn_len, const int new_int_len,
    const int old_bn_len, const int old_int_len)
{
    const int new_fraction = new_bn_len - new_int_len;
    const int old_fraction = old_bn_len - old_int_len;
    const int fraction = std::min(new_fraction, old_fraction);
    const int integer = std::min(new_int_len, old_int_len);
    std::memset(new_num, 0, new_bn_len);
    std::memcpy(new_num + new_fraction - fraction, old_num + old_fraction - fraction, fraction + integer);
    return 0;
}

/********************************************************************/
// bn_hexdump() - for debugging, dumps to stdout

void bn_hex_dump(const BigNum &r)
{
    for (int i = 0; i < g_bn_length; i++)
    {
        std::printf("%02X ", r.data()[i]);
    }
    std::printf("\n");
}

/**********************************************************************/
// strtobn() - converts a string into a bignumer
//   r - pointer to a bignumber
//   s - string in the floating point format [-][digits].[digits]
//   note: the string may not be empty or have extra space and may
//   not use scientific notation (2.3e4).

BigNumPtr str_to_bn(BigNumPtr r, std::string_view s)
{
    bool sign_flag = false;

    clear_bn(r);
    BigNumPtr ones_byte = r + g_bn_length - g_int_length;

    if (s[0] == '+') // for + sign
    {
        s.remove_prefix(1);
    }
    else if (s[0] == '-') // for neg sign
    {
        sign_flag = true;
        s.remove_prefix(1);
    }

    if (s.find('.') != std::string_view::npos)              // is there a decimal point?
    {
        std::size_t l = s.size();                           // start just past the last digit
        while (l > 0 && s[l - 1] >= '0' && s[l - 1] <= '9') // while a digit
        {
            *ones_byte = static_cast<Byte>(s[--l] - '0');
            div_a_bn_int(r, 10);
        }

        if (l > 0 && s[l - 1] == '.')
        {
            set_integer_part(ones_byte, parse_long(s));
        }
    }
    else
    {
        set_integer_part(ones_byte, parse_long(s));
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }

    return r;
}

/********************************************************************/
// bntostr() - converts a bignumber into a string
//   s - string, must be large enough to hold the number.
//   r - bignumber
//   will covert to a floating point notation
//   SIDE-EFFECT: the bignumber, r, is destroyed.
//                Copy it first if necessary.

//  b = l
//  Converts a long to a bignumber
BigNumPtr int_to_bn(BigNumPtr r, const long value)
{
    clear_bn(r);
    BigNumPtr ones_byte = r + g_bn_length - g_int_length;
    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = static_cast<Byte>(value);
        break;
    case 2:
        BIG_SET16(ones_byte, static_cast<U16>(value));
        break;
    case 4:
        BIG_SET32(ones_byte, value);
        break;
    }
    return r;
}

/*********************************************************************/
//  b = f
//  Converts a double to a bignumber
BigNumPtr float_to_bn(BigNumPtr r, LDouble f)
{
    bool sign_flag = false;

    clear_bn(r);
    BigNumPtr ones_byte = r + g_bn_length - g_int_length;

    if (f < 0)
    {
        sign_flag = true;
        f = -f;
    }

    switch (g_int_length)
    {
        // only 1, 2, or 4 are allowed
    case 1:
        *ones_byte = static_cast<Byte>(f);
        break;
    case 2:
        BIG_SET16(ones_byte, static_cast<U16>(f));
        break;
    case 4:
        BIG_SET32(ones_byte, static_cast<U32>(f));
        break;
    }

    f -= static_cast<long>(f); // keep only the decimal part
    for (int i = g_bn_length-g_int_length-1; i >= 0 && f != 0.0; i--)
    {
        f *= 256;
        r[i] = static_cast<Byte>(f);  // keep use the integer part
        f -= static_cast<Byte>(f); // now throw away the integer part
    }

    if (sign_flag)
    {
        neg_a_bn(r);
    }

    return r;
}

/**********************************************************************/
std::string bn_to_string(BigNumConstPtr r, const int dec)
{
    return unsafe_bn_to_string(copy_bn(g_bn_tmp_copy2.data(), r), dec);
}

} // namespace id::math
