// SPDX-License-Identifier: GPL-3.0-only
//
///////////////////////////////////////////////////////////
//    PERTURBATION.CPP a module to explore Perturbation
//    Thanks to Shirom Makkad fractaltodesktop@gmail.com
//    Written in Microsoft Visual C++ by Paul de Leeuw.
///////////////////////////////////////////////////////////

#include "fractalp.h"
#include "pert_engine.h"
#include "drivers.h"
#include "id_data.h"

//extern  int     driver_key_pressed();
extern  void    (*g_plot)(int, int, int); // function pointer
extern	int	    potential(double, long);
extern  int     g_screen_x_dots, g_screen_y_dots; // # of dots on the physical screen
extern calc_status_value g_calc_status;

extern	long	g_max_iterations;
extern  double  g_x_max, g_x_min, g_y_max, g_y_min;
extern  bool    g_potential_flag;

extern  double  g_magnitude_limit;          // bailout level
int decimals = /*bflength * 4*/ 24;         // we can sort this out later
extern	double	g_params[];
extern	int	    g_outside_color;			// outside filters
extern	int	    g_inside_color;			    // inside  filters
extern	int	    g_biomorph;			        // biomorph colour
//extern	RGBTRIPLE FilterRGB;			// for Tierazon filters
//extern	double	dStrands;
//extern	BOOL	UseCurrentPalette;		// do we use the ManpWIN palette? If false, generate internal filter palette

/*
static	CTZfilter	TZfilter;		// Tierazon filters
*/
CPertEngine PertEngine;
char		PertStatus[200];

int         DoPerturbation(int);
void        BigCvtcentermag(bf_t *Xctr, bf_t *Yctr, double *Magnification, double *Xmagfactor, double *Rotation, double *Skew);
extern void cvtcentermag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew);

#ifdef ALLOW_MPFR
void bf2BigNum(BigDouble *BigNum, bf_t bfNum);

/*************************************************************************
    Format string derived from a Bignum
    mpf_get_str() generates strings without decimal points and gives the exponent
    so we need to format it as a normal number
*************************************************************************/

void ConvertBignum2String(char *s, mpfr_t num)                                // can be used for debugging
    {
    char FormatString[24];

    sprintf(FormatString, "%%.%dRf", decimals + PRECISION_FACTOR);
    mpfr_sprintf(s, FormatString, num);
    }
#endif ALLOW_MPFR

    /**************************************************************************
	Initialise Perturbation engine
**************************************************************************/

