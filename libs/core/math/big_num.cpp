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
