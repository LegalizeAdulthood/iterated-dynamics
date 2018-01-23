/* MPMath_c.c (C) 1989, Mark C. Peterson, CompuServe [70441,3353]
     All rights reserved.

   Code may be used in any program provided the author is credited
     either during program execution or in the documentation.  Source
     code may be distributed only in combination with public domain or
     shareware source code.  Source code may be modified provided the
     copyright notice and this message is left unchanged and all
     modifications are clearly documented.

     I would appreciate a copy of any work which incorporates this code,
     however this is optional.

     Mark C. Peterson
     405-C Queen St. Suite #181
     Southington, CT 06489
     (203) 276-9721
*/
#include "port.h"
#include "prototyp.h"

#include "calcfrac.h"
#include "cmdfiles.h"
#include "fpu087.h"
#include "fractals.h"
#include "jiim.h"
#include "mpmath_c.h"
#include "parser.h"

#include <algorithm>

namespace
{

struct MP Ans = { 0 };

} // namespace

int g_mp_overflow = 0;

#if !defined(XFRACT)

MP *MPsub(MP x, MP y)
{
    y.Exp ^= 0x8000;
    return (MPadd(x, y));
}

MP *MPsub086(MP x, MP y)
{
    y.Exp ^= 0x8000;
    return (MPadd086(x, y));
}

MP *MPsub386(MP x, MP y)
{
    y.Exp ^= 0x8000;
    return (MPadd386(x, y));
}

MP *MPabs(MP x)
{
    Ans = x;
    Ans.Exp &= 0x7fff;
    return (&Ans);
}

MPC MPCsqr(MPC x)
{
    MPC z;

    z.x = *pMPsub(*pMPmul(x.x, x.x), *pMPmul(x.y, x.y));
    z.y = *pMPmul(x.x, x.y);
    z.y.Exp++;
    return (z);
}

MP MPCmod(MPC x)
{
    return (*pMPadd(*pMPmul(x.x, x.x), *pMPmul(x.y, x.y)));
}

MPC MPCmul(MPC x, MPC y)
{
    MPC z;

    z.x = *pMPsub(*pMPmul(x.x, y.x), *pMPmul(x.y, y.y));
    z.y = *pMPadd(*pMPmul(x.x, y.y), *pMPmul(x.y, y.x));
    return (z);
}

MPC MPCdiv(MPC x, MPC y)
{
    MP mod;

    mod = MPCmod(y);
    y.y.Exp ^= 0x8000;
    y.x = *pMPdiv(y.x, mod);
    y.y = *pMPdiv(y.y, mod);
    return (MPCmul(x, y));
}

MPC MPCadd(MPC x, MPC y)
{
    MPC z;

    z.x = *pMPadd(x.x, y.x);
    z.y = *pMPadd(x.y, y.y);
    return (z);
}

MPC MPCsub(MPC x, MPC y)
{
    MPC z;

    z.x = *pMPsub(x.x, y.x);
    z.y = *pMPsub(x.y, y.y);
    return (z);
}

MPC g_mpc_one =
{
    {0x3fff, 0x80000000l},
    {0, 0l}
};

MPC MPCpow(MPC x, int exp)
{
    MPC z;
    MPC zz;

    if (exp & 1)
    {
        z = x;
    }
    else
    {
        z = g_mpc_one;
    }
    exp >>= 1;
    while (exp)
    {
        zz.x = *pMPsub(*pMPmul(x.x, x.x), *pMPmul(x.y, x.y));
        zz.y = *pMPmul(x.x, x.y);
        zz.y.Exp++;
        x = zz;
        if (exp & 1)
        {
            zz.x = *pMPsub(*pMPmul(z.x, x.x), *pMPmul(z.y, x.y));
            zz.y = *pMPadd(*pMPmul(z.x, x.y), *pMPmul(z.y, x.x));
            z = zz;
        }
        exp >>= 1;
    }
    return (z);
}

int MPCcmp(MPC x, MPC y)
{
    MPC z;

    if (pMPcmp(x.x, y.x) || pMPcmp(x.y, y.y))
    {
        z.x = MPCmod(x);
        z.y = MPCmod(y);
        return (pMPcmp(z.x, z.y));
    }
    else
    {
        return (0);
    }
}

DComplex MPC2cmplx(MPC x)
{
    DComplex z;

    z.x = *pMP2d(x.x);
    z.y = *pMP2d(x.y);
    return (z);
}

MPC cmplx2MPC(DComplex z)
{
    MPC x;

    x.x = *pd2MP(z.x);
    x.y = *pd2MP(z.y);
    return (x);
}

int (*pMPcmp)(MP x, MP y) = MPcmp086;
MP  *(*pMPmul)(MP x, MP y) = MPmul086;
MP  *(*pMPdiv)(MP x, MP y) = MPdiv086;
MP  *(*pMPadd)(MP x, MP y) = MPadd086;
MP  *(*pMPsub)(MP x, MP y) = MPsub086;
MP  *(*pd2MP)(double x)                 = d2MP086 ;
double *(*pMP2d)(MP m)                  = MP2d086 ;

void setMPfunctions()
{
    pMPmul = MPmul386;
    pMPdiv = MPdiv386;
    pMPadd = MPadd386;
    pMPsub = MPsub386;
    pMPcmp = MPcmp386;
    pd2MP  = d2MP386 ;
    pMP2d  = MP2d386 ;
}
#endif // XFRACT

DComplex ComplexPower(DComplex xx, DComplex yy)
{
    DComplex z;
    DComplex cLog;
    DComplex t;

    if (!g_ld_check)
    {
        if (xx.x == 0 && xx.y == 0)
        {
            z.y = 0.0;
            z.x = z.y;
            return (z);
        }
    }

    FPUcplxlog(&xx, &cLog);
    FPUcplxmul(&cLog, &yy, &t);
    FPUcplxexp(&t, &z);
    return (z);
}

/*

  The following Complex function routines added by Tim Wegner November 1994.

*/

#define Sqrtz(z, rz) (*(rz) = ComplexSqrtFloat((z).x, (z).y))

