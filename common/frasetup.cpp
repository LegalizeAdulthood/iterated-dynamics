#include <float.h>
#include <limits.h>
#include <string.h>
#if !defined(_WIN32)
#include <malloc.h>
#endif

#include "port.h"
#include "prototyp.h"
#include "helpdefs.h"
#include "fractype.h"

#if !defined(XFRACT)
#define MPCmod(m) (*pMPadd(*pMPmul((m).x, (m).x), *pMPmul((m).y, (m).y)))
#endif

// --------------------------------------------------------------------
//              Setup (once per fractal image) routines
// --------------------------------------------------------------------

bool
MandelSetup()           // Mandelbrot Routine
{
    if (debugflag != debug_flags::force_standard_fractal
            && !invert && decomp[0] == 0 && rqlim == 4.0
            && bitshift == 29 && !potflag
            && biomorph == -1 && inside > -59 && outside >= -1
            && useinitorbit != 1 && !using_jiim && bailoutest == bailouts::Mod
            && (orbitsave&2) == 0)
        calctype = calcmand; // the normal case - use CALCMAND
    else
    {
        // special case: use the main processing loop
        calctype = StandardFractal;
        longparm = &linit;
    }
    return true;
}

bool
JuliaSetup()            // Julia Routine
{
    if (debugflag != debug_flags::force_standard_fractal
            && !invert && decomp[0] == 0 && rqlim == 4.0
            && bitshift == 29 && !potflag
            && biomorph == -1 && inside > -59 && outside >= -1
            && !finattract && !using_jiim && bailoutest == bailouts::Mod
            && (orbitsave&2) == 0)
        calctype = calcmand; // the normal case - use CALCMAND
    else
    {
        // special case: use the main processing loop
        calctype = StandardFractal;
        longparm = &lparm;
        get_julia_attractor(0.0, 0.0);    // another attractor?
    }
    return true;
}

bool
NewtonSetup()           // Newton/NewtBasin Routines
{
#if !defined(XFRACT)
    if (debugflag != debug_flags::allow_mp_newton_type)
    {
        if (fractype == MPNEWTON)
            fractype = NEWTON;
        else if (fractype == MPNEWTBASIN)
            fractype = NEWTBASIN;
        curfractalspecific = &fractalspecific[fractype];
    }
#else
    if (fractype == MPNEWTON)
        fractype = NEWTON;
    else if (fractype == MPNEWTBASIN)
        fractype = NEWTBASIN;

    curfractalspecific = &fractalspecific[fractype];
#endif
    // set up table of roots of 1 along unit circle
    degree = (int)parm.x;
    if (degree < 2)
        degree = 3;   // defaults to 3, but 2 is possible
    root = 1;

    // precalculated values
    roverd       = (double)root / (double)degree;
    d1overd      = (double)(degree - 1) / (double)degree;
    maxcolor     = 0;
    threshold    = .3*PI/degree; // less than half distance between roots
#if !defined(XFRACT)
    if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
    {
        mproverd     = *pd2MP(roverd);
        mpd1overd    = *pd2MP(d1overd);
        mpthreshold  = *pd2MP(threshold);
        mpone        = *pd2MP(1.0);
    }
#endif

    floatmin = FLT_MIN;
    floatmax = FLT_MAX;

    basin = 0;
    roots.resize(16);
    if (fractype == NEWTBASIN)
    {
        if (parm.y)
            basin = 2; //stripes
        else
            basin = 1;
        roots.resize(degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < degree; i++)
        {
            roots[i].x = cos(i*twopi/(double)degree);
            roots[i].y = sin(i*twopi/(double)degree);
        }
    }
#if !defined(XFRACT)
    else if (fractype == MPNEWTBASIN)
    {
        if (parm.y)
            basin = 2; //stripes
        else
            basin = 1;

        MPCroots.resize(degree);

        // list of roots to discover where we converged for newtbasin
        for (int i = 0; i < degree; i++)
        {
            MPCroots[i].x = *pd2MP(cos(i*twopi/(double)degree));
            MPCroots[i].y = *pd2MP(sin(i*twopi/(double)degree));
        }
    }
#endif

    param[0] = (double)degree;
    if (degree%4 == 0)
        symmetry = symmetry_type::XY_AXIS;
    else
        symmetry = symmetry_type::X_AXIS;

    calctype = StandardFractal;
#if !defined(XFRACT)
    if (fractype == MPNEWTON || fractype == MPNEWTBASIN)
        setMPfunctions();
#endif
    return true;
}


bool
StandaloneSetup()
{
    timer(0,curfractalspecific->calctype);
    return false;               // effectively disable solid-guessing
}

bool
UnitySetup()
{
    periodicitycheck = 0;
    FgOne = (1L << bitshift);
    FgTwo = FgOne + FgOne;
    return true;
}