bool	InitPerturbation(int subtype)
    {
    double  mandel_width;    // width of display
    double  xCentre, yCentre, Xmagfactor, Rotation, Skew;

#ifdef ALLOW_MPFR
    int bitcount = decimals * 5;
    if (bitcount < 30)
        bitcount = 30;
    if (bitcount > SIZEOF_BF_VARS - 10)
        bitcount = SIZEOF_BF_VARS - 10;

//    mpfr_set_default_prec(bitcount);
        mpfr_set_default_prec(1600);
        xBigCentre.ChangePrecision(1600);
        yBigCentre.ChangePrecision(1600);
#endif ALLOW_MPFR
    bf_t xBigCentre = NULL, yBigCentre = NULL, BigTmp = NULL;
    int saved;

    double  Magnification;

    if (g_bf_math != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
        {
        saved = save_stack();
        xBigCentre = alloc_stack(g_bf_length + 2);
        yBigCentre = alloc_stack(g_bf_length + 2);
        BigTmp = alloc_stack(g_bf_length + 2);
        BigCvtcentermag(&xBigCentre, &yBigCentre, &Magnification, &Xmagfactor, &Rotation, &Skew);
        neg_bf(yBigCentre, yBigCentre);
        xCentre = yCentre = 0.0; // get rid of silly runtime error messages
        }
    else
        {
        LDBL LDMagnification;
        cvtcentermag(&xCentre, &yCentre, &LDMagnification, &Xmagfactor, &Rotation, &Skew);
        yCentre = -yCentre;
        }

    if (g_bf_math == bf_math_type::NONE) 
        mandel_width = g_y_max - g_y_min;
    else
        {
        sub_bf(BigTmp, g_bf_y_max, g_bf_y_min);
        mandel_width = bftofloat(BigTmp);
        }

    /*
    if (method >= TIERAZONFILTERS)
	    TZfilter.InitFilter(method, threshold, dStrands, &FilterRGB, UseCurrentPalette);		// initialise the constants used by Tierazon fractals
    if (BigNumFlag)
	    mandel_width = mpfr_get_d(BigWidth.x, MPFR_RNDN);
    */
    PertEngine.initialiseCalculateFrame(g_screen_x_dots, g_screen_y_dots, g_max_iterations, xBigCentre,
            yBigCentre, xCentre, yCentre, mandel_width / 2, g_potential_flag, g_bf_math, g_params/*, &TZfilter*/);
    DoPerturbation(subtype);
    if (g_bf_math != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
        restore_stack(saved);
    g_calc_status = calc_status_value::COMPLETED;
    return false;
    }

/**************************************************************************
	The Perturbation engine
**************************************************************************/

int DoPerturbation(int subtype)
    {
    int (*UserData)() = driver_key_pressed;
    Complex a = {0, 0};             // future TheRedshiftRider
    bool    IsPositive = false;
    int     degree;  // power

    degree = (int) g_params[2];
/*        
    if (subtype == 53)              // future TheRedshiftRider
	    {
	    a.x = g_params[2];
	    a.y = g_params[3];
	    IsPositive = (g_params[4] == 1.0);
	    }
*/
    if (PertEngine.calculateOneFrame(g_magnitude_limit, PertStatus, degree, g_inside_color, g_outside_color, g_biomorph, subtype, a, IsPositive, UserData, g_plot, potential/*, &TZfilter, &TrueCol*/) < 0)
	    return -1;
    return 0;
    }

#ifdef ALLOW_MPFR
/*************************************************************************
    Format Bignum derived from a string
    mpf_set_str() Bignum from strings in the format:
    MeN, where M = mantissa and N exponent
*************************************************************************/

void	ConvertString2Bignum(mpfr_t num, char *s)
    {
    mpfr_set_str(num, s, 10, MPFR_RNDN);
    }

/**************************************************************************
	Convert bf to Bignum
**************************************************************************/

void bf2BigNum(BigDouble *BigNum, bf_t bfNum)
    {
    // let's just do a quick and dirty for now going via text.
    int     bfLengthNeeded = strlen_needed_bf();
    char    *bigstr = NULL;
    int     dec = g_decimals;

    bigstr = new char[bfLengthNeeded];

    bftostr(bigstr, dec, bfNum);
    ConvertString2Bignum(BigNum->x, bigstr);

    if (bigstr)  {delete[] bigstr; bigstr = NULL;}
    }
    
/**************************************************************************
	Convert Bignum to bf
**************************************************************************/

void BigNum2bf(bf_t *bfNum, BigDouble BigNum)
    {
    // let's just do a quick and dirty for now going via text.
    char    *bigstr = NULL;
    int     dec = g_decimals;

    bigstr = new char[SIZEOF_BF_VARS];
    ConvertBignum2String(bigstr, BigNum.x);
    strtobf(*bfNum, bigstr);
    if (bigstr)  {delete[] bigstr; bigstr = NULL;}
    }
#endif ALLOW_MPFR


/**************************************************************************
	Convert corners to centre/mag using BigNum
**************************************************************************/

void BigCvtcentermag(bf_t *Xctr, bf_t *Yctr, double *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
    {
    // needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits
    double      Height;
    bf_t BigWidth;
    bf_t BigHeight;
    bf_t BigTmpx;

    int saved;
    saved = save_stack();
    BigWidth = alloc_stack(g_bf_length + 2);
    BigHeight = alloc_stack(g_bf_length + 2);
    BigTmpx = alloc_stack(g_bf_length + 2);

    // simple normal case first
    // if (g_x_3rd == g_x_min && g_y_3rd == g_y_min)
    if (!cmp_bf(g_bf_x_3rd, g_bf_x_min) && !cmp_bf(g_bf_y_3rd, g_bf_y_min))
        {
        // no rotation or skewing, but stretching is allowed
        // Width  = g_x_max - g_x_min;
        sub_bf(BigWidth, g_bf_x_max, g_bf_x_min);
        double Width = bftofloat(BigWidth);
        // Height = g_y_max - g_y_min;
        sub_bf(BigHeight, g_bf_y_max, g_bf_y_min);
        Height = bftofloat(BigHeight);
        // *Xctr = (g_x_min + g_x_max)/2;
        add_bf(BigTmpx, g_bf_x_max, g_bf_x_min);
        half_bf(*Xctr, BigTmpx);
        // *Yctr = (g_y_min + g_y_max)/2;
        add_bf(BigTmpx, g_bf_y_max, g_bf_y_min);
        half_bf(*Yctr, BigTmpx);
        *Magnification = 2 / Height;
        *Xmagfactor = (double) (Height / (DEFAULT_ASPECT * Width));
        *Rotation = 0.0;
        *Skew = 0.0;
        }
/*              // something for later...
        else
        {
            bftmpx = alloc_stack(bflength + 2);
            bf_t bftmpy = alloc_stack(bflength + 2);

            // set up triangle ABC, having sides abc
            // side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd)
            // IMPORTANT: convert from bf AFTER subtracting

            // tmpx = g_x_max - g_x_min;
            sub_bf(bftmpx, g_bf_x_max, g_bf_x_min);
            LDBL tmpx1 = bftofloat(bftmpx);
            // tmpy = g_y_max - g_y_min;
            sub_bf(bftmpy, g_bf_y_max, g_bf_y_min);
            LDBL tmpy1 = bftofloat(bftmpy);
            LDBL c2 = tmpx1 * tmpx1 + tmpy1 * tmpy1;

            // tmpx = g_x_max - g_x_3rd;
            sub_bf(bftmpx, g_bf_x_max, g_bf_x_3rd);
            tmpx1 = bftofloat(bftmpx);

            // tmpy = g_y_min - g_y_3rd;
            sub_bf(bftmpy, g_bf_y_min, g_bf_y_3rd);
            tmpy1 = bftofloat(bftmpy);
            LDBL a2 = tmpx1 * tmpx1 + tmpy1 * tmpy1;
            LDBL a = sqrtl(a2);

            // divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used
            // atan2() only depends on the ratio, this puts it in double's range
            int signx = sign(tmpx1);
            LDBL tmpy = 0.0;
            if (signx)
            {
                tmpy = tmpy1 / tmpx1 * signx; // tmpy = tmpy / |tmpx|
            }
            *Rotation =
                (double) (-rad_to_deg(std::atan2((double) tmpy, signx))); // negative for image rotation

            // tmpx = g_x_min - g_x_3rd;
            sub_bf(bftmpx, g_bf_x_min, g_bf_x_3rd);
            LDBL tmpx2 = bftofloat(bftmpx);
            // tmpy = g_y_max - g_y_3rd;
            sub_bf(bftmpy, g_bf_y_max, g_bf_y_3rd);
            LDBL tmpy2 = bftofloat(bftmpy);
            LDBL b2 = tmpx2 * tmpx2 + tmpy2 * tmpy2;
            LDBL b = sqrtl(b2);

            double tmpa = std::acos((double) ((a2 + b2 - c2) / (2 * a * b))); // save tmpa for later use
            *Skew = 90 - rad_to_deg(tmpa);

            // these are the only two variables that must use big precision
            // *Xctr = (g_x_min + g_x_max)/2;
            add_bf(Xctr, g_bf_x_min, g_bf_x_max);
            half_a_bf(Xctr);
            // *Yctr = (g_y_min + g_y_max)/2;
            add_bf(Yctr, g_bf_y_min, g_bf_y_max);
            half_a_bf(Yctr);

            Height = b * std::sin(tmpa);
            *Magnification = 2 / Height; // 1/(h/2)
            *Xmagfactor = (double) (Height / (DEFAULT_ASPECT * a));

            // if vector_a cross vector_b is negative
            // then adjust for left-hand coordinate system
            if (tmpx1 * tmpy2 - tmpx2 * tmpy1 < 0 &&
                g_debug_flag != debug_flags::allow_negative_cross_product)
            {
                *Skew = -*Skew;
                *Xmagfactor = -*Xmagfactor;
                *Magnification = -*Magnification;
            }
        }
        if (*Magnification < 0)
        {
            *Magnification = -*Magnification;
            *Rotation += 180;
        }
*/
        restore_stack(saved);
    }