// rz=Arcsin(z)=-i*Log{i*z+sqrt(1-z*z)}
void Arcsinz(DComplex z, DComplex *rz)
{
    DComplex tempz1, tempz2;

    FPUcplxmul(&z, &z, &tempz1);
    tempz1.x = 1 - tempz1.x;
    tempz1.y = -tempz1.y;  // tempz1 = 1 - tempz1
    Sqrtz(tempz1, &tempz1);

    tempz2.x = -z.y;
    tempz2.y = z.x;                // tempz2 = i*z
    tempz1.x += tempz2.x;
    tempz1.y += tempz2.y;    // tempz1 += tempz2
    FPUcplxlog(&tempz1, &tempz1);
    rz->x = tempz1.y;
    rz->y = -tempz1.x;           // rz = (-i)*tempz1
}   // end. Arcsinz


// rz=Arccos(z)=-i*Log{z+sqrt(z*z-1)}
void Arccosz(DComplex z, DComplex *rz)
{
    DComplex temp;

    FPUcplxmul(&z, &z, &temp);
    temp.x -= 1;                                 // temp = temp - 1
    Sqrtz(temp, &temp);

    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp

    FPUcplxlog(&temp, &temp);
    rz->x = temp.y;
    rz->y = -temp.x;              // rz = (-i)*tempz1
}   // end. Arccosz

void Arcsinhz(DComplex z, DComplex *rz)
{
    DComplex temp;

    FPUcplxmul(&z, &z, &temp);
    temp.x += 1;                                 // temp = temp + 1
    Sqrtz(temp, &temp);
    temp.x += z.x;
    temp.y += z.y;                // temp = z + temp
    FPUcplxlog(&temp, rz);
}  // end. Arcsinhz

// rz=Arccosh(z)=Log(z+sqrt(z*z-1)}
void Arccoshz(DComplex z, DComplex *rz)
{
    DComplex tempz;
    FPUcplxmul(&z, &z, &tempz);
    tempz.x -= 1;                              // tempz = tempz - 1
    Sqrtz(tempz, &tempz);
    tempz.x = z.x + tempz.x;
    tempz.y = z.y + tempz.y;  // tempz = z + tempz
    FPUcplxlog(&tempz, rz);
}   // end. Arccoshz

// rz=Arctanh(z)=1/2*Log{(1+z)/(1-z)}
void Arctanhz(DComplex z, DComplex *rz)
{
    DComplex temp0, temp1, temp2;

    if (z.x == 0.0)
    {
        rz->x = 0;
        rz->y = atan(z.y);
        return;
    }
    else
    {
        if (fabs(z.x) == 1.0 && z.y == 0.0)
        {
            return;
        }
        else if (fabs(z.x) < 1.0 && z.y == 0.0)
        {
            rz->x = log((1+z.x)/(1-z.x))/2;
            rz->y = 0;
            return;
        }
        else
        {
            temp0.x = 1 + z.x;
            temp0.y = z.y;             // temp0 = 1 + z
            temp1.x = 1 - z.x;
            temp1.y = -z.y;            // temp1 = 1 - z
            FPUcplxdiv(&temp0, &temp1, &temp2);
            FPUcplxlog(&temp2, &temp2);
            rz->x = .5*temp2.x;
            rz->y = .5*temp2.y;       // rz = .5*temp2
            return;
        }
    }
}   // end. Arctanhz

// rz=Arctan(z)=i/2*Log{(1-i*z)/(1+i*z)}
void Arctanz(DComplex z, DComplex *rz)
{
    DComplex temp0, temp1, temp2, temp3;
    if (z.x == 0.0 && z.y == 0.0)
    {
        rz->y = 0;
        rz->x = rz->y;
    }
    else if (z.x != 0.0 && z.y == 0.0)
    {
        rz->x = atan(z.x);
        rz->y = 0;
    }
    else if (z.x == 0.0 && z.y != 0.0)
    {
        temp0.x = z.y;
        temp0.y = 0.0;
        Arctanhz(temp0, &temp0);
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

        FPUcplxdiv(&temp1, &temp2, &temp3);
        FPUcplxlog(&temp3, &temp3);
        rz->x = -temp3.y*.5;
        rz->y = .5*temp3.x;           // .5*i*temp0
    }
}   // end. Arctanz

#define SinCosFudge 0x10000L
#ifdef LONGSQRT
long lsqrt(long f)
{
    int N;
    unsigned long y0, z;
    static long a = 0, b = 0, c = 0;                  // constant factors

    if (f == 0)
    {
        return f;
    }
    if (f <  0)
    {
        return 0;
    }

    if (a == 0)                                   // one-time compute consts
    {
        a = (long)(g_fudge_factor * .41731);
        b = (long)(g_fudge_factor * .59016);
        c = (long)(g_fudge_factor * .7071067811);
    }

    N  = 0;
    while (f & 0xff000000L)                     // shift arg f into the
    {
        // range: 0.5 <= f < 1
        N++;
        f /= 2;
    }
    while (!(f & 0xff800000L))
    {
        N--;
        f *= 2;
    }

    y0 = a + multiply(b, f,  g_bit_shift);         // Newton's approximation

    z  = y0 + divide(f, y0, g_bit_shift);
    y0 = (z >> 2) + divide(f, z,  g_bit_shift);

    if (N % 2)
    {
        N++;
        y0 = multiply(c, y0, g_bit_shift);
    }
    N /= 2;
    if (N >= 0)
    {
        return y0 <<  N;    // correct for shift above
    }
    else
    {
        return y0 >> -N;
    }
}
#endif
LComplex ComplexSqrtLong(long x, long y)
{
    double theta;
    long      maglong, thetalong;
    LComplex    result;

#ifndef LONGSQRT
    double mag = sqrt(sqrt(((double) multiply(x, x, g_bit_shift))/g_fudge_factor +
                           ((double) multiply(y, y, g_bit_shift))/ g_fudge_factor));
    maglong   = (long)(mag * g_fudge_factor);
#else
    maglong   = lsqrt(lsqrt(multiply(x, x, g_bit_shift)+multiply(y, y, g_bit_shift)));
#endif
    theta     = atan2((double) y/g_fudge_factor, (double) x/g_fudge_factor)/2;
    thetalong = (long)(theta * SinCosFudge);
    SinCos086(thetalong, &result.y, &result.x);
    result.x  = multiply(result.x << (g_bit_shift - 16), maglong, g_bit_shift);
    result.y  = multiply(result.y << (g_bit_shift - 16), maglong, g_bit_shift);
    return result;
}