bool
MandelfpSetup()
{
    bf_math = bf_math_type::NONE;
    c_exp = (int)param[2];
    pwr.x = param[2] - 1.0;
    pwr.y = param[3];
    floatparm = &init;
    switch (fractype)
    {
    case MARKSMANDELFP:
        if (c_exp < 1)
        {
            c_exp = 1;
            param[2] = 1;
        }
        if (!(c_exp & 1))
            symmetry = symmetry_type::XY_AXIS_NO_PARAM;    // odd exponents
        if (c_exp & 1)
            symmetry = symmetry_type::X_AXIS_NO_PARAM;
        break;
    case MANDELFP:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using StandardFractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
        */
        if (debugflag != debug_flags::force_standard_fractal
                && !distest
                && decomp[0] == 0
                && biomorph == -1
                && (inside >= -1)
                && outside >= -6
                && useinitorbit != 1
                && (soundflag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
                && !using_jiim && bailoutest == bailouts::Mod
                && (orbitsave&2) == 0)
        {
            calctype = calcmandfp; // the normal case - use calcmandfp
            calcmandfpasmstart();
        }
        else
        {
            // special case: use the main processing loop
            calctype = StandardFractal;
        }
        break;
    case FPMANDELZPOWER:
        if ((double)c_exp == param[2] && (c_exp & 1)) // odd exponents
            symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        if (param[3] != 0)
            symmetry = symmetry_type::NONE;
        if (param[3] == 0.0 && debugflag != debug_flags::force_complex_power && (double)c_exp == param[2])
            fractalspecific[fractype].orbitcalc = floatZpowerFractal;
        else
            fractalspecific[fractype].orbitcalc = floatCmplxZpowerFractal;
        break;
    case MAGNET1M:
    case MAGNET2M:
        attr[0].x = 1.0;      // 1.0 + 0.0i always attracts
        attr[0].y = 0.0;      // - both MAGNET1 and MAGNET2
        attrperiod[0] = 1;
        attractors = 1;
        break;
    case SPIDERFP:
        if (periodicitycheck == 1) // if not user set
            periodicitycheck = 4;
        break;
    case MANDELEXP:
        symmetry = symmetry_type::X_AXIS_NO_PARAM;
        break;
    case FPMANTRIGPLUSEXP:
    case FPMANTRIGPLUSZSQRD:
        if (parm.y == 0.0)
            symmetry = symmetry_type::X_AXIS;
        else
            symmetry = symmetry_type::NONE;
        if ((trigndx[0] == trig_fn::LOG) || (trigndx[0] == trig_fn::FLIP))
            symmetry = symmetry_type::NONE;
        break;
    case QUATFP:
        floatparm = &tmp;
        attractors = 0;
        periodicitycheck = 0;
        break;
    case HYPERCMPLXFP:
        floatparm = &tmp;
        attractors = 0;
        periodicitycheck = 0;
        if (param[2] != 0)
            symmetry = symmetry_type::NONE;
        if (trigndx[0] == trig_fn::FLIP)
            symmetry = symmetry_type::NONE;
        break;
    case TIMSERRORFP:
        if (trigndx[0] == trig_fn::FLIP)
            symmetry = symmetry_type::NONE;
        break;
    case MARKSMANDELPWRFP:
        if (trigndx[0] == trig_fn::FLIP)
            symmetry = symmetry_type::NONE;
        break;
    default:
        break;
    }
    return true;
}

bool
JuliafpSetup()
{
    c_exp = (int)param[2];
    floatparm = &parm;
    if (fractype == COMPLEXMARKSJUL)
    {
        pwr.x = param[2] - 1.0;
        pwr.y = param[3];
        coefficient = ComplexPower(*floatparm, pwr);
    }
    switch (fractype)
    {
    case JULIAFP:
        /*
           floating point code could probably be altered to handle many of
           the situations that otherwise are using StandardFractal().
           calcmandfp() can currently handle invert, any rqlim, potflag
           zmag, epsilon cross, and all the current outside options
        */
        if (debugflag != debug_flags::force_standard_fractal
                && !distest
                && decomp[0] == 0
                && biomorph == -1
                && (inside >= -1)
                && outside >= -6
                && useinitorbit != 1
                && (soundflag & SOUNDFLAG_ORBITMASK) < SOUNDFLAG_X
                && !finattract
                && !using_jiim && bailoutest == bailouts::Mod
                && (orbitsave&2) == 0)
        {
            calctype = calcmandfp; // the normal case - use calcmandfp
            calcmandfpasmstart();
        }
        else
        {
            // special case: use the main processing loop
            calctype = StandardFractal;
            get_julia_attractor(0.0, 0.0);    // another attractor?
        }
        break;
    case FPJULIAZPOWER:
        if ((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2])
            symmetry = symmetry_type::NONE;
        if (param[3] == 0.0 && debugflag != debug_flags::force_complex_power && (double)c_exp == param[2])
            fractalspecific[fractype].orbitcalc = floatZpowerFractal;
        else
            fractalspecific[fractype].orbitcalc = floatCmplxZpowerFractal;
        get_julia_attractor(param[0], param[1]);  // another attractor?
        break;
    case MAGNET2J:
        FloatPreCalcMagnet2();
    case MAGNET1J:
        attr[0].x = 1.0;      // 1.0 + 0.0i always attracts
        attr[0].y = 0.0;      // - both MAGNET1 and MAGNET2
        attrperiod[0] = 1;
        attractors = 1;
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case LAMBDAFP:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        get_julia_attractor(0.5, 0.0);    // another attractor?
        break;
    case LAMBDAEXP:
        if (parm.y == 0.0)
            symmetry = symmetry_type::X_AXIS;
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case FPJULTRIGPLUSEXP:
    case FPJULTRIGPLUSZSQRD:
        if (parm.y == 0.0)
            symmetry = symmetry_type::X_AXIS;
        else
            symmetry = symmetry_type::NONE;
        if ((trigndx[0] == trig_fn::LOG) || (trigndx[0] == trig_fn::FLIP))
            symmetry = symmetry_type::NONE;
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case HYPERCMPLXJFP:
        if (param[2] != 0)
            symmetry = symmetry_type::NONE;
        if (trigndx[0] != trig_fn::SQR)
            symmetry = symmetry_type::NONE;
    case QUATJULFP:
        attractors = 0;   // attractors broken since code checks r,i not j,k
        periodicitycheck = 0;
        if (param[4] != 0.0 || param[5] != 0)
            symmetry = symmetry_type::NONE;
        break;
    case FPPOPCORN:
    case FPPOPCORNJUL:
    {
        bool default_functions = false;
        if (trigndx[0] == trig_fn::SIN &&
                trigndx[1] == trig_fn::TAN &&
                trigndx[2] == trig_fn::SIN &&
                trigndx[3] == trig_fn::TAN &&
                fabs(parm2.x - 3.0) < .0001 &&
                parm2.y == 0 &&
                parm.y == 0)
        {
            default_functions = true;
            if (fractype == FPPOPCORNJUL)
                symmetry = symmetry_type::ORIGIN;
        }
        if (save_release <= 1960)
            curfractalspecific->orbitcalc = PopcornFractal_Old;
        else if (default_functions && debugflag == debug_flags::force_real_popcorn)
            curfractalspecific->orbitcalc = PopcornFractal;
        else
            curfractalspecific->orbitcalc = PopcornFractalFn;
        get_julia_attractor(0.0, 0.0);    // another attractor?
    }
    break;
    case FPCIRCLE:
        if (inside == STARTRAIL) // FPCIRCLE locks up when used with STARTRAIL
            inside = 0; // arbitrarily set inside = NUMB
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    default:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }
    return true;
}

bool
MandellongSetup()
{
    FgHalf = fudge >> 1;
    c_exp = (int)param[2];
    if (fractype == MARKSMANDEL && c_exp < 1)
    {
        c_exp = 1;
        param[2] = 1;
    }
    if ((fractype == MARKSMANDEL   && !(c_exp & 1)) ||
            (fractype == LMANDELZPOWER && (c_exp & 1)))
        symmetry = symmetry_type::XY_AXIS_NO_PARAM;    // odd exponents
    if ((fractype == MARKSMANDEL && (c_exp & 1)) || fractype == LMANDELEXP)
        symmetry = symmetry_type::X_AXIS_NO_PARAM;
    if (fractype == SPIDER && periodicitycheck == 1)
        periodicitycheck = 4;
    longparm = &linit;
    if (fractype == LMANDELZPOWER)
    {
        if (param[3] == 0.0 && debugflag != debug_flags::force_complex_power && (double)c_exp == param[2])
            fractalspecific[fractype].orbitcalc = longZpowerFractal;
        else
            fractalspecific[fractype].orbitcalc = longCmplxZpowerFractal;
        if (param[3] != 0 || (double)c_exp != param[2])
            symmetry = symmetry_type::NONE;
    }
    if ((fractype == LMANTRIGPLUSEXP) || (fractype == LMANTRIGPLUSZSQRD))
    {
        if (parm.y == 0.0)
            symmetry = symmetry_type::X_AXIS;
        else
            symmetry = symmetry_type::NONE;
        if ((trigndx[0] == trig_fn::LOG) || (trigndx[0] == trig_fn::FLIP))
            symmetry = symmetry_type::NONE;
    }
    if (fractype == TIMSERROR)
    {
        if (trigndx[0] == trig_fn::FLIP)
            symmetry = symmetry_type::NONE;
    }
    if (fractype == MARKSMANDELPWR)
    {
        if (trigndx[0] == trig_fn::FLIP)
            symmetry = symmetry_type::NONE;
    }
    return true;
}

bool
JulialongSetup()
{
    c_exp = (int)param[2];
    longparm = &lparm;
    switch (fractype)
    {
    case LJULIAZPOWER:
        if ((c_exp & 1) || param[3] != 0.0 || (double)c_exp != param[2])
            symmetry = symmetry_type::NONE;
        if (param[3] == 0.0 && debugflag != debug_flags::force_complex_power && (double)c_exp == param[2])
            fractalspecific[fractype].orbitcalc = longZpowerFractal;
        else
            fractalspecific[fractype].orbitcalc = longCmplxZpowerFractal;
        break;
    case LAMBDA:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        get_julia_attractor(0.5, 0.0);    // another attractor?
        break;
    case LLAMBDAEXP:
        if (lparm.y == 0)
            symmetry = symmetry_type::X_AXIS;
        break;
    case LJULTRIGPLUSEXP:
    case LJULTRIGPLUSZSQRD:
        if (parm.y == 0.0)
            symmetry = symmetry_type::X_AXIS;
        else
            symmetry = symmetry_type::NONE;
        if ((trigndx[0] == trig_fn::LOG) || (trigndx[0] == trig_fn::FLIP))
            symmetry = symmetry_type::NONE;
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    case LPOPCORN:
    case LPOPCORNJUL:
    {
        bool default_functions = false;
        if (trigndx[0] == trig_fn::SIN &&
                trigndx[1] == trig_fn::TAN &&
                trigndx[2] == trig_fn::SIN &&
                trigndx[3] == trig_fn::TAN &&
                fabs(parm2.x - 3.0) < .0001 &&
                parm2.y == 0 &&
                parm.y == 0)
        {
            default_functions = true;
            if (fractype == LPOPCORNJUL)
                symmetry = symmetry_type::ORIGIN;
        }
        if (save_release <= 1960)
            curfractalspecific->orbitcalc = LPopcornFractal_Old;
        else if (default_functions && debugflag == debug_flags::force_real_popcorn)
            curfractalspecific->orbitcalc = LPopcornFractal;
        else
            curfractalspecific->orbitcalc = LPopcornFractalFn;
        get_julia_attractor(0.0, 0.0);    // another attractor?
    }
    break;
    default:
        get_julia_attractor(0.0, 0.0);    // another attractor?
        break;
    }
    return true;
}

bool
TrigPlusSqrlongSetup()
{
    curfractalspecific->per_pixel =  julia_per_pixel;
    curfractalspecific->orbitcalc =  TrigPlusSqrFractal;
    if (lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != debug_flags::force_standard_fractal)
    {
        if (lparm2.x == fudge)        // Scott variant
            curfractalspecific->orbitcalc =  ScottTrigPlusSqrFractal;
        else if (lparm2.x == -fudge)  // Skinner variant
            curfractalspecific->orbitcalc =  SkinnerTrigSubSqrFractal;
    }
    return JulialongSetup();
}

bool
TrigPlusSqrfpSetup()
{
    curfractalspecific->per_pixel =  juliafp_per_pixel;
    curfractalspecific->orbitcalc =  TrigPlusSqrfpFractal;
    if (parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != debug_flags::force_standard_fractal)
    {
        if (parm2.x == 1.0)        // Scott variant
            curfractalspecific->orbitcalc =  ScottTrigPlusSqrfpFractal;
        else if (parm2.x == -1.0)  // Skinner variant
            curfractalspecific->orbitcalc =  SkinnerTrigSubSqrfpFractal;
    }
    return JuliafpSetup();
}

bool
TrigPlusTriglongSetup()
{
    FnPlusFnSym();
    if (trigndx[1] == trig_fn::SQR)
        return TrigPlusSqrlongSetup();
    curfractalspecific->per_pixel =  long_julia_per_pixel;
    curfractalspecific->orbitcalc =  TrigPlusTrigFractal;
    if (lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != debug_flags::force_standard_fractal)
    {
        if (lparm2.x == fudge)        // Scott variant
            curfractalspecific->orbitcalc =  ScottTrigPlusTrigFractal;
        else if (lparm2.x == -fudge)  // Skinner variant
            curfractalspecific->orbitcalc =  SkinnerTrigSubTrigFractal;
    }
    return JulialongSetup();
}

bool
TrigPlusTrigfpSetup()
{
    FnPlusFnSym();
    if (trigndx[1] == trig_fn::SQR)
        return TrigPlusSqrfpSetup();
    curfractalspecific->per_pixel =  otherjuliafp_per_pixel;
    curfractalspecific->orbitcalc =  TrigPlusTrigfpFractal;
    if (parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != debug_flags::force_standard_fractal)
    {
        if (parm2.x == 1.0)        // Scott variant
            curfractalspecific->orbitcalc =  ScottTrigPlusTrigfpFractal;
        else if (parm2.x == -1.0)  // Skinner variant
            curfractalspecific->orbitcalc =  SkinnerTrigSubTrigfpFractal;
    }
    return JuliafpSetup();
}

bool
FnPlusFnSym() // set symmetry matrix for fn+fn type
{
    static symmetry_type fnplusfn[7][7] =
    {   // fn2 ->sin     cos    sinh    cosh   exp    log    sqr
        // fn1
        /* sin */ {symmetry_type::PI_SYM,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* cos */ {symmetry_type::X_AXIS,  symmetry_type::PI_SYM,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* sinh*/ {symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* cosh*/ {symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* exp */ {symmetry_type::X_AXIS,  symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::XY_AXIS,symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* log */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::X_AXIS},
        /* sqr */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS, symmetry_type::X_AXIS, symmetry_type::XY_AXIS}
    };
    if (parm.y == 0.0 && parm2.y == 0.0)
    {
        if (trigndx[0] <= trig_fn::SQR && trigndx[1] < trig_fn::SQR)  // bounds of array
            symmetry = fnplusfn[static_cast<int>(trigndx[0])][static_cast<int>(trigndx[1])];
        if (trigndx[0] == trig_fn::FLIP || trigndx[1] == trig_fn::FLIP)
            symmetry = symmetry_type::NONE;
    }                 // defaults to symmetry::X_AXIS symmetry
    else
        symmetry = symmetry_type::NONE;
    return (0);
}

bool
LambdaTrigOrTrigSetup()
{
    // default symmetry is symmetry::ORIGIN
    longparm = &lparm;
    floatparm = &parm;
    if ((trigndx[0] == trig_fn::EXP) || (trigndx[1] == trig_fn::EXP))
        symmetry = symmetry_type::NONE;
    if ((trigndx[0] == trig_fn::LOG) || (trigndx[1] == trig_fn::LOG))
        symmetry = symmetry_type::X_AXIS;
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool
JuliaTrigOrTrigSetup()
{
    // default symmetry is symmetry::X_AXIS
    longparm = &lparm;
    floatparm = &parm;
    if (parm.y != 0.0)
        symmetry = symmetry_type::NONE;
    if (trigndx[0] == trig_fn::FLIP || trigndx[1] == trig_fn::FLIP)
        symmetry = symmetry_type::NONE;
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return (1);
}

bool
ManlamTrigOrTrigSetup()
{   // psuedo
    // default symmetry is symmetry::X_AXIS
    longparm = &linit;
    floatparm = &init;
    if (trigndx[0] == trig_fn::SQR)
        symmetry = symmetry_type::NONE;
    if ((trigndx[0] == trig_fn::LOG) || (trigndx[1] == trig_fn::LOG))
        symmetry = symmetry_type::NONE;
    return true;
}

bool
MandelTrigOrTrigSetup()
{
    // default symmetry is symmetry::X_AXIS_NO_PARAM
    longparm = &linit;
    floatparm = &init;
    if ((trigndx[0] == trig_fn::FLIP) || (trigndx[1] == trig_fn::FLIP))
        symmetry = symmetry_type::NONE;
    return true;
}


bool
ZXTrigPlusZSetup()
{
    //   static char ZXTrigPlusZSym1[] =
    // fn1 ->  sin   cos    sinh  cosh exp   log   sqr
    //           {symmetry::X_AXIS,symmetry::XY_AXIS,symmetry::X_AXIS,symmetry::XY_AXIS,symmetry::X_AXIS,symmetry::NONE,symmetry::XY_AXIS};
    //   static char ZXTrigPlusZSym2[] =
    // fn1 ->  sin   cos    sinh  cosh exp   log   sqr
    //           {symmetry::NONE,symmetry::ORIGIN,symmetry::NONE,symmetry::ORIGIN,symmetry::NONE,symmetry::NONE,symmetry::ORIGIN};

    if (param[1] == 0.0 && param[3] == 0.0)
        //      symmetry = ZXTrigPlusZSym1[trigndx[0]];
        switch (trigndx[0])
        {
        case trig_fn::COS:
        case trig_fn::COSH:
        case trig_fn::SQR:
        case trig_fn::REAL_COS:
            symmetry = symmetry_type::XY_AXIS;
            break;
        case trig_fn::FLIP:
            symmetry = symmetry_type::Y_AXIS;
            break;
        case trig_fn::LOG:
            symmetry = symmetry_type::NONE;
            break;
        default:
            symmetry = symmetry_type::X_AXIS;
            break;
        }
    else
        //      symmetry = ZXTrigPlusZSym2[trigndx[0]];
        switch (trigndx[0])
        {
        case trig_fn::COS:
        case trig_fn::COSH:
        case trig_fn::SQR:
        case trig_fn::REAL_COS:
            symmetry = symmetry_type::ORIGIN;
            break;
        case trig_fn::FLIP:
            symmetry = symmetry_type::NONE;
            break;
        default:
            symmetry = symmetry_type::NONE;
            break;
        }
    if (curfractalspecific->isinteger)
    {
        curfractalspecific->orbitcalc =  ZXTrigPlusZFractal;
        if (lparm.x == fudge && lparm.y == 0L && lparm2.y == 0L && debugflag != debug_flags::force_standard_fractal)
        {
            if (lparm2.x == fudge)     // Scott variant
                curfractalspecific->orbitcalc =  ScottZXTrigPlusZFractal;
            else if (lparm2.x == -fudge)  // Skinner variant
                curfractalspecific->orbitcalc =  SkinnerZXTrigSubZFractal;
        }
        return JulialongSetup();
    }
    else
    {
        curfractalspecific->orbitcalc =  ZXTrigPlusZfpFractal;
        if (parm.x == 1.0 && parm.y == 0.0 && parm2.y == 0.0 && debugflag != debug_flags::force_standard_fractal)
        {
            if (parm2.x == 1.0)     // Scott variant
                curfractalspecific->orbitcalc =  ScottZXTrigPlusZfpFractal;
            else if (parm2.x == -1.0)       // Skinner variant
                curfractalspecific->orbitcalc =  SkinnerZXTrigSubZfpFractal;
        }
    }
    return JuliafpSetup();
}

bool
LambdaTrigSetup()
{
    bool const isinteger = curfractalspecific->isinteger != 0;
    if (isinteger)
        curfractalspecific->orbitcalc =  LambdaTrigFractal;
    else
        curfractalspecific->orbitcalc =  LambdaTrigfpFractal;
    switch (trigndx[0])
    {
    case trig_fn::SIN:
    case trig_fn::COS:
    case trig_fn::REAL_COS:
        symmetry = symmetry_type::PI_SYM;
        if (isinteger)
            curfractalspecific->orbitcalc =  LambdaTrigFractal1;
        else
            curfractalspecific->orbitcalc =  LambdaTrigfpFractal1;
        break;
    case trig_fn::SINH:
    case trig_fn::COSH:
        symmetry = symmetry_type::ORIGIN;
        if (isinteger)
            curfractalspecific->orbitcalc =  LambdaTrigFractal2;
        else
            curfractalspecific->orbitcalc =  LambdaTrigfpFractal2;
        break;
    case trig_fn::SQR:
        symmetry = symmetry_type::ORIGIN;
        break;
    case trig_fn::EXP:
        if (isinteger)
            curfractalspecific->orbitcalc =  LongLambdaexponentFractal;
        else
            curfractalspecific->orbitcalc =  LambdaexponentFractal;
        symmetry = symmetry_type::NONE;
        break;
    case trig_fn::LOG:
        symmetry = symmetry_type::NONE;
        break;
    default:   // default for additional functions
        symmetry = symmetry_type::ORIGIN;
        break;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    if (isinteger)
        return JulialongSetup();
    else
        return JuliafpSetup();
}

bool
JuliafnPlusZsqrdSetup()
{
    //   static char fnpluszsqrd[] =
    // fn1 ->  sin   cos    sinh  cosh   sqr    exp   log
    // sin    {symmetry::NONE,symmetry::ORIGIN,symmetry::NONE,symmetry::ORIGIN,symmetry::ORIGIN,symmetry::NONE,symmetry::NONE};

    switch (trigndx[0]) // fix sqr symmetry & add additional functions
    {
    case trig_fn::COS: // cosxx
    case trig_fn::COSH:
    case trig_fn::SQR:
    case trig_fn::REAL_COS:
    case trig_fn::TAN:
    case trig_fn::TANH:
        symmetry = symmetry_type::ORIGIN;
        break;
        // default is for symmetry::NONE symmetry

    default:
        break;
    }
    if (curfractalspecific->isinteger)
        return JulialongSetup();
    else
        return JuliafpSetup();
}

bool
SqrTrigSetup()
{
    //   static char SqrTrigSym[] =
    // fn1 ->  sin    cos    sinh   cosh   sqr    exp   log
    //           {symmetry::PI_SYM,symmetry::PI_SYM,symmetry::XY_AXIS,symmetry::XY_AXIS,symmetry::XY_AXIS,symmetry::X_AXIS,symmetry::X_AXIS};
    switch (trigndx[0]) // fix sqr symmetry & add additional functions
    {
    case trig_fn::SIN:
    case trig_fn::COS: // cosxx
    case trig_fn::REAL_COS:   // 'real' cos
        symmetry = symmetry_type::PI_SYM;
        break;
        // default is for symmetry::X_AXIS symmetry

    default:
        break;
    }
    if (curfractalspecific->isinteger)
        return JulialongSetup();
    else
        return JuliafpSetup();
}

bool
FnXFnSetup()
{
    static symmetry_type fnxfn[7][7] =
    {   // fn2 ->sin     cos    sinh    cosh  exp    log    sqr
        // fn1
        /* sin */ {symmetry_type::PI_SYM,  symmetry_type::Y_AXIS,  symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* cos */ {symmetry_type::Y_AXIS,  symmetry_type::PI_SYM,  symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* sinh*/ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* cosh*/ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* exp */ {symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::X_AXIS,  symmetry_type::NONE,   symmetry_type::XY_AXIS},
        /* log */ {symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::NONE,    symmetry_type::X_AXIS, symmetry_type::NONE},
        /* sqr */ {symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::XY_AXIS, symmetry_type::NONE,   symmetry_type::XY_AXIS},
    };
    if (trigndx[0] <= trig_fn::SQR && trigndx[1] <= trig_fn::SQR)  // bounds of array
        symmetry = fnxfn[static_cast<int>(trigndx[0])][static_cast<int>(trigndx[1])];
    // defaults to symmetry::X_AXIS symmetry
    else
    {
        if (trigndx[0] == trig_fn::LOG || trigndx[1] == trig_fn::LOG)
            symmetry = symmetry_type::NONE;
        if (trigndx[0] == trig_fn::REAL_COS || trigndx[1] == trig_fn::REAL_COS)
        {
            if (trigndx[0] == trig_fn::SIN || trigndx[1] == trig_fn::SIN)
                symmetry = symmetry_type::PI_SYM;
            if (trigndx[0] == trig_fn::COS || trigndx[1] == trig_fn::COS)
                symmetry = symmetry_type::PI_SYM;
        }
        if (trigndx[0] == trig_fn::REAL_COS && trigndx[1] == trig_fn::REAL_COS)
            symmetry = symmetry_type::PI_SYM;
    }
    if (curfractalspecific->isinteger)
        return JulialongSetup();
    else
        return JuliafpSetup();
}

bool
MandelTrigSetup()
{
    bool const isinteger = curfractalspecific->isinteger != 0;
    if (isinteger)
        curfractalspecific->orbitcalc =  LambdaTrigFractal;
    else
        curfractalspecific->orbitcalc =  LambdaTrigfpFractal;
    symmetry = symmetry_type::XY_AXIS_NO_PARAM;
    switch (trigndx[0])
    {
    case trig_fn::SIN:
    case trig_fn::COS:
        if (isinteger)
            curfractalspecific->orbitcalc =  LambdaTrigFractal1;
        else
            curfractalspecific->orbitcalc =  LambdaTrigfpFractal1;
        break;
    case trig_fn::SINH:
    case trig_fn::COSH:
        if (isinteger)
            curfractalspecific->orbitcalc =  LambdaTrigFractal2;
        else
            curfractalspecific->orbitcalc =  LambdaTrigfpFractal2;
        break;
    case trig_fn::EXP:
        symmetry = symmetry_type::X_AXIS_NO_PARAM;
        if (isinteger)
            curfractalspecific->orbitcalc =  LongLambdaexponentFractal;
        else
            curfractalspecific->orbitcalc =  LambdaexponentFractal;
        break;
    case trig_fn::LOG:
        symmetry = symmetry_type::X_AXIS_NO_PARAM;
        break;
    default:
        symmetry = symmetry_type::XY_AXIS_NO_PARAM;
        break;
    }
    if (isinteger)
        return MandellongSetup();
    else
        return MandelfpSetup();
}

bool
MarksJuliaSetup()
{
#if !defined(XFRACT)
    if (param[2] < 1)
        param[2] = 1;
    c_exp = (int)param[2];
    longparm = &lparm;
    lold = *longparm;
    if (c_exp > 3)
        lcpower(&lold,c_exp-1,&lcoefficient,bitshift);
    else if (c_exp == 3)
    {
        lcoefficient.x = multiply(lold.x,lold.x,bitshift) - multiply(lold.y,lold.y,bitshift);
        lcoefficient.y = multiply(lold.x,lold.y,bitshiftless1);
    }
    else if (c_exp == 2)
        lcoefficient = lold;
    else if (c_exp < 2)
    {
        lcoefficient.x = 1L << bitshift;
        lcoefficient.y = 0L;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
#endif
    return true;
}

bool
MarksJuliafpSetup()
{
    if (param[2] < 1)
        param[2] = 1;
    c_exp = (int)param[2];
    floatparm = &parm;
    old = *floatparm;
    if (c_exp > 3)
        cpower(&old,c_exp-1,&coefficient);
    else if (c_exp == 3)
    {
        coefficient.x = sqr(old.x) - sqr(old.y);
        coefficient.y = old.x * old.y * 2;
    }
    else if (c_exp == 2)
        coefficient = old;
    else if (c_exp < 2)
    {
        coefficient.x = 1.0;
        coefficient.y = 0.0;
    }
    get_julia_attractor(0.0, 0.0);       // an attractor?
    return true;
}

bool
SierpinskiSetup()
{
    // sierpinski
    periodicitycheck = 0;                // disable periodicity checks
    ltmp.x = 1;
    ltmp.x = ltmp.x << bitshift; // ltmp.x = 1
    ltmp.y = ltmp.x >> 1;                        // ltmp.y = .5
    return true;
}

bool
SierpinskiFPSetup()
{
    // sierpinski
    periodicitycheck = 0;                // disable periodicity checks
    tmp.x = 1;
    tmp.y = 0.5;
    return true;
}

bool
HalleySetup()
{
    // Halley
    periodicitycheck = 0;

    if (usr_floatflag)
        fractype = HALLEY; // float on
    else
        fractype = MPHALLEY;

    curfractalspecific = &fractalspecific[fractype];

    degree = (int)parm.x;
    if (degree < 2)
        degree = 2;
    param[0] = (double)degree;

    //  precalculated values
    AplusOne = degree + 1; // a+1
    Ap1deg = AplusOne * degree;

#if !defined(XFRACT)
    if (fractype == MPHALLEY)
    {
        setMPfunctions();
        mpAplusOne = *pd2MP((double)AplusOne);
        mpAp1deg = *pd2MP((double)Ap1deg);
        mpctmpparm.x = *pd2MP(parm.y);
        mpctmpparm.y = *pd2MP(parm2.y);
        mptmpparm2x = *pd2MP(parm2.x);
        mpone        = *pd2MP(1.0);
    }
#endif

    if (degree % 2)
        symmetry = symmetry_type::X_AXIS;   // odd
    else
        symmetry = symmetry_type::XY_AXIS; // even
    return true;
}

bool
PhoenixSetup()
{
    longparm = &lparm;
    floatparm = &parm;
    degree = (int)parm2.x;
    if (degree < 2 && degree > -3)
        degree = 0;
    param[2] = (double)degree;
    if (degree == 0)
    {
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixFractal;
    }
    if (degree >= 2)
    {
        degree = degree - 1;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixPlusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixPlusFractal;
    }
    if (degree <= -3)
    {
        degree = abs(degree) - 2;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixMinusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixMinusFractal;
    }

    return true;
}

bool
PhoenixCplxSetup()
{
    longparm = &lparm;
    floatparm = &parm;
    degree = (int)param[4];
    if (degree < 2 && degree > -3)
        degree = 0;
    param[4] = (double)degree;
    if (degree == 0)
    {
        if (parm2.x != 0 || parm2.y != 0)
            symmetry = symmetry_type::NONE;
        else
            symmetry = symmetry_type::ORIGIN;
        if (parm.y == 0 && parm2.y == 0)
            symmetry = symmetry_type::X_AXIS;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixFractalcplx;
        else
            curfractalspecific->orbitcalc =  LongPhoenixFractalcplx;
    }
    if (degree >= 2)
    {
        degree = degree - 1;
        if (parm.y == 0 && parm2.y == 0)
            symmetry = symmetry_type::X_AXIS;
        else
            symmetry = symmetry_type::NONE;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixCplxPlusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixCplxPlusFractal;
    }
    if (degree <= -3)
    {
        degree = abs(degree) - 2;
        if (parm.y == 0 && parm2.y == 0)
            symmetry = symmetry_type::X_AXIS;
        else
            symmetry = symmetry_type::NONE;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixCplxMinusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixCplxMinusFractal;
    }

    return true;
}

bool
MandPhoenixSetup()
{
    longparm = &linit;
    floatparm = &init;
    degree = (int)parm2.x;
    if (degree < 2 && degree > -3)
        degree = 0;
    param[2] = (double)degree;
    if (degree == 0)
    {
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixFractal;
    }
    if (degree >= 2)
    {
        degree = degree - 1;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixPlusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixPlusFractal;
    }
    if (degree <= -3)
    {
        degree = abs(degree) - 2;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixMinusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixMinusFractal;
    }

    return true;
}

bool
MandPhoenixCplxSetup()
{
    longparm = &linit;
    floatparm = &init;
    degree = (int)param[4];
    if (degree < 2 && degree > -3)
        degree = 0;
    param[4] = (double)degree;
    if (parm.y != 0 || parm2.y != 0)
        symmetry = symmetry_type::NONE;
    if (degree == 0)
    {
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixFractalcplx;
        else
            curfractalspecific->orbitcalc =  LongPhoenixFractalcplx;
    }
    if (degree >= 2)
    {
        degree = degree - 1;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixCplxPlusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixCplxPlusFractal;
    }
    if (degree <= -3)
    {
        degree = abs(degree) - 2;
        if (usr_floatflag)
            curfractalspecific->orbitcalc =  PhoenixCplxMinusFractal;
        else
            curfractalspecific->orbitcalc =  LongPhoenixCplxMinusFractal;
    }

    return true;
}

bool
StandardSetup()
{
    if (fractype == UNITYFP)
        periodicitycheck = 0;
    return true;
}

bool
VLSetup()
{
    if (param[0] < 0.0)
        param[0] = 0.0;
    if (param[1] < 0.0)
        param[1] = 0.0;
    if (param[0] > 1.0)
        param[0] = 1.0;
    if (param[1] > 1.0)
        param[1] = 1.0;
    floatparm = &parm;
    return true;
}
