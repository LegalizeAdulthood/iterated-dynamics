// SPDX-License-Identifier: GPL-3.0-only
//
///////////////////////////////////////////////////////////
//    PERTURBATION.CPP a module to explore Perturbation
//    Thanks to Shirom Makkad fractaltodesktop@gmail.com
//    Written in Microsoft Visual C++ by Paul de Leeuw.
///////////////////////////////////////////////////////////

#include "PertEngine.h"
#include "biginit.h"
#include "drivers.h"
#include "fractalp.h"
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
CPertEngine g_pert_engine;
char		g_pert_status[200];

int         perturbation(int);
void        cvt_centermag_bf(bf_t *Xctr, bf_t *Yctr, double *Magnification, double *Xmagfactor, double *Rotation, double *Skew);
extern void cvtcentermag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew);

/**************************************************************************
	Initialise Perturbation engine
**************************************************************************/

bool	init_perturbation(int subtype)
    {
    double  mandel_width;    // width of display
    double  x_centre, y_centre, x_magfactor, rotation, skew;

    bf_t x_centre_bf = NULL, y_centre_bf = NULL, tmp_bf = NULL;
    int saved;

    double  magnification;

    if (g_bf_math != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
        {
        saved = save_stack();
        x_centre_bf = alloc_stack(g_bf_length + 2);
        y_centre_bf = alloc_stack(g_bf_length + 2);
        tmp_bf = alloc_stack(g_bf_length + 2);
        cvt_centermag_bf(&x_centre_bf, &y_centre_bf, &magnification, &x_magfactor, &rotation, &skew);
        neg_bf(y_centre_bf, y_centre_bf);
        x_centre = y_centre = 0.0; // get rid of silly runtime error messages
        }
    else
        {
        LDBL magnification_ld;
            cvtcentermag(&x_centre, &y_centre, &magnification_ld, &x_magfactor, &rotation, &skew);
        y_centre = -y_centre;
        }

    if (g_bf_math == bf_math_type::NONE) 
        mandel_width = g_y_max - g_y_min;
    else
        {
        sub_bf(tmp_bf, g_bf_y_max, g_bf_y_min);
        mandel_width = bftofloat(tmp_bf);
        }

    /*
    if (method >= TIERAZONFILTERS)
	    TZfilter.InitFilter(method, threshold, dStrands, &FilterRGB, UseCurrentPalette);		// initialise the constants used by Tierazon fractals
    if (BigNumFlag)
	    mandel_width = mpfr_get_d(BigWidth.x, MPFR_RNDN);
    */
    g_pert_engine.initialize_frame(x_centre_bf, y_centre_bf, x_centre, y_centre, mandel_width / 2);
    perturbation(subtype);
    if (g_bf_math != bf_math_type::NONE) // we assume bignum is flagged and bf variables are initialised
        restore_stack(saved);
    g_calc_status = calc_status_value::COMPLETED;
    return false;
    }

/**************************************************************************
	The Perturbation engine
**************************************************************************/

int perturbation(int subtype)
    {
    int     degree;  // power

    degree = (int) g_params[2];
    if (g_pert_engine.calculate_one_frame(g_magnitude_limit, g_pert_status, degree, g_inside_color, g_outside_color, g_biomorph, subtype, g_plot, potential/*, &TZfilter, &TrueCol*/) < 0)
	    return -1;
    return 0;
    }

/**************************************************************************
	Convert corners to centre/mag using BigNum
**************************************************************************/

void cvt_centermag_bf(bf_t *Xctr, bf_t *Yctr, double *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
    {
    // needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits
    double      Height;
    bf_t width_bf;
    bf_t height_bf;
    bf_t temp_bf;

    int saved;
    saved = save_stack();
    width_bf = alloc_stack(g_bf_length + 2);
    height_bf = alloc_stack(g_bf_length + 2);
    temp_bf = alloc_stack(g_bf_length + 2);

    // simple normal case first
    // if (g_x_3rd == g_x_min && g_y_3rd == g_y_min)
    if (!cmp_bf(g_bf_x_3rd, g_bf_x_min) && !cmp_bf(g_bf_y_3rd, g_bf_y_min))
        {
        // no rotation or skewing, but stretching is allowed
        // Width  = g_x_max - g_x_min;
        sub_bf(width_bf, g_bf_x_max, g_bf_x_min);
        double Width = bftofloat(width_bf);
        // Height = g_y_max - g_y_min;
        sub_bf(height_bf, g_bf_y_max, g_bf_y_min);
        Height = bftofloat(height_bf);
        // *Xctr = (g_x_min + g_x_max)/2;
        add_bf(temp_bf, g_bf_x_max, g_bf_x_min);
        half_bf(*Xctr, temp_bf);
        // *Yctr = (g_y_min + g_y_max)/2;
        add_bf(temp_bf, g_bf_y_max, g_bf_y_min);
        half_bf(*Yctr, temp_bf);
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