DComplex ComplexSqrtFloat(double x, double y)
{
    DComplex  result;

    if (x == 0.0 && y == 0.0)
    {
        result.x = 0.0;
        result.y = 0.0;
    }
    else
    {
        double mag = sqrt(sqrt(x*x + y*y));
        double theta = atan2(y, x) / 2;
        FPUsincos(&theta, &result.y, &result.x);
        result.x *= mag;
        result.y *= mag;
    }
    return result;
}


//**** FRACTINT specific routines and variables ****

#ifndef TESTING_MATH

std::vector<BYTE> g_log_map_table;
long g_log_map_table_max_size;
bool g_log_map_calculate = false;
static double mlf;
static unsigned long lf;

/* int LogFlag;
   LogFlag == 1  -- standard log palettes
   LogFlag == -1 -- 'old' log palettes
   LogFlag >  1  -- compress counts < LogFlag into color #1
   LogFlag < -1  -- use quadratic palettes based on square roots && compress
*/

void SetupLogTable()
{
    float l, f, c, m;
    unsigned long limit;

    // set up on-the-fly variables
    if (g_log_map_flag > 0)
    {
        // new log function
        lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        mlf = (g_colors - (lf?2:1)) / log(static_cast<double>(g_log_map_table_max_size - lf));
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        mlf = (g_colors - 1) / log(static_cast<double>(g_log_map_table_max_size));
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        lf = 0 - g_log_map_flag;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        mlf = (g_colors - 2) / sqrt(static_cast<double>(g_log_map_table_max_size - lf));
    }

    if (g_log_map_calculate)
    {
        return; // LogTable not defined, bail out now
    }

    if (!g_log_map_calculate)
    {
        g_log_map_calculate = true;   // turn it on
        for (unsigned long prev = 0U; prev <= (unsigned long)g_log_map_table_max_size; prev++)
        {
            g_log_map_table[prev] = (BYTE)logtablecalc((long)prev);
        }
        g_log_map_calculate = false;   // turn it off, again
        return;
    }

    if (g_log_map_flag > -2)
    {
        lf = (g_log_map_flag > 1) ? g_log_map_flag : 0;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        Fg2Float((long)(g_log_map_table_max_size-lf), 0, m);
        fLog14(m, m);
        Fg2Float((long)(g_colors-(lf?2:1)), 0, c);
        fDiv(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = (lf ? 2U : 1U); n < (unsigned int)g_colors; n++)
        {
            Fg2Float((long)n, 0, f);
            fMul16(f, m, f);
            fExp14(f, l);
            limit = (unsigned long)Float2Fg(l, 0) + lf;
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (BYTE)n;
            }
        }
    }
    else
    {
        lf = 0 - g_log_map_flag;
        if (lf >= (unsigned long)g_log_map_table_max_size)
        {
            lf = g_log_map_table_max_size - 1;
        }
        Fg2Float((long)(g_log_map_table_max_size-lf), 0, m);
        fSqrt14(m, m);
        Fg2Float((long)(g_colors-2), 0, c);
        fDiv(m, c, m);
        unsigned long prev;
        for (prev = 1; prev <= lf; prev++)
        {
            g_log_map_table[prev] = 1;
        }
        for (unsigned n = 2U; n < (unsigned int)g_colors; n++)
        {
            Fg2Float((long)n, 0, f);
            fMul16(f, m, f);
            fMul16(f, f, l);
            limit = (unsigned long)(Float2Fg(l, 0) + lf);
            if (limit > (unsigned long)g_log_map_table_max_size || n == (unsigned int)(g_colors-1))
            {
                limit = g_log_map_table_max_size;
            }
            while (prev <= limit)
            {
                g_log_map_table[prev++] = (BYTE)n;
            }
        }
    }
    g_log_map_table[0] = 0;
    if (g_log_map_flag != -1)
    {
        for (unsigned long sptop = 1U; sptop < (unsigned long)g_log_map_table_max_size; sptop++)   // spread top to incl unused colors
        {
            if (g_log_map_table[sptop] > g_log_map_table[sptop-1])
            {
                g_log_map_table[sptop] = (BYTE)(g_log_map_table[sptop-1]+1);
            }
        }
    }
}

long logtablecalc(long citer)
{
    long ret = 0;

    if (g_log_map_flag == 0 && !g_iteration_ranges_len)   // Oops, how did we get here?
    {
        return (citer);
    }
    if (!g_log_map_table.empty() && !g_log_map_calculate)
    {
        return (g_log_map_table[(long)std::min(citer, g_log_map_table_max_size)]);
    }

    if (g_log_map_flag > 0)
    {
        // new log function
        if ((unsigned long)citer <= lf + 1)
        {
            ret = 1;
        }
        else if ((citer - lf)/log(static_cast<double>(citer - lf)) <= mlf)
        {
            ret = (long)(citer - lf);
        }
        else
        {
            ret = (long)(mlf * log(static_cast<double>(citer - lf))) + 1;
        }
    }
    else if (g_log_map_flag == -1)
    {
        // old log function
        if (citer == 0)
        {
            ret = 1;
        }
        else
        {
            ret = (long)(mlf * log(static_cast<double>(citer))) + 1;
        }
    }
    else if (g_log_map_flag <= -2)
    {
        // sqrt function
        if ((unsigned long)citer <= lf)
        {
            ret = 1;
        }
        else if ((unsigned long)(citer - lf) <= (unsigned long)(mlf * mlf))
        {
            ret = (long)(citer - lf + 1);
        }
        else
        {
            ret = (long)(mlf * sqrt(static_cast<double>(citer - lf))) + 1;
        }
    }
    return (ret);
}

long ExpFloat14(long xx)
{
    static float fLogTwo = 0.6931472F;
    int f;
    long Ans;

    f = 23 - (int)RegFloat2Fg(RegDivFloat(xx, *(long*)&fLogTwo), 0);
    Ans = ExpFudged(RegFloat2Fg(xx, 16), f);
    return (RegFg2Float(Ans, (char)f));
}

