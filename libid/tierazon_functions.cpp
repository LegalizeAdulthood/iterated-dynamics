// SPDX-License-Identifier: GPL-3.0-only
//
////////////////////////////////////////////////////////////////
// Tierazon.cpp a module for the per pixel calculations of Tierazon fractals. 
//
// By: stephen c. ferguson
// http://home1.gte.net/itriazon/
// email: itriazon@gte.net
//
/////////////////////////////////////////////////////
// Updated in Microsoft Visual 'C++' by Paul de Leeuw.
// These are listed in numerical order of the functions.
// Note that CSqr(z) is used in place of z*z and CCube(z) in place of z*z*z for speed
/////////////////////////////////////////////////////

#include "tierazon.h"

bool juliaflag = false; // for the time being

static Complex g_z, g_z2, g_q;
static int g_degree, g_subtype;

/**************************************************************************
	Initialise functions for each pixel
**************************************************************************/

int	InitTierazonFunctions(int subtype, Complex *z, Complex *q)
    {
    switch (g_fractal_type)
        {
        case fractal_type::TALIS:                   // Talis, z=((z*z)/(1+z))+c
        case fractal_type::NEWTONVARIATION:         // Newton variation, z=((z*z*z-z-1)/(3*z*z-1)-z)*c
    	    if (!juliaflag)                         // let's worry about Julai sets later
		        {
		        z->x = q->x + g_params[0];
		        z->y = q->y + g_params[1];
		        }
	        break;
        case fractal_type::NEWTONNOVA:              // Nova, init: z=1; iterate: z=z-((z*z*z-1)/(3*z*z))+c
    	    if (!juliaflag)
		        {
		        //	Note that julia parameters are also used as starting values for Mandelbrot
		        z->x = g_params[0] + 1.0;
		        z->y = g_params[1];
		        }
          else
     	        z = q;
	        break;

/**************************************************************************
	Newton M-Set, Explanation by Paul Carlson
***************************************************************************/

/*
From - Sun Nov 09 16:15:28 1997
Path: news.gte.net!newsfeed.gte.net!europa.clark.net!199.0.154.56!ais.net!ix.netcom.com!news
From: pjcarlsn@ix.netcom.com (Paul and/or Joyce Carlson)
Newsgroups: alt.binaries.pictures.fractals
Subject: Fractal: Newton Msets Explained
Date: Sun, 09 Nov 1997 21:54:12 GMT
Organization: Netcom
Lines: 72
Expires: 30 days
Message-ID: <6457u5$p8p@sjx-ixn2.ix.netcom.com>
NNTP-Posting-Host: ftc-co2-12.ix.netcom.com
X-NETCOM-Date: Sun Nov 09 12:54:29 PM PST 1997
X-Newsreader: Forte Free Agent 1.0.82
Xref: news.gte.net alt.binaries.pictures.fractals:9670

Quite often in the past when I've posted a zoom into a
Newton Mset I've received a message or two asking me exactly
what a Newton Mset is, and how to go about programming
or writing a formula for one.  Because I plan on posting
several of them in the near future, I thought I'd save
some time by posting an explanation in advance.

The Newton method is a numerical method of solving for
the roots of real or complex valued polynomial equations.
An initial guess is made at a root and then the method is
iterated, producing new values that (hopefully) converge
to a root of the equation.

The traditional Newton fractal is similar to a Julia set
in that the pixel values are used as the initial values
(or guesses).  A typical equation that is used to produce
a traditional Newton fractal is:

    Z^3 - 1 = 0

On the other hand, what I call the Newton Mset method
solves for the roots of an equation in which the pixel
value appears in the equation.  For example, in this
equation the pixel value is denoted as C:

    Z^3 + C*Z^2 - Z + C = 0

Because C has a different value for every pixel, the
method actually solves a different equation for every
pixel.  Now the question is what to use for the initial
guess for each solution.  The answer is to use one of
the "critical points."  These are the values of Z for
which the second derivative vanishes and can be found
by setting the second derivative of the equation to
zero and solving for Z.  In the example equation
above:

    The function:          f(Z)   = Z^3 + C*Z^2 - Z + C
    The first derivative:  f'(Z)  = 3*Z^2 + 2*C*Z - 1
    The second derivative: f''(Z) = 6*Z + 2*C
    Therefore,  setting 6*Z + 2*C = 0 we have Z = -C/3

The variable Z is initialized to -C/3 prior to the
iteration loop.  From there, everything proceeds as
usual using the general Newton Method formula:

    Z[n+1] = Z[n] - f(Z[n]) / f'(Z[n])

A root is assumed to be found when Z[n] and Z[n+1] are
very close together.

In the Newton Mset fractals that I will be posting the
colors of the pixels have nothing to do with which
root a pixel converged to, unlike the more traditional
coloring method.

I hope this helps someone.

Regards,

Paul Carlson
------------------------------------------------------------------
                email   pjcarlsn@ix.netcom.com

WWW Fractal Galleries   http://sprott.physics.wisc.edu/carlson.htm
                        http://fractal.mta.ca/fractals/carlson/
                        http://www.cnam.fr/fractals/carlson.html

        anonymous FTP   ftp.cnam.fr   /pub/Fractals/carlson
------------------------------------------------------------------
*/
        case fractal_type::NEWTONPOLYGON:           // Newton/Mandel, Newton Polygon
        case fractal_type::NEWTONFLOWER:            // Newton/Mandel, Newton Flower
    	    if (!juliaflag)
		        {
                z->x = -q->x * (double) (g_degree - 2) / (double) g_degree + g_params[0];
                z->y = -q->y * (double) (g_degree - 2) / (double) g_degree + g_params[1];
		        }
	        break;
        case fractal_type::NEWTONAPPLE:             // More Newton Msets, 4th order Newton's apple
    	    if (!juliaflag)
                *z = *q * ((double) g_degree);
	        *z = z->CInvert();
	        z->x = z->x + g_params[0];
	        z->y = z->y + g_params[1];
	        break;

	    case fractal_type::NEWTONCROSS:             // More Newton Msets, 5th order Newton Mset
    	    if (!juliaflag)
		        *z = *q * 2.0;
	        *z = z->CInvert();
	        z->x = g_params[0] - z->x;
	        z->y = g_params[1] - z->y;
	        break;

        case fractal_type::QUARTET:
            {
            switch (subtype)
                {
                case 0:                             // Quartets, z2=2; z=z*z*z*z+z2+c; z2=z1
                case 1:                             // Quartets, z2=z; z=z*z*z*z+5*z2*c; z2=z1
                    if (!juliaflag)
                        *z = *q;
                    g_z2 = *z;
                    z->x += g_params[0];
                    z->y += g_params[1];
                    break;

                case 2:                             // Quartets, t=0; z1=z; z=z*z*z-t*t*t+c; z=z1
                    g_z2 = 0;
                    if (!juliaflag)
                        *z = *q;
                    z->x += g_params[0];
                    z->y += g_params[1];
                    break;

                case 3:                             // Quartets, t=0; z1=z; z=z*z*z*z-t*t*t*t+c; t=z1
                    g_z2.x = g_params[0]; // t = 0;
                    g_z2.y = g_params[1];
                    if (!juliaflag)
                        *z = 0;
                    break;

                case 4:                             // Quartets, z2=z; z=(z^4)+c; c=z2
                    if (!juliaflag)
                    {
                        z->x = q->x + g_params[0];
                        z->y = q->y + g_params[1];
                    }
                    break;

                case 5:                             // Quartets, z1=z; z=z*z*z*z-z2+c; z2 = z1
                    if (!juliaflag)
                        *z = *q;
                    g_z2 = *z;
                    z->x += g_params[0];
                    z->y += g_params[1];
                    break;

                case 6:                             // Quartets, z1=z; z=z*z*z*z+z2/2+c;; z2=z1
                    g_z2 = *q;
                    if (!juliaflag)
                    {
                        z->x = g_params[0];
                        z->y = g_params[1];
                    }
                    break;

                default:                            // Quartets, z1=z; z=z*z*z*z+z2/2+c;; z2=z1
                    g_z2 = *q;
                    if (!juliaflag)
                    {
                        z->x = g_params[0];
                        z->y = g_params[1];
                    }
                    break;
                }
            }
	    }
    return 0;
    }

