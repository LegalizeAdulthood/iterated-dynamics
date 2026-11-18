// SPDX-License-Identifier: GPL-3.0-only
// Copyright 2026 Richard Thomson.
//
// C routines equivalent to ASM routines in bignuma.asm

/*
Wesley Loewer's Big Numbers.        (C) 1994-95, Wesley B. Loewer
*/
#include <math/big.h>
#include <math/big_init.h>

#include <array>
#include <cfloat>
#include <cstring>

using namespace id::misc;

namespace id::math
{

/********************************************************************
 The following code contains the C versions of the routines from the
 file BIGNUMA.ASM.  It is provided here for portibility and for clarity.
*********************************************************************/

/********************************************************************
 Note:  The C code must be able to detect over/underflow.  Therefore,
 32-bit  integers must be used when doing 16 bit math.  All we really
 need is one more bit, such as is provided in asm with the carry bit.
 Functions that don't need the test for over/underflow, such as cmp_bn()
 and is_bn_not_zero(), can use 32-bit integers as long as g_bn_step
 is set to 4.

 The 16/32 bit combination of integer sizes could be increased to
 32/64 bit to improve efficiency, but since many compilers don't offer
 64-bit integers, this option was not included.

*********************************************************************/

/*****************************************/
// the following used to be in bigfltc.c

} // namespace id::math