double TwoPi;
DComplex temp, BaseLog;
DComplex cdegree = { 3.0, 0.0 }, croot   = { 1.0, 0.0 };

bool ComplexNewtonSetup()
{
    g_threshold = .001;
    g_periodicity_check = 0;
    if (g_params[0] != 0.0 || g_params[1] != 0.0 || g_params[2] != 0.0 ||
            g_params[3] != 0.0)
    {
        croot.x = g_params[2];
        croot.y = g_params[3];
        cdegree.x = g_params[0];
        cdegree.y = g_params[1];
        FPUcplxlog(&croot, &BaseLog);
        TwoPi = asin(1.0) * 4;
    }
    return true;
}

int ComplexNewton()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = cdegree.x - 1.0;
    cd1.y = cdegree.y;

    temp = ComplexPower(g_old_z, cd1);
    FPUcplxmul(&temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - croot.x;
    g_tmp_z.y = g_new_z.y - croot.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < g_threshold)
    {
        return (1);
    }

    FPUcplxmul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += croot.x;
    g_tmp_z.y += croot.y;

    FPUcplxmul(&temp, &cdegree, &cd1);
    FPUcplxdiv(&g_tmp_z, &cd1, &g_old_z);
    if (g_overflow)
    {
        return (1);
    }
    g_new_z = g_old_z;
    return (0);
}

int ComplexBasin()
{
    DComplex cd1;

    /* new = ((cdegree-1) * old**cdegree) + croot
             ----------------------------------
                  cdegree * old**(cdegree-1)         */

    cd1.x = cdegree.x - 1.0;
    cd1.y = cdegree.y;

    temp = ComplexPower(g_old_z, cd1);
    FPUcplxmul(&temp, &g_old_z, &g_new_z);

    g_tmp_z.x = g_new_z.x - croot.x;
    g_tmp_z.y = g_new_z.y - croot.y;
    if ((sqr(g_tmp_z.x) + sqr(g_tmp_z.y)) < g_threshold)
    {
        if (fabs(g_old_z.y) < .01)
        {
            g_old_z.y = 0.0;
        }
        FPUcplxlog(&g_old_z, &temp);
        FPUcplxmul(&temp, &cdegree, &g_tmp_z);
        double mod = g_tmp_z.y/TwoPi;
        g_color_iter = (long)mod;
        if (fabs(mod - g_color_iter) > 0.5)
        {
            if (mod < 0.0)
            {
                g_color_iter--;
            }
            else
            {
                g_color_iter++;
            }
        }
        g_color_iter += 2;
        if (g_color_iter < 0)
        {
            g_color_iter += 128;
        }
        return (1);
    }

    FPUcplxmul(&g_new_z, &cd1, &g_tmp_z);
    g_tmp_z.x += croot.x;
    g_tmp_z.y += croot.y;

    FPUcplxmul(&temp, &cdegree, &cd1);
    FPUcplxdiv(&g_tmp_z, &cd1, &g_old_z);
    if (g_overflow)
    {
        return (1);
    }
    g_new_z = g_old_z;
    return (0);
}

/*
 * Generate a gaussian distributed number.
 * The right half of the distribution is folded onto the lower half.
 * That is, the curve slopes up to the peak and then drops to 0.
 * The larger slope is, the smaller the standard deviation.
 * The values vary from 0+offset to range+offset, with the peak
 * at range+offset.
 * To make this more complicated, you only have a
 * 1 in Distribution*(1-Probability/Range*con)+1 chance of getting a
 * Gaussian; otherwise you just get offset.
 */
int g_distribution = 30;
int g_slope = 25;
namespace
{
int Offset = 0;
}
int GausianNumber(int Probability, int Range)
{
    long p;

    p = divide((long)Probability << 16, (long)Range << 16, 16);
    p = multiply(p, g_concentration, 16);
    p = multiply((long)g_distribution << 16, p, 16);
    if (!(rand15() % (g_distribution - (int)(p >> 16) + 1)))
    {
        long Accum = 0;
        for (int n = 0; n < g_slope; n++)
        {
            Accum += rand15();
        }
        Accum /= g_slope;
        int r = (int)(multiply((long)Range << 15, Accum, 15) >> 14);
        r = r - Range;
        if (r < 0)
        {
            r = -r;
        }
        return (Range - r + Offset);
    }
    return (Offset);
}

#endif

#if defined(_WIN32)
/*
MP2d086     PROC     uses si di, xExp:WORD, xMant:DWORD
   sub   xExp, (1 SHL 14) - (1 SHL 10)
   jo    Overflow

   mov   bx, xExp
   and   bx, 0111100000000000b
   jz    InRangeOfDouble

Overflow:
   mov   g_mp_overflow, 1
   xor   ax, ax
   xor   dx, dx
   xor   bx, bx
   jmp   StoreAns

InRangeOfDouble:
   mov   si, xExp
   mov   ax, si
   mov   cl, 5
   shl   si, cl
   shl   ax, 1
   rcr   si, 1

   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]
   shl   ax, 1
   rcl   dx, 1

   mov   bx, ax
   mov   di, dx
   mov   cl, 12
   shr   dx, cl
   shr   ax, cl
   mov   cl, 4
   shl   bx, cl
   shl   di, cl
   or    ax, di
   or    dx, si

StoreAns:
   mov   WORD PTR Double+6, dx
   mov   WORD PTR Double+4, ax
   mov   WORD PTR Double+2, bx
   xor   bx, bx
   mov   WORD PTR Double, bx

   lea   ax, Double
   mov   dx, ds
   ret
MP2d086     ENDP
*/
double *MP2d086(MP x)
{
    // TODO: implement
    static double ans = 0.0;
    _ASSERTE(0 && "MP2d086 called.");
    return &ans;
}

