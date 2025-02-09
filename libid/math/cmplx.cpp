// SPDX-License-Identifier: GPL-3.0-only
//
#include "math/cmplx.h"

#include "io/loadfile.h"
#include "math/fixed_pt.h"
#include "math/fpu087.h"

#include <cmath>

DComplex complex_sqrt_float(double x, double y)
{
    DComplex  result;

    if (x == 0.0 && y == 0.0)
    {
        result.x = 0.0;
        result.y = 0.0;
    }
    else
    {
        double mag = std::sqrt(std::sqrt(x*x + y*y));
        double theta = std::atan2(y, x) / 2;
        sin_cos(&theta, &result.y, &result.x);
        result.x *= mag;
        result.y *= mag;
    }
    return result;
}

DComplex complex_power(DComplex xx, DComplex yy)
{
    DComplex z;
    DComplex c_log;
    DComplex t;

    if (xx.x == 0 && xx.y == 0)
    {
        z.y = 0.0;
        z.x = z.y;
        return z;
    }

    fpu_cmplx_log(&xx, &c_log);
    fpu_cmplx_mul(&c_log, &yy, &t);
    fpu_cmplx_exp(&t, &z);
    return z;
}

/*

  The following Complex function routines added by Tim Wegner November 1994.

*/

// rz=Arcsin(z)=-i*Log{i*z+sqrt(1-z*z)}
void asin_z(DComplex z, DComplex *rz)
{
    DComplex temp_z1;
    DComplex temp_z2;

    fpu_cmplx_mul(&z, &z, &temp_z1);
    temp_z1.x = 1 - temp_z1.x;
    temp_z1.y = -temp_z1.y;  // tempz1 = 1 - tempz1
    temp_z1 = complex_sqrt_float(temp_z1);

    temp_z2.x = -z.y;
    temp_z2.y = z.x;                // tempz2 = i*z
    temp_z1.x += temp_z2.x;
    temp_z1.y += temp_z2.y;    // tempz1 += tempz2
    fpu_cmplx_log(&temp_z1, &temp_z1);
    rz->x = temp_z1.y;
    rz->y = -temp_z1.x;           // rz = (-i)*tempz1
}   // end. Arcsinz

// rz=Arccos(z)=-i*Log{z+sqrt(z*z-1)}
void acos_z(DComplex z, DComplex *rz)
{
    DComplex temp;

    fpu_cmplx_mul(&z, &z, &temp);
    temp.x -= 1;                                 // temp = temp - 1
    temp = complex_sqrt_float(temp);

    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp

    fpu_cmplx_log(&temp, &temp);
    rz->x = temp.y;
    rz->y = -temp.x;              // rz = (-i)*tempz1
}   // end. Arccosz

void asinh_z(DComplex z, DComplex *rz)
{
    DComplex temp;

    fpu_cmplx_mul(&z, &z, &temp);
    temp.x += 1;                                 // temp = temp + 1
    temp = complex_sqrt_float(temp);
    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp
    fpu_cmplx_log(&temp, rz);
}  // end. Arcsinhz

// rz=Arccosh(z)=Log(z+sqrt(z*z-1)}
void acosh_z(DComplex z, DComplex *rz)
{
    DComplex temp_z;
    fpu_cmplx_mul(&z, &z, &temp_z);
    temp_z.x -= 1;                              // tempz = tempz - 1
    temp_z = complex_sqrt_float(temp_z);
    temp_z.x = z.x + temp_z.x;
    temp_z.y = z.y + temp_z.y;  // tempz = z + tempz
    fpu_cmplx_log(&temp_z, rz);
}   // end. Arccoshz

// rz=Arctanh(z)=1/2*Log{(1+z)/(1-z)}
void atanh_z(DComplex z, DComplex *rz)
{
    DComplex temp0;
    DComplex temp1;
    DComplex temp2;

    if (z.x == 0.0)
    {
        rz->x = 0;
        rz->y = std::atan(z.y);
    }
    else
    {
        if (std::abs(z.x) == 1.0 && z.y == 0.0)
        {
        }
        else if (std::abs(z.x) < 1.0 && z.y == 0.0)
        {
            rz->x = std::log((1+z.x)/(1-z.x))/2;
            rz->y = 0;
        }
        else
        {
            temp0.x = 1 + z.x;
            temp0.y = z.y;             // temp0 = 1 + z
            temp1.x = 1 - z.x;
            temp1.y = -z.y;            // temp1 = 1 - z
            fpu_cmplx_div(&temp0, &temp1, &temp2);
            fpu_cmplx_log(&temp2, &temp2);
            rz->x = .5*temp2.x;
            rz->y = .5*temp2.y;       // rz = .5*temp2
        }
    }
}   // end. Arctanhz

// rz=Arctan(z)=i/2*Log{(1-i*z)/(1+i*z)}
void atan_z(DComplex z, DComplex *rz)
{
    DComplex temp0;
    DComplex temp1;
    DComplex temp2;
    DComplex temp3;
    if (z.x == 0.0 && z.y == 0.0)
    {
        rz->y = 0;
        rz->x = 0;
    }
    else if (z.x != 0.0 && z.y == 0.0)
    {
        rz->x = std::atan(z.x);
        rz->y = 0;
    }
    else if (z.x == 0.0 && z.y != 0.0)
    {
        temp0.x = z.y;
        temp0.y = 0.0;
        atanh_z(temp0, &temp0);
        rz->x = -temp0.y;
        rz->y = temp0.x;              // i*temp0
    }
    else if (z.x != 0.0 && z.y != 0.0)
    {

        temp0.x = -z.y;
        temp0.y = z.x;                  // i*z
        temp1.x = 1 - temp0.x;
        temp1.y = -temp0.y;      // temp1 = 1 - temp0
        temp2.x = 1 + temp0.x;
        temp2.y = temp0.y;       // temp2 = 1 + temp0

        fpu_cmplx_div(&temp1, &temp2, &temp3);
        fpu_cmplx_log(&temp3, &temp3);
        rz->x = -temp3.y*.5;
        rz->y = .5*temp3.x;           // .5*i*temp0
    }
}   // end. Arctanz