/**************************************************************************
	Run functions for each iteration
**************************************************************************/

int	RunTierazonFunctions(int subtype, Complex *z, Complex *q)
{
    Complex a, b, c, zd, z1, zt;
    double d;

    switch (g_fractal_type)
        {
    case fractal_type::NEWTONNOVA:                  // Nova, init: z=1; iterate: z=z-((z*z*z-1)/(3*z*z))+c
	        z1 = *z;
	        g_z2 = z->CSqr();
	        *z = *z - ((*z*g_z2 - 1) / (g_z2 * 3)) + *q;
	        zd = *z - z1;
	        d = zd.CSumSqr();
	        return (d < MINSIZE);

	    case fractal_type::TALIS:                   // Talis, z=((z^degreez)/(m+z^(degree-1))+c 
            {
            double m = g_params[3];
            a = z->CPolynomial(g_degree - 1);
            *z = (a * *z) / (m + a) + *q;
            g_new_z.x = z->x;
            g_new_z.y = z->y;
            return g_bailout_float();
            }

	    case fractal_type::NEWTONVARIATION:         // Newton variation, z=((z*z*z-z-1)/(3*z*z-1)-z)*c
	        z1 = *z;
	        g_z2 = z->CSqr();
	        *z = ((*z*g_z2 - *z - 1) / (g_z2 * 3 - 1) - *z) * *q;  
	        zd = *z - z1;
	        d = zd.CSumSqr();
	        return (d < MINSIZE);

	    case fractal_type::NEWTONPOLYGON:           // Newton/Mandel, Newton Polygon
	        {
	        Complex	fn, f1n, f2n, a, b;

	        z1 = *z;
            f2n = z->CPolynomial(g_degree - 2);     // z^(deg - 2) - second derivative power
	        f1n = f2n * *z;				            // z^(deg - 1) - first derivative power
	        fn = f1n * *z;				            // z^deg - function power
	        a = f1n * *q + fn + *z + *q;
            b = f1n * ((double) g_degree) + 1.0 + *q * (f2n * ((double) (g_degree - 1)));
	        *z = *z - a / b;
	        zd = *z - z1;
	        d = zd.CSumSqr();
	        return (d < MINSIZE);
	        }

	    case fractal_type::NEWTONFLOWER:            // Newton/Mandel, Newton Flower
	        {
	        Complex	 fn, f1n, f2n, a, b;

	        z1 = *z;
            f2n = z->CPolynomial(g_degree - 2);     // z^(deg - 2) - second derivative power
	        f1n = f2n * *z;				            // z^(deg - 1) - first derivative power
	        fn = f1n * *z;				            // z^deg - function power
	        a = f1n * *q + fn - *z;
            b = f1n * ((double) g_degree) + *q * (f2n * ((double) (g_degree - 1))) - 1.0;
	        *z = *z - a / b;
	        zd = *z - z1;
	        d = zd.CSumSqr();

	        return (d < MINSIZE);
	        }

	    case fractal_type::NEWTONAPPLE:             // More Newton Msets, 4th order Newton's apple
	        {
	        double	s, t, u;
	        Complex	fn, f1n, f2n, f3n, e, f;

	        z1 = *z;
	        f3n = z->CPolynomial(g_degree - 4);		// z^(deg - 4) - third derivative power
	        f2n = f3n * *z;				            // z^(deg - 2) - second derivative power
	        f1n = f2n * *z;				            // z^(deg - 1) - first derivative power
	        fn = f1n * *z;				            // z^deg - function power
	        c = *q * fn*((double)g_degree);
	        e = f1n * ((double)(g_degree - 1));
	        a = c - e - f2n * ((double)(g_degree - 2));	// top row
	        s = (double)g_degree * (double)(g_degree - 1);
	        t = (double)(g_degree - 1) * (double)(g_degree - 2);
	        u = (double)(g_degree - 2) * (double)(g_degree - 3);
	        c = *q * f1n*s;
	        e = f2n * t;
	        b = c - e - f3n * u;			        // bottom row
	        *z = *z - a / b;
	        zd = *z - z1;
	        d = zd.CSumSqr();
	        return (d < MINSIZE);
	        }

	    case fractal_type::NEWTONCROSS:             // More Newton Msets, 5th order Newton Mset
	        {
	        double	s, t;
	        Complex	fn, f1n, f2n;

	        z1 = *z;
	        f2n = z->CPolynomial(g_degree - 3);		// z^(deg - 3) - second derivative power
	        f1n = f2n * *z;				            // z^(deg - 2) - first derivative power
	        fn = f1n * *z;				            // z^(deg - 1) - function power
	        c = *q * fn*((double)g_degree);
	        a = f1n * ((double)(g_degree - 1)) + c - 1.0;
	        t = (double)g_degree * (double)(g_degree - 1);
	        s = (double)(g_degree - 1) * (double)(g_degree - 2);
	        c = *q * f1n*t;
	        b = f2n * s + c;
	        *z = *z - a / b;
	        zd = *z - z1;
	        d = zd.CSumSqr();
	        return (d < MINSIZE);
	        }

        case fractal_type::QUARTET:
            {
            switch (subtype)
                {
                case 0:                             // Quartets, z2=2; z=z*z*z*z+z2+c; z2=z1
                    z1 = *z;
                    *z = z->CPolynomial(g_degree) + g_z2 + *q;
                    g_z2 = z1;
                    zd = *z - z1;
                    d = zd.CSumSqr();
                    return (d < MINSIZE || d > MAXSIZE);

                case 1:                             // Quartets, z2=z; z=z*z*z*z+5*z2*c; z2=z1
                    z1 = *z;
                    *z = z->CPolynomial(g_degree) + g_z2 * *q * 5;
                    g_z2 = z1;
                    zd = *z - z1;
                    d = zd.CSumSqr();
                    return (d < MINSIZE || d > MAXSIZE);

	            case 2:					            // Quartets, t=0; z1=z; z=z*z*z-t*t*t+c; z=z1
	                z1 = *z;
	                a = z->CPolynomial(g_degree) + *q;
	                b = g_z2.CPolynomial(g_degree);	// global z2 used for t
	                *z = a - b;
	                g_z2 = z1;
	                                                //    z=z*z*z-t*t*t+c;
	                                                //    t=z1;
	                zd = *z - z1;
	                d = zd.CSumSqr();
	                return (d < MINSIZE || d > MAXSIZE);

	            case 3:					            // Quartets, t=0; z1=z; z=z*z*z*z-t*t*t*t+c; t=z1
	                z1 = *z;
	                a = z->CPolynomial(g_degree) + *q;
	                b = g_z2.CPolynomial(g_degree);	    // global z2 used for t
	                *z = a - b;
	                g_z2 = z1;				        // t = z1
	                zd = *z - z1;
	                d = zd.CSumSqr();
	                return (d < MINSIZE || d > MAXSIZE);

	            case 4:					            // Quartets, z2=z; z=(z^4)+c; c=z2
	                z1 = *z;
	                g_z2 = *z;
	                                                //  z = z*z*z*z+q;
	                *z = z->CPolynomial(g_degree) + *q;
	                *q = g_z2;
	                zd = *z - z1;
	                d = zd.CSumSqr();
	                return (d < MINSIZE || d > MAXSIZE);

	            case 5:					            // Quartets, z1=z; z=z*z*z*z-z2+c; z2 = z1
	                z1 = *z;
	                *z = z->CPolynomial(g_degree) - g_z2 + *q;
	                g_z2 = z1;
	                zd = *z - z1;
	                d = zd.CSumSqr();
	                return (d < MINSIZE || d > MAXSIZE);

	            case 6:					            // Quartets, z1=z; z=z*z*z*z+z2/2+c;; z2=z1
	                z1 = *z;
	                                                //    z=z*z*z*z+z2/2+q;
	                *z = z->CPolynomial(g_degree) + g_z2 / 2.0 + *q;
	                g_z2 = z1;
	                zd = *z - z1;
	                d = zd.CSumSqr();
	                return (d < MINSIZE || d > MAXSIZE);

	            default:					        // Quartets, z1=z; z=z*z*z*z+z2/2+c;; z2=z1
	                z1 = *z;
	                                                //    z=z*z*z*z+z2/2+q;
	                *z = z->CPolynomial(g_degree) + g_z2 / 2.0 + *q;
	                g_z2 = z1;
	                zd = *z - z1;
	                d = zd.CSumSqr();
	                return (d < MINSIZE || d > MAXSIZE);
                }
            }
	    }
    return 0;
    }

/**************************************************************************
	Initialise functions for each pixel
**************************************************************************/

int tierazonfp_per_pixel()
    {
    g_z.x = g_old_z.x;
    g_z.y = g_old_z.y;
    g_q.x = g_dx_pixel();
    g_q.y = g_dy_pixel();
    g_subtype = (int) g_params[3];
    g_degree = (int) g_params[2];
    g_bail_out_test = (bailouts) g_params[4];
    if (g_degree < 2)
        g_degree = 2;
    InitTierazonFunctions(g_subtype, &g_z, &g_q);
    return 0;
    }

/**************************************************************************
	Run functions for each pixel
**************************************************************************/

int tierazonfpOrbit()
    {
    int ReturnMode;
    ReturnMode = RunTierazonFunctions(g_subtype, &g_z, &g_q);
    g_new_z.x = g_z.x;
    g_new_z.y = g_z.y;
    return ReturnMode;
    }