/*
d2MP086     PROC     uses si di, x:QWORD
   mov   dx, word ptr [x+6]
   mov   ax, word ptr [x+4]
   mov   bx, word ptr [x+2]
   mov   cx, word ptr [x]
   mov   si, dx
   shl   si, 1
   pushf
   mov   cl, 4
   shr   si, cl
   popf
   rcr   si, 1
   add   si, (1 SHL 14) - (1 SHL 10)

   mov   di, ax                           ; shl dx:ax:bx 12 bits
   mov   cl, 12
   shl   dx, cl
   shl   ax, cl
   mov   cl, 4
   shr   di, cl
   shr   bx, cl
   or    dx, di
   or    ax, bx
   stc
   rcr   dx, 1
   rcr   ax, 1

StoreAns:
   mov   Ans.Exp, si
   mov   word ptr Ans.Mant+2, dx
   mov   word ptr Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
d2MP086     ENDP
*/
MP *d2MP086(double x)
{
    // TODO: implement
    if (0.0 == x)
    {
        Ans.Exp = 0;
        Ans.Mant = 0;
    }
    else
    {
        __asm
        {
            mov dx, word ptr [x+6]
            mov ax, word ptr [x+4]
            mov bx, word ptr [x+2]
            mov cx, word ptr [x]
            xor esi, esi
            mov   si, dx
            shl   si, 1
            pushf
            mov   cl, 4
            shr   si, cl
            popf
            rcr   si, 1
            add   si, (1 SHL 14) - (1 SHL 10)

            mov   di, ax                           ; shl dx:ax:bx 12 bits
            mov   cl, 12
            shl   dx, cl
            shl   ax, cl
            mov   cl, 4
            shr   di, cl
            shr   bx, cl
            or    dx, di
            or    ax, bx
            stc
            rcr   dx, 1
            rcr   ax, 1

            mov   Ans.Exp, esi
            mov   word ptr Ans.Mant+2, dx
            mov   word ptr Ans.Mant, ax
        }
    }
    return &Ans;
}

/*
MPadd086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   si, xExp
   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]

   mov   di, yExp

   mov   cx, si
   xor   cx, di
   js    Subtract
   jz    SameMag

   cmp   si, di
   jg    XisGreater

   xchg  si, di
   xchg  dx, WORD PTR [yMant+2]
   xchg  ax, WORD PTR [yMant]

XisGreater:
   mov   cx, si
   sub   cx, di
   cmp   cx, 32
   jl    ChkSixteen
   jmp   StoreAns

ChkSixteen:
   cmp   cx, 16
   jl    SixteenBitShift

   sub   cx, 16
   mov   bx, WORD PTR [yMant+2]
   shr   bx, cl
   mov   WORD PTR [yMant], bx
   mov   WORD PTR [yMant+2], 0
   jmp   SameMag

SixteenBitShift:
   mov   bx, WORD PTR [yMant+2]
   shr   WORD PTR [yMant+2], cl
   shr   WORD PTR [yMant], cl
   neg   cl
   add   cl, 16
   shl   bx, cl
   or    WORD PTR [yMant], bx

SameMag:
   add   ax, WORD PTR [yMant]
   adc   dx, WORD PTR [yMant+2]
   jc    ShiftCarry
   jmp   StoreAns

ShiftCarry:
   rcr   dx, 1
   rcr   ax, 1
   add   si, 1
   jo    Overflow
   jmp   StoreAns

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   si, si
   xor   ax, ax
   xor   dx, dx
   jmp   StoreAns

Subtract:
   xor   di, 8000h
   mov   cx, si
   sub   cx, di
   jnz   DifferentMag

   cmp   dx, WORD PTR [yMant+2]
   jg    SubtractNumbers
   jne   SwapNumbers

   cmp   ax, WORD PTR [yMant]
   jg    SubtractNumbers
   je    ZeroAns

SwapNumbers:
   xor   si, 8000h
   xchg  ax, WORD PTR [yMant]
   xchg  dx, WORD PTR [yMant+2]
   jmp   SubtractNumbers

DifferentMag:
   or    cx, cx
   jns   NoSwap

   xchg  si, di
   xchg  ax, WORD PTR [yMant]
   xchg  dx, WORD PTR [yMant+2]
   xor   si, 8000h
   neg   cx

NoSwap:
   cmp   cx, 32
   jge   StoreAns

   cmp   cx, 16
   jl    SixteenBitShift2

   sub   cx, 16
   mov   bx, WORD PTR [yMant+2]
   shr   bx, cl
   mov   WORD PTR [yMant], bx
   mov   WORD PTR [yMant+2], 0
   jmp   SubtractNumbers

SixteenBitShift2:
   mov   bx, WORD PTR [yMant+2]
   shr   WORD PTR [yMant+2], cl
   shr   WORD PTR [yMant], cl
   neg   cl
   add   cl, 16
   shl   bx, cl
   or    WORD PTR [yMant], bx

SubtractNumbers:
   sub   ax, WORD PTR [yMant]
   sbb   dx, WORD PTR [yMant+2]

BitScanRight:
   or    dx, dx
   js    StoreAns

   shl   ax, 1
   rcl   dx, 1
   sub   si, 1
   jno   BitScanRight
   jmp   Overflow

StoreAns:
   mov   Ans.Exp, si
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
MPadd086    ENDP
*/
MP *MPadd086(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPadd086 called.");
    return &Ans;
}

/*
MPcmp086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
LOCAL Rev:WORD, Flag:WORD
   mov   Rev, 0
   mov   Flag, 0
   mov   ax, xExp
   mov   dx, WORD PTR [xMant]
   mov   si, WORD PTR [xMant+2]
   mov   bx, yExp
   mov   cx, WORD PTR [yMant]
   mov   di, WORD PTR [yMant+2]
   or    ax, ax
   jns   AtLeastOnePos

   or    bx, bx
   jns   AtLeastOnePos

   mov   Rev, 1
   and   ah, 7fh
   and   bh, 7fh

AtLeastOnePos:
   cmp   ax, bx
   jle   Cmp1

   mov   Flag, 1
   jmp   ChkRev

Cmp1:
   je    Cmp2

   mov   Flag, -1
   jmp   ChkRev

Cmp2:
   cmp   si, di
   jbe   Cmp3

   mov   Flag, 1
   jmp   ChkRev

Cmp3:
   je    Cmp4

   mov   Flag, -1
   jmp   ChkRev

Cmp4:
   cmp   dx, cx
   jbe   Cmp5

   mov   Flag, 1
   jmp   ChkRev

Cmp5:
   je    ChkRev

   mov   Flag, -1

ChkRev:
        or    Rev, 0
   jz    ExitCmp

   neg   Flag

ExitCmp:
   mov   ax, Flag
   ret
MPcmp086    ENDP
*/
int MPcmp086(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPcmp086 called.");
    return 0;
}

/*
MPdiv086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   sub   ax, bx
   jno   NoOverflow

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   ax, ax
   xor   dx, dx
   mov   Ans.Exp, dx
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]
   or    dx, dx
   jz    ZeroAns

   mov   cx, WORD PTR [yMant+2]
   mov   bx, WORD PTR [yMant]
   or    cx, cx
   jz    Overflow

   cmp   dx, cx
   jl    Divide

   shr   dx, 1
   rcr   ax, 1
   add   Ans.Exp, 1
   jo    Overflow

Divide:
   div   cx
   mov   si, dx
   mov   dx, bx
   mov   bx, ax
   mul   dx
   xor   di, di
   cmp   dx, si
   jnc   RemReallyNeg

   xchg  ax, di
   xchg  dx, si
   sub   ax, di
   sbb   dx, si

   shr   dx, 1
   rcr   ax, 1
   div   cx
   mov   dx, bx
   shl   ax, 1
   adc   dx, 0
   jmp   StoreMant

RemReallyNeg:
   sub   ax, di
   sbb   dx, si

   shr   dx, 1
   rcr   ax, 1
   div   cx
   mov   dx, bx
   mov   bx, ax
   xor   ax, ax
   xor   cx, cx
   shl   bx, 1
   rcl   cx, 1
   sub   ax, bx
   sbb   dx, cx
   jno   StoreMant

   shl   ax, 1
   rcl   dx, 1
   dec   Ans.Exp

StoreMant:
   mov   WORD PTR Ans.Mant, ax
   mov   WORD PTR Ans.Mant+2, dx
   lea   ax, Ans
   mov   dx, ds
   ret
MPdiv086    ENDP
*/
MP *MPdiv086(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPdiv086 called.");
    return &Ans;
}

/*
MPmul086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   add   ax, bx
   jno   NoOverflow

Overflow:
   or    word ptr [xMant+2], 0
   jz    ZeroAns
   or    word ptr [yMant+2], 0
   jz    ZeroAns

   mov   g_mp_overflow, 1

ZeroAns:
   xor   ax, ax
   xor   dx, dx
   mov   Ans.Exp, ax
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   si, word ptr [xMant+2]
   mov   bx, word ptr [xMant]
   mov   di, word ptr [yMant+2]
   mov   cx, word ptr [yMant]

   mov   ax, si
   or    ax, bx
   jz    ZeroAns

   mov   ax, di
   or    ax, cx
   jz    ZeroAns

   mov   ax, cx
   mul   bx
   push  dx

   mov   ax, cx
   mul   si
   push  ax
   push  dx

   mov   ax, bx
   mul   di
   push  ax
   push  dx

   mov   ax, si
   mul   di
   pop   bx
   pop   cx
   pop   si
   pop   di

   add   ax, bx
   adc   dx, 0
   pop   bx
   add   di, bx
   adc   ax, 0
   adc   dx, 0
   add   di, cx
   adc   ax, si
   adc   dx, 0

   or    dx, dx
   js    StoreMant

   shl   di, 1
   rcl   ax, 1
   rcl   dx, 1
   sub   Ans.Exp, 1
   jo    Overflow

StoreMant:
   mov   word ptr Ans.Mant+2, dx
   mov   word ptr Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
MPmul086    ENDP
*/
MP *MPmul086(MP x, MP y)
{
    // TODO: implement
    __asm
    {
        xor   eax, eax
        xor   ebx, ebx
        mov   eax, x.Exp
        mov   ebx, y.Exp
        xor   ch, ch
        shl   bh, 1
        rcr   ch, 1
        shr   bh, 1
        xor   ah, ch

        sub   bx, (1 SHL 14) - 2
        add   ax, bx
        jno   NoOverflow

    Overflow:
        or    word ptr [x.Mant+2], 0
        jz    ZeroAns
        or    word ptr [y.Mant+2], 0
        jz    ZeroAns

        mov   g_mp_overflow, 1

    ZeroAns:
        xor   ax, ax
        xor   dx, dx
        mov   Ans.Exp, eax
        jmp   StoreMant

    NoOverflow:
        mov   Ans.Exp, eax

        mov   si, word ptr [x.Mant+2]
        mov   bx, word ptr [x.Mant]
        mov   di, word ptr [y.Mant+2]
        mov   cx, word ptr [y.Mant]

        mov   ax, si
        or    ax, bx
        jz    ZeroAns

        mov   ax, di
        or    ax, cx
        jz    ZeroAns

        mov   ax, cx
        mul   bx
        push  dx

        mov   ax, cx
        mul   si
        push  ax
        push  dx

        mov   ax, bx
        mul   di
        push  ax
        push  dx

        mov   ax, si
        mul   di
        pop   bx
        pop   cx
        pop   si
        pop   di

        add   ax, bx
        adc   dx, 0
        pop   bx
        add   di, bx
        adc   ax, 0
        adc   dx, 0
        add   di, cx
        adc   ax, si
        adc   dx, 0

        or    dx, dx
        js    StoreMant

        shl   di, 1
        rcl   ax, 1
        rcl   dx, 1
        sub   Ans.Exp, 1
        jo    Overflow

    StoreMant:
        mov   word ptr Ans.Mant+2, dx
        mov   word ptr Ans.Mant, ax
    }

    return &Ans;
}

/*
d2MP386     PROC     uses si di, x:QWORD
   mov   si, WORD PTR [x+6]
.386
   mov   edx, DWORD PTR [x+4]
   mov   eax, DWORD PTR [x]

   mov   ebx, edx
   shl   ebx, 1
   or    ebx, eax
   jnz   NonZero

   xor   si, si
   xor   edx, edx
   jmp   StoreAns

NonZero:
   shl   si, 1
   pushf
   shr   si, 4
   popf
   rcr   si, 1
   add   si, (1 SHL 14) - (1 SHL 10)

   shld  edx, eax, 12
   stc
   rcr   edx, 1

StoreAns:
   mov   Ans.Exp, si
   mov   Ans.Mant, edx

   lea   ax, Ans
   mov   dx, ds
.8086
   ret
d2MP386     ENDP
*/
MP *d2MP386(double x)
{
    // TODO: implement
    _ASSERTE(0 && "d2MP386 called.");
    return &Ans;
}

/*
MP2d386     PROC     uses si di, xExp:WORD, xMant:DWORD
   sub   xExp, (1 SHL 14) - (1 SHL 10)
.386
   jo    Overflow

   mov   bx, xExp
   and   bx, 0111100000000000b
   jz    InRangeOfDouble

Overflow:
   mov   g_mp_overflow, 1
   xor   eax, eax
   xor   edx, edx
   jmp   StoreAns

InRangeOfDouble:
   mov   bx, xExp
   mov   ax, bx
   shl   bx, 5
   shl   ax, 1
   rcr   bx, 1
   shr   bx, 4

   mov   edx, xMant
   shl   edx, 1
   xor   eax, eax
   shrd  eax, edx, 12
   shrd  edx, ebx, 12

StoreAns:
   mov   DWORD PTR Double+4, edx
   mov   DWORD PTR Double, eax

   lea   ax, Double
   mov   dx, ds
.8086
   ret
MP2d386     ENDP
*/
double *MP2d386(MP x)
{
    // TODO: implement
    static double ans = 0.0;
    _ASSERTE(0 && "MP2d386 called.");
    return &ans;
}

/*
MPadd086    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   si, xExp
   mov   dx, WORD PTR [xMant+2]
   mov   ax, WORD PTR [xMant]

   mov   di, yExp

   mov   cx, si
   xor   cx, di
   js    Subtract
   jz    SameMag

   cmp   si, di
   jg    XisGreater

   xchg  si, di
   xchg  dx, WORD PTR [yMant+2]
   xchg  ax, WORD PTR [yMant]

XisGreater:
   mov   cx, si
   sub   cx, di
   cmp   cx, 32
   jl    ChkSixteen
   jmp   StoreAns

ChkSixteen:
   cmp   cx, 16
   jl    SixteenBitShift

   sub   cx, 16
   mov   bx, WORD PTR [yMant+2]
   shr   bx, cl
   mov   WORD PTR [yMant], bx
   mov   WORD PTR [yMant+2], 0
   jmp   SameMag

SixteenBitShift:
   mov   bx, WORD PTR [yMant+2]
   shr   WORD PTR [yMant+2], cl
   shr   WORD PTR [yMant], cl
   neg   cl
   add   cl, 16
   shl   bx, cl
   or    WORD PTR [yMant], bx

SameMag:
   add   ax, WORD PTR [yMant]
   adc   dx, WORD PTR [yMant+2]
   jc    ShiftCarry
   jmp   StoreAns

ShiftCarry:
   rcr   dx, 1
   rcr   ax, 1
   add   si, 1
   jo    Overflow
   jmp   StoreAns

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   si, si
   xor   ax, ax
   xor   dx, dx
   jmp   StoreAns

Subtract:
   xor   di, 8000h
   mov   cx, si
   sub   cx, di
   jnz   DifferentMag

   cmp   dx, WORD PTR [yMant+2]
   jg    SubtractNumbers
   jne   SwapNumbers

   cmp   ax, WORD PTR [yMant]
   jg    SubtractNumbers
   je    ZeroAns

SwapNumbers:
   xor   si, 8000h
   xchg  ax, WORD PTR [yMant]
   xchg  dx, WORD PTR [yMant+2]
   jmp   SubtractNumbers

DifferentMag:
   or    cx, cx
   jns   NoSwap

   xchg  si, di
   xchg  ax, WORD PTR [yMant]
   xchg  dx, WORD PTR [yMant+2]
   xor   si, 8000h
   neg   cx

NoSwap:
   cmp   cx, 32
   jge   StoreAns

   cmp   cx, 16
   jl    SixteenBitShift2

   sub   cx, 16
   mov   bx, WORD PTR [yMant+2]
   shr   bx, cl
   mov   WORD PTR [yMant], bx
   mov   WORD PTR [yMant+2], 0
   jmp   SubtractNumbers

SixteenBitShift2:
   mov   bx, WORD PTR [yMant+2]
   shr   WORD PTR [yMant+2], cl
   shr   WORD PTR [yMant], cl
   neg   cl
   add   cl, 16
   shl   bx, cl
   or    WORD PTR [yMant], bx

SubtractNumbers:
   sub   ax, WORD PTR [yMant]
   sbb   dx, WORD PTR [yMant+2]

BitScanRight:
   or    dx, dx
   js    StoreAns

   shl   ax, 1
   rcl   dx, 1
   sub   si, 1
   jno   BitScanRight
   jmp   Overflow

StoreAns:
   mov   Ans.Exp, si
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax

   lea   ax, Ans
   mov   dx, ds
   ret
MPadd086    ENDP
*/
MP *MPadd(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPadd called.");
    return &Ans;
}

/*
MPadd386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   si, xExp
.386
   mov   eax, xMant

   mov   di, yExp
   mov   edx, yMant

   mov   cx, si
   xor   cx, di
   js    Subtract
   jz    SameMag

   cmp   si, di
   jg    XisGreater

   xchg  si, di
   xchg  eax, edx

XisGreater:
   mov   cx, si
   sub   cx, di
   cmp   cx, 32
   jge   StoreAns

   shr   edx, cl

SameMag:
   add   eax, edx
   jnc   StoreAns

   rcr   eax, 1
   add   si, 1
   jno   StoreAns

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   si, si
   xor   edx, edx
   jmp   StoreAns

Subtract:
   xor   di, 8000h
   mov   cx, si
   sub   cx, di
   jnz   DifferentMag

   cmp   eax, edx
   jg    SubtractNumbers
   je    ZeroAns

   xor   si, 8000h
   xchg  eax, edx
   jmp   SubtractNumbers

DifferentMag:
   or    cx, cx
   jns   NoSwap

   xchg  si, di
   xchg  eax, edx
   xor   si, 8000h
   neg   cx

NoSwap:
   cmp   cx, 32
   jge   StoreAns

   shr   edx, cl

SubtractNumbers:
   sub   eax, edx
   bsr   ecx, eax
   neg   cx
   add   cx, 31
   shl   eax, cl
   sub   si, cx
   jo    Overflow

StoreAns:
   mov   Ans.Exp, si
   mov   Ans.Mant, eax

   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPadd386    ENDP
*/
MP *MPadd386(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPadd386 called.");
    return &Ans;
}

/*
MPcmp386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, yMant:DWORD
   mov   si, 0
   mov   di, si
.386
   mov   ax, xExp
   mov   edx, xMant
   mov   bx, yExp
   mov   ecx, yMant
   or    ax, ax
   jns   AtLeastOnePos

   or    bx, bx
   jns   AtLeastOnePos

   mov   si, 1
   and   ah, 7fh
   and   bh, 7fh

AtLeastOnePos:
   cmp   ax, bx
   jle   Cmp1

   mov   di, 1
   jmp   ChkRev

Cmp1:
   je    Cmp2

   mov   di, -1
   jmp   ChkRev

Cmp2:
   cmp   edx, ecx
   jbe   Cmp3

   mov   di, 1
   jmp   ChkRev

Cmp3:
   je    ChkRev

   mov   di, -1

ChkRev:
        or    si, si
   jz    ExitCmp

   neg   di

ExitCmp:
   mov   ax, di
.8086
   ret
MPcmp386    ENDP
*/
int MPcmp386(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPcmp386 called.");
    return 0;
}

/*
MPdiv386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
.386
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   sub   ax, bx
   jno   NoOverflow

Overflow:
   mov   g_mp_overflow, 1

ZeroAns:
   xor   eax, eax
   mov   Ans.Exp, ax
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   xor   eax, eax
   mov   edx, xMant
   mov   ecx, yMant
   or    edx, edx
   jz    ZeroAns

   or    ecx, ecx
   jz    Overflow

   cmp   edx, ecx
   jl    Divide

   shr   edx, 1
   rcr   eax, 1
   add   Ans.Exp, 1
   jo    Overflow

Divide:
   div   ecx

StoreMant:
   mov   Ans.Mant, eax
   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPdiv386    ENDP
*/
MP *MPdiv386(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPdiv386 called.");
    return &Ans;
}

/*
MPmul386    PROC     uses si di, xExp:WORD, xMant:DWORD, yExp:WORD, \
                     yMant:DWORD
   mov   ax, xExp
   mov   bx, yExp
.386
   xor   ch, ch
   shl   bh, 1
   rcr   ch, 1
   shr   bh, 1
   xor   ah, ch

   sub   bx, (1 SHL 14) - 2
   add   ax, bx
   jno   NoOverflow

Overflow:
   or    WORD PTR [xMant+2], 0
   jz    ZeroAns
   or    WORD PTR [yMant+2], 0
   jz    ZeroAns

   mov   g_mp_overflow, 1

ZeroAns:
   xor   edx, edx
   mov   Ans.Exp, dx
   jmp   StoreMant

NoOverflow:
   mov   Ans.Exp, ax

   mov   eax, xMant
   mov   edx, yMant
   or    eax, eax
   jz    ZeroAns

   or    edx, edx
   jz    ZeroAns

   mul   edx

   or    edx, edx
   js    StoreMant

   shld  edx, eax, 1
   sub   Ans.Exp, 1
   jo    Overflow

StoreMant:
   mov   Ans.Mant, edx
   lea   ax, Ans
   mov   dx, ds
.8086
   ret
MPmul386    ENDP
*/
MP *MPmul386(MP x, MP y)
{
    // TODO: implement
    _ASSERTE(0 && "MPmul386 called.");
    return &Ans;
}

//

MP *d2MP(double x)
{
    return d2MP386(x);
}

MP *MPmul(MP x, MP y)
{
    return MPmul386(x, y);
}

MP *MPdiv(MP x, MP y)
{
    return MPdiv386(x, y);
}

int MPcmp(MP x, MP y)
{
    return MPcmp386(x, y);
}

/*
fg2MP086    PROC     x:DWORD, fg:WORD
   mov   ax, WORD PTR [x]
   mov   dx, WORD PTR [x+2]
   mov   cx, ax
   or    cx, dx
   jz    ExitFg2MP

   mov   cx, 1 SHL 14 + 30
   sub   cx, fg

   or    dx, dx
   jns   BitScanRight

   or    ch, 80h
   not   ax
   not   dx
   add   ax, 1
   adc   dx, 0

BitScanRight:
   shl   ax, 1
   rcl   dx, 1
   dec   cx
   or    dx, dx
   jns   BitScanRight

ExitFg2MP:
   mov   Ans.Exp, cx
   mov   WORD PTR Ans.Mant+2, dx
   mov   WORD PTR Ans.Mant, ax
   lea   ax, Ans
   mov   dx, ds
   ret
fg2MP086    ENDP
*/
MP *fg2MP086(long x, int fg)
{
    _ASSERTE(0 && "fg2MP086 called");
    return &Ans;
}

/*
fg2MP386    PROC     x:DWORD, fg:WORD
   mov   bx, 1 SHL 14 + 30
   sub   bx, fg
.386
   mov   edx, x

   or    edx, edx
   jnz   ChkNegMP

   mov   bx, dx
   jmp   StoreAns

ChkNegMP:
   jns   BitScanRight

   or    bh, 80h
   neg   edx

BitScanRight:
   bsr   ecx, edx
   neg   cx
   add   cx, 31
   sub   bx, cx
   shl   edx, cl

StoreAns:
   mov   Ans.Exp, bx
   mov   Ans.Mant, edx
.8086
   lea   ax, Ans
   mov   dx, ds
   ret
fg2MP386    ENDP
*/
MP *fg2MP386(long x, int fg)
{
    _ASSERTE(0 && "fg2MP386 called");
    return &Ans;
}

MP *fg2MP(long x, int fg)
{
    return fg2MP386(x, fg);
}
#endif
